// =====================================================================================================
// Stella DS/DSi Pheonix Edition - Improved Version by Dave Bernazzani (wavemotion)
//
// Copyright (c) 2020-2024 by Dave Bernazzani
//
// Copying and distribution of this emulator, it's source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided this copyright notice is used and wavemotion-dave (Phoenix-Edition),
// Alekmaul (original port) are thanked profusely along with the entire Stella Team.
//
// The StellaDS emulator is offered as-is, without any warranty.
// =====================================================================================================
#include <nds.h>
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "StellaDS.h"

#include "printf.h"
#include "bgBottom.h"
#include "bgTop.h"
#include "bgFileSel.h"
#include "bgPaddles.h"
#include "bgKeypad.h"
#include "bgDualKeypad.h"
#include "bgStarRaiders.h"
#include "bgInstructions.h"

#include "clickNoQuit_wav.h"
#include "clickQuit_wav.h"

#include "Console.hxx"
#include "SaveKey.hxx"
#include "TIASound.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"
#include "EventHandler.hxx"
#include "Cart.hxx"
#include "highscore.h"
#include "config.h"
#include "instructions.h"
#include "screenshot.h"
#include "savestate.h"
#include "Thumbulator.hxx"
#include "TIA.hxx"

#define VERSION "7.9"

#define MAX_RESISTANCE  1030000
#define MIN_RESISTANCE  70000

uInt16 atari_frames=0;

uInt8 bInitialDiffSet = 0;

uInt8 tv_type_requested = NTSC;

uInt8 gSaveKeyEEWritten = false;
uInt8 gSaveKeyIsDirty = false;

uInt16 mySoundFreq = 20933;

#define MAX_DEBUG 40
Int32 debug[MAX_DEBUG]={0};
char DEBUG_DUMP = 0;
char my_filename[MAX_FILE_NAME_LEN+1] = {0};

FICA2600 vcsromlist[MAX_ROMS_PER_DIRECTORY];
uInt16 countvcs=0, ucFicAct=0;

static short bShowKeyboard = false;
static short bShowPaddles = false;
static short bShowInfo = false;

Console* theConsole = (Console*) NULL;

int bg0, bg0b, bg1b;
uInt16 emuState;

uint8 sound_buffer[SOUND_SIZE]  __attribute__ ((aligned (4)))  = {0};  // Can't be placed in fast memory as ARM7 needs to access it...
uint16 *aptr                    __attribute__((section(".dtcm"))) = (uint16*)((uint32)&sound_buffer[0] + 0xA000000); 
uint16 *bptr                    __attribute__((section(".dtcm"))) = (uint16*)((uint32)&sound_buffer[2] + 0xA000000); 
uint8  bHaltEmulation           __attribute__((section(".dtcm"))) = 0; 
uint8  bScreenRefresh           __attribute__((section(".dtcm"))) = 0;
uInt32 gAtariFrames             __attribute__((section(".dtcm"))) = 0;
uInt32 gTotalAtariFrames = 0;

static uInt8 full_speed=0;
static uInt8 fpsDisplay = false;

uInt16 last_keys_pressed,keys_touch=0, console_color=1, romSel;

char szName[MAX_FILE_NAME_LEN+1];
char szName2[MAX_FILE_NAME_LEN+1];

int getMemUsed(void)      // returns the amount of used memory in bytes
{
    struct mallinfo mi = mallinfo();
    return mi.uordblks;
}

char dbgbuf[36];
static void DumpDebugData(void)
{
    extern uInt32 gTotalSystemCycles;
    extern uInt16 gPC;
    
#ifdef CPU_PROFILER
    int j=0;
    for (int i=0; i<256; i++)
    {
        if (profiler[i] > 10000000)
        {
            debug[j] = profiler[i];
            debug[20+j] = i;
            j++;
        }
    }
#endif

    for (int i=0; i<17; i++)
    {
        sprintf(dbgbuf, "%02d: %-10u %08X %02d: %04X", i, debug[i], debug[i], i+20, debug[20+i]);
        dsPrintValue(0,2+i,0, dbgbuf);
    }
    uInt8 idx = 19;
    sprintf(dbgbuf, "Build Date:    %-11s V%s",    __DATE__, VERSION);
    dsPrintValue(0, idx++, 0, dbgbuf);
    sprintf(dbgbuf, "CPU Mode:      %-18s",        isDSiMode() ? "DSI 134MHz 16MB":"DS 67MHz 4 MB");  
    dsPrintValue(0, idx++, 0, dbgbuf);
    sprintf(dbgbuf, "Memory Used:   %-9d BYTES  ", getMemUsed());                       
    dsPrintValue(0, idx++, 0, dbgbuf);        
    sprintf(dbgbuf, "CY:%-11u FR:%-7uPC:%04X", gTotalSystemCycles, gTotalAtariFrames, gPC);
    dsPrintValue(0,idx++,0, dbgbuf);
    sprintf(dbgbuf, "%32s", myCartInfo.md5);
    dsPrintValue(0,idx++,0, dbgbuf);
}


// --------------------------------------------------------------------------------------
// Color fading effect
void FadeToColor(unsigned char ucSens, unsigned short ucBG, unsigned char ucScr, unsigned char valEnd, unsigned char uWait) 
{
  unsigned short ucFade;
  unsigned char ucBcl;

  // Fade-out the main screen
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

Int16 temp_shift __attribute__((section(".dtcm"))) = 0;
uInt8 shiftTime;
ITCM_CODE void vblankIntr() 
{
    if (bScreenRefresh || temp_shift)
    {
        REG_BG3PD = ((100 / myCartInfo.yScale)  << 8) | (100 % myCartInfo.yScale);
        REG_BG3Y = (myCartInfo.yOffset + temp_shift) << 8;
        REG_BG3X = (myCartInfo.xOffset)<<8;
        REG_BG3PA = (((A26_VID_WIDTH / 256) << 8) | (A26_VID_WIDTH % 256)) - myCartInfo.xStretch;
        bScreenRefresh = 0;
    }
    
    if (temp_shift)
    {
        ++shiftTime;
        if (shiftTime > 40) 
        {
            if (temp_shift < 0) temp_shift++; else temp_shift--;
            if (temp_shift == 0)
            {
                shiftTime = 0;
                bScreenRefresh = 1; // Force the next vBlank interrupt to put screen right
            }
        }
    }
    
    // -----------------------------------------------------------------------------------
    // This is a nice speedup so we don't have to check for too many cycles as part of
    // the main M6502 execute() loop. We can just check periodically once per DS frame.
    // -----------------------------------------------------------------------------------
    if (gSystemCycles & 0x8000) // 32K cycles is enough... force break
    {
        extern uInt16 myExecutionStatus;
        myExecutionStatus = 0x01;   // STOP bit
    }
}

// --------------------------------------------------------------------------------------
void dsInitScreenMain(void)
{
    // Init the VSYNC and enable VBLANK interrupts.
    SetYtrigger(190); //trigger 2 lines before vsync
    irqSet(IRQ_VBLANK, vblankIntr);
    irqEnable(IRQ_VBLANK);
    
    vramSetBankD(VRAM_D_LCD );    // Not using this for video but 128K of faster RAM always useful! Mapped at 0x06860000
    vramSetBankE(VRAM_E_LCD );    // Not using this for video but 64K of faster RAM always useful!  Mapped at 0x06880000
    vramSetBankF(VRAM_F_LCD );    // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06890000
    vramSetBankG(VRAM_G_LCD );    // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06894000
    vramSetBankH(VRAM_H_LCD );    // Not using this for video but 32K of faster RAM always useful!  Mapped at 0x06898000
    vramSetBankI(VRAM_I_LCD );    // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x068A0000
    
    WAITVBL;
}

void dsInitTimer(void)
{
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
}

void dsInitPalette(void) 
{
    // Init DS Specific palette
    const uInt32* gamePalette = theTIA.palette();
    for(uInt32 i = 0; i < 256; i++)   
    {
        uInt8 r, g, b;

        r = (uInt8) ((gamePalette[i] & 0x00ff0000) >> 19);
        g = (uInt8) ((gamePalette[i] & 0x0000ff00) >> 11);
        b = (uInt8) ((gamePalette[i] & 0x000000ff) >> 3);

        BG_PALETTE[i]=RGB15(r,g,b);
    }
}

void dsWarnIncompatibileCart(void)
{
    dsPrintValue(5,0,0, (char*)"CART TYPE NOT SUPPORTED");
    bHaltEmulation = 1; // And force the game to not run 
}

void dsPrintCartType(char *type, int size)
{
    if (DEBUG_DUMP)
    {
        sprintf(dbgbuf, "%s %dK [%d]", type, size/1024, cartDriver);
        dsPrintValue(16-(strlen(dbgbuf)/2),0,0, (char*)dbgbuf);
    }
}

void dsShowScreenEmu(void)
{
  videoSetMode(MODE_5_2D);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);  // The main emulated (top screen) display.
  vramSetBankB(VRAM_B_MAIN_BG_0x06060000);  // This is where we will put our frame buffers to aid DMA Copy routines...
    
  bg0 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0,0);
  memset((void*)0x06000000, 0x00, 128*1024);

  REG_BG3PA = (((A26_VID_WIDTH / 256) << 8) | (A26_VID_WIDTH % 256)) - myCartInfo.xStretch;
  REG_BG3PB = 0; REG_BG3PC = 0;
  REG_BG3PD = ((100 / myCartInfo.yScale)  << 8) | (100 % myCartInfo.yScale) ;
  REG_BG3X = (myCartInfo.xOffset)<<8;
  REG_BG3Y = (myCartInfo.yOffset)<<8;

  ShowStatusLine(); 
    
  bScreenRefresh = 1;
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
  else if (myCartInfo.controllerType == CTR_KEYBOARD1)
  {
    decompress(bgDualKeypadTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgDualKeypadMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgDualKeypadPal,(u16*) BG_PALETTE_SUB,256*2);
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
  uInt8 i;

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
  dsDisplayButton(10+myCartInfo.left_difficulty);
  dsDisplayButton(12+myCartInfo.right_difficulty);
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
  for (u16 i=0; i<(u16)strlen(filename)+1; i++)
  {
      my_filename[i] = tolower(filename[i]);
  }
  my_filename[MAX_FILE_NAME_LEN] = 0;
    
  // Load the file
  FILE *romfile = fopen(filename, "rb");
  if (romfile != NULL)
  {
    // Free buffer if needed
    TIMER2_CR=0; irqDisable(IRQ_TIMER2);
    
    // Clear out debug information for new game
    memset(debug, 0x00, sizeof(debug));      
      
    extern uInt8 OptionPage;
    OptionPage = 0;

    if (theConsole)
    {
      delete theConsole;
    }

    fseek(romfile, 0, SEEK_END);
    buffer_size = ftell(romfile);
    if (buffer_size <= MAX_CART_FILE_SIZE)
    {
        memset(cart_buffer, 0xFF, MAX_CART_FILE_SIZE);
        rewind(romfile);
        fread(cart_buffer, buffer_size, 1, romfile);
        fclose(romfile);

        // Init the emulation
        theConsole = new Console((const uInt8*) cart_buffer, buffer_size, "noname");
        dsInitPalette();

        memset(sound_buffer, 0x00, SOUND_SIZE);
        
        TIMER2_DATA = TIMER_FREQ((myCartInfo.soundQuality == SOUND_WAVE) ? (mySoundFreq+75) : mySoundFreq); // For Wave Direct we run a little faster so as always to keep sampling ahead of TIA output
        TIMER2_CR = TIMER_DIV_1 | TIMER_IRQ_REQ | TIMER_ENABLE;
        if (myCartInfo.soundQuality == SOUND_WAVE)
        {
            irqSet(IRQ_TIMER2, Tia_process_wave);
        }
        else
        {
            irqSet(IRQ_TIMER2, Tia_process);
        }
        
        if (myCartInfo.soundQuality)
        {
            irqEnable(IRQ_TIMER2);
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }
        else    // Mute
        {
            irqDisable(IRQ_TIMER2);
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
        }

        // Center all paddles...
        theConsole->fakePaddleResistance = ((MAX_RESISTANCE+MIN_RESISTANCE)/2);
        myStellaEvent.set(Event::PaddleZeroResistance,  theConsole->fakePaddleResistance);
        myStellaEvent.set(Event::PaddleOneResistance,   theConsole->fakePaddleResistance);
        myStellaEvent.set(Event::PaddleTwoResistance,   theConsole->fakePaddleResistance);
        myStellaEvent.set(Event::PaddleThreeResistance, theConsole->fakePaddleResistance);
            
        // Make sure the difficulty switches are set right on loding
        bInitialDiffSet = true;
        
        // Always color TV to start...
        myStellaEvent.set(Event::ConsoleColor, 1);
        myStellaEvent.set(Event::ConsoleBlackWhite, 0);
        
        TIMER0_CR=0;
        TIMER0_DATA=0;
        TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
        atari_frames=0;
        gAtariFrames = 0;
        gTotalAtariFrames = 0;
        
        return true;
    }
    else return false;
  }
  return false;
}

unsigned int dsReadPad(void)
{
    unsigned short keys_pressed, ret_keys_pressed;

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
  u8 bRet=false, bDone=false;
  unsigned short keys_pressed;
  unsigned short posdeb=0;

  decompress(bgFileSelTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgFileSelMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgFileSelPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

  strcpy(szName,"Quit StellaDS?");
  dsPrintValue(16-strlen(szName)/2,3,0,szName);
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

void dsDisplayFiles(unsigned int NoDebGame,u32 ucSel)
{
  u16 ucBcl,ucGame;

  // Display all games if possible
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) (bgGetMapPtr(bg1b)),32*24*2);
  sprintf(szName,"%04d/%04d GAMES",(int)(1+ucSel+NoDebGame),countvcs);
  dsPrintValue(16-strlen(szName)/2,3,0,szName);
  dsPrintValue(31,5,0,(char *) (NoDebGame>0 ? "<" : " "));
  dsPrintValue(31,22,0,(char *) (NoDebGame+14<countvcs ? ">" : " "));
  sprintf(szName,"%s %s","A=CHOOSE Y=HALT B=BACK  VER:", VERSION);
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
  u8 bDone=false, bRet=false;
  u16 ucHaut=0x00, ucBas=0x00,ucSHaut=0x00, ucSBas=0x00, romSelected= 0, firstRomDisplay=0,nbRomPerPage, uNbRSPage;
  u8 uLenFic=0, ucFlip=0, ucFlop=0;

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

    if (keysCurrent() & KEY_A || keysCurrent() & KEY_Y || keysCurrent() & KEY_SELECT)
    {
      if (!vcsromlist[ucFicAct].directory)
      {
        bRet=true;
        bDone=true;
        bHaltEmulation = 0;
        if (keysCurrent() & KEY_SELECT)
        {
            full_speed = 1;
            fpsDisplay = 1;
        }
        if (keysCurrent() & KEY_Y) 
        {
           bHaltEmulation = 1;
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
        if ((uLenFic+29)>(u16)strlen(vcsromlist[ucFicAct].filename)) 
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
  unsigned short int uState=STELLADS_PLAYINIT;
  unsigned short int keys_pressed;
  u8 bDone=false, romSel;
  short int iTx,iTy;

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

void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr)
{
  static u16 *ptrScreen,*ptrMap;
  u16 usCharac;
  char *pTrTxt=pchStr;
  char ch;

  ptrScreen=(u16*) (bgGetMapPtr(bg1b))+x+(y<<5);
  ptrMap=(u16*) (bgGetMapPtr(bg0b)+(2*isSelect+24)*32);

  while((*pTrTxt)!='\0' )
  {
    ch = *pTrTxt;
    if (ch >= 'a' && ch <= 'z') ch -= 32; // Faster than strcpy/strtoupper
    if ((ch) == '|')
      usCharac=*(ptrMap);
    else if (((ch)<' ') || ((ch)>'_'))
      usCharac=*(ptrMap);
    else if((ch)<'@')
      usCharac=*(ptrMap+(ch)-' ');
    else
      usCharac=*(ptrMap+32+(ch)-'@');
    *ptrScreen++=usCharac;
    pTrTxt++;
  }
}

void dsPrintFPS(char *pchStr)
{
  u16 *ptrScreen,*ptrMap;
  char *pTrTxt=pchStr;

  ptrScreen=(u16*) (bgGetMapPtr(bg1b))+0+(0<<5);
  ptrMap=(u16*) (bgGetMapPtr(bg0b)+(2*0+24)*32);

  while((*pTrTxt)!='\0')
  {
    *ptrScreen++ = *(ptrMap+(*pTrTxt++)-' ');
  }
}


//---------------------------------------------------------------------------------
void dsInstallSoundEmuFIFO(void)
{
    if (isDSiMode())
    {
        aptr = (uint16*)((uint32)&sound_buffer[0] + 0xA000000); 
        bptr = (uint16*)((uint32)&sound_buffer[2] + 0xA000000); 
    }
    else
    {
        aptr = (uint16*)((uint32)&sound_buffer[0] + 0x00400000); 
        bptr = (uint16*)((uint32)&sound_buffer[2] + 0x00400000); 
    }
    
    FifoMessage msg;
    msg.SoundPlay.data = &sound_buffer;
    msg.SoundPlay.freq = 44100;
    msg.SoundPlay.volume = 127;
    msg.SoundPlay.pan = 64;
    msg.SoundPlay.loop = 1;
    msg.SoundPlay.format = ((1)<<4) | SoundFormat_16Bit;
    msg.SoundPlay.loopPoint = 0;
    msg.SoundPlay.dataSize = 4 >> 2;
    msg.type = EMUARM7_PLAY_SND;
    fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
}

__attribute__((noinline)) void ProcessAtariKeypad(void)
{
    touchPosition touch;
    touchRead(&touch);
    
    if (myCartInfo.controllerType == CTR_KEYBOARD0)
    {
        if (touch.px > 60  && touch.px < 105 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardZero1,     1);
        if (touch.px >105  && touch.px < 150 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardZero2,     1);
        if (touch.px >151  && touch.px < 195 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardZero3,     1);

        if (touch.px > 60  && touch.px < 105 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardZero4,   1);
        if (touch.px >105  && touch.px < 150 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardZero5,   1);
        if (touch.px >151  && touch.px < 195 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardZero6,   1);

        if (touch.px > 60  && touch.px < 105 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardZero7,  1);
        if (touch.px >105  && touch.px < 150 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardZero8,  1);
        if (touch.px >151  && touch.px < 195 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardZero9,  1);

        if (touch.px > 60  && touch.px < 105 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardZeroStar,  1);
        if (touch.px >105  && touch.px < 150 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardZero0,     1);
        if (touch.px >151  && touch.px < 195 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardZeroPound, 1);
    }
    else
    {
        // Keyboard 0
        if (touch.px > 10  && touch.px < 45  && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardZero1,     1);
        if (touch.px >=45  && touch.px < 81  && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardZero2,     1);
        if (touch.px >=81  && touch.px < 120 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardZero3,     1);

        if (touch.px > 10  && touch.px < 45  && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardZero4,   1);
        if (touch.px >=45  && touch.px < 81  && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardZero5,   1);
        if (touch.px >=81  && touch.px < 120 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardZero6,   1);

        if (touch.px > 10  && touch.px < 45  && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardZero7,  1);
        if (touch.px >=45  && touch.px < 81  && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardZero8,  1);
        if (touch.px >=81  && touch.px < 120 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardZero9,  1);

        if (touch.px > 10  && touch.px < 45  && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardZeroStar,  1);
        if (touch.px >=45  && touch.px < 81  && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardZero0,     1);
        if (touch.px >=81  && touch.px < 120 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardZeroPound, 1);

        // Keyboard 1
        if (touch.px >138  && touch.px < 174 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardOne1,     1);
        if (touch.px >=174 && touch.px < 211 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardOne2,     1);
        if (touch.px >=211 && touch.px < 249 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardOne3,     1);

        if (touch.px >138  && touch.px < 174 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardOne4,   1);
        if (touch.px >=174 && touch.px < 211 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardOne5,   1);
        if (touch.px >=211 && touch.px < 249 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardOne6,   1);

        if (touch.px >138  && touch.px < 174 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardOne7,  1);
        if (touch.px >=174 && touch.px < 211 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardOne8,  1);
        if (touch.px >=211 && touch.px < 249 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardOne9,  1);

        if (touch.px >138  && touch.px < 174 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardOneStar,  1);
        if (touch.px >=174 && touch.px < 211 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardOne0,     1);
        if (touch.px >=211 && touch.px < 249 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardOnePound, 1);
    }   
}

__attribute__((noinline)) void ClearAtariKeypad(void)
{
    myStellaEvent.set(Event::KeyboardZero1,     0);
    myStellaEvent.set(Event::KeyboardZero2,     0);
    myStellaEvent.set(Event::KeyboardZero3,     0);
    myStellaEvent.set(Event::KeyboardZero4,     0);
    myStellaEvent.set(Event::KeyboardZero5,     0);
    myStellaEvent.set(Event::KeyboardZero6,     0);
    myStellaEvent.set(Event::KeyboardZero7,     0);
    myStellaEvent.set(Event::KeyboardZero8,     0);
    myStellaEvent.set(Event::KeyboardZero9,     0);
    myStellaEvent.set(Event::KeyboardZeroStar,  0);
    myStellaEvent.set(Event::KeyboardZero0,     0);
    myStellaEvent.set(Event::KeyboardZeroPound, 0);

    myStellaEvent.set(Event::KeyboardOne1,     0);
    myStellaEvent.set(Event::KeyboardOne2,     0);
    myStellaEvent.set(Event::KeyboardOne3,     0);
    myStellaEvent.set(Event::KeyboardOne4,     0);
    myStellaEvent.set(Event::KeyboardOne5,     0);
    myStellaEvent.set(Event::KeyboardOne6,     0);
    myStellaEvent.set(Event::KeyboardOne7,     0);
    myStellaEvent.set(Event::KeyboardOne8,     0);
    myStellaEvent.set(Event::KeyboardOne9,     0);
    myStellaEvent.set(Event::KeyboardOneStar,  0);
    myStellaEvent.set(Event::KeyboardOne0,     0);
    myStellaEvent.set(Event::KeyboardOnePound, 0);
}

__attribute__((noinline)) void CheckSpecialKeypadHandling(unsigned short keys_pressed)
{
    // Allow directional arrows instead of keypad for convenience - most of the "edutainment"
    // games use the kids controller but really for nothing more than direction/fire.
    if ((myCartInfo.special == SPEC_ALPHABM) || (myCartInfo.special == SPEC_COOKIEM))
    {
        if (keys_pressed & (KEY_UP))    myStellaEvent.set(Event::KeyboardZero2,     1);
        if (keys_pressed & (KEY_DOWN))  myStellaEvent.set(Event::KeyboardZero8,     1);
        if (keys_pressed & (KEY_LEFT))  myStellaEvent.set(Event::KeyboardZero4,     1);
        if (keys_pressed & (KEY_RIGHT)) myStellaEvent.set(Event::KeyboardZero6,     1);
        if (keys_pressed & (KEY_A | KEY_B)) myStellaEvent.set(Event::KeyboardZero5, 1);
    }
    else if (myCartInfo.special == SPEC_OSCAR)
    {
        if (keys_pressed & (KEY_UP))    myStellaEvent.set(Event::KeyboardZero5,     1);
        if (keys_pressed & (KEY_DOWN))  myStellaEvent.set(Event::KeyboardZero0,     1);
        if (keys_pressed & (KEY_LEFT))  myStellaEvent.set(Event::KeyboardZero7,     1);
        if (keys_pressed & (KEY_RIGHT)) myStellaEvent.set(Event::KeyboardZero9,     1);
        if (keys_pressed & (KEY_Y))     myStellaEvent.set(Event::KeyboardZero1,     1);
        if (keys_pressed & (KEY_X))     myStellaEvent.set(Event::KeyboardZero2,     1);
        if (keys_pressed & (KEY_A))     myStellaEvent.set(Event::KeyboardZero3,     1);
        if (keys_pressed & (KEY_B))     myStellaEvent.set(Event::KeyboardZero8,     1);
    }
    else if (myCartInfo.special == SPEC_BIGBIRD)
    {
        if (keys_pressed & (KEY_LEFT))  myStellaEvent.set(Event::KeyboardZero5,     1);
        if (keys_pressed & (KEY_RIGHT)) myStellaEvent.set(Event::KeyboardZero6,     1);
    }
}


char fpsbuf[12];
short int iTx,iTy;
static u16 dampen=10;
static u16 info_dampen=0;
static u16 driving_dampen = 0;
static uInt8 bSelectOrResetWasPressed = 0;

void dsMainLoop(void)
{
    uInt16 keys_pressed;
    uInt8 rapid_fire   = 0;
    uInt8 button_fire  = false;
    uInt8 button_up    = false;
    uInt8 button_down  = false;
    uInt8 button_left  = false;
    uInt8 button_right = false;
    uInt8 temp_full_speed = 0;

    last_keys_pressed = -1;
    full_speed = 0;
    fpsDisplay = 0;
    bSelectOrResetWasPressed = 0;

    // Timers are fed with 33.513982 MHz clock.
    // With DIV_1024 the clock is 32,728.5 ticks per sec...
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
    TIMER1_DATA=0;
    TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
    
    while(emuState != STELLADS_QUITSTDS)
    {
        switch (emuState)
        {
        case STELLADS_MENUINIT:
            dsShowScreenMain(true);
            emuState = STELLADS_MENUSHOW;
            break;

        case STELLADS_MENUSHOW:
            emuState = dsWaitOnMenu(STELLADS_MENUSHOW);
            break;

        case STELLADS_PLAYINIT:
            dsShowScreenEmu();
            emuState = STELLADS_PLAYGAME;
            dampen=10;
            break;

        case STELLADS_PLAYGAME:
        
            // ------------------------------------------------------------------------------------
            // If we get above 50K, that means we've fallen way behind - so just unthrottle until
            // the next 50/60 frame mark. If we don't do this and we do wrap the timer at at 64K,
            // we will end up with a pause/gap in play as we catch back up to the frame counter.
            // ------------------------------------------------------------------------------------
            if (TIMER0_DATA > 50000)
            {   
                temp_full_speed = 1; 
            }
        
            // 32,728.5 ticks = 1 second
            // 1 frame = 1/50 or 1/60 (0.02 or 0.016)
            // 655 -> 50 fps and 546 -> 60 fps
            if ((full_speed || temp_full_speed) == 0)
            {
                while(TIMER0_DATA < ((myCartInfo.tv_type ? 655:546)*atari_frames))
                    ;
            }

            // Have we processed 50/60 frames... start over...
            if (++atari_frames == (myCartInfo.tv_type ? 50:60))
            {
                TIMER0_CR=0;
                TIMER0_DATA=0;
                TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
                atari_frames=0;
                temp_full_speed = 0;
            }
                
            // Wait for keys
            scanKeys();
            keys_pressed = keysCurrent();

            switch (myCartInfo.controllerType)
            {
                case CTR_LJOY:
                case CTR_RJOY:                    
                    button_fire  = false;
                    button_up    = false;
                    button_down  = false;
                    button_left  = false;
                    button_right = false;
                    
                    if (keys_pressed & (KEY_A))
                    {
                        if ((myCartInfo.aButton == BUTTON_FIRE))           button_fire  = true;
                        else if ((myCartInfo.aButton == BUTTON_JOY_UP))    button_up    = true;
                        else if ((myCartInfo.aButton == BUTTON_JOY_DOWN))  button_down  = true;
                        else if ((myCartInfo.aButton == BUTTON_JOY_LEFT))  button_left  = true;
                        else if ((myCartInfo.aButton == BUTTON_JOY_RIGHT)) button_right = true;
                        else if ((myCartInfo.aButton == BUTTON_AUTOFIRE))  button_fire  = (++rapid_fire & 0x08);
                        else if ((myCartInfo.aButton == BUTTON_SHIFT_UP))  {temp_shift = -16; shiftTime=0;}
                        else if ((myCartInfo.aButton == BUTTON_SHIFT_DN))  {temp_shift = +16; shiftTime=0;}
                    }
                    if (keys_pressed & (KEY_B))
                    {
                        if ((myCartInfo.bButton == BUTTON_FIRE))           button_fire  = true;
                        else if ((myCartInfo.bButton == BUTTON_JOY_UP))    button_up    = true;
                        else if ((myCartInfo.bButton == BUTTON_JOY_DOWN))  button_down  = true;
                        else if ((myCartInfo.bButton == BUTTON_JOY_LEFT))  button_left  = true;
                        else if ((myCartInfo.bButton == BUTTON_JOY_RIGHT)) button_right = true;
                        else if ((myCartInfo.bButton == BUTTON_AUTOFIRE))  button_fire  = (++rapid_fire & 0x08);
                        else if ((myCartInfo.bButton == BUTTON_SHIFT_UP))  {temp_shift = -16; shiftTime=0;}
                        else if ((myCartInfo.bButton == BUTTON_SHIFT_DN))  {temp_shift = +16; shiftTime=0;}
                    }
                    if (keys_pressed & (KEY_X))
                    {
                        if ((myCartInfo.xButton == BUTTON_FIRE))           button_fire  = true;
                        else if ((myCartInfo.xButton == BUTTON_JOY_UP))    button_up    = true;
                        else if ((myCartInfo.xButton == BUTTON_JOY_DOWN))  button_down  = true;
                        else if ((myCartInfo.xButton == BUTTON_JOY_LEFT))  button_left  = true;
                        else if ((myCartInfo.xButton == BUTTON_JOY_RIGHT)) button_right = true;
                        else if ((myCartInfo.xButton == BUTTON_AUTOFIRE))  button_fire  = (++rapid_fire & 0x08);
                        else if ((myCartInfo.xButton == BUTTON_SHIFT_UP))  {temp_shift = -16; shiftTime=0;}
                        else if ((myCartInfo.xButton == BUTTON_SHIFT_DN))  {temp_shift = +16; shiftTime=0;}
                    }
                    if (keys_pressed & (KEY_Y))
                    {
                        if ((myCartInfo.yButton == BUTTON_FIRE))           button_fire  = true;
                        else if ((myCartInfo.yButton == BUTTON_JOY_UP))    button_up    = true;
                        else if ((myCartInfo.yButton == BUTTON_JOY_DOWN))  button_down  = true;
                        else if ((myCartInfo.yButton == BUTTON_JOY_LEFT))  button_left  = true;
                        else if ((myCartInfo.yButton == BUTTON_JOY_RIGHT)) button_right = true;
                        else if ((myCartInfo.yButton == BUTTON_AUTOFIRE))  button_fire  = (++rapid_fire & 0x08);
                        else if ((myCartInfo.yButton == BUTTON_SHIFT_UP))  {temp_shift = -16; shiftTime=0;}
                        else if ((myCartInfo.yButton == BUTTON_SHIFT_DN))  {temp_shift = +16; shiftTime=0;}
                    }
                    
                    if (keys_pressed & (KEY_UP))                      button_up    = true;
                    else if (keys_pressed & (KEY_DOWN))               button_down  = true;
                    if (keys_pressed & (KEY_LEFT))                    button_left  = true;
                    else if (keys_pressed & (KEY_RIGHT))              button_right = true;
                    
                    if (myCartInfo.controllerType == CTR_LJOY)
                    {
                        myStellaEvent.set(Event::JoystickZeroFire,  button_fire);
                        myStellaEvent.set(Event::JoystickZeroUp,    button_up);
                        myStellaEvent.set(Event::JoystickZeroDown,  button_down);
                        myStellaEvent.set(Event::JoystickZeroLeft,  button_left);
                        myStellaEvent.set(Event::JoystickZeroRight, button_right);
                    }
                    else // RIGHT JOY
                    {
                        myStellaEvent.set(Event::JoystickOneFire,  button_fire);
                        myStellaEvent.set(Event::JoystickOneUp,    button_up);
                        myStellaEvent.set(Event::JoystickOneDown,  button_down);
                        myStellaEvent.set(Event::JoystickOneLeft,  button_left);
                        myStellaEvent.set(Event::JoystickOneRight, button_right);
                    }
                    break;
                    
                case CTR_MCA:
                    myStellaEvent.set(Event::JoystickOneLeft,       (keys_pressed & (KEY_Y)));
                    myStellaEvent.set(Event::JoystickOneDown,       (keys_pressed & (KEY_B)));
                    myStellaEvent.set(Event::JoystickOneRight,      (keys_pressed & (KEY_A)));
                    
                    myStellaEvent.set(Event::JoystickZeroUp,        keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,      keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,      keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,     keys_pressed & (KEY_RIGHT));
                    break;

                case CTR_STARGATE:
                    myStellaEvent.set(Event::JoystickZeroUp,        keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,      keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,      keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,     keys_pressed & (KEY_RIGHT));
                    myStellaEvent.set(Event::JoystickZeroFire,      keys_pressed & (KEY_A));
                    
                    myStellaEvent.set(Event::JoystickOneUp,         keys_pressed & (KEY_X));
                    myStellaEvent.set(Event::JoystickOneDown,       keys_pressed & (KEY_Y));
                    myStellaEvent.set(Event::JoystickOneFire,       keys_pressed & (KEY_B));
                    
                    break;
                    
                case CTR_SOLARIS:
                    myStellaEvent.set(Event::JoystickZeroUp,        keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,      keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,      keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,     keys_pressed & (KEY_RIGHT));
                    myStellaEvent.set(Event::JoystickZeroFire,      ((keys_pressed & (KEY_A)) | (keys_pressed & (KEY_B))));
                    myStellaEvent.set(Event::JoystickOneFire,       ((keys_pressed & (KEY_X)) | (keys_pressed & (KEY_Y))));
                    break;
                    
                case CTR_RAIDERS:
                    myStellaEvent.set(Event::JoystickOneUp,         keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickOneDown,       keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickOneLeft,       keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickOneRight,      keys_pressed & (KEY_RIGHT));
                    myStellaEvent.set(Event::JoystickOneFire,       keys_pressed & (KEY_A));
                    
                    myStellaEvent.set(Event::JoystickZeroLeft,      keys_pressed & (KEY_X));
                    myStellaEvent.set(Event::JoystickZeroRight,     keys_pressed & (KEY_Y));
                    myStellaEvent.set(Event::JoystickZeroFire,      keys_pressed & (KEY_B));
                    break;

                case CTR_TWINSTICK:
                    myStellaEvent.set(Event::JoystickZeroUp,        keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,      keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,      keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,     keys_pressed & (KEY_RIGHT));

                    myStellaEvent.set(Event::JoystickOneUp,         keys_pressed & (KEY_X));
                    myStellaEvent.set(Event::JoystickOneDown,       keys_pressed & (KEY_B));
                    myStellaEvent.set(Event::JoystickOneLeft,       keys_pressed & (KEY_Y));
                    myStellaEvent.set(Event::JoystickOneRight,      keys_pressed & (KEY_A));
                    
                    myStellaEvent.set(Event::JoystickZeroFire,      keys_pressed & (KEY_L));
                    myStellaEvent.set(Event::JoystickOneFire,       keys_pressed & (KEY_R));
                    break;

                case CTR_QUADTARI:
                    myStellaEvent.set(Event::JoystickZeroUp,        keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,      keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,      keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,     keys_pressed & (KEY_RIGHT));

                    myStellaEvent.set(Event::JoystickOneUp,         keys_pressed & (KEY_X));
                    myStellaEvent.set(Event::JoystickOneDown,       keys_pressed & (KEY_B));
                    myStellaEvent.set(Event::JoystickOneLeft,       keys_pressed & (KEY_Y));
                    myStellaEvent.set(Event::JoystickOneRight,      keys_pressed & (KEY_A));
                    
                    myStellaEvent.set(Event::JoystickZeroFire,      keys_pressed & (KEY_L));
                    myStellaEvent.set(Event::JoystickOneFire,       keys_pressed & (KEY_R));
                    break;
                    
                case CTR_BOOSTER:
                    myStellaEvent.set(Event::JoystickZeroUp,         keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,       keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,       keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,      keys_pressed & (KEY_RIGHT));
                    myStellaEvent.set(Event::BoosterGripZeroTrigger, keys_pressed & (KEY_A));
                    myStellaEvent.set(Event::BoosterGripZeroBooster, keys_pressed & (KEY_B));
                    break;
                    
                case CTR_GENESIS:
                    myStellaEvent.set(Event::JoystickZeroFire,      (keys_pressed & (KEY_A)) | (keys_pressed & (KEY_Y))); // Normal fire button
                    myStellaEvent.set(Event::BoosterGripZeroBooster,(keys_pressed & (KEY_B)));    // Second Genesis button "C" is mapped here
                    myStellaEvent.set(Event::JoystickZeroUp,        keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,      keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,      keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,     keys_pressed & (KEY_RIGHT));
                    
                    if (keys_pressed & (KEY_X))
                    {
                             if ((myCartInfo.xButton == BUTTON_SHIFT_UP))  {temp_shift = -16; shiftTime=0;}
                        else if ((myCartInfo.xButton == BUTTON_SHIFT_DN))  {temp_shift = +16; shiftTime=0;}
                    }
                    break;
                    
                case CTR_STARRAID:
                    myStellaEvent.set(Event::JoystickZeroUp,         keys_pressed & (KEY_UP));
                    myStellaEvent.set(Event::JoystickZeroDown,       keys_pressed & (KEY_DOWN));
                    myStellaEvent.set(Event::JoystickZeroLeft,       keys_pressed & (KEY_LEFT));
                    myStellaEvent.set(Event::JoystickZeroRight,      keys_pressed & (KEY_RIGHT));
                    myStellaEvent.set(Event::JoystickZeroFire,       ((keys_pressed & (KEY_A)) | (keys_pressed & (KEY_B))));
                    
                    if (bShowKeyboard  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        keys_touch = 1;

                        if (touch.px > 60  && touch.px < 105 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardOne1,     1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardOne2,     1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 5 && touch.py < 50) myStellaEvent.set(Event::KeyboardOne3,     1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardOne4,   1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardOne5,   1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 55 && touch.py < 100) myStellaEvent.set(Event::KeyboardOne6,   1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardOne7,  1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardOne8,  1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 105 && touch.py < 150) myStellaEvent.set(Event::KeyboardOne9,  1);

                        if (touch.px > 60  && touch.px < 105 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardOneStar,  1);
                        if (touch.px >105  && touch.px < 150 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardOne0,     1);
                        if (touch.px >151  && touch.px < 195 && touch.py > 150 && touch.py < 200) myStellaEvent.set(Event::KeyboardOnePound, 1);
                    }
                    else
                    {
                        myStellaEvent.set(Event::KeyboardOne1,     0);
                        myStellaEvent.set(Event::KeyboardOne2,     0);
                        myStellaEvent.set(Event::KeyboardOne3,     0);
                        myStellaEvent.set(Event::KeyboardOne4,     0);
                        myStellaEvent.set(Event::KeyboardOne5,     0);
                        myStellaEvent.set(Event::KeyboardOne6,     0);
                        myStellaEvent.set(Event::KeyboardOne7,     0);
                        myStellaEvent.set(Event::KeyboardOne8,     0);
                        myStellaEvent.set(Event::KeyboardOne9,     0);
                        myStellaEvent.set(Event::KeyboardOneStar,  0);
                        myStellaEvent.set(Event::KeyboardOne0,     0);
                        myStellaEvent.set(Event::KeyboardOnePound, 0);
                    }   
                    break;
                
                case CTR_BUMPBASH:
                    myStellaEvent.set(Event::PaddleZeroFire,     (keys_pressed & (KEY_B | KEY_Y | KEY_L)));
                    myStellaEvent.set(Event::PaddleOneFire,      (keys_pressed & (KEY_A | KEY_X | KEY_R)));                    
                    break;
                    
                case CTR_PADDLE0:
                case CTR_PADDLE1:
                    if(keys_pressed & (KEY_LEFT))
                    {
                        uInt8 sens = myCartInfo.analogSensitivity;
                        if (keys_pressed & KEY_X) sens += 4; 
                        if (keys_pressed & KEY_Y) sens -= 4;
                        theConsole->fakePaddleResistance += (10000 * sens) / 10;
                        if (theConsole->fakePaddleResistance > MAX_RESISTANCE) theConsole->fakePaddleResistance = MAX_RESISTANCE;
                        myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE0 ?  Event::PaddleZeroResistance : Event::PaddleOneResistance, theConsole->fakePaddleResistance);
                    }
                    else
                    if(keys_pressed & (KEY_RIGHT))
                    {
                        uInt8 sens = myCartInfo.analogSensitivity;
                        if (keys_pressed & KEY_X) sens += 4; 
                        if (keys_pressed & KEY_Y) sens -= 4;
                        theConsole->fakePaddleResistance -= (10000 * sens) / 10;
                        if (theConsole->fakePaddleResistance < MIN_RESISTANCE) theConsole->fakePaddleResistance = MIN_RESISTANCE;
                        myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE0 ?  Event::PaddleZeroResistance : Event::PaddleOneResistance, theConsole->fakePaddleResistance);
                    }
                    else
                    if (bShowPaddles  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        theConsole->fakePaddleResistance = ((MIN_RESISTANCE+MAX_RESISTANCE) - ((MAX_RESISTANCE / 255) * touch.px));
                        keys_touch = 1;
                        myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE0 ?  Event::PaddleZeroResistance : Event::PaddleOneResistance, theConsole->fakePaddleResistance);
                    }

                    myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE0 ?  Event::PaddleZeroFire : Event::PaddleOneFire, (keys_pressed & (KEY_A | KEY_B | KEY_R | KEY_L)));  // Any of these trigger it
                    if (keys_pressed & (KEY_L | KEY_R))
                    {
                        keys_pressed = 0;   // If these were pressed... don't handle them below...
                    }
                    break;
                    
                case CTR_PADDLE2:
                case CTR_PADDLE3:
                    if(keys_pressed & (KEY_LEFT))
                    {
                        theConsole->fakePaddleResistance += (10000 * myCartInfo.analogSensitivity) / 10;
                        if (theConsole->fakePaddleResistance > MAX_RESISTANCE) theConsole->fakePaddleResistance = MAX_RESISTANCE;
                        myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE2 ?  Event::PaddleTwoResistance : Event::PaddleThreeResistance, theConsole->fakePaddleResistance);
                    }
                    else
                    if(keys_pressed & (KEY_RIGHT))
                    {
                        theConsole->fakePaddleResistance -= (10000 * myCartInfo.analogSensitivity) / 10;
                        if (theConsole->fakePaddleResistance < MIN_RESISTANCE) theConsole->fakePaddleResistance = MIN_RESISTANCE;
                        myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE2 ?  Event::PaddleTwoResistance : Event::PaddleThreeResistance, theConsole->fakePaddleResistance);
                    }
                    else
                    if (bShowPaddles  && (keys_pressed & KEY_TOUCH))
                    {
                        touchPosition touch;
                        touchRead(&touch);
                        theConsole->fakePaddleResistance = ((MIN_RESISTANCE+MAX_RESISTANCE) - ((MAX_RESISTANCE / 255) * touch.px));
                        keys_touch = 1;
                        myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE2 ?  Event::PaddleTwoResistance : Event::PaddleThreeResistance, theConsole->fakePaddleResistance);
                    }

                    myStellaEvent.set(myCartInfo.controllerType == CTR_PADDLE2 ?  Event::PaddleTwoFire : Event::PaddleThreeFire, (keys_pressed & (KEY_A | KEY_B | KEY_R | KEY_L)));  // Any of these trigger it
                    if (keys_pressed & (KEY_L | KEY_R))
                    {
                        keys_pressed = 0;   // If these were pressed... don't handle them below...
                    }
                    break;
                    
                case CTR_DRIVING:
                    if (++driving_dampen % 2)
                    {
                        myStellaEvent.set(Event::DrivingZeroCounterClockwise, keys_pressed & (KEY_LEFT));
                        myStellaEvent.set(Event::DrivingZeroClockwise, keys_pressed & (KEY_RIGHT));
                        myStellaEvent.set(Event::DrivingZeroFire, (keys_pressed & (KEY_A)) || (keys_pressed & (KEY_B)));
                    }
                    break;
                    
                // Just the left/P1 keyboard shown
                case CTR_KEYBOARD0:
                    if (bShowKeyboard  && (keys_pressed & KEY_TOUCH))
                    {
                        keys_touch = 1;
                        ProcessAtariKeypad();
                    }
                    else
                    {
                        ClearAtariKeypad();
                    }
                    if (myCartInfo.special) CheckSpecialKeypadHandling(keys_pressed);
                    break;                    

                // Both keyboards shown...
                case CTR_KEYBOARD1:
                    if (bShowKeyboard  && (keys_pressed & KEY_TOUCH))
                    {
                        keys_touch = 1;
                        ProcessAtariKeypad();
                    }
                    else
                    {
                        ClearAtariKeypad();
                    }
                    if (myCartInfo.special) CheckSpecialKeypadHandling(keys_pressed);
                    break;
            } // End Controller Switch

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
                    if (!(keys_pressed & KEY_TOUCH)) myStellaEvent.set(Event::ConsoleSelect,  keys_pressed & (KEY_SELECT));
                }
                if (!(keys_pressed & KEY_TOUCH)) myStellaEvent.set(Event::ConsoleReset, keys_pressed & (KEY_START));
                
                if (bInitialDiffSet)
                {
                    bInitialDiffSet = false;
                    // Make sure the difficulty switches are correct...
                    if (myCartInfo.left_difficulty)
                    {
                        myStellaEvent.set(Event::ConsoleLeftDifficultyA, 1);
                        myStellaEvent.set(Event::ConsoleLeftDifficultyB, 0);
                    }
                    else
                    {
                        myStellaEvent.set(Event::ConsoleLeftDifficultyA, 0);
                        myStellaEvent.set(Event::ConsoleLeftDifficultyB, 1);
                    }
                    dsDisplayButton(10+myCartInfo.left_difficulty);
                    
                    if (myCartInfo.right_difficulty)
                    {
                        myStellaEvent.set(Event::ConsoleRightDifficultyA, 1);
                        myStellaEvent.set(Event::ConsoleRightDifficultyB, 0);
                    }
                    else
                    {
                        myStellaEvent.set(Event::ConsoleRightDifficultyA, 0);
                        myStellaEvent.set(Event::ConsoleRightDifficultyB, 1);
                    }
                    dsDisplayButton(12+myCartInfo.right_difficulty);
                    
                    dsDisplayButton(3-console_color);
                }                    

                // -----------------------------------------------------------------------
                // Check the UI keys... this is for offset/scale shift of the display.
                // -----------------------------------------------------------------------
                static u16 ss_dampen = 0;
                if (keys_pressed & (KEY_R | KEY_L))
                {
                    if (myCartInfo.controllerType != CTR_QUADTARI)
                    {
                        if ((keys_pressed & KEY_R) && (keys_pressed & KEY_UP))   myCartInfo.yOffset++;
                        if ((keys_pressed & KEY_R) && (keys_pressed & KEY_DOWN)) myCartInfo.yOffset--;
                        if ((keys_pressed & KEY_R) && (keys_pressed & KEY_LEFT))  myCartInfo.xOffset++;
                        if ((keys_pressed & KEY_R) && (keys_pressed & KEY_RIGHT)) myCartInfo.xOffset--;

                        // Allow vertical scaling from 51% to 100%
                        if ((keys_pressed & KEY_L) && (keys_pressed & KEY_UP))   if (myCartInfo.yScale < 100) myCartInfo.yScale++;
                        if ((keys_pressed & KEY_L) && (keys_pressed & KEY_DOWN)) if (myCartInfo.yScale > 51) myCartInfo.yScale--;

                        // Allow horizontal scaling ... not hugely needed but for some games that don't utilize the entire horizontal width this can be used to zoom in a bit
                        if ((keys_pressed & KEY_L) && (keys_pressed & KEY_LEFT))  if (myCartInfo.xStretch > 0)  myCartInfo.xStretch--;
                        if ((keys_pressed & KEY_L) && (keys_pressed & KEY_RIGHT)) if (myCartInfo.xStretch < 32) myCartInfo.xStretch++;
                    }
                    
                    if ((keys_pressed & (KEY_R | KEY_L)) == (KEY_R | KEY_L) && (myCartInfo.controllerType != CTR_BUMPBASH))
                    {
                        if (++ss_dampen == 5) 
                        {
                            dsPrintValue(12,0,0, (char*)"SNAPSHOT");
                            (void)screenshot();
                            WAITVBL;WAITVBL;
                            dsPrintValue(12,0,0, (char*)"        ");
                        }
                    }
                    
                    bScreenRefresh = 1;
                } else ss_dampen = 0;

                
                if (keys_pressed != last_keys_pressed)
                {
                    if ((keys_pressed & KEY_R) && (keys_pressed & KEY_L))
                    {
                        if (keys_pressed & KEY_A)   {lcdSwap();}                        
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
                if ((!full_speed) && (gAtariFrames>60)) gAtariFrames--;
                fpsbuf[0] = '0' + (gAtariFrames/100);
                fpsbuf[1] = '0' + (gAtariFrames%100)/10;
                fpsbuf[2] = '0' + (gAtariFrames%10);
                fpsbuf[3] = 0;
                gAtariFrames=0;
                dsPrintFPS(fpsbuf);
            }
            if (gSaveKeyIsDirty)
            {
                gSaveKeyEEprom->WriteEEtoFile();
            }
            if (gSaveKeyEEWritten == 2)
            {
                dsPrintValue(9,0,0, (char*)"               ");
                gSaveKeyEEWritten = 0;
            }
            else if (gSaveKeyEEWritten == 1)
            {
                dsPrintValue(9,0,0, (char*)"SAVEKEY WRITTEN");
                gSaveKeyEEWritten = 2;
            }
            if (DEBUG_DUMP) DumpDebugData();
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
                    irqDisable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
                    dsDisplayButton(1);
                    if (dsWaitOnQuit()) emuState=STELLADS_QUITSTDS;
                    else
                    {
                        WAITVBL;
                    }
                    if (myCartInfo.soundQuality)
                    {
                        irqEnable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
                    }
                }
                else if ((iTx>240) && (iTx<256) && (iTy>0) && (iTy<20))  
                { // Full Speed Toggle ... upper corner...
                    full_speed = 1-full_speed; 
                    ShowStatusLine();
                }                
                else if ((iTx>0) && (iTx<20) && (iTy>0) && (iTy<20))  
                { // FPS Toggle ... upper corner...
                    fpsDisplay = 1-fpsDisplay;
                    if (!fpsDisplay)
                    {
                        strcpy(fpsbuf, "          ");
                        dsPrintValue(0,0,0, fpsbuf);
                    }
                    else gAtariFrames=0;                    
                    ShowStatusLine();
                }                
                else if ((iTx>54) && (iTx<85) && (iTy>26) && (iTy<65)) 
                { // tv type
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    console_color=1-console_color;
                    if (console_color)
                    {
                        myStellaEvent.set(Event::ConsoleColor, 1);
                        myStellaEvent.set(Event::ConsoleBlackWhite, 0);
                    }
                    else
                    {
                        myStellaEvent.set(Event::ConsoleColor, 0);
                        myStellaEvent.set(Event::ConsoleBlackWhite, 1);
                    }
                    dampen=5;
                    dsDisplayButton(3-console_color);
                }
                else if ((iTx>100) && (iTx<125) && (iTy>26) && (iTy<65)) 
                { // Left Difficulty Switch
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    myCartInfo.left_difficulty=1-myCartInfo.left_difficulty;
                    if (myCartInfo.left_difficulty)
                    {
                        myStellaEvent.set(Event::ConsoleLeftDifficultyA, 1);
                        myStellaEvent.set(Event::ConsoleLeftDifficultyB, 0);
                    }
                    else
                    {
                        myStellaEvent.set(Event::ConsoleLeftDifficultyA, 0);
                        myStellaEvent.set(Event::ConsoleLeftDifficultyB, 1);
                    }
                    dampen=5;
                    dsDisplayButton(10+myCartInfo.left_difficulty);
                }
                else if ((iTx>135) && (iTx<160) && (iTy>26) && (iTy<65)) 
                { // Right Difficulty Switch
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    myCartInfo.right_difficulty=1-myCartInfo.right_difficulty;
                    if (myCartInfo.right_difficulty)
                    {
                        myStellaEvent.set(Event::ConsoleRightDifficultyA, 1);
                        myStellaEvent.set(Event::ConsoleRightDifficultyB, 0);
                    }
                    else
                    {
                        myStellaEvent.set(Event::ConsoleRightDifficultyA, 0);
                        myStellaEvent.set(Event::ConsoleRightDifficultyB, 1);
                    }
                    dampen=5;
                    dsDisplayButton(12+myCartInfo.right_difficulty);
                }
                else if ((iTx>170) && (iTx<203) && (iTy>26) && (iTy<70)) 
                { // game select
                    dsDisplayButton(5);
                    bSelectOrResetWasPressed = 1;
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    myStellaEvent.set(Event::ConsoleSelect, 1);
                    dampen=10;
                    WAITVBL; 
                }
                else if ((iTx>215) && (iTx<253) && (iTy>26) && (iTy<70)) 
                { // game reset
                    dsDisplayButton(7);
                    bSelectOrResetWasPressed = 1;
                    soundPlaySample(clickNoQuit_wav, SoundFormat_16Bit, clickNoQuit_wav_size, 22050, 127, 64, false, 0);
                    myStellaEvent.set(Event::ConsoleReset, 1);
                    dampen=10;
                    WAITVBL;
                }
                else if ((iTx>47) && (iTx<209) && (iTy>99) && (iTy<133)) 
                {     // 48,100 -> 208,132 cartridge slot
                    // Find files in current directory and show it
                    irqDisable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
                    vcsFindFiles();
                    romSel=dsWaitForRom();
                    if (romSel) 
                    {
                        emuState=STELLADS_PLAYINIT;
                        dsLoadGame(vcsromlist[ucFicAct].filename);
                        dsDisplayButton(3-console_color);
                        dsDisplayButton(10+myCartInfo.left_difficulty);
                        dsDisplayButton(12+myCartInfo.right_difficulty);
                        ShowStatusLine();
                        bInitialDiffSet=true;
                        dampen=0;
                        continue;
                    }
                    else 
                    { 
                        if (myCartInfo.soundQuality)
                        {
                            irqEnable(IRQ_TIMER2);
                            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
                        }
                    }
                }
                else if ((iTx>210) && (iTx<250) && (iTy>140) && (iTy<200))  // Configuration
                {
                    irqDisable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
                    ShowConfig();
                    dsShowScreenMain(false);
                    for (int i=0; i<6; i++)
                    {
                        WAITVBL;
                    }
                    if (myCartInfo.soundQuality)
                    {
                        irqEnable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
                    }
                }
                else if ((iTx>5) && (iTx<45) && (iTy>150) && (iTy<200)) 
                { // Save/Restore State
                    u8 bSaveRestoreAllowed = true;
                    if (gSaveKeyEEprom)
                    {
                        // If the SaveKey / EEPROM is busy in any way, we don't allow Save/Restore State
                        if (gSaveKeyEEprom->IsBusy()) bSaveRestoreAllowed = false;
                    }
                    if (bSaveRestoreAllowed)
                    {
                        irqDisable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
                        dsSaveStateHandler();
                        dsShowScreenMain(false);
                        if (myCartInfo.soundQuality)
                        {
                            irqEnable(IRQ_TIMER2); fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
                        }
                    }
                }
                else if ((iTx>=45) && (iTx<90) && (iTy>150) && (iTy<200)) 
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
                    
                    if (myCartInfo.soundQuality)
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
            if (bSelectOrResetWasPressed)
            {
                dsDisplayButton(4); dsDisplayButton(6); // Select and Reset back to normal
                bSelectOrResetWasPressed = 0;
            }
            keys_touch=0;
        }

        // -------------------------------------------------------------
        // Now, here, at the bottom of the world - update the frame! 
        // -------------------------------------------------------------
        if (!bHaltEmulation) theConsole->update();
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

static char filenametmp[MAX_FILE_NAME_LEN+1];
void vcsFindFiles(void) 
{
  DIR *pdir;
  struct dirent *pent;

  countvcs = 0;

  pdir = opendir(".");

  if (pdir) {

    while (((pent=readdir(pdir))!=NULL)) 
    {
      if (countvcs >= MAX_ROMS_PER_DIRECTORY)
      {
          break;    // That's it... no more room to display
      }
      if (strlen(pent->d_name) > MAX_FILE_NAME_LEN)
      {
          continue; // Name is too long... skip it
      }
      strncpy(filenametmp,pent->d_name, MAX_FILE_NAME_LEN);
      filenametmp[MAX_FILE_NAME_LEN-1] = 0;
      if (pent->d_type == DT_DIR)
      {
        // Do not include the [sav] directory
        if (strcasecmp(filenametmp, "sav") != 0)
        {
            if (!( (filenametmp[0] == '.') && (strlen(filenametmp) == 1))) {
              vcsromlist[countvcs].directory = true;
              strcpy(vcsromlist[countvcs].filename,filenametmp);
              countvcs++;
            }
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
  {
    qsort (vcsromlist, countvcs, sizeof (FICA2600), a26Filescmp);
  }
}

void _putchar(char character) {};   // Not used but needed to link printf()

// End of file
