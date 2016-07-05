#include <3ds.h>
#include <stdio.h>
#include <string.h>

static SwkbdCallbackResult MyCallback(void* user, const char** ppMessage, const char* text, size_t textlen)
{
	if (strcasestr(text, "https://"))
	{
		*ppMessage = "Sorry, HTTPS is not supported.";
		return SWKBD_CALLBACK_CONTINUE;
	}

	if (!strcasestr(text, "http://"))
	{
		*ppMessage = "Sorry, only HTTP is supported.";
		return SWKBD_CALLBACK_CONTINUE;
	}
	
	/*if (strstr(text, "gopher://"))
	{
		*ppMessage = "Omg, what year is this, 1985?";
		return SWKBD_CALLBACK_CONTINUE;
	}*/
	
	return SWKBD_CALLBACK_OK;
}

int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("%s %s by %s\n", APP_TITLE, APP_VERSION, APP_AUTHOR);
	printf("Build date: %s %s\n\n", __DATE__, __TIME__);
	printf("Press A to bring up text input\n");
	printf("Press B to bring up numpad input\n");
	printf("Press X to bring up western input\n");
	printf("Press Y to bring up text filter demo\n");
	printf("Press START to exit\n");

	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break;

		static SwkbdState swkbd;
		static char mybuf[60];
		static SwkbdStatusData swkbdStatus;
		static SwkbdLearningData swkbdLearning;
		SwkbdButton button = SWKBD_BUTTON_NONE;
		bool didit = false;

		if (kDown & KEY_A)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
			swkbdSetInitialText(&swkbd, mybuf);
			swkbdSetHintText(&swkbd, "Enter http:// URL to download (HTTPS is not yet supported)");
			swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Quit", false);
		//	swkbdSetButton(&swkbd, SWKBD_BUTTON_MIDDLE, "~Middle~", true);
			swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Download", true);
			swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
			SwkbdDictWord words[3];
			swkbdSetDictWord(&words[0], ".3dsx", ".3dsx");
			swkbdSetDictWord(&words[1], ".cia", ".cia");
			swkbdSetDictWord(&words[2], "http://", "http://");
			swkbdSetDictionary(&swkbd, words, sizeof(words)/sizeof(SwkbdDictWord));
			static bool reload = false;
			swkbdSetStatusData(&swkbd, &swkbdStatus, reload, true);
			swkbdSetLearningData(&swkbd, &swkbdLearning, reload, true);
			reload = true;
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		}

		if (kDown & KEY_B)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 8);
			swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
			swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);
			swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
			swkbdSetNumpadKeys(&swkbd, L'ツ', L'益');
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		}

		if (kDown & KEY_X)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, -1);
			swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, SWKBD_FILTER_DIGITS | SWKBD_FILTER_AT | SWKBD_FILTER_PERCENT | SWKBD_FILTER_BACKSLASH | SWKBD_FILTER_PROFANITY, 2);
			swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
			swkbdSetHintText(&swkbd, "No Japanese text allowed ¯\\_(ツ)_/¯");
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		}

		if (kDown & KEY_Y)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, -1);
			swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
			swkbdSetFilterCallback(&swkbd, MyCallback, NULL);
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		}

		if (didit)
		{
			if (button != SWKBD_BUTTON_NONE)
			{
				printf("You pressed button %d\n", button);
				printf("Text: %s\n", mybuf);
			} else
				printf("swkbd event: %d\n", swkbdGetResult(&swkbd));
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
