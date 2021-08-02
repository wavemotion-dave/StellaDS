// =====================================================================================================
// Stella DSi - Improved Version by Dave Bernazzani (wavemotion)
//
// See readme.txt for a list of everything that has changed in the baseline 1.0 code.
// =====================================================================================================
#include <nds.h>
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "ds_tools.h"

#include "bgBottom.h"
#include "bgTop.h"
#include "bgFileSel.h"
#include "bgPaddles.h"
#include "bgKeypad.h"
#include "bgStarRaiders.h"
#include "bgInfo.h"
#include "bgInstructions.h"

#include "clickNoQuit_wav.h"
#include "clickQuit_wav.h"

#include "Console.hxx"
#include "MediaSrc.hxx"
#include "TIASound.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"
#include "EventHandler.hxx"
#include "Cart.hxx"
#include "highscore.h"
#include "instructions.h"

#define VERSION "4.2"

//#define WRITE_TWEAKS

#define SOUND_SIZE (8192)
extern uInt8 sound_buffer[];  // Can't be placed in fast memory as ARM7 needs to access it...
extern uInt8 *psound_buffer;

#define MAX_RESISTANCE  1000000
#define MIN_RESISTANCE  80000

int atari_frames=0;

uInt8 tv_type_requested = NTSC;

#define MAX_DEBUG 8
Int32 debug[MAX_DEBUG]={0};
char DEBUG_DUMP = 0;
char my_filename[128];

FICA2600 vcsromlist[1200];
unsigned short int countvcs=0, ucFicAct=0;

static short bShowKeyboard = false;
static short bShowPaddles = false;
static short bShowInfo = false;

Console* theConsole = (Console*) NULL;

#define MAX_FILE_SIZE   (1024 * 64)
uInt8  filebuffer[MAX_FILE_SIZE];

int bg0, bg0b,bg1b;
unsigned int etatEmu;
bool fpsDisplay = false;

static int bSoundEnabled = 1;
static int full_speed=0;
int gTotalAtariFrames=0;

unsigned short int keys_pressed,last_keys_pressed,keys_touch=0, console_color=1, left_difficulty=0, right_difficulty=0,romSel;

extern Int8 ourPokeDelayTable[64];

#define WAITVBL swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank();

static void DumpDebugData(void)
{
    if (DEBUG_DUMP)
    {        
        char dbgbuf[36];
        int idx=0;
        int val=0;

        sprintf(dbgbuf, "%32s", myCartInfo.md5.c_str());                            dsPrintValue(0,2,0, dbgbuf);
        sprintf(dbgbuf, "Cart.Scale:     %03d", myCartInfo.screenScale);            dsPrintValue(1,3,0, dbgbuf);
        sprintf(dbgbuf, "Cart.xOffset:   %03d", myCartInfo.xOffset);                dsPrintValue(1,4,0, dbgbuf);
        sprintf(dbgbuf, "Cart.yOffset:   %03d", myCartInfo.yOffset);                dsPrintValue(1,5,0, dbgbuf);
        sprintf(dbgbuf, "Cart.startDisp: %03d", myCartInfo.displayStartScanline);   dsPrintValue(1,6,0, dbgbuf);
        sprintf(dbgbuf, "Cart.stopDisp:  %03d", myCartInfo.displayStopScanline);    dsPrintValue(1,7,0, dbgbuf);
        
        for (int i=0; i<MAX_DEBUG; i++)
        {
            idx=0;
            val = debug[i];
            dbgbuf[idx++] = 'R';
            dbgbuf[idx++] = '0' + (i / 10);
            dbgbuf[idx++] = '0' + (i % 10);
            dbgbuf[idx++] = ':';
            if (val < 0)
            {
                dbgbuf[idx++] = '-';
                val = -val;
            }
            else
            {
                dbgbuf[idx++] = '0' + (int)val/100000;
            }
            val = val % 100000;
            dbgbuf[idx++] = '0' + (int)val/10000;
            val = val % 10000;
            dbgbuf[idx++] = '0' + (int)val/1000;
            val= val % 1000;
            dbgbuf[idx++] = '0' + (int)val/100;
            val = val % 100;
            dbgbuf[idx++] = '0' + (int)val/10;
            dbgbuf[idx++] = '0' + (int)val%10;
            dbgbuf[idx++] = 0;

            if (i > 22)
                dsPrintValue(22,9+(i-23),0, dbgbuf);
            else if (i > 11)
                dsPrintValue(11,9+(i-12),0, dbgbuf);
            else dsPrintValue(0,9+i,0, dbgbuf);
        }
    }
}


// --------------------------------------------------------------------------------------
// Color fading effect
void FadeToColor(unsigned char ucSens, unsigned short ucBG, unsigned char ucScr, unsigned char valEnd, unsigned char uWait) 
{
  unsigned short ucFade;
  unsigned char ucBcl;

  // Fade-out vers le noir
  if (ucScr & 0x01) REG_BLDCNT=ucBG;
  if (ucScr & 0x02) REG_BLDCNT_SUB=ucBG;
  if (ucSens == 1) {
    for(ucFade=0;ucFade<valEnd;ucFade++) {
      if (ucScr & 0x01) REG_BLDY=ucFade;
      if (ucScr & 0x02) REG_BLDY_SUB=ucFade;
      for (ucBcl=0;ucBcl<uWait;ucBcl++) {
        swiWaitForVBlank();
      }
    }
  }
  else {
    for(ucFade=16;ucFade>valEnd;ucFade--) {
      if (ucScr & 0x01) REG_BLDY=ucFade;
      if (ucScr & 0x02) REG_BLDY_SUB=ucFade;
      for (ucBcl=0;ucBcl<uWait;ucBcl++) {
        swiWaitForVBlank();
      }
    }
  }
}

inline void ShowStatusLine(void)
{
    if (myCartInfo.tv_type == PAL)
    {
        dsPrintValue(29,0,0, (full_speed ? (char *)" FS" : (char *)"PAL"));
    }
    else
    {
        dsPrintValue(29,0,0, (full_speed ? (char *)" FS" : (char *)"   "));
    }
}


char bScreenRefresh = 0;
void vblankIntr() 
{
    if (bScreenRefresh)
    {
        REG_BG3PD = ((100 / myCartInfo.screenScale)  << 8) | (100 % myCartInfo.screenScale);
        REG_BG3Y = (myCartInfo.yOffset)<<8;
        REG_BG3X = (myCartInfo.xOffset)<<8;
        bScreenRefresh = 0;
    }
}

// --------------------------------------------------------------------------------------
void dsInitScreenMain(void)
{
    // Init vbl and hbl func
    SetYtrigger(190); //trigger 2 lines before vsync
    irqSet(IRQ_VBLANK, vblankIntr);
    irqEnable(IRQ_VBLANK);
    vramSetBankE(VRAM_E_LCD );                // Not using this for video but 64K of faster RAM always useful! Mapped at 0x06880000
    WAITVBL;
}

ITCM_CODE void dsInitTimer(void)
{
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
}

ITCM_CODE void dsInitPalette(void) 
{
    // Init DS Specific palette
    const uInt32* gamePalette = theConsole->myMediaSource->palette();
    for(uInt32 i = 0; i < 256; i++)   
    {
        uInt8 r, g, b;

        r = (uInt8) ((gamePalette[i] & 0x00ff0000) >> 19);
        g = (uInt8) ((gamePalette[i] & 0x0000ff00) >> 11);
        b = (uInt8) ((gamePalette[i] & 0x000000ff) >> 3);

        BG_PALETTE[i]=RGB15(r,g,b);
    }
}

ITCM_CODE void dsWarnIncompatibileCart(void)
{
    dsPrintValue(5,0,0, (char*)"DPC+ CART NOT SUPPORTED");
}

ITCM_CODE void dsPrintCartType(char * type)
{
    if (DEBUG_DUMP)
    {
        dsPrintValue(16-(strlen(type)/2),0,0, (char*)type);
    }
}

ITCM_CODE void dsWriteTweaks(void)
{
#ifdef WRITE_TWEAKS
    FILE *fp;
    dsPrintValue(22,0,0, (char*)"CFG");
    fp = fopen("../StellaDS.txt", "a+");
    if (fp != NULL)
    {
        fprintf(fp, "%-32s %4s %2s    %3d  %3d  %3d  %3d  %3d  %s\n", myCartInfo.md5.c_str(), myCartInfo.type.c_str(), 
                (myCartInfo.mode == MODE_FF ? "FF":"NO"), myCartInfo.displayStartScanline, myCartInfo.displayStopScanline, 
                myCartInfo.screenScale, myCartInfo.xOffset, myCartInfo.yOffset, my_filename);
        fflush(fp);
        fclose(fp);
    }
    WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
    dsPrintValue(22,0,0, (char*)"   ");
#endif    
}

void dsShowScreenEmu(void)
{
  videoSetMode(MODE_5_2D);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);  // The main emulated (top screen) display.
  vramSetBankB(VRAM_B_MAIN_BG_0x06060000);  // This is where we will put our frame buffers to aid DMA Copy routines...
    
  bg0 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0,0);
  memset((void*)0x06000000, 0x00, 128*1024);

  REG_BG3PA = ((A26_VID_WIDTH / 256) << 8) | (A26_VID_WIDTH % 256) ;
  REG_BG3PB = 0; REG_BG3PC = 0;
  REG_BG3PD = ((100 / myCartInfo.screenScale)  << 8) | (100 % myCartInfo.screenScale) ;
  REG_BG3X = (myCartInfo.xOffset)<<8;
  REG_BG3Y = (myCartInfo.yOffset)<<8;

  ShowStatusLine(); 
    
  bScreenRefresh = 1;
  swiWaitForVBlank();
}

void dsShowScreenInfo(void) 
{
  decompress(bgInfoTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgInfoMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgInfoPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
  swiWaitForVBlank();
}


void dsShowScreenPaddles(void) 
{
  decompress(bgPaddlesTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgPaddlesMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgPaddlesPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
  swiWaitForVBlank();
}

void dsShowScreenKeypad(void) 
{
  if (myCartInfo.controllerType == CTR_STARRAID)
  {
    decompress(bgStarRaidersTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgStarRaidersMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgStarRaidersPal,(u16*) BG_PALETTE_SUB,256*2);
  }
  else
  {
    decompress(bgKeypadTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgKeypadMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgKeypadPal,(u16*) BG_PALETTE_SUB,256*2);
  }
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
  swiWaitForVBlank();
}

void dsDisplayButton(unsigned char button)
{
  unsigned short *ptrBg1 = bgGetMapPtr(bg0b) +32*26;
  unsigned short *ptrBg0 = bgGetMapPtr(bg0b);
  unsigned int i;

  switch (button) {
    case 0: // ON/OFF
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+4) = *(ptrBg1+(0+i)*32+0);
        *(ptrBg0+(4+i)*32+5) = *(ptrBg1+(0+i)*32+1);
      }
      break;
    case 1: // ON/OFF
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+4) = *(ptrBg1+(0+i)*32+2);
        *(ptrBg0+(4+i)*32+5) = *(ptrBg1+(0+i)*32+3);
      }
      break;
    case 2: // BW/Color
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+9) = *(ptrBg1+(0+i)*32+4);
        *(ptrBg0+(4+i)*32+10) = *(ptrBg1+(0+i)*32+5);
      }
      break;
    case 3: // BW/Color
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+9) = *(ptrBg1+(0+i)*32+6);
        *(ptrBg0+(4+i)*32+10) = *(ptrBg1+(0+i)*32+7);
      }
      break;

    case 4: // Select
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+22) = *(ptrBg1+(0+i)*32+8);
        *(ptrBg0+(4+i)*32+23) = *(ptrBg1+(0+i)*32+9);
      }
      break;
    case 5: // Select
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+22) = *(ptrBg1+(0+i)*32+10);
        *(ptrBg0+(4+i)*32+23) = *(ptrBg1+(0+i)*32+11);
      }
      break;
    case 6: // Reset
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+27) = *(ptrBg1+(0+i)*32+12);
        *(ptrBg0+(4+i)*32+28) = *(ptrBg1+(0+i)*32+13);
      }
      break;
    case 7: // Reset
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+27) = *(ptrBg1+(0+i)*32+14);
        *(ptrBg0+(4+i)*32+28) = *(ptrBg1+(0+i)*32+15);
      }
      break;
    case 8: // PAL<> NTSC
      // No longer used...
      break;
    case 9: // PAL<> NTSC
      // No longer used...
      break;

    case 10: // Left Difficulty - B
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+14) = *(ptrBg1+(0+i)*32+4);
        *(ptrBg0+(4+i)*32+15) = *(ptrBg1+(0+i)*32+5);
      }
      break;
    case 11: // Left Difficulty - A
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+14) = *(ptrBg1+(0+i)*32+6);
        *(ptrBg0+(4+i)*32+15) = *(ptrBg1+(0+i)*32+7);
      }
      break;

    case 12: // Right Difficulty - B
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+17) = *(ptrBg1+(0+i)*32+8);
        *(ptrBg0+(4+i)*32+18) = *(ptrBg1+(0+i)*32+9);
      }
      break;
    case 13: // Right Difficulty - A
      for (i=0;i<4;i++) {
        *(ptrBg0+(4+i)*32+17) = *(ptrBg1+(0+i)*32+10);
        *(ptrBg0+(4+i)*32+18) = *(ptrBg1+(0+i)*32+11);
      }
      break;

    case 14: // Flicker Free - ON
      for (i=0;i<4;i++) {
        *(ptrBg0+(22)*32+(27+i)) = *(ptrBg1+(2)*32+(16+i));
        *(ptrBg0+(23)*32+(27+i)) = *(ptrBg1+(3)*32+(16+i));
      }
      break;
    case 15: // Flicker Free - OFF (normal mode)
      for (i=0;i<4;i++) {
        *(ptrBg0+(22)*32+(27+i)) = *(ptrBg1+(0)*32+(16+i));
        *(ptrBg0+(23)*32+(27+i)) = *(ptrBg1+(1)*32+(16+i));
      }
      break;
          
    case 16: // Sound - ON
      for (i=0;i<4;i++) {
        *(ptrBg0+(19)*32+(27+i)) = *(ptrBg1+(2)*32+(16+i));
        *(ptrBg0+(20)*32+(27+i)) = *(ptrBg1+(3)*32+(16+i));
      }
      break;
    case 17: // Sound - OFF
      for (i=0;i<4;i++) {
        *(ptrBg0+(19)*32+(27+i)) = *(ptrBg1+(0)*32+(16+i));
        *(ptrBg0+(20)*32+(27+i)) = *(ptrBg1+(1)*32+(16+i));
      }
      break;          
  }
  swiWaitForVBlank();
}

void dsShowScreenMain(bool bFull) 
{
  // Init BG mode for 16 bits colors
  if (bFull)
  {
      videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE );
      videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);
      vramSetBankA(VRAM_A_MAIN_BG); vramSetBankC(VRAM_C_SUB_BG);
      bg0 = bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 31,0);
      bg0b = bgInitSub(0, BgType_Text8bpp, BgSize_T_256x256, 31,0);
      bg1b = bgInitSub(1, BgType_Text8bpp, BgSize_T_256x256, 30,0);
      bgSetPriority(bg0b,1);bgSetPriority(bg1b,0);

      decompress(bgTopTiles, bgGetGfxPtr(bg0), LZ77Vram);
      decompress(bgTopMap, (void*) bgGetMapPtr(bg0), LZ77Vram);
      dmaCopy((void *) bgTopPal,(u16*) BG_PALETTE,256*2);
  }

  decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

  REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;
    
  dsDisplayButton(3-console_color);
  dsDisplayButton(10+left_difficulty);
  dsDisplayButton(12+right_difficulty);
  dsDisplayButton(14+(myCartInfo.mode == MODE_NO ? 1:0));
  dsDisplayButton(16+bSoundEnabled);
  ShowStatusLine();    

  swiWaitForVBlank();
}


void dsFreeEmu(void) 
{
  // Stop timer of sound
  TIMER2_CR=0; irqDisable(IRQ_TIMER2);

  if (theConsole)
    delete theConsole;
}

bool dsLoadGame(char *filename) 
{
  unsigned int buffer_size=0;
  strncpy(my_filename, filename, 127);
  my_filename[127] = 0;
    
  // Load the file
  FILE *romfile = fopen(filename, "r");
  if (romfile != NULL)
  {
    // Free buffer if needed
    TIMER2_CR=0; irqDisable(IRQ_TIMER2);

    if (theConsole)
      delete theConsole;

    fseek(romfile, 0, SEEK_END);
    buffer_size = ftell(romfile);
    if (buffer_size < MAX_FILE_SIZE)
    {
        rewind(romfile);
        fread(filebuffer, buffer_size, 1, romfile);
        fclose(romfile);

        // Init the emulation
        theConsole = new Console((const uInt8*) filebuffer, buffer_size, "noname");
        dsInitPalette();

        left_difficulty=0; right_difficulty=0;

        psound_buffer=sound_buffer;
        memset(sound_buffer, 0x00, SOUND_SIZE);
        TIMER2_DATA = TIMER_FREQ(22050);
        TIMER2_CR = TIMER_DIV_1 | TIMER_IRQ_REQ | TIMER_ENABLE;
        irqSet(IRQ_TIMER2, Tia_process);
        if (bSoundEnabled)
        {
            irqEnable(IRQ_TIMER2);
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }
        else
        {
            irqDisable(IRQ_TIMER2);
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
        }

        // Center all paddles...
        theConsole->fakePaddleResistance = ((MAX_RESISTANCE+MIN_RESISTANCE)/2);
        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DELETE, theConsole->fakePaddleResistance);
        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F11,    theConsole->fakePaddleResistance);
        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F9,     theConsole->fakePaddleResistance);
        
        TIMER0_CR=0;
        TIMER0_DATA=0;
        TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
        atari_frames=0;
        memset(debug, 0x00, sizeof(debug));
        
        return true;
    }
    else return false;
  }
  return false;
}

unsigned int dsReadPad(void)
{
    unsigned int keys_pressed, ret_keys_pressed;

    do {
        keys_pressed = keysCurrent();
    } while ((keys_pressed & (KEY_LEFT | KEY_RIGHT | KEY_DOWN | KEY_UP | KEY_A | KEY_B | KEY_L | KEY_R))==0);
    ret_keys_pressed = keys_pressed;

    do {
        keys_pressed = keysCurrent();
    } while ((keys_pressed & (KEY_LEFT | KEY_RIGHT | KEY_DOWN | KEY_UP | KEY_A | KEY_B | KEY_L | KEY_R))!=0);

    return ret_keys_pressed;
}

bool dsWaitOnQuit(void)
{
  bool bRet=false, bDone=false;
  unsigned int keys_pressed;
  unsigned int posdeb=0;
  char szName[32];

  decompress(bgFileSelTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgFileSelMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgFileSelPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

  strcpy(szName,"Quit StellaDS?");
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  sprintf(szName,"%s","A TO CONFIRM, B TO GO BACK");
  dsPrintValue(16-strlen(szName)/2,23,0,szName);

  while(!bDone)
  {
    strcpy(szName,"          YES          ");
    dsPrintValue(5,10+0,(posdeb == 0 ? 1 :  0),szName);
    strcpy(szName,"          NO           ");
    dsPrintValue(5,14+1,(posdeb == 2 ? 1 :  0),szName);
    swiWaitForVBlank();

    // Check pad
    keys_pressed=dsReadPad();
    if (keys_pressed & KEY_UP) {
      if (posdeb) posdeb-=2;
    }
    if (keys_pressed & KEY_DOWN) {
      if (posdeb<1) posdeb+=2;
    }
    if (keys_pressed & KEY_A) {
      bRet = (posdeb ? false : true);
      bDone = true;
    }
    if (keys_pressed & KEY_B) {
      bDone = true;
    }
  }
    
  dsShowScreenMain(false);

  return bRet;
}

char szName[256];
char szName2[256];
void dsDisplayFiles(unsigned int NoDebGame,u32 ucSel)
{
  unsigned int ucBcl,ucGame;

  // Display all games if possible
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) (bgGetMapPtr(bg1b)),32*24*2);
  sprintf(szName,"%04d/%04d GAMES",(int)(1+ucSel+NoDebGame),countvcs);
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  dsPrintValue(31,5,0,(char *) (NoDebGame>0 ? "<" : " "));
  dsPrintValue(31,22,0,(char *) (NoDebGame+14<countvcs ? ">" : " "));
  sprintf(szName,"%s [%s]","A=CHOOSE, Y=PAL, B=GO BACK", VERSION);
  dsPrintValue(16-strlen(szName)/2,23,0,szName);
  for (ucBcl=0;ucBcl<17; ucBcl++) {
    ucGame= ucBcl+NoDebGame;
    if (ucGame < countvcs)
    {
      strcpy(szName,vcsromlist[ucGame].filename);
      szName[29]='\0';
      if (vcsromlist[ucGame].directory)
      {
        szName[27]='\0';
        sprintf(szName2,"[%s]",szName);
        dsPrintValue(0,5+ucBcl,(ucSel == ucBcl ? 1 :  0),szName2);
      }
      else
      {
        dsPrintValue(1,5+ucBcl,(ucSel == ucBcl ? 1 : 0),szName);
      }
    }
  }
}



unsigned int dsWaitForRom(void)
{
  bool bDone=false, bRet=false;
  u32 ucHaut=0x00, ucBas=0x00,ucSHaut=0x00, ucSBas=0x00,romSelected= 0, firstRomDisplay=0,nbRomPerPage, uNbRSPage;
  u32 uLenFic=0, ucFlip=0, ucFlop=0;

  decompress(bgFileSelTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgFileSelMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgFileSelPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

  nbRomPerPage = (countvcs>=17 ? 17 : countvcs);
  uNbRSPage = (countvcs>=5 ? 5 : countvcs);
  if (ucFicAct>countvcs-nbRomPerPage)
  {
    firstRomDisplay=countvcs-nbRomPerPage;
    romSelected=ucFicAct-countvcs+nbRomPerPage;
  }
  else
  {
    firstRomDisplay=ucFicAct;
    romSelected=0;
  }
  dsDisplayFiles(firstRomDisplay,romSelected);
  while (!bDone)
  {
    if (keysCurrent() & KEY_UP)
    {
      if (!ucHaut)
      {
        ucFicAct = (ucFicAct>0 ? ucFicAct-1 : countvcs-1);
        if (romSelected>uNbRSPage) { romSelected -= 1; }
        else {
          if (firstRomDisplay>0) { firstRomDisplay -= 1; }
          else {
            if (romSelected>0) { romSelected -= 1; }
            else {
              firstRomDisplay=countvcs-nbRomPerPage;
              romSelected=nbRomPerPage-1;
            }
          }
        }
        ucHaut=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else {

        ucHaut++;
        if (ucHaut>10) ucHaut=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else
    {
      ucHaut = 0;
    }
    if (keysCurrent() & KEY_DOWN)
    {
      if (!ucBas) {
        ucFicAct = (ucFicAct< countvcs-1 ? ucFicAct+1 : 0);
        if (romSelected<uNbRSPage-1) { romSelected += 1; }
        else {
          if (firstRomDisplay<countvcs-nbRomPerPage) { firstRomDisplay += 1; }
          else {
            if (romSelected<nbRomPerPage-1) { romSelected += 1; }
            else {
              firstRomDisplay=0;
              romSelected=0;
            }
          }
        }
        ucBas=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucBas++;
        if (ucBas>10) ucBas=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucBas = 0;
    }
    if ((keysCurrent() & KEY_R) || (keysCurrent() & KEY_RIGHT))
    {
      if (!ucSBas)
      {
        ucFicAct = (ucFicAct< countvcs-nbRomPerPage ? ucFicAct+nbRomPerPage : countvcs-nbRomPerPage);
        if (firstRomDisplay<countvcs-nbRomPerPage) { firstRomDisplay += nbRomPerPage; }
        else { firstRomDisplay = countvcs-nbRomPerPage; }
        if (ucFicAct == countvcs-nbRomPerPage) romSelected = 0;
        ucSBas=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucSBas++;
        if (ucSBas>10) ucSBas=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucSBas = 0;
    }
    if ((keysCurrent() & KEY_L) || (keysCurrent() & KEY_LEFT))
    {
      if (!ucSHaut)
      {
        ucFicAct = (ucFicAct> nbRomPerPage ? ucFicAct-nbRomPerPage : 0);
        if (firstRomDisplay>nbRomPerPage) { firstRomDisplay -= nbRomPerPage; }
        else { firstRomDisplay = 0; }
        if (ucFicAct == 0) romSelected = 0;
        if (romSelected > ucFicAct) romSelected = ucFicAct;          
        ucSHaut=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucSHaut++;
        if (ucSHaut>10) ucSHaut=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucSHaut = 0;
    }

    if ( keysCurrent() & KEY_B )
    {
      bDone=true;
      while (keysCurrent() & KEY_B);
    }

    if (keysCurrent() & KEY_A || keysCurrent() & KEY_Y)
    {
      if (!vcsromlist[ucFicAct].directory)
      {
        bRet=true;
        bDone=true;
        if (keysCurrent() & KEY_Y) 
        {
            tv_type_requested = PAL;
            myCartInfo.tv_type = PAL;
        }
        else
        {
            tv_type_requested = NTSC;
            myCartInfo.tv_type = NTSC;
        }
       
        if (keysCurrent() & KEY_X)
        {
            DEBUG_DUMP = 1;
        }
        else
        {
            DEBUG_DUMP = 0;   
        }
      }
      else
      {
        chdir(vcsromlist[ucFicAct].filename);
        vcsFindFiles();
        ucFicAct = 0;
        nbRomPerPage = (countvcs>=16 ? 16 : countvcs);
        uNbRSPage = (countvcs>=5 ? 5 : countvcs);
        if (ucFicAct>countvcs-nbRomPerPage) {
          firstRomDisplay=countvcs-nbRomPerPage;
          romSelected=ucFicAct-countvcs+nbRomPerPage;
        }
        else {
          firstRomDisplay=ucFicAct;
          romSelected=0;
        }
        dsDisplayFiles(firstRomDisplay,romSelected);
        while (keysCurrent() & KEY_A);
      }
    }
      
    // If the filename is too long... scroll it.
    if (strlen(vcsromlist[ucFicAct].filename) > 29) 
    {
      ucFlip++;
      if (ucFlip >= 15) 
      {
        ucFlip = 0;
        uLenFic++;
        if ((uLenFic+29)>strlen(vcsromlist[ucFicAct].filename)) 
        {
          ucFlop++;
          if (ucFlop >= 15) 
          {
            uLenFic=0;
            ucFlop = 0;
          }
          else
            uLenFic--;
        }
        strncpy(szName,vcsromlist[ucFicAct].filename+uLenFic,29);
        szName[29] = '\0';
        dsPrintValue(1,5+romSelected,1,szName);
      }
    }
      
    swiWaitForVBlank();
  }

  dsShowScreenMain(false);

  return bRet;
}

unsigned int dsWaitOnMenu(unsigned int actState)
{
  unsigned int uState=STELLADS_PLAYINIT;
  unsigned int keys_pressed;
  bool bDone=false, romSel;
  int iTx,iTy;

  while (!bDone)
  {
    // wait for stylus
    keys_pressed = keysCurrent();
    if (keys_pressed & KEY_TOUCH) {
      touchPosition touch;
      touchRead(&touch);
      iTx = touch.px;
      iTy = touch.py;
      if ((iTx>10) && (iTx<40) && (iTy>26) && (iTy<65)) { // 24,36  -> 38,64   quit
        dsDisplayButton(1);
        bDone=dsWaitOnQuit();
        if (bDone) uState=STELLADS_QUITSTDS;
      }
      if ((iTx>47) && (iTx<209) && (iTy>99) && (iTy<133)) {     // 48,100 -> 208,132 cartridge slot
        bDone=true;
        // Find files in current directory and show it
        vcsFindFiles();
        romSel=dsWaitForRom();
        if (romSel)
        {
          uState=STELLADS_PLAYINIT;
          bDone = dsLoadGame(vcsromlist[ucFicAct].filename);
        }
        else { uState=actState; }
      }
    }
    swiWaitForVBlank();
  }

  return uState;
}

ITCM_CODE void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr)
{
  u16 *pusEcran,*pusMap;
  u16 usCharac;
  char *pTrTxt=pchStr;
  char ch;

  pusEcran=(u16*) (bgGetMapPtr(bg1b))+x+(y<<5);
  pusMap=(u16*) (bgGetMapPtr(bg0b)+(2*isSelect+24)*32);

  while((*pTrTxt)!='\0' )
  {
    ch = *pTrTxt;
    if (ch >= 'a' && ch <= 'z') ch -= 32; // Faster than strcpy/strtoupper
    usCharac=0x0000;
    if ((ch) == '|')
      usCharac=*(pusMap);
    else if (((ch)<' ') || ((ch)>'_'))
      usCharac=*(pusMap);
    else if((ch)<'@')
      usCharac=*(pusMap+(ch)-' ');
    else
      usCharac=*(pusMap+32+(ch)-'@');
    *pusEcran++=usCharac;
    pTrTxt++;
  }
}

//---------------------------------------------------------------------------------
void dsInstallSoundEmuFIFO(void)
{
    FifoMessage msg;
    msg.SoundPlay.data = &sound_buffer;
    msg.SoundPlay.freq = 22050;
    msg.SoundPlay.volume = 127;
    msg.SoundPlay.pan = 64;
    msg.SoundPlay.loop = 1;
    msg.SoundPlay.format = ((1)<<4) | SoundFormat_8Bit;
    msg.SoundPlay.loopPoint = 0;
    msg.SoundPlay.dataSize = SOUND_SIZE >> 2;
    msg.type = EMUARM7_PLAY_SND;
    fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
}


char fpsbuf[32];
int iTx,iTy;
uInt8 mca_dampen_y=0;
uInt8 mca_dampen_a=0;
uInt8 rapid_fire = 0;
ITCM_CODE void dsMainLoop(void)
{
    static int dampen=0;
    static int info_dampen=0;

    last_keys_pressed = -1;
    full_speed = 0;
    fpsDisplay = 0;

    // Timers are fed with 33.513982 MHz clock.
    // With DIV_1024 the clock is 32,728.5 ticks per sec...
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
    TIMER1_DATA=0;
    TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
    
    while(etatEmu != STELLADS_QUITSTDS)
    {
        switch (etatEmu)
        {
        case STELLADS_MENUINIT:
            dsShowScreenMain(true);
            etatEmu = STELLADS_MENUSHOW;
            break;

        case STELLADS_MENUSHOW:
            etatEmu = dsWaitOnMenu(STELLADS_MENUSHOW);
            break;

        case STELLADS_PLAYINIT:
            dsShowScreenEmu();
            dsDisplayButton(14+(myCartInfo.mode == MODE_NO ? 1:0));
            etatEmu = STELLADS_PLAYGAME;
            break;

        case STELLADS_PLAYGAME:
            // 32,728.5 ticks = 1 second
            // 1 frame = 1/50 or 1/60 (0.02 or 0.016)
            // 655 -> 50 fps and 546 -> 60 fps
            if (!full_speed)
            {
                while(TIMER0_DATA < ((myCartInfo.tv_type ? 655:546)*atari_frames))
                    ;
            }

            // Have we processed 60 frames... start over...
            if (++atari_frames == (myCartInfo.tv_type ? 50:60))
            {
                TIMER0_CR=0;
                TIMER0_DATA=0;
                TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
                atari_frames=0;
            }            
                
            // Wait for keys
            scanKeys();
            keys_pressed = keysCurrent();

            switch (myCartInfo.controllerType)
            {
                case CTR_LJOY:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SPACE, ((keys_pressed & (KEY_A)) | (keys_pressed & (KEY_B))));
                    if (keys_pressed & (KEY_Y))
                    {
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SPACE, ++rapid_fire & 0x08);
                    }
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_UP,    keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DOWN,  keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_RIGHT));
                    break;
                    
                case CTR_MCA:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_g, (keys_pressed & (KEY_Y)));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_h, (keys_pressed & (KEY_B)));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_j, (keys_pressed & (KEY_A)));
                    
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_UP,    keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DOWN,  keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_RIGHT));
                    break;

                case CTR_STARGATE:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SPACE, ((keys_pressed & (KEY_A))));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_UP,    keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DOWN,  keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_RIGHT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_y, keys_pressed & (KEY_X));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_h, keys_pressed & (KEY_Y));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_f, keys_pressed & (KEY_B));
                    // Unfortunately for Stargate, we can't use these keys for UI handling below...
                    if ((keys_pressed & (KEY_X)) || (keys_pressed & (KEY_Y)))
                    {
                        keys_pressed = 0;
                    }
                    break;
                    
                case CTR_SOLARIS:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SPACE, ((keys_pressed & (KEY_A)) | (keys_pressed & (KEY_B))));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_UP,    keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DOWN,  keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_RIGHT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_f, keys_pressed & (KEY_Y));
                    if (keys_pressed & (KEY_Y))
                    {
                        keys_pressed = 0;
                    }
                    break;
                    
                case CTR_RJOY:
                case CTR_RAIDERS:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_f, keys_pressed & (KEY_A));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_y, keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_h, keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_g, keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_j, keys_pressed & (KEY_RIGHT));

                    // For Raiders of the Lost Ark!
                    if (myCartInfo.controllerType == CTR_RAIDERS)
                    {
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_X));
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_Y));
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SPACE, keys_pressed & (KEY_B));

                        // Unfortunately for Raiders, we can't use these keys for UI handling below...
                        if ((keys_pressed & (KEY_X)) || (keys_pressed & (KEY_Y)))
                        {
                            keys_pressed = 0;
                        }
                    }
                    break;
                    
                case CTR_BOOSTER:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_4, (keys_pressed & (KEY_A)));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_5, (keys_pressed & (KEY_B)));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_UP,    keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DOWN,  keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_RIGHT));
                    break;
                    
                case CTR_GENESIS:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SPACE, (keys_pressed & (KEY_A)) | (keys_pressed & (KEY_Y))); // Normal fire button
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_5,     (keys_pressed & (KEY_B)));    // Second Genesis button "C"
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_UP,    keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DOWN,  keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_RIGHT));
                    break;                    
                    
                case CTR_STARRAID:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SPACE, ((keys_pressed & (KEY_A)) | (keys_pressed & (KEY_B))));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_UP,    keys_pressed & (KEY_UP));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_DOWN,  keys_pressed & (KEY_DOWN));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_LEFT,  keys_pressed & (KEY_LEFT));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_RIGHT, keys_pressed & (KEY_RIGHT));
                    if (bShowKeyboard  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        keys_touch = 1;

                        if (touch.px > 60  && touch.px < 105 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_8, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_9, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_0, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_i, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_o, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_p, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_k, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_l, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SEMICOLON, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_COMMA, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_PERIOD, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SLASH, 1);
                    }
                    else
                    {
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_8, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_9, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_0, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_i, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_o, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_p, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_k, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_l, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SEMICOLON, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_COMMA, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_PERIOD, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SLASH, 0);
                    }   
                    break;
                
                case CTR_BUMPBASH:
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F12, (keys_pressed & (KEY_A)) || (keys_pressed & (KEY_R)));
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_END, (keys_pressed & (KEY_B)) || (keys_pressed & (KEY_L)));
                    break;
                    
                case CTR_PADDLE0:
                case CTR_PADDLE1:
                    if(keys_pressed & (KEY_LEFT))
                    {
                        theConsole->fakePaddleResistance += (10000 * myCartInfo.analogSensitivity) / 10;
                        if (theConsole->fakePaddleResistance > MAX_RESISTANCE) theConsole->fakePaddleResistance = MAX_RESISTANCE;
                        theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE0 ? StellaEvent::KCODE_DELETE:StellaEvent::KCODE_F11), theConsole->fakePaddleResistance);
                    }
                    else
                    if(keys_pressed & (KEY_RIGHT))
                    {
                        theConsole->fakePaddleResistance -= (10000 * myCartInfo.analogSensitivity) / 10;
                        if (theConsole->fakePaddleResistance < MIN_RESISTANCE) theConsole->fakePaddleResistance = MIN_RESISTANCE;
                        theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE0 ? StellaEvent::KCODE_DELETE:StellaEvent::KCODE_F11), theConsole->fakePaddleResistance);
                    }
                    else
                    if (bShowPaddles  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        theConsole->fakePaddleResistance = ((MIN_RESISTANCE+MAX_RESISTANCE) - ((MAX_RESISTANCE / 255) * touch.px));
                        keys_touch = 1;
                        theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE0 ? StellaEvent::KCODE_DELETE:StellaEvent::KCODE_F11), theConsole->fakePaddleResistance);
                    }

                    theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE0 ? StellaEvent::KCODE_END:StellaEvent::KCODE_F12), (keys_pressed & (KEY_A)) || (keys_pressed & (KEY_B)) ||
                                                                                      (keys_pressed & (KEY_R)) || (keys_pressed & (KEY_L)));      // RIGHT is the Paddle Button... either A or B will trigger this on Paddle Games

                    if ((keys_pressed & (KEY_A)) || (keys_pressed & (KEY_B)) || (keys_pressed & (KEY_R)) || (keys_pressed & (KEY_L)) || (keys_pressed & (KEY_LEFT)) || (keys_pressed & (KEY_RIGHT)))
                    {
                        if (!((keys_pressed & (KEY_R)) || (keys_pressed & (KEY_L))))  // We want to still process for L/R shoulder buttons for possible screen scaling...
                        {
                            keys_pressed = 0;   // If these were pressed... don't handle them below...
                        }
                    }
                    break;
                    
                case CTR_PADDLE2:
                case CTR_PADDLE3:
                    if(keys_pressed & (KEY_LEFT))
                    {
                        theConsole->fakePaddleResistance += (10000 * myCartInfo.analogSensitivity) / 10;
                        if (theConsole->fakePaddleResistance > MAX_RESISTANCE) theConsole->fakePaddleResistance = MAX_RESISTANCE;
                        theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE2 ? StellaEvent::KCODE_F9:StellaEvent::KCODE_F13), theConsole->fakePaddleResistance);
                    }
                    else
                    if(keys_pressed & (KEY_RIGHT))
                    {
                        theConsole->fakePaddleResistance -= (10000 * myCartInfo.analogSensitivity) / 10;
                        if (theConsole->fakePaddleResistance < MIN_RESISTANCE) theConsole->fakePaddleResistance = MIN_RESISTANCE;
                        theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE2 ? StellaEvent::KCODE_F9:StellaEvent::KCODE_F13), theConsole->fakePaddleResistance);
                    }
                    else
                    if (bShowPaddles  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        theConsole->fakePaddleResistance = ((MIN_RESISTANCE+MAX_RESISTANCE) - ((MAX_RESISTANCE / 255) * touch.px));
                        keys_touch = 1;
                        theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE2 ? StellaEvent::KCODE_F9:StellaEvent::KCODE_F13), theConsole->fakePaddleResistance);
                    }

                    theConsole->eventHandler().sendKeyEvent((myCartInfo.controllerType == CTR_PADDLE2 ? StellaEvent::KCODE_F10:StellaEvent::KCODE_F14), (keys_pressed & (KEY_A)) || (keys_pressed & (KEY_B)) ||
                                                                                      (keys_pressed & (KEY_R)) || (keys_pressed & (KEY_L)));      // RIGHT is the Paddle Button... either A or B will trigger this on Paddle Games
                    if ((keys_pressed & (KEY_A)) || (keys_pressed & (KEY_B)) || (keys_pressed & (KEY_R)) || (keys_pressed & (KEY_L)) || (keys_pressed & (KEY_LEFT)) || (keys_pressed & (KEY_RIGHT)))
                    {
                        if (!((keys_pressed & (KEY_R)) || (keys_pressed & (KEY_L))))  // We want to still process for L/R shoulder buttons for possible screen scaling...
                        {
                            keys_pressed = 0;   // If these were pressed... don't handle them below...
                        }
                    }
                    break;
                    
                case CTR_DRIVING:
                    static int driving_dampen = 0;
                    if (++driving_dampen % 2)
                    {
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_INSERT, keys_pressed & (KEY_LEFT));
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_PAGEUP, keys_pressed & (KEY_RIGHT));
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_HOME, (keys_pressed & (KEY_A)) || (keys_pressed & (KEY_B)));
                        if ((keys_pressed & (KEY_A)) || (keys_pressed & (KEY_B)) || (keys_pressed & (KEY_LEFT)) || (keys_pressed & (KEY_RIGHT)))
                        {
                            keys_pressed = 0;   // If these were pressed... don't handle them below...
                        }
                    }
                    break;
                    
                case CTR_KEYBOARD0:
                    if (bShowKeyboard  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        keys_touch = 1;

                        if (touch.px > 60  && touch.px < 105 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_8, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_9, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_0, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_i, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_o, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_p, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_k, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_l, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SEMICOLON, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_COMMA, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_PERIOD, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SLASH, 1);
                    }
                    else
                    {
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_8, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_9, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_0, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_i, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_o, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_p, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_k, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_l, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SEMICOLON, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_COMMA, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_PERIOD, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_SLASH, 0);
                    }
                    
                case CTR_KEYBOARD1:
                    if (bShowKeyboard  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        keys_touch = 1;

                        if (touch.px > 60  && touch.px < 105 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_1, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_2, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 5 && touch.py < 50) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_3, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_q, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_w, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 55 && touch.py < 100) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_e, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_a, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_s, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 105 && touch.py < 150) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_d, 1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_z, 1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_x, 1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 150 && touch.py < 200) theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_c, 1);
                    }
                    else
                    {
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_1, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_2, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_3, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_q, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_w, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_e, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_a, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_s, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_d, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_z, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_x, 0);
                        theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_c, 0);
                    }            } // End Controller Switch

            // --------------------------------------------------------------------------------------
            // For things like showing paddles or console switches, we can do this much slower...
            // --------------------------------------------------------------------------------------
            if (dampen==0) 
            {
                dampen=10;      // Every 10 frames is good enough...
                if (bShowPaddles || bShowKeyboard || bShowInfo)
                {
                    if (keys_pressed & (KEY_SELECT))
                    {
                        bShowPaddles = false;
                        bShowKeyboard = false;
                        bShowInfo = false;
                        dsShowScreenMain(false);
                    }
                }
                else
                {
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F1,  keys_pressed & (KEY_SELECT));
                }
                theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F2,  keys_pressed & (KEY_START));
                theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F3, 0);
                theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F4, 0);
                theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F5, 0);
                theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F6, 0);
                theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F7, 0);
                theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F8, 0);

                // -----------------------------------------------------------------------
                // Check the UI keys... full speed, FSP display, offset/scale shift, etc.
                // -----------------------------------------------------------------------
                if ((keys_pressed & KEY_R) || (keys_pressed & KEY_L))
                {
                    if ((keys_pressed & KEY_R) && (keys_pressed & KEY_UP))   myCartInfo.yOffset++;
                    if ((keys_pressed & KEY_R) && (keys_pressed & KEY_DOWN)) myCartInfo.yOffset--;
                    if ((keys_pressed & KEY_R) && (keys_pressed & KEY_LEFT))  myCartInfo.xOffset++;
                    if ((keys_pressed & KEY_R) && (keys_pressed & KEY_RIGHT)) myCartInfo.xOffset--;

                    // Allow scaling from 51% to 100%
                    if ((keys_pressed & KEY_L) && (keys_pressed & KEY_UP))   if (myCartInfo.screenScale < 100) myCartInfo.screenScale++;
                    if ((keys_pressed & KEY_L) && (keys_pressed & KEY_DOWN)) if (myCartInfo.screenScale > 51) myCartInfo.screenScale--;

#ifdef WRITE_TWEAKS
                    if ((keys_pressed & KEY_L) && (keys_pressed & KEY_LEFT))  if (myCartInfo.displayStartScanline < 100) myCartInfo.displayStartScanline++;
                    if ((keys_pressed & KEY_L) && (keys_pressed & KEY_RIGHT)) if (myCartInfo.displayStartScanline > 15) myCartInfo.displayStartScanline--;
                    extern uInt32 myStartDisplayOffset;
                    extern uInt32 myStopDisplayOffset;
                    myStartDisplayOffset = 228 * myCartInfo.displayStartScanline;                              // Allow for some underscan lines on a per-cart basis
                    myStopDisplayOffset = myStartDisplayOffset + (228 * (myCartInfo.displayStopScanline));     // Allow for some overscan lines on a per-cart basis
                        
#endif              
                    bScreenRefresh = 1;
                }

                
                if (keys_pressed != last_keys_pressed)
                {
                    if ((keys_pressed & KEY_R) && (keys_pressed & KEY_L))
                    {
                        if (keys_pressed & KEY_A)   {lcdSwap();}                        
                        dsWriteTweaks();
                    }
                    else if (keys_pressed & KEY_X)
                    {
                        fpsDisplay = !fpsDisplay;
                        if (!fpsDisplay)
                        {
                            fpsbuf[0] = ' ';
                            fpsbuf[1] = ' ';
                            fpsbuf[2] = ' ';
                            fpsbuf[3] = 0;
                            dsPrintValue(0,0,0, fpsbuf);
                        }
                        else gTotalAtariFrames=0;
                    }
                    last_keys_pressed = keys_pressed;
                }
            }
            else
            {
                dampen--;
            }

        // -------------------------------------------------------------
        // Stuff to do once/second such as FPS display and Debug Data
        // -------------------------------------------------------------
        if (TIMER1_DATA >= 32728)   // 1000MS (1 sec)
        {
            TIMER1_CR = 0;
            TIMER1_DATA = 0;
            TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;

            if (fpsDisplay)
            {
                int x = gTotalAtariFrames;
                gTotalAtariFrames = 0;
                if ((!full_speed) && (x>60)) x--;
                fpsbuf[0] = '0' + (int)x/100;
                x = x % 100;
                fpsbuf[1] = '0' + (int)x/10;
                fpsbuf[2] = '0' + (int)x%10;
                fpsbuf[3] = 0;
                dsPrintValue(0,0,0, fpsbuf);
            }
            DumpDebugData();
        }
                
        // ----------------------------------------
        // If the information screen is shown... 
        // any press of the touch dismisses it...
        // ----------------------------------------
        if (bShowInfo && (info_dampen == 0))
        {
            if (keys_pressed & KEY_TOUCH)
            {
                bShowInfo = false;
                dsShowScreenMain(false);
            }
            keys_touch = 1;
        } else info_dampen--;
        
        // -----------------------------------------------------------------------------------------
        // If the touch screen was pressed and we are not showing one of the special sub-screens...
        // -----------------------------------------------------------------------------------------
        if ((keys_pressed & KEY_TOUCH) &&  !bShowPaddles && !bShowKeyboard && !bShowInfo)
        {
            if (!keys_touch)
            {
                touchPosition touch;
                keys_touch=1;
                touchRead(&touch);
                iTx = touch.px;
                iTy = touch.py;

                if ((iTx>10) && (iTx<40) && (iTy>26) && (iTy<65)) 
                { // quit
                    dsDisplayButton(1);
                    if (dsWaitOnQuit()) etatEmu=STELLADS_QUITSTDS;
                    else
                    {
                        WAITVBL;
                    }
                }
                else if ((iTx>240) && (iTx<256) && (iTy>0) && (iTy<20))  
                { // Full Speed Toggle ... upper corner...
                    full_speed = 1-full_speed; 
                    ShowStatusLine();
                }                
                else if ((iTx>54) && (iTx<85) && (iTy>26) && (iTy<65)) 
                { // tv type
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    console_color=1-console_color;
                    theConsole->eventHandler().sendKeyEvent(console_color ? StellaEvent::KCODE_F3 : StellaEvent::KCODE_F4, 1);
                    dampen=5;
                    dsDisplayButton(3-console_color);
                }
                else if ((iTx>100) && (iTx<125) && (iTy>26) && (iTy<65)) 
                { // Left Difficulty Switch
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    left_difficulty=1-left_difficulty;
                    theConsole->eventHandler().sendKeyEvent(left_difficulty ? StellaEvent::KCODE_F5 : StellaEvent::KCODE_F6, 1);
                    dampen=5;
                    dsDisplayButton(10+left_difficulty);
                }
                else if ((iTx>135) && (iTx<160) && (iTy>26) && (iTy<65)) 
                { // Right Difficulty Switch
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    right_difficulty=1-right_difficulty;
                    theConsole->eventHandler().sendKeyEvent(right_difficulty ? StellaEvent::KCODE_F7 : StellaEvent::KCODE_F8, 1);
                    dampen=5;
                    dsDisplayButton(12+right_difficulty);
                }
                else if ((iTx>170) && (iTx<203) && (iTy>26) && (iTy<70)) 
                { // game select
                    dsDisplayButton(5);
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F1, 1);
                    dampen=10;
                    WAITVBL; dsDisplayButton(4);
                }
                else if ((iTx>215) && (iTx<253) && (iTy>26) && (iTy<70)) 
                { // game reset
                    dsDisplayButton(7);
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    theConsole->eventHandler().sendKeyEvent(StellaEvent::KCODE_F2, 1);
                    dampen=10;
                    WAITVBL; dsDisplayButton(6);
                }
                else if ((iTx>47) && (iTx<209) && (iTy>99) && (iTy<133)) 
                {     // 48,100 -> 208,132 cartridge slot
                    // Find files in current directory and show it
                    irqDisable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
                    vcsFindFiles();
                    romSel=dsWaitForRom();
                    if (romSel) 
                    {
                        etatEmu=STELLADS_PLAYINIT;
                        dsLoadGame(vcsromlist[ucFicAct].filename);
                        dsDisplayButton(3-console_color);
                        dsDisplayButton(10+left_difficulty);
                        dsDisplayButton(12+right_difficulty);
                        dsDisplayButton(14+(myCartInfo.mode == MODE_NO ? 1:0));
                        dsDisplayButton(16+bSoundEnabled);
                        ShowStatusLine();
                    }
                    else 
                    { 
                        if (bSoundEnabled)
                        {
                            irqEnable(IRQ_TIMER2);
                            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
                        }
                    }
                }
                else if ((iTx>210) && (iTx<250) && (iTy>170) && (iTy<200))  // Fast vs Flicker-Free Mode
                {
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    if (myCartInfo.mode == MODE_NO)
                    {
                        extern uint8 original_flicker_mode;
                        myCartInfo.mode = (original_flicker_mode == MODE_NO ? MODE_FF:original_flicker_mode);
                    }
                    else
                    {
                        myCartInfo.mode = MODE_NO;
                    }
                    dsDisplayButton(14+(myCartInfo.mode == MODE_NO ? 1:0));
                }
                else if ((iTx>210) && (iTx<250) && (iTy>140) && (iTy<170))  // Sound ON/OFF
                {
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    bSoundEnabled = 1-bSoundEnabled;
                    if (bSoundEnabled)
                    {
                        irqEnable(IRQ_TIMER2);
                        fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
                    }
                    else
                    {
                        irqDisable(IRQ_TIMER2);
                        fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
                    }
                    dsDisplayButton(16+bSoundEnabled);
                }
                else if ((iTx>5) && (iTx<35) && (iTy>150) && (iTy<200)) 
                { // Show Info Mode!
                    if (bShowInfo == false)
                    {
                        bShowInfo = true;
                        info_dampen = 15;
                        dsShowScreenInfo();
                    }
                    else
                    {
                        bShowInfo = false;
                        dsShowScreenMain(false);
                    }
                }
                else if ((iTx>50) && (iTx<90) && (iTy>150) && (iTy<200)) 
                { // Paddle Mode!
                    if (bShowPaddles == false)
                    {
                        bShowPaddles = true;
                        dsShowScreenPaddles();
                    }
                    else
                    {
                        bShowPaddles = false;
                        dsShowScreenMain(false);
                    }
                }
                else if ((iTx>100) && (iTx<130) && (iTy>150) && (iTy<200)) 
                { // Keyboard Mode!
                    if (bShowKeyboard == false)
                    {
                        bShowKeyboard = true;
                        dsShowScreenKeypad();
                    }
                    else
                    {
                        bShowKeyboard = false;
                        dsShowScreenMain(false);
                    }
                }
                else if ((iTx>140) && (iTx<160) && (iTy>150) && (iTy<200)) 
                { // High Score!
                    irqDisable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
                    
                    highscore_display();
                    dsShowScreenMain(false);
                    for (int i=0; i<12; i++)
                    {
                        WAITVBL;
                    }
                    
                    
                    if (bSoundEnabled)
                    {
                        irqEnable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
                    }
                }
                else if ((iTx>170) && (iTx<200) && (iTy>150) && (iTy<200)) 
                { // Instruction Manual!
                    if (bShowInfo == false)
                    {
                        bShowInfo = true;
                        info_dampen = 15;
                        dsShowScreenInstructions();
                    }
                    else
                    {
                        bShowInfo = false;
                        dsShowScreenMain(false);
                    }
                }
            }
        }
        else
        {
            keys_touch=0;
        }

        // -------------------------------------------------------------
        // Now, here, at the bottom of the world - update the frame! 
        // -------------------------------------------------------------
        theConsole->update();
        break;
        }
    }
}

//----------------------------------------------------------------------------------
// Find files (a26 / bin) available
int a26Filescmp (const void *c1, const void *c2) 
{
  FICA2600 *p1 = (FICA2600 *) c1;
  FICA2600 *p2 = (FICA2600 *) c2;

  if (p1->filename[0] == '.' && p2->filename[0] != '.')
      return -1;
  if (p2->filename[0] == '.' && p1->filename[0] != '.')
      return 1;
  if (p1->directory && !(p2->directory))
      return -1;
  if (p2->directory && !(p1->directory))
      return 1;
  return strcasecmp (p1->filename, p2->filename);    
}

static char filenametmp[255];
void vcsFindFiles(void) 
{
  DIR *pdir;
  struct dirent *pent;

  countvcs = 0;

  pdir = opendir(".");

  if (pdir) {

    while (((pent=readdir(pdir))!=NULL)) 
    {
      strcpy(filenametmp,pent->d_name);
      if (pent->d_type == DT_DIR)
      {
        if (!( (filenametmp[0] == '.') && (strlen(filenametmp) == 1))) {
          vcsromlist[countvcs].directory = true;
          strcpy(vcsromlist[countvcs].filename,filenametmp);
          countvcs++;
        }
      }
      else {
        if (strlen(filenametmp)>4) {
          if ( (strcasecmp(strrchr(filenametmp, '.'), ".a26") == 0) )  {
            vcsromlist[countvcs].directory = false;
            strcpy(vcsromlist[countvcs].filename,filenametmp);
            countvcs++;
          }
          if ( (strcasecmp(strrchr(filenametmp, '.'), ".bin") == 0) )  {
            vcsromlist[countvcs].directory = false;
            strcpy(vcsromlist[countvcs].filename,filenametmp);
            countvcs++;
          }
        }
      }
    }
    closedir(pdir);
  }
  if (countvcs)
    qsort (vcsromlist, countvcs, sizeof (FICA2600), a26Filescmp);
}

// End of file
