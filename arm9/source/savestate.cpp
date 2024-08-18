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
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "StellaDS.h"

#include "printf.h"
#include "bgBottom.h"
#include "bgTop.h"
#include "Console.hxx"
#include "SaveKey.hxx"
#include "MediaSrc.hxx"
#include "TIASound.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"
#include "EventHandler.hxx"
#include "Cart.hxx"
#include "CartAR.hxx"
#include "highscore.h"
#include "config.h"
#include "instructions.h"
#include "screenshot.h"
#include "savestate.h"
#include "M6532.hxx"
#include "M6502.hxx"
#include "TIA.hxx"
#include "System.hxx"
#include "Thumbulator.hxx"
#include "bgFileSel.h"

#define SAVE_VERSION 0x0001

char tmp_buf[SOUND_SIZE];

extern char my_filename[];
extern char szName[];
extern int bg0, bg0b,bg1b;
extern unsigned int dsReadPad(void);
uInt16 savedTimerData = 0;

#define TYPE_RAW        0
#define TYPE_RAM        1
#define TYPE_CART       2
#define TYPE_FASTCART   3

typedef struct
{
    uInt8  peek_type;
    uInt8  poke_type;
    uInt32 peek_offset;
    uInt32 poke_offset;
}  Offsets_t;

Offsets_t myPageOffsets[64];

u8 IsSaveSupported(void)
{
    switch (myCartInfo.banking)
    {
        case BANK_2K:
        case BANK_4K:
        case BANK_F8:
        case BANK_F6:
        case BANK_F4:
        case BANK_F8SC:
        case BANK_F6SC:
        case BANK_F4SC:
        case BANK_FE:
        case BANK_JANE:
        case BANK_DPC:
        case BANK_BF:
        case BANK_DF:
        case BANK_EF:
        case BANK_BFSC:
        case BANK_DFSC:
        case BANK_EFSC:
        case BANK_E0:
        case BANK_E7:
        case BANK_3F:
        case BANK_TV:
        case BANK_WF8:
        case BANK_AR:
        case BANK_SB:
        case BANK_UA:
        case BANK_FA2:
        case BANK_X07:
        case BANK_CV:
        case BANK_MB:
        case BANK_0840:
        //case BANK_3E:
        //case BANK_3EPLUS:
            return true;    // Save State supported for these carts...

        default:
            return false;   // Save State not supported for all others...
    }
}

char save_filename[256];

void MakeSaveName(void)
{
    DIR* dir = opendir("sav");    // See if directory exists
    if (dir) closedir(dir);       // Directory exists. All good.
    else mkdir("sav", 0777);      // Doesn't exist - make it...

    strcpy(save_filename, (char *)"sav/");
    strcat(save_filename, my_filename);
    save_filename[strlen(save_filename)-3] = 's';
    save_filename[strlen(save_filename)-2] = 'a';
    save_filename[strlen(save_filename)-1] = 'v';
}

void SaveState(void)
{
    dsPrintValue(13,0,0, (char*)"SAVING");
    MakeSaveName();

    // Default everything to RAW and no offset...
    memset(myPageOffsets, 0x00, sizeof(myPageOffsets));

    // --------------------------------------------------------------------------------------------------------------------------
    // Because direct page peek/poke offsets can change from build to build, we need to store these as offsets into the
    // buffers that house our cart data or RAM. 99% of all peek/poke direct access is to either RAM or Cart Buffer (sometimes
    // slow buffer and sometimes the fast cart buffer if less than 8K). So we handle that translation here... this helps to
    // future-proof save state files when upgrading software as the memory locations of those buffers might change and a direct
    // store of the pointers would generally lead to corruption on a save from one version and a load on another...
    // --------------------------------------------------------------------------------------------------------------------------
    for (int i=0; i<64; i++)
    {
        // Direct PEEKs
        if ((myPageAccessTable[i].directPeekBase >= &myRAM[0]) && (myPageAccessTable[i].directPeekBase <= &myRAM[255]))
        {
            myPageOffsets[i].peek_type = TYPE_RAM;
            myPageOffsets[i].peek_offset = (myPageAccessTable[i].directPeekBase - myRAM);
        }
        else if ((myPageAccessTable[i].directPeekBase >= &cart_buffer[0]) && (myPageAccessTable[i].directPeekBase <= &cart_buffer[MAX_CART_FILE_SIZE]))
        {
            myPageOffsets[i].peek_type = TYPE_CART;
            myPageOffsets[i].peek_offset = (myPageAccessTable[i].directPeekBase - cart_buffer);
        }
        else if ((myPageAccessTable[i].directPeekBase >= &fast_cart_buffer[0]) && (myPageAccessTable[i].directPeekBase <= &fast_cart_buffer[8*1024]))
        {
            myPageOffsets[i].peek_type = TYPE_FASTCART;
            myPageOffsets[i].peek_offset = (myPageAccessTable[i].directPeekBase - fast_cart_buffer);
        }
        else if (myPageAccessTable[i].directPeekBase != 0)
        {
            myPageOffsets[i].peek_type = TYPE_RAW;
            myPageOffsets[i].peek_offset = (uInt32)myPageAccessTable[i].directPeekBase;
        }

        // Direct POKEs
        if ((myPageAccessTable[i].directPokeBase >= &myRAM[0]) && (myPageAccessTable[i].directPokeBase <= &myRAM[255]))
        {
            myPageOffsets[i].poke_type = TYPE_RAM;
            myPageOffsets[i].poke_offset = (myPageAccessTable[i].directPokeBase - myRAM);
        }
        else if ((myPageAccessTable[i].directPokeBase >= &cart_buffer[0]) && (myPageAccessTable[i].directPokeBase <= &cart_buffer[MAX_CART_FILE_SIZE]))
        {
            myPageOffsets[i].poke_type = TYPE_CART;
            myPageOffsets[i].poke_offset = (myPageAccessTable[i].directPokeBase - cart_buffer);
        }
        else if ((myPageAccessTable[i].directPokeBase >= &fast_cart_buffer[0]) && (myPageAccessTable[i].directPokeBase <= &fast_cart_buffer[8*1024]))
        {
            myPageOffsets[i].poke_type = TYPE_FASTCART;
            myPageOffsets[i].poke_offset = (myPageAccessTable[i].directPokeBase - fast_cart_buffer);
        }
        else if (myPageAccessTable[i].directPokeBase != 0)
        {
            myPageOffsets[i].poke_type = TYPE_RAW;
            myPageOffsets[i].poke_offset = (uInt32)myPageAccessTable[i].directPokeBase;
        }
    }

    FILE * fp = fopen(save_filename, "wb");
    savedTimerData = TIMER0_DATA;
    
    // Version
    uInt16 save_version = SAVE_VERSION;
    fwrite(&save_version,               sizeof(save_version),               1, fp);
    
    // StellaDS
    fwrite(fast_cart_buffer,            8*1024,                             1, fp);
    fwrite(sound_buffer,                SOUND_SIZE,                         1, fp);
    fwrite(&etatEmu,                    sizeof(etatEmu),                    1, fp);
    fwrite(&bHaltEmulation,             sizeof(bHaltEmulation),             1, fp);
    fwrite(&bScreenRefresh,             sizeof(bScreenRefresh),             1, fp);
    fwrite(&gAtariFrames,               sizeof(gAtariFrames),               1, fp);
    fwrite(&gTotalAtariFrames,          sizeof(gTotalAtariFrames),          1, fp);
    fwrite(&atari_frames,               sizeof(atari_frames),               1, fp);
    fwrite(&gSaveKeyEEWritten,          sizeof(gSaveKeyEEWritten),          1, fp);
    fwrite(&gSaveKeyIsDirty,            sizeof(gSaveKeyIsDirty),            1, fp);
    fwrite(&mySoundFreq,                sizeof(mySoundFreq),                1, fp);
    fwrite(&savedTimerData,             sizeof(savedTimerData),             1, fp);
    fwrite(&console_color,              sizeof(console_color),              1, fp);   
    fwrite(&myCartInfo.left_difficulty, sizeof(uInt8),                      1, fp);
    fwrite(&myCartInfo.right_difficulty,sizeof(uInt8),                      1, fp);

    // 6532
    fwrite(myRAM,                       256,                                1, fp);
    fwrite(&myTimer,                    sizeof(myTimer),                    1, fp);
    fwrite(&myIntervalShift,            sizeof(myIntervalShift),            1, fp);
    fwrite(&myCyclesWhenTimerSet,       sizeof(myCyclesWhenTimerSet),       1, fp);
    fwrite(&myCyclesWhenInterruptReset, sizeof(myCyclesWhenInterruptReset), 1, fp);
    fwrite(&myTimerReadAfterInterrupt,  sizeof(myTimerReadAfterInterrupt),  1, fp);

    fwrite(&myDDRA,                     sizeof(myDDRA),                     1, fp);
    fwrite(&myDDRA,                     sizeof(myDDRA),                     1, fp);
    fwrite(&myOutA,                     sizeof(myOutA),                     1, fp);

    // 6502
    fwrite(&A,                          sizeof(A),                          1, fp);
    fwrite(&X,                          sizeof(X),                          1, fp);
    fwrite(&Y,                          sizeof(Y),                          1, fp);
    fwrite(&SP,                         sizeof(SP),                         1, fp);
    fwrite(&gPC,                        sizeof(gPC),                        1, fp);
    fwrite(&N,                          sizeof(N),                          1, fp);
    fwrite(&V,                          sizeof(V),                          1, fp);
    fwrite(&B,                          sizeof(B),                          1, fp);
    fwrite(&D,                          sizeof(D),                          1, fp);
    fwrite(&I,                          sizeof(I),                          1, fp);
    fwrite(&C,                          sizeof(C),                          1, fp);
    fwrite(&notZ,                       sizeof(notZ),                       1, fp);
    fwrite(&myExecutionStatus,          sizeof(myExecutionStatus),          1, fp);
    fwrite(&myDataBusState,             sizeof(myDataBusState),             1, fp);
    fwrite(&NumberOfDistinctAccesses,   sizeof(NumberOfDistinctAccesses),   1, fp);

    // Cart Driver
    fwrite(&myCurrentBank,              sizeof(myCurrentBank),              1, fp);
    fwrite(&myCurrentOffset,            sizeof(myCurrentOffset),            1, fp);
    fwrite(&myCurrentOffset32,          sizeof(myCurrentOffset32),          1, fp);
    fwrite(&cartDriver,                 sizeof(cartDriver),                 1, fp);
    fwrite(&f8_bankbit,                 sizeof(f8_bankbit),                 1, fp);

    fwrite(&myRandomNumber,             sizeof(myRandomNumber),             1, fp);
    fwrite(&myMusicCycles,              sizeof(myMusicCycles),              1, fp);
    fwrite(myTops,                      sizeof(myTops),                     1, fp);
    fwrite(myBottoms,                   sizeof(myBottoms),                  1, fp);
    fwrite(myCounters,                  sizeof(myCounters),                 1, fp);
    fwrite(myFlags,                     sizeof(myFlags),                    1, fp);
    fwrite(myMusicMode,                 sizeof(myMusicMode),                1, fp);

    // System
    fwrite(&gSystemCycles,              sizeof(gSystemCycles),              1, fp);
    fwrite(&gTotalSystemCycles,         sizeof(gTotalSystemCycles),         1, fp);
    fwrite(&myPageOffsets,              sizeof(myPageOffsets),              1, fp);

    // TIA
    fwrite(ourCollisionTable,           sizeof(ourCollisionTable),          1, fp);
    fwrite(myPriorityEncoder,           sizeof(myPriorityEncoder),          1, fp);
    fwrite(&myCollision,                sizeof(myCollision),                1, fp);

    fwrite(&myPOSP0,                    sizeof(myPOSP0),                    1, fp);
    fwrite(&myPOSP1,                    sizeof(myPOSP1),                    1, fp);
    fwrite(&myPOSM0,                    sizeof(myPOSM0),                    1, fp);
    fwrite(&myPOSM1,                    sizeof(myPOSM1),                    1, fp);
    fwrite(&myPOSBL,                    sizeof(myPOSBL),                    1, fp);

    fwrite(&myPlayfieldPriorityAndScore,sizeof(myPlayfieldPriorityAndScore),1, fp);
    fwrite(myColor,                     sizeof(myColor),                    1, fp);
    fwrite(&myCTRLPF,                   sizeof(myCTRLPF),                   1, fp);
    fwrite(&myREFP0,                    sizeof(myREFP0),                    1, fp);
    fwrite(&myREFP0,                    sizeof(myREFP0),                    1, fp);
    fwrite(&myREFP1,                    sizeof(myREFP1),                    1, fp);
    fwrite(&myPF,                       sizeof(myPF),                       1, fp);
    fwrite(&myGRP0,                     sizeof(myGRP0),                     1, fp);
    fwrite(&myGRP1,                     sizeof(myGRP1),                     1, fp);
    fwrite(&myDGRP0,                    sizeof(myDGRP0),                    1, fp);
    fwrite(&myDGRP1,                    sizeof(myDGRP1),                    1, fp);
    fwrite(&myENAM0,                    sizeof(myENAM0),                    1, fp);
    fwrite(&myENAM1,                    sizeof(myENAM1),                    1, fp);
    fwrite(&myENABL,                    sizeof(myENABL),                    1, fp);
    fwrite(&myDENABL,                   sizeof(myDENABL),                   1, fp);
    fwrite(&myHMP0,                     sizeof(myHMP0),                     1, fp);
    fwrite(&myHMP1,                     sizeof(myHMP1),                     1, fp);
    fwrite(&myHMM0,                     sizeof(myHMM0),                     1, fp);
    fwrite(&myHMM1,                     sizeof(myHMM1),                     1, fp);
    fwrite(&myHMBL,                     sizeof(myHMBL),                     1, fp);
    fwrite(&myVDELP0,                   sizeof(myVDELP0),                   1, fp);
    fwrite(&myVDELP1,                   sizeof(myVDELP1),                   1, fp);
    fwrite(&myVDELBL,                   sizeof(myVDELBL),                   1, fp);
    fwrite(&myRESMP0,                   sizeof(myRESMP0),                   1, fp);
    fwrite(&myRESMP1,                   sizeof(myRESMP1),                   1, fp);

    fwrite(&myStartDisplayOffset,       sizeof(myStartDisplayOffset),       1, fp);
    fwrite(&myStopDisplayOffset,        sizeof(myStopDisplayOffset),        1, fp);
    fwrite(&myVSYNCFinishClock,         sizeof(myVSYNCFinishClock),         1, fp);
    fwrite(&myEnabledObjects,           sizeof(myEnabledObjects),           1, fp);
    fwrite(&myClockWhenFrameStarted,    sizeof(myClockWhenFrameStarted),    1, fp);
    fwrite(&myCyclesWhenFrameStarted,   sizeof(myCyclesWhenFrameStarted),   1, fp);

    fwrite(&myClockStartDisplay,        sizeof(myClockStartDisplay),        1, fp);
    fwrite(&myClockStopDisplay,         sizeof(myClockStopDisplay),         1, fp);
    fwrite(&myClockAtLastUpdate,        sizeof(myClockAtLastUpdate),        1, fp);
    fwrite(&myClocksToEndOfScanLine,    sizeof(myClocksToEndOfScanLine),    1, fp);
    fwrite(&myVSYNC,                    sizeof(myVSYNC),                    1, fp);
    fwrite(&myVBLANK,                   sizeof(myVBLANK),                   1, fp);
    fwrite(&myLastHMOVEClock,           sizeof(myLastHMOVEClock),           1, fp);
    fwrite(&myHMOVEBlankEnabled,        sizeof(myHMOVEBlankEnabled),        1, fp);

    fwrite(&myM0CosmicArkMotionEnabled, sizeof(myM0CosmicArkMotionEnabled), 1, fp);
    fwrite(&myM0CosmicArkCounter,       sizeof(myM0CosmicArkCounter),       1, fp);
    fwrite(&myCurrentGRP0,              sizeof(myCurrentGRP0),              1, fp);
    fwrite(&myCurrentGRP1,              sizeof(myCurrentGRP1),              1, fp);

    fwrite(&myNUSIZ0,                   sizeof(myNUSIZ0),                   1, fp);
    fwrite(&myNUSIZ0,                   sizeof(myNUSIZ0),                   1, fp);

    fwrite(ourPlayerReflectTable,       sizeof(ourPlayerReflectTable),      1, fp);
    fwrite(ourPlayfieldTable,           sizeof(ourPlayfieldTable),          1, fp);

    // TIA Sound
    fwrite(AUDC,                        sizeof(AUDC),                       1, fp);
    fwrite(AUDF,                        sizeof(AUDF),                       1, fp);
    fwrite(AUDV,                        sizeof(AUDV),                       1, fp);
    fwrite(Outvol,                      sizeof(Outvol),                     1, fp);
    fwrite(&bProcessingSample,          sizeof(bProcessingSample),          1, fp);
    fwrite(&tia_buf_idx,                sizeof(tia_buf_idx),                1, fp);
    fwrite(&tia_out_idx,                sizeof(tia_out_idx),                1, fp);

    fwrite(&Samp_n_max,                 sizeof(Samp_n_max),                 1, fp);
    fwrite(&Samp_n_cnt,                 sizeof(Samp_n_cnt),                 1, fp);
    fwrite(Bit9,                        sizeof(Bit9),                       1, fp);
    fwrite(P4,                          sizeof(P4),                         1, fp);
    fwrite(P9,                          sizeof(P9),                         1, fp);
    fwrite(Div_n_cnt,                   sizeof(Div_n_cnt),                  1, fp);
    fwrite(Div_n_max,                   sizeof(Div_n_max),                  1, fp);

    // Complicated Carts
    fwrite(&NumberOfDistinctAccesses,   sizeof(NumberOfDistinctAccesses),  1, fp);
    fwrite(&myWriteEnabled,             sizeof(myWriteEnabled),            1, fp);
    fwrite(&myDataHoldRegister,         sizeof(myDataHoldRegister),        1, fp);
    fwrite(&myWritePending,             sizeof(myWritePending),            1, fp);
    fwrite(&bPossibleLoad,              sizeof(bPossibleLoad),             1, fp);
    fwrite(&myNumberOfLoadImages,       sizeof(myNumberOfLoadImages),      1, fp);
    fwrite(&LastConfigurationAR,        sizeof(LastConfigurationAR),       1, fp);

    // tia_buf[] is in VRAM so we have to use an intermediate buffer...
    memcpy(tmp_buf, tia_buf,            SOUND_SIZE);
    fwrite(tmp_buf,                     SOUND_SIZE,                        1, fp);

    fclose(fp);

    WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;

    dsPrintValue(13,0,0, (char*)"      ");

    TIMER0_DATA = savedTimerData;
}


void LoadState(void)
{
    MakeSaveName();

    FILE * fp = fopen(save_filename, "rb");

    // Version
    uInt16 save_version = 0xFFFF;
    fread(&save_version,               sizeof(save_version),               1, fp);
    if (save_version == SAVE_VERSION) 
    {
        // StellaDS
        fread(fast_cart_buffer,            8*1024,                             1, fp);
        fread(sound_buffer,                SOUND_SIZE,                         1, fp);
        fread(&etatEmu,                    sizeof(etatEmu),                    1, fp);
        fread(&bHaltEmulation,             sizeof(bHaltEmulation),             1, fp);
        fread(&bScreenRefresh,             sizeof(bScreenRefresh),             1, fp);
        fread(&gAtariFrames,               sizeof(gAtariFrames),               1, fp);
        fread(&gTotalAtariFrames,          sizeof(gTotalAtariFrames),          1, fp);
        fread(&atari_frames,               sizeof(atari_frames),               1, fp);
        fread(&gSaveKeyEEWritten,          sizeof(gSaveKeyEEWritten),          1, fp);
        fread(&gSaveKeyIsDirty,            sizeof(gSaveKeyIsDirty),            1, fp);
        fread(&mySoundFreq,                sizeof(mySoundFreq),                1, fp);
        fread(&savedTimerData,             sizeof(savedTimerData),             1, fp);
        fread(&console_color,              sizeof(console_color),              1, fp);   
        fread(&myCartInfo.left_difficulty, sizeof(uInt8),                      1, fp);
        fread(&myCartInfo.right_difficulty,sizeof(uInt8),                      1, fp);
        
        // 6532
        fread(myRAM,                       256,                                1, fp);
        fread(&myTimer,                    sizeof(myTimer),                    1, fp);
        fread(&myIntervalShift,            sizeof(myIntervalShift),            1, fp);
        fread(&myCyclesWhenTimerSet,       sizeof(myCyclesWhenTimerSet),       1, fp);
        fread(&myCyclesWhenInterruptReset, sizeof(myCyclesWhenInterruptReset), 1, fp);
        fread(&myTimerReadAfterInterrupt,  sizeof(myTimerReadAfterInterrupt),  1, fp);

        fread(&myDDRA,                     sizeof(myDDRA),                     1, fp);
        fread(&myDDRA,                     sizeof(myDDRA),                     1, fp);
        fread(&myOutA,                     sizeof(myOutA),                     1, fp);

        // 6502
        fread(&A,                          sizeof(A),                          1, fp);
        fread(&X,                          sizeof(X),                          1, fp);
        fread(&Y,                          sizeof(Y),                          1, fp);
        fread(&SP,                         sizeof(SP),                         1, fp);
        fread(&gPC,                        sizeof(gPC),                        1, fp);
        fread(&N,                          sizeof(N),                          1, fp);
        fread(&V,                          sizeof(V),                          1, fp);
        fread(&B,                          sizeof(B),                          1, fp);
        fread(&D,                          sizeof(D),                          1, fp);
        fread(&I,                          sizeof(I),                          1, fp);
        fread(&C,                          sizeof(C),                          1, fp);
        fread(&notZ,                       sizeof(notZ),                       1, fp);
        fread(&myExecutionStatus,          sizeof(myExecutionStatus),          1, fp);
        fread(&myDataBusState,             sizeof(myDataBusState),             1, fp);
        fread(&NumberOfDistinctAccesses,   sizeof(NumberOfDistinctAccesses),   1, fp);

        // Cart Driver
        fread(&myCurrentBank,              sizeof(myCurrentBank),              1, fp);
        fread(&myCurrentOffset,            sizeof(myCurrentOffset),            1, fp);
        fread(&myCurrentOffset32,          sizeof(myCurrentOffset32),          1, fp);
        fread(&cartDriver,                 sizeof(cartDriver),                 1, fp);
        fread(&f8_bankbit,                 sizeof(f8_bankbit),                 1, fp);

        fread(&myRandomNumber,             sizeof(myRandomNumber),             1, fp);
        fread(&myMusicCycles,              sizeof(myMusicCycles),              1, fp);
        fread(myTops,                      sizeof(myTops),                     1, fp);
        fread(myBottoms,                   sizeof(myBottoms),                  1, fp);
        fread(myCounters,                  sizeof(myCounters),                 1, fp);
        fread(myFlags,                     sizeof(myFlags),                    1, fp);
        fread(myMusicMode,                 sizeof(myMusicMode),                1, fp);

        // System
        fread(&gSystemCycles,              sizeof(gSystemCycles),              1, fp);
        fread(&gTotalSystemCycles,         sizeof(gTotalSystemCycles),         1, fp);
        fread(&myPageOffsets,              sizeof(myPageOffsets),              1, fp);

        // TIA
        fread(ourCollisionTable,           sizeof(ourCollisionTable),          1, fp);
        fread(myPriorityEncoder,           sizeof(myPriorityEncoder),          1, fp);
        fread(&myCollision,                sizeof(myCollision),                1, fp);

        fread(&myPOSP0,                    sizeof(myPOSP0),                    1, fp);
        fread(&myPOSP1,                    sizeof(myPOSP1),                    1, fp);
        fread(&myPOSM0,                    sizeof(myPOSM0),                    1, fp);
        fread(&myPOSM1,                    sizeof(myPOSM1),                    1, fp);
        fread(&myPOSBL,                    sizeof(myPOSBL),                    1, fp);

        fread(&myPlayfieldPriorityAndScore,sizeof(myPlayfieldPriorityAndScore),1, fp);
        fread(myColor,                     sizeof(myColor),                    1, fp);
        fread(&myCTRLPF,                   sizeof(myCTRLPF),                   1, fp);
        fread(&myREFP0,                    sizeof(myREFP0),                    1, fp);
        fread(&myREFP0,                    sizeof(myREFP0),                    1, fp);
        fread(&myREFP1,                    sizeof(myREFP1),                    1, fp);
        fread(&myPF,                       sizeof(myPF),                       1, fp);
        fread(&myGRP0,                     sizeof(myGRP0),                     1, fp);
        fread(&myGRP1,                     sizeof(myGRP1),                     1, fp);
        fread(&myDGRP0,                    sizeof(myDGRP0),                    1, fp);
        fread(&myDGRP1,                    sizeof(myDGRP1),                    1, fp);
        fread(&myENAM0,                    sizeof(myENAM0),                    1, fp);
        fread(&myENAM1,                    sizeof(myENAM1),                    1, fp);
        fread(&myENABL,                    sizeof(myENABL),                    1, fp);
        fread(&myDENABL,                   sizeof(myDENABL),                   1, fp);
        fread(&myHMP0,                     sizeof(myHMP0),                     1, fp);
        fread(&myHMP1,                     sizeof(myHMP1),                     1, fp);
        fread(&myHMM0,                     sizeof(myHMM0),                     1, fp);
        fread(&myHMM1,                     sizeof(myHMM1),                     1, fp);
        fread(&myHMBL,                     sizeof(myHMBL),                     1, fp);
        fread(&myVDELP0,                   sizeof(myVDELP0),                   1, fp);
        fread(&myVDELP1,                   sizeof(myVDELP1),                   1, fp);
        fread(&myVDELBL,                   sizeof(myVDELBL),                   1, fp);
        fread(&myRESMP0,                   sizeof(myRESMP0),                   1, fp);
        fread(&myRESMP1,                   sizeof(myRESMP1),                   1, fp);

        fread(&myStartDisplayOffset,       sizeof(myStartDisplayOffset),       1, fp);
        fread(&myStopDisplayOffset,        sizeof(myStopDisplayOffset),        1, fp);
        fread(&myVSYNCFinishClock,         sizeof(myVSYNCFinishClock),         1, fp);
        fread(&myEnabledObjects,           sizeof(myEnabledObjects),           1, fp);
        fread(&myClockWhenFrameStarted,    sizeof(myClockWhenFrameStarted),    1, fp);
        fread(&myCyclesWhenFrameStarted,   sizeof(myCyclesWhenFrameStarted),   1, fp);

        fread(&myClockStartDisplay,        sizeof(myClockStartDisplay),        1, fp);
        fread(&myClockStopDisplay,         sizeof(myClockStopDisplay),         1, fp);
        fread(&myClockAtLastUpdate,        sizeof(myClockAtLastUpdate),        1, fp);
        fread(&myClocksToEndOfScanLine,    sizeof(myClocksToEndOfScanLine),    1, fp);
        fread(&myVSYNC,                    sizeof(myVSYNC),                    1, fp);
        fread(&myVBLANK,                   sizeof(myVBLANK),                   1, fp);
        fread(&myLastHMOVEClock,           sizeof(myLastHMOVEClock),           1, fp);
        fread(&myHMOVEBlankEnabled,        sizeof(myHMOVEBlankEnabled),        1, fp);

        fread(&myM0CosmicArkMotionEnabled, sizeof(myM0CosmicArkMotionEnabled), 1, fp);
        fread(&myM0CosmicArkCounter,       sizeof(myM0CosmicArkCounter),       1, fp);
        fread(&myCurrentGRP0,              sizeof(myCurrentGRP0),              1, fp);
        fread(&myCurrentGRP1,              sizeof(myCurrentGRP1),              1, fp);

        fread(&myNUSIZ0,                   sizeof(myNUSIZ0),                   1, fp);
        fread(&myNUSIZ0,                   sizeof(myNUSIZ0),                   1, fp);

        fread(ourPlayerReflectTable,       sizeof(ourPlayerReflectTable),      1, fp);
        fread(ourPlayfieldTable,           sizeof(ourPlayfieldTable),          1, fp);

        // TIA Sound
        fread(AUDC,                        sizeof(AUDC),                       1, fp);
        fread(AUDF,                        sizeof(AUDF),                       1, fp);
        fread(AUDV,                        sizeof(AUDV),                       1, fp);
        fread(Outvol,                      sizeof(Outvol),                     1, fp);
        fread(&bProcessingSample,          sizeof(bProcessingSample),          1, fp);
        fread(&tia_buf_idx,                sizeof(tia_buf_idx),                1, fp);
        fread(&tia_out_idx,                sizeof(tia_out_idx),                1, fp);

        fread(&Samp_n_max,                 sizeof(Samp_n_max),                 1, fp);
        fread(&Samp_n_cnt,                 sizeof(Samp_n_cnt),                 1, fp);
        fread(Bit9,                        sizeof(Bit9),                       1, fp);
        fread(P4,                          sizeof(P4),                         1, fp);
        fread(P9,                          sizeof(P9),                         1, fp);
        fread(Div_n_cnt,                   sizeof(Div_n_cnt),                  1, fp);
        fread(Div_n_max,                   sizeof(Div_n_max),                  1, fp);

        // Complicated Carts
        fread(&NumberOfDistinctAccesses,   sizeof(NumberOfDistinctAccesses),  1, fp);
        fread(&myWriteEnabled,             sizeof(myWriteEnabled),            1, fp);
        fread(&myDataHoldRegister,         sizeof(myDataHoldRegister),        1, fp);
        fread(&myWritePending,             sizeof(myWritePending),            1, fp);
        fread(&bPossibleLoad,              sizeof(bPossibleLoad),             1, fp);
        fread(&myNumberOfLoadImages,       sizeof(myNumberOfLoadImages),      1, fp);
        fread(&LastConfigurationAR,        sizeof(LastConfigurationAR),       1, fp);
        if (LastConfigurationAR != 255) SetConfigurationAR(LastConfigurationAR);

        // tia_buf[] is in VRAM so we have to use an intermediate buffer...
        fread(tmp_buf,                     SOUND_SIZE,                        1, fp);
        memcpy(tia_buf, tmp_buf,           SOUND_SIZE);

        fclose(fp);

        // ----------------------------------------------------------------------------------------------
        // Since the page offsets point into memory buffers that can change from build-to-build, we must
        // restore these carefully by checking the peek/poke type and rebuilding the pointer correctly.
        // ----------------------------------------------------------------------------------------------
        for (int i=0; i<64; i++)
        {
            // Direct PEEKs
            if (myPageOffsets[i].peek_type == TYPE_RAM)
            {
                myPageAccessTable[i].directPeekBase = &myRAM[myPageOffsets[i].peek_offset];
            }
            else if (myPageOffsets[i].peek_type == TYPE_CART)
            {
                myPageAccessTable[i].directPeekBase = &cart_buffer[myPageOffsets[i].peek_offset];
            }
            else if (myPageOffsets[i].peek_type == TYPE_FASTCART)
            {
                myPageAccessTable[i].directPeekBase = &fast_cart_buffer[myPageOffsets[i].peek_offset];
            }
            else // Must be RAW
            {
                myPageAccessTable[i].directPeekBase = (uInt8 *)myPageOffsets[i].peek_offset;
            }

            // Direct POKEs
            if (myPageOffsets[i].poke_type == TYPE_RAM)
            {
                myPageAccessTable[i].directPokeBase = &myRAM[myPageOffsets[i].poke_offset];
            }
            else if (myPageOffsets[i].poke_type == TYPE_CART)
            {
                myPageAccessTable[i].directPokeBase = &cart_buffer[myPageOffsets[i].poke_offset];
            }
            else if (myPageOffsets[i].poke_type == TYPE_FASTCART)
            {
                myPageAccessTable[i].directPokeBase = &fast_cart_buffer[myPageOffsets[i].poke_offset];
            }
            else // Must be RAW
            {
                myPageAccessTable[i].directPokeBase = (uInt8 *)myPageOffsets[i].poke_offset;
            }
        }

        bInitialDiffSet = true;
        TIMER0_DATA = savedTimerData;
    }
}

void dsSaveStateHandler(void)
{
    if (!IsSaveSupported()) 
    {
        dsPrintValue(10,0,0, (char*)"NOT SUPPORTED");
        WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
        WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
        dsPrintValue(10,0,0, (char*)"      ");
        return;
    }

    u8 bDone=false;
    unsigned short keys_pressed;
    unsigned short posdeb=0;

    decompress(bgFileSelTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgFileSelMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgFileSelPal,(u16*) BG_PALETTE_SUB,256*2);
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    strcpy(szName,"Save / Restore State");
    dsPrintValue(16-strlen(szName)/2,3,0,szName);
    sprintf(szName,"%s","A TO CONFIRM, B TO GO BACK");
    dsPrintValue(16-strlen(szName)/2,23,0,szName);

    while(!bDone)
    {
        strcpy(szName,"      SAVE STATE       ");
        dsPrintValue(5,10,(posdeb == 0 ? 1 :  0),szName);
        strcpy(szName,"    RESTORE STATE      ");
        dsPrintValue(5,13,(posdeb == 1 ? 1 :  0),szName);
        strcpy(szName,"      EXIT MENU        ");
        dsPrintValue(5,16,(posdeb == 2 ? 1 :  0),szName);
        swiWaitForVBlank();

        // Check pad
        keys_pressed=dsReadPad();
        if (keys_pressed & KEY_UP)
        {
            if (posdeb) posdeb--;
            WAITVBL;
        }
        if (keys_pressed & KEY_DOWN)
        {
            if (posdeb<2) posdeb++;
            WAITVBL;
        }

        // Menu choice selected
        if (keys_pressed & KEY_A)
        {
            dsShowScreenMain(false);
            if (posdeb == 0) SaveState();
            else if (posdeb == 1) LoadState();
            bDone = true;
        }

        // Cancel / Exit
        if (keys_pressed & KEY_B)
        {
            dsShowScreenMain(false);
            bDone = true;
        }
    }
}

// End of file
