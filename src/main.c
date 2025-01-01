#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <debug.h>
#include <string.h>

#include "gckeybrd.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {

    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    printf("\n\nGC Keyboard Example\n");

    PAD_Init();

    int keyboardChan = 0;
    for (keyboardChan = 0; keyboardChan < 4; ++keyboardChan) {
		if (SI_GetType(keyboardChan) == SI_GC_KEYBOARD) {
            printf("GC Keyboard located at chan %d\n", keyboardChan);
            break;
		}
	}
	//Alternately you can use GCKB_ScanKybd() or GCKB_Read() to find the keyboard.
	//GCKB_ScanKybd() also handles errors.
    
    int keyboardEnabled = GCKB_Init();
    if (keyboardEnabled) {
        printf("GC Keyboard initialized on chan %d\n", keyboardChan);
    }

	//Keys are stored in a buffer of 3 bytes, one for each key.
	//Because of this, more than 3 keys pressed at once will not be recognized.
	u32 keys = 0;
    u32 keysLast = 0;
    u8* key = (u8*)&keys;
	u8* keyLast = (u8*)&keysLast;
	
    char output[256] = "";

    printf("Start of main loop.\nPress Start on the controller plugged into slot 1 to exit ...\n");
    while(1) {
        if (keyboardEnabled) {
			keys = GCKB_ReadKeys(keyboardChan);
			if (keys == 0) {
				continue;
			}
			//printf("keys pressed: 0x%02x, 0x%02x, 0x%02x\n", key[0], key[1], key[2]);
                
            //I believe that Shift keys can only be detected in keys[1] and keys[2],
			//and similarly alphanumeric keys can only be detected in keys[0] and keys[1].
			//Finally, keys[3] should not be accessed.
            int isShift = (key[1] == KEY_LEFTSHIFT || key[1] == KEY_RIGHTSHIFT
                || key[2] == KEY_LEFTSHIFT || key[2] == KEY_RIGHTSHIFT);
			char character = '\0';

            if (key[0] && key[0] != keyLast[0] && key[0] != keyLast[1]) {
                character = GCKB_GetMap(key[0], isShift);
            }
			else if (key[1] && key[1] != keyLast[0] && key[1] != keyLast[1]) {
				character = GCKB_GetMap(key[1], isShift);
			}

			if ((key[0] == KEY_BACKSPACE || key[1] == KEY_BACKSPACE || key[2] == KEY_BACKSPACE) &&
				keyLast[0] != KEY_BACKSPACE && keyLast[1] != KEY_BACKSPACE && keyLast[2] != KEY_BACKSPACE) {
                size_t len = strlen(output);
                if (len > 0) {
                    output[len - 1] = '\0';
                }
            }
            else {
                //Character to output. The good 'ol += in C.
                size_t len = strlen(output);
                if (character == '\0') {
                    //printf("unrecognized key: 0x%02x\n", key[0]);
                    continue;
                }
                output[len] = character;
                output[len + 1] = '\0';
            }

            printf("output: %s\n", output);
        }

		//Update last keys pressed so we can detect key release,
		//and potentially check when a key is held down.
		keysLast = keys;
		
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsDown(0);
        if (pressed)
            printf("PAD 0 buttons pressed: 0x%08x\n", pressed);

        if ( pressed & PAD_BUTTON_START ) {
            printf("exiting ...\n");
            exit(0);
        }

        VIDEO_WaitVSync();
    }

    return 0;
}
