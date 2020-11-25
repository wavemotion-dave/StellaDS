#ifndef __DS_TOOLS_H
#define __DS_TOOLS_H

#include <nds.h>

#include "Console.hxx"
#include "Sound.hxx"

#define STELLADS_MENUINIT 0x01
#define STELLADS_MENUSHOW 0x02
#define STELLADS_PLAYINIT 0x03 
#define STELLADS_PLAYGAME 0x04 
#define STELLADS_QUITSTDS 0x05

extern unsigned int etatEmu;

typedef enum {
  EMUARM7_INIT_SND = 0x123C,
  EMUARM7_STOP_SND = 0x123D,
  EMUARM7_PLAY_SND = 0x123E,
} FifoMesType;

typedef struct FICtoLoad {
  char filename[255];
  bool directory;
} FICA2600;

extern Console* theConsole;
extern Sound* theSound;

extern unsigned int etatEmu;
extern FICA2600 vcsromlist[1024];  

extern volatile u32 emuFps;
extern volatile u32 emuActFrames;
extern volatile u16 g_framePending;

#define ds_GetTicks() (TIMER0_DATA)

extern void FadeToColor(unsigned char ucSens, unsigned short ucBG, unsigned char ucScr, unsigned char valEnd, unsigned char uWait);

extern void dsInitScreenMain(void);
extern void dsInitTimer(void);
extern void dsInitPalette(void);

extern void dsShowScreenEmu(void);
extern void dsShowScreenMain(void);
extern void dsFreeEmu(void);
extern bool dsLoadGame(char *filename);

extern bool dsWaitOnQuit(void);
extern unsigned int dsWaitForRom(void);
extern unsigned int dsWaitOnMenu(unsigned int actState);

extern void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr);

extern ITCM_CODE void dsMainLoop(void);

extern void dsInstallSoundEmuFIFO(void);

extern void vcsFindFiles(void);


#endif
