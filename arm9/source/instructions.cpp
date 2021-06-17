#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>
#include "ds_tools.h"
#include "highscore.h"
#include "Console.hxx"
#include "Cart.hxx"
#include "bgInstructions.h"
#include "bgBottom.h"

#define MAX_HS_GAMES 715
extern int bg0, bg0b,bg1b;

struct instructions_t 
{
    string md5;
    const char *text;
};

const struct instructions_t instructions[] =
{
    {"157bddb7192754a45372be196797f284",    "Adventure\n(c)1980 Atari\nby Warren Robinett\n"
                                            " \n"
                                            "Game 1:  Easy - Fixed layout\n"
                                            "Game 2:  Medium - Fixed layout\n"
                                            "Game 3:  Hard - Random layout\n"
                                            "L-Diff:  A=Dragon Bites Faster\n"
                                            " \n"
                                            "The Evil Magician has stolen\n"
                                            "the Golden Chalice. It's hidden\n"
                                            "somewhere in the realm and you\n"
                                            "must find it and return it to\n"
                                            "the Yellow Castle. Beware the\n"
                                            "Dragons and Bat who look to\n"
                                            "spoil your Adventure!\n"
    },
    
    {"ca4f8c5b4d6fb9d608bb96bc7ebd26c7",  "Adventures of Tron (c)1982!"
                                            " \n"
                                            "L-Diff:  A=Faster Gameplay\n"
                                            " \n"
                                            "Tron! Must cross the I/O beam\n"
                                            "to allow Elevators to Lift.\n"
                                            "Intercept BITS on 4 floors.\n"
                                             " \n"
                                            "Bit on 1ST FLOOR  100\n"
                                            "Bit on 2ND FLOOR  200\n"
                                            "Bit on 3RD FLOOR  400\n"
                                            "Bit on TOP FLOOR  800\n"
                                            "BONUS Level Clear 2,000\n"
                                            " \n"
                                            "Press UP/DN to ride Elevators.\n"
                                            "When all bits are intercepted\n"
                                            "ride the I/O beam to next level\n"
                                            " \n"
    },
    
    {"79ab4123a83dc11d468fb2108ea09e2e",  "Beamrider (c)1984 Activision\n"
                                          " \n"
                                          "L-Diff:  A=Advanced Game\n"
                                          " \n"
                                          "Welcome to the Beam! There are\n"
                                          "99 sectors - a new enemy shows\n"
                                          "up for the first 16 sectors.\n"
                                          "15 Saucers must be destroyed\n"
                                          "in each sector then the Sector\n"
                                          "Sentinel ship appears. Only\n"
                                          "a torpedo will take it out.\n"
                                          "Yellow Rejuvenators=Extra Life\n"
                                          "White and Yellow Ships are\n"
                                          "vunerable to normal fire.\n"
                                          "Press UP to fire torpedo (3x)\n"
                                          "Press UP to start new Sector\n"
    },
    
    
    {"72ffbef6504b75e69ee1045af9075f66",  "Space Invaders (c)1980 Atari\n"
                                          " \n"
                                          "Game 1:  Normal Game\n"
                                          "Game 2:  Moving Shields\n"
                                          "Game 3:  ZigZag Bombs\n"
                                          "Game 4:  Moving Shields+Zigzag\n"
                                          "Game 5:  Fast Bombs\n"
                                          "Game 6:  Moving Shields+Fast\n"
                                          "Game 7:  Zigzag + Fast Bombs\n"
                                          "Game 8:  Moving+Zigzag+Fast\n"
                                          "Game 9:  Invisible Invaders!\n"
                                          " \n"
                                          "L-Diff:  A=Fat Missile Base\n"
                                          " \n"            
                                          "The SPACE INVADERS are worth\n"
                                          "5, 10, 15, 20, 25, 30 points in\n"
                                          "the first through sixth rows.\n"
                                          "Alien Mothership is worth 200\n"
    },
    
    {"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",  (char*)0},
};

char inst_text[1024];
void dsShowScreenInstructions(void)
{
  short int idx=0;
  char bFound=0;
    
  decompress(bgInstructionsTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgInstructionsMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgInstructionsPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
  swiWaitForVBlank();
    
  while (instructions[idx].text != (char*)0)
  {
    if (instructions[idx].md5 == myCartInfo.md5)
    {
        bFound = 1;
        strcpy(inst_text, instructions[idx].text);
        break;
    }
    idx++;
  }
    
  if (bFound)
  {
       short line=4;
       char *token = NULL;
       token = strtok(inst_text, "\n");
       while( token != NULL ) 
       {
          dsPrintValue(1,line++,0, token);
          token = strtok(NULL, "\n");
       }
  }
  else
  {
      dsPrintValue(1,4,0, (char *)"Game Manual Not Found");
      dsPrintValue(1,5,0, (char *)"You can add your own soon!");
  }
}
