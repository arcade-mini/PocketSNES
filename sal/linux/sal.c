#include <stdio.h>
#include <dirent.h>
#include <SDL.h>
#include <sys/time.h>
#include "sal.h"

#define PALETTE_BUFFER_LENGTH	256*2*4
#define SNES_WIDTH  256
#define SNES_HEIGHT 239

static SDL_Surface *mScreen = NULL;
static SDL_Surface *rs97Screen = NULL;
static u32 mSoundThreadFlag=0;
static u32 mSoundLastCpuSpeed=0;
static u32 mPaletteBuffer[PALETTE_BUFFER_LENGTH];
static u32 *mPaletteCurr=(u32*)&mPaletteBuffer[0];
static u32 *mPaletteLast=(u32*)&mPaletteBuffer[0];
static u32 *mPaletteEnd=(u32*)&mPaletteBuffer[PALETTE_BUFFER_LENGTH];
static u32 mInputFirst=0;

s32 mCpuSpeedLookup[1]={0};

#include <sal_common.h>

static u32 inputHeld[2] = {0, 0};
static SDL_Joystick* joy[2];

static u32 sal_Input(int held, u32 joynumb)
{
	SDL_Event event;
	u8 *keystate;
	
	inputHeld[joynumb] = 0;
	
	{
		if (joy[joynumb] == NULL)
			joy[joynumb] = SDL_JoystickOpen(joynumb);
				
		if (SDL_JoystickGetAxis(joy[joynumb], 0) < -300)
			inputHeld[joynumb] |= SAL_INPUT_LEFT;
		if (SDL_JoystickGetAxis(joy[joynumb], 0) > 300)
			inputHeld[joynumb] |= SAL_INPUT_RIGHT;
				
		if (SDL_JoystickGetAxis(joy[joynumb], 1) < -300)
			inputHeld[joynumb] |= SAL_INPUT_UP;
		if (SDL_JoystickGetAxis(joy[joynumb], 1) > 300)
			inputHeld[joynumb] |= SAL_INPUT_DOWN;
						
		if (SDL_JoystickGetButton(joy[joynumb], 2)) inputHeld[joynumb] |= SAL_INPUT_B;
		if (SDL_JoystickGetButton(joy[joynumb], 1)) inputHeld[joynumb] |= SAL_INPUT_A;
		if (SDL_JoystickGetButton(joy[joynumb], 0)) inputHeld[joynumb] |= SAL_INPUT_X;
		if (SDL_JoystickGetButton(joy[joynumb], 3)) inputHeld[joynumb] |= SAL_INPUT_Y;
		if (SDL_JoystickGetButton(joy[joynumb], 4)) inputHeld[joynumb] |= SAL_INPUT_L;
		if (SDL_JoystickGetButton(joy[joynumb], 5)) inputHeld[joynumb] |= SAL_INPUT_R;
		// For some reasons, there are no buttons labelled 6 or 7. they went from 5 to 8..
		if (SDL_JoystickGetButton(joy[joynumb], 8)) inputHeld[joynumb] |= SAL_INPUT_SELECT;
		if (SDL_JoystickGetButton(joy[joynumb], 9)) inputHeld[joynumb] |= SAL_INPUT_START;
		//
		if (SDL_JoystickGetButton(joy[joynumb], 6) || (SDL_JoystickGetButton(joy[joynumb], 8) && SDL_JoystickGetButton(joy[joynumb], 9))) inputHeld[joynumb] |= SAL_INPUT_MENU;
	}	

	if (joynumb == 0)
	{
		keystate = SDL_GetKeyState(NULL);
		if ( keystate[SDLK_LCTRL] )		inputHeld[joynumb] |= SAL_INPUT_A;
		if ( keystate[SDLK_LALT] )		inputHeld[joynumb] |= SAL_INPUT_B;
		if ( keystate[SDLK_SPACE] )		inputHeld[joynumb] |= SAL_INPUT_X;
		if ( keystate[SDLK_LSHIFT] )	inputHeld[joynumb] |= SAL_INPUT_Y;
		if ( keystate[SDLK_TAB] )		inputHeld[joynumb] |= SAL_INPUT_L;
		if ( keystate[SDLK_BACKSPACE] )	inputHeld[joynumb] |= SAL_INPUT_R;
		if ( keystate[SDLK_RETURN] )	inputHeld[joynumb] |= SAL_INPUT_START;
		if ( keystate[SDLK_ESCAPE] )	inputHeld[joynumb] |= SAL_INPUT_SELECT;
		if ( keystate[SDLK_UP] )		inputHeld[joynumb] |= SAL_INPUT_UP;
		if ( keystate[SDLK_DOWN] )		inputHeld[joynumb] |= SAL_INPUT_DOWN;
		if ( keystate[SDLK_LEFT] )		inputHeld[joynumb] |= SAL_INPUT_LEFT;
		if ( keystate[SDLK_RIGHT] )		inputHeld[joynumb] |= SAL_INPUT_RIGHT;
		if ( keystate[SDLK_ESCAPE] &&  keystate[SDLK_RETURN])		inputHeld[joynumb] |= SAL_INPUT_MENU;
	}
	
	if (!SDL_PollEvent(&event)) 
	{
		if (held)
			return inputHeld[joynumb];
		return 0;
	}
	
	return inputHeld[joynumb];
}

static int key_repeat_enabled = 1;

u32 sal_InputPollRepeat(u32 joynumb)
{
	if (!key_repeat_enabled) {
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		key_repeat_enabled = 1;
	}
	return sal_Input(0, joynumb);
}

u32 sal_InputPoll(u32 joynumb)
{
	if (key_repeat_enabled) {
		SDL_EnableKeyRepeat(0, 0);
		key_repeat_enabled = 0;
	}
	return sal_Input(1, joynumb);
}

const char* sal_DirectoryGetTemp(void)
{
	return "/tmp";
}

void sal_CpuSpeedSet(u32 mhz)
{
}

u32 sal_CpuSpeedNext(u32 currSpeed)
{
	u32 newSpeed=currSpeed+1;
	if(newSpeed > 500) newSpeed = 500;
	return newSpeed;
}

u32 sal_CpuSpeedPrevious(u32 currSpeed)
{
	u32 newSpeed=currSpeed-1;
	if(newSpeed > 500) newSpeed = 0;
	return newSpeed;
}

u32 sal_CpuSpeedNextFast(u32 currSpeed)
{
	u32 newSpeed=currSpeed+10;
	if(newSpeed > 500) newSpeed = 500;
	return newSpeed;
}

u32 sal_CpuSpeedPreviousFast(u32 currSpeed)
{
	u32 newSpeed=currSpeed-10;
	if(newSpeed > 500) newSpeed = 0;
	return newSpeed;
}

s32 sal_Init(void)
{
	if( SDL_Init( SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK ) == -1 )
	{
		return SAL_ERROR;
	}
	sal_TimerInit(60);

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	return SAL_OK;
}

u32 sal_VideoInit(u32 bpp)
{
	SDL_ShowCursor(0);
	
	mBpp=bpp;
	setenv("SDL_NOMOUSE", "1", 1);

	//Set up the screen
	// mScreen = SDL_SetVideoMode( 320, 480, bpp, SDL_HWSURFACE);
	rs97Screen = SDL_SetVideoMode(480, 272, 16, SDL_HWSURFACE | SDL_ANYFORMAT);
	mScreen = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_ANYFORMAT, SAL_SCREEN_WIDTH, SAL_SCREEN_HEIGHT, 16, 0, 0, 0, 0);
	//If there was an error in setting up the screen
	if( mScreen == NULL )
	{
		sal_LastErrorSet("SDL_SetVideoMode failed");        	
		return SAL_ERROR;
	}

	return SAL_OK;
}

u32 sal_VideoGetWidth()
{
	return mScreen->w;
}

u32 sal_VideoGetHeight()
{
	return mScreen->h;
}

u32 sal_VideoGetPitch()
{
	return rs97Screen->pitch;
}

void sal_VideoEnterGame(u32 fullscreenOption, u32 pal, u32 refreshRate)
{
	memset(rs97Screen->pixels, 0, 480*272*2);
}

void sal_VideoSetPAL(u32 fullscreenOption, u32 pal)
{
	if (fullscreenOption == 3) /* hardware scaling */
	{
		sal_VideoEnterGame(fullscreenOption, pal, mRefreshRate);
	}
}

void sal_VideoExitGame()
{
	memset(rs97Screen->pixels, 0, 480*272*2);
}

void sal_VideoBitmapDim(u16* img, u32 pixelCount)
{
	u32 i;
	for (i = 0; i < pixelCount; i += 2)
		*(u32 *) &img[i] = (*(u32 *) &img[i] & 0xF7DEF7DE) >> 1;
	if (pixelCount & 1)
		img[i - 1] = (img[i - 1] & 0xF7DE) >> 1;
}

void sal_VideoFlip(s32 vsync)
{
	uint32_t *s = (uint32_t*)mScreen->pixels;
	uint32_t *d = (uint32_t*)rs97Screen->pixels;// + (SAL_SCREEN_WIDTH - SNES_WIDTH) / 4;
	SDL_BlitSurface(mScreen, NULL, rs97Screen, NULL);
	//for(uint8_t y = 0; y < 240; y++, s += SAL_SCREEN_WIDTH/2, d += 320) memmove(d, s, SAL_SCREEN_WIDTH * 4);
	//SDL_Flip(rs97Screen);
}

void *sal_VideoGetBuffer()
{
	return (void*)mScreen->pixels;
}

void *sal_RS97VideoGetBuffer()
{
	return (void*)rs97Screen->pixels;
}

void sal_VideoPaletteSync() 
{ 	
	
} 

void sal_VideoPaletteSet(u32 index, u32 color)
{
	*mPaletteCurr++=index;
	*mPaletteCurr++=color;
	if(mPaletteCurr>mPaletteEnd) mPaletteCurr=&mPaletteBuffer[0];
}

void sal_Reset(void)
{
	for( uint8_t i=0; i < SDL_NumJoysticks(); i++ ) 
	{
		if (joy[i] == NULL)
		{
			SDL_JoystickClose(joy[i]);
		}
	}
	sal_AudioClose();
	SDL_Quit();
}

int mainEntry(int argc, char *argv[]);

// Prove entry point wrapper
int main(int argc, char *argv[])
{	
	return mainEntry(argc,argv);
//	return mainEntry(argc-1,&argv[1]);
}
