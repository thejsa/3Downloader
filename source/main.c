#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>

#include <3ds.h>

PrintConsole topScreen, bottomScreen;

// Copied from CIAngel utils.cpp
void PrintProgress(PrintConsole *console, u32 nSize, u32 nCurrent)
	{
	// Don't attempt to calculate anything if we don't have a final size
	if (nSize == 0) return;
	
	// Switch to the progress console
	PrintConsole* currentConsole = consoleSelect(console);
	consoleClear();
	
	// Set the start time if nLastSize is 0
	static u64 nStartTime;
	if (nCurrent == 0)
	{
	nStartTime = osGetTime();
	}
	
	// Offset everything by 10 lines to kinda center it
	printf("\n\n\n\n\n\n\n\n\n\n");
	
	// Calculate percent and bar width
	double fPercent = ((double)nCurrent / nSize) * 100.0;
	u16 barDrawWidth = (fPercent / 100) * 40;
	
	int i = 0;
	for (i = 0; i < barDrawWidth; i++)
	{
	printf("|");
	}
	printf("\n");
	
	// Output current progress
	printf(" %0.2f / %0.2fMB % 3.2f%%\n", ((double)nCurrent) / 1024 / 1024, ((double)nSize) / 1024 / 1024, fPercent);
	
	// Calculate download speed
	if (nCurrent > 0)
	{
	u64 nTime = osGetTime();
	double seconds = ((double)(nTime - nStartTime)) / 1000;
	
	if (seconds > 0)
	{
	double speed = ((nCurrent / seconds) / 1024);
	printf(" Avg Speed: %.02f KB/s\n", speed);
	}
	}
	
	// Make sure the screen updates
	gfxFlushBuffers();
	gspWaitForVBlank();
	
	// Switch back to the original console
	consoleSelect(currentConsole);
	}

void DownloadFile_InternalSave(void* out, unsigned char* buffer, u32 readSize)
	{
	FILE* os = (FILE*)out;
	fwrite(buffer, readSize, 1, os);
	}

Result DownloadFile_Internal(const char *url, void *out, bool bProgress,
	void (*write)(void* out, unsigned char* buffer, u32 readSize))
	{
	httpcContext context;
	u32 fileSize = 0;
	u32 procSize = 0;
	Result ret = 0;
	Result dlret = HTTPC_RESULTCODE_DOWNLOADPENDING;
	u32 status;
	u32 bufSize = 1024 * 256;
	u32 readSize = 0;
	httpcOpenContext(&context, HTTPC_METHOD_GET, (char*)url, 1);
	httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
	
	// If we're showing progress, set up a console on the bottom screen
	GSPGPU_FramebufferFormats infoOldFormat = gfxGetScreenFormat(GFX_BOTTOM);
	PrintConsole infoConsole;
	if (bProgress)
	{
	PrintConsole* currentConsole = consoleSelect(&infoConsole);
	consoleInit(GFX_BOTTOM, &infoConsole);
	consoleSelect(currentConsole);
	}
	
	ret = httpcBeginRequest(&context);
	if (ret != 0) goto _out;
	
	ret = httpcGetResponseStatusCode(&context, &status, 0);
	if (ret != 0) goto _out;
	
	if (status != 200)
	{
	ret = status;
	goto _out;
	}
	
	ret = httpcGetDownloadSizeState(&context, NULL, &fileSize);
	if (ret != 0) goto _out;
	
	{
	unsigned char *buffer = (unsigned char *)linearAlloc(bufSize);
	if (buffer == NULL)
	{
	printf("Error allocating download buffer\n");
	ret = -1;
	goto _out;
	}
	
	// Initialize the Progress bar if we're showing one
	if (bProgress)
	{
	PrintProgress(&infoConsole, fileSize, procSize);
	}
	
	while (dlret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
	{
	// Check if the app is closing
	if (!aptMainLoop()) {
	ret = -1;
	break;
	}
	
	// Check if the user has pressed B, and cancel if so
	hidScanInput();
	u32 keys = hidKeysDown();
	if (keys & KEY_B)
	{
	ret = -1;
	break;
	}
	
	memset(buffer, 0, bufSize);
	
	dlret = httpcDownloadData(&context, buffer, bufSize, &readSize);
	write(out, buffer, readSize);
	
	procSize += readSize;
	if (bProgress)
	{
	PrintProgress(&infoConsole, fileSize, procSize);
	}
	}
	
	printf("\n");
	linearFree(buffer);
	}
	_out:
	httpcCloseContext(&context);
	
	// If showing progress, restore the bottom screen
	if (bProgress)
	{
	gfxSetScreenFormat(GFX_BOTTOM, infoOldFormat);
	}
	
	return ret;
	}
	
	Result DownloadFile(const char *url, FILE *os, bool bProgress)
	{
	return DownloadFile_Internal(url, os, bProgress, DownloadFile_InternalSave);
	}

int main()
{
	gfxInitDefault();
	//gfxSet3D(true); // uncomment if using stereoscopic 3D

        consoleInit(GFX_TOP, &topScreen);
        consoleInit(GFX_BOTTOM, &bottomScreen);
        consoleSelect(&topScreen);
	
	printf("3Downloader by jsa\nVersion: %s", VERSION);


	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		// Your code goes here

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	return 0;
}
