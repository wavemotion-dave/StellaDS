#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>
#include "ds_tools.h"
#include "highscore.h"
#include "Console.hxx"
#include "Cart.hxx"
#include "bgHighScore.h"
#include "bgBottom.h"

#define MAX_HS_GAMES 715

#define WAITVBL swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank();

struct score_t 
{
    char    initials[4];
    char    score[7];
    uint16  year;
    uint8   month;
    uint8   day;
};

struct highscore_t
{
    char    md5sum[33];
    struct score_t scores[10];
};

struct highscore_t highscore_table[MAX_HS_GAMES];

extern int bg0, bg0b,bg1b;

char last_initials[4] = {0};

void highscore_init(void) 
{
    FILE *fp;
    
    strcpy(last_initials, "   ");
    
    // --------------------------------------------------------------
    // See if the StellaDS high score file exists... if so, read it!
    // --------------------------------------------------------------
    fp = fopen("/data/StellaDS.hi", "rb");
    if (fp != NULL)
    {
        fread(highscore_table, sizeof(highscore_table), 1, fp);
        fclose(fp);
    }
    else // Doesn't exist yet... create defaults and save it...
    {
        for (int i=0; i<MAX_HS_GAMES; i++)
        {
            strcpy(highscore_table[i].md5sum, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
            for (int j=0; j<10; j++)
            {
                strcpy(highscore_table[i].scores[j].score, "000000");
                strcpy(highscore_table[i].scores[j].initials, "   ");
                highscore_table[i].scores[j].year = 0;
                highscore_table[i].scores[j].month = 0;
                highscore_table[i].scores[j].day = 0;
            }
        }
        highscore_save();
    }
}


void highscore_save(void) 
{
    FILE *fp;

    DIR* dir = opendir("/data");
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else
    {
        mkdir("/data", 0777);
    }

    fp = fopen("/data/StellaDS.hi", "wb+");
    if (fp != NULL)
    {
        fwrite(&highscore_table, sizeof(highscore_table), 1, fp);
        fclose(fp);
    }
}

struct score_t score_entry;
char hs_line[33];
char md5[33];

void show_scores(short foundIdx)
{
    dsPrintValue(7,3,0, (char*)"** HIGH SCORES **");
    for (int i=0; i<10; i++)
    {
        sprintf(hs_line, "%04d-%02d-%02d   %-3s   %-6s", highscore_table[foundIdx].scores[i].year, highscore_table[foundIdx].scores[i].month,highscore_table[foundIdx].scores[i].day, highscore_table[foundIdx].scores[i].initials, highscore_table[foundIdx].scores[i].score);
        dsPrintValue(3,6+i, 0, hs_line);
    }
    dsPrintValue(3,20,0, (char*)"PRESS X FOR NEW HI SCORE     ");
    dsPrintValue(3,21,0, (char*)"PRESS B TO EXIT              ");
    dsPrintValue(3,22,0, (char*)"SCORES AUTO SORT AFTER ENTRY ");
    dsPrintValue(3,17,0, (char*)"                             ");
}

void highscore_display(void) 
{
    short foundIdx = -1;
    short firstBlank = -1;
    char bDone = 0;
    char bEntryDone = 0;
    char blink=0;
    unsigned short entry_idx=0;
    char dampen=0;
    time_t unixTime = time(NULL);
    struct tm* timeStruct = gmtime((const time_t *)&unixTime);

    decompress(bgHighScoreTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgHighScoreMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgHighScorePal,(u16*) BG_PALETTE_SUB,256*2);
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    swiWaitForVBlank();

    // ---------------------------------------------------------------------------------
    // Get the current CART md5 so we can search for it in our High Score database...
    // ---------------------------------------------------------------------------------
    strcpy(md5, myCartInfo.md5.c_str());
    for (int i=0; i<MAX_HS_GAMES; i++)
    {
        if (firstBlank == -1)
        {
            if (strcmp("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", highscore_table[i].md5sum) == 0)
            {
                firstBlank = i;
            }
        }

        if (strcmp(md5, highscore_table[i].md5sum) == 0)
        {
            foundIdx = i;
            break;
        }
    }
    
    if (foundIdx == -1)
    {
        foundIdx = firstBlank;   
    }
    
    show_scores(foundIdx);

    while (!bDone)
    {
        if (keysCurrent() & KEY_A) bDone=1;
        if (keysCurrent() & KEY_B) bDone=1;
        if (keysCurrent() & KEY_X)
        {
            dsPrintValue(3,20,0, (char*)"UP/DN/LEFT/RIGHT ENTER SCORE");
            dsPrintValue(3,21,0, (char*)"PRESS START TO SAVE SCORE   ");
            dsPrintValue(3,22,0, (char*)"PRESS SELECT TO CANCEL      ");
            
            strcpy(score_entry.score, "000000");
            strcpy(score_entry.initials, last_initials);
            score_entry.year  = timeStruct->tm_year +1900;
            score_entry.month = timeStruct->tm_mon+1;
            score_entry.day   = timeStruct->tm_mday;
            while (!bEntryDone)
            {
                swiWaitForVBlank();
                if (keysCurrent() & KEY_SELECT) {bEntryDone=1;}
                
                if (keysCurrent() & KEY_START) 
                {
                    strcpy(last_initials, score_entry.initials);
                    memcpy(&highscore_table[foundIdx].scores[9], &score_entry, sizeof(score_entry));
                    strcpy(highscore_table[foundIdx].md5sum, md5);                    
                    for (int i=0; i<9; i++)
                    {
                        for (int j=0; j<9; j++)
                        {
                            if (strcmp(highscore_table[foundIdx].scores[j+1].score, highscore_table[foundIdx].scores[j].score) > 0)
                            {
                                // Swap...
                                memcpy(&score_entry, &highscore_table[foundIdx].scores[j], sizeof(score_entry));
                                memcpy(&highscore_table[foundIdx].scores[j], &highscore_table[foundIdx].scores[j+1], sizeof(score_entry));
                                memcpy(&highscore_table[foundIdx].scores[j+1], &score_entry, sizeof(score_entry));
                            }
                        }
                    }
                    highscore_save();
                    bEntryDone=1;
                }
                
                if (dampen == 0)
                {
                    if ((keysCurrent() & KEY_RIGHT) || (keysCurrent() & KEY_A))
                    {
                        if (entry_idx < 8) entry_idx++; 
                        blink=25;
                        dampen=15;
                    }

                    if (keysCurrent() & KEY_LEFT)  
                    {
                        if (entry_idx > 0) entry_idx--; 
                        blink=25;
                        dampen=15;
                    }

                    if (keysCurrent() & KEY_UP)
                    {
                        if (entry_idx < 3)
                        {
                            if (score_entry.initials[entry_idx] == ' ')
                                score_entry.initials[entry_idx] = 'A';
                            else if (score_entry.initials[entry_idx] == 'Z')
                                score_entry.initials[entry_idx] = ' ';
                            else score_entry.initials[entry_idx]++;
                        }
                        else
                        {
                            score_entry.score[entry_idx-3]++;
                            if (score_entry.score[entry_idx-3] > '9') score_entry.score[entry_idx-3] = '0';
                        }
                        blink=0;
                        dampen=10;
                    }

                    if (keysCurrent() & KEY_DOWN)
                    {
                        if (entry_idx < 3)
                        {
                            if (score_entry.initials[entry_idx] == ' ')
                                score_entry.initials[entry_idx] = 'Z';
                            else if (score_entry.initials[entry_idx] == 'A')
                                score_entry.initials[entry_idx] = ' ';
                            else score_entry.initials[entry_idx]--;
                        }
                        else
                        {
                            score_entry.score[entry_idx-3]--;
                            if (score_entry.score[entry_idx-3] < '0') score_entry.score[entry_idx-3] = '9';
                        }
                        blink=0;
                        dampen=10;
                    }
                }
                else
                {
                    dampen--;
                }

                sprintf(hs_line, "%04d-%02d-%02d   %-3s   %-6s", score_entry.year, score_entry.month, score_entry.day, score_entry.initials, score_entry.score);
                if ((++blink % 60) > 30)
                {
                    if (entry_idx < 3)
                        hs_line[13+entry_idx] = '_';
                    else
                        hs_line[16+entry_idx] = '_';
                }
                dsPrintValue(3,17, 0, (char*)hs_line);
            }
            
            show_scores(foundIdx);
            
            bEntryDone=0;
            entry_idx=0;
        }
    }    
    
    decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
    dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    for (int i=0; i<12; i++)
    {
        WAITVBL;
    }
}


// End of file
