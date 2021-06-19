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
    
    {"c5930d0e8cdae3e037349bfa08e871be",  "YARS' REVENGE (c)1982 Atari\n"
                                          " \n"
                                          "Game 0:  Easy - Slow Destroyer\n"
                                          "Game 2:  Normal Game\n"
                                          "Game 4:  Hard - Cannon Bounce\n"
                                          "Game 6:  Ultimate Yars!\n"
                                          " \n"
                                          "L-Diff:  A=Faster Swirls\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Cell hit by Missile          69\n"
                                          "Cell devoured by Yar         69\n"
                                          "Qotile destroyed           1000\n"
                                          "Swirl destroyed in place   2000\n"
                                          "Swirl destroyed in mid-air 6000\n"
                                          " \n"            
                                          "AT 70,000 Swirl Triples in Freq\n"            
                                          "AT 150,000 Swirl Heat Seaking\n"
    },
    
    {"7dcbfd2acc013e817f011309c7504daa",  "PHASER PATROL (c)1982 STARPATH\n"
                                          " \n"
                                          "L-Diff: A=Sector Map, B=Combat\n"
                                          "R-Diff: A=Harder Game\n"
                                          "Color:  Shields ON/OFF\n"
                                          " \n"  
                                          "Destroy the Dracon Armada!\n"
                                          "Use Sector Map to find enemy.\n"            
                                          "Find Starbase for Repair\n"  
                                          " \n"  
                                          "DAMAGE COLOR PER SYSTEM:\n"  
                                          "Green:  Fully Operational\n"  
                                          "Yellow: System Damaged\n"  
                                          "Red:    System Destroyed\n"  
                                          " \n"  
                                          "RANKS: HERO,ACE,PILOT,GREENHORN\n"  
    },
    
    {"2c8835aed7f52a0da9ade5226ee5aa75",  "COMMUNIST MUTANTS FROM SPACE\n"
                                          "(c)1982 STARPATH CORP \n"
                                          " \n"  
                                          "Options:\n"
                                          "PL  1   Number of Players\n"
                                          "DIF 1   Difficulty Lvl (9=hard)\n"
                                          "SH NO   SHIELDS (PRESS DOWN)\n"
                                          "TW NO   TIME WARP (PRESS UP)\n"
                                          "PF NO   PENETRATING FIRE\n"
                                          "GF NO   GUIDED FIRE\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Mother Creature      500\n"
                                          "Diving Mutant Attk    60\n"
                                          "Mutant Eggs           10\n"
                                          "Clearing the screen  100 \n"
                                          "Bonus Life after odd waves\n"
        },
    
    {"bcb2967b6a9254bcccaf906468a22241",  "DEMON ATTACK (c)1981 IMAGIC\n"
                                          " \n"  
                                          "Game Select:\n"
                                          "1  Demon Attack\n"
                                          "3  Tracer Shot Demon Attack\n"
                                          "5  Advanced Demon Attack\n"
                                          "7  Advanced Tracer Demon Attack\n"
                                          "L-DIFF:  Faster Demon Attack\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Wave  Demons  Split  Dive\n"
                                          "1-2    10     --     --\n"
                                          "3-4    15     --     --\n"
                                          "5-6    20     40     80\n"
                                          "7-8    25     50     100\n"
                                          "9-10   30     60     120\n"
                                          "11+    35     70     140\n"
        },

    {"f0e0addc07971561ab80d9abe1b8d333",  "DEMON ATTACK (c)1981 IMAGIC\n"
                                          " \n"  
                                          "Game Select:\n"
                                          "1  Demon Attack\n"
                                          "3  Tracer Shot Demon Attack\n"
                                          "5  Advanced Demon Attack\n"
                                          "7  Advanced Tracer Demon Attack\n"
                                          "L-DIFF:  Faster Demon Attack\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Wave  Demons  Split  Dive\n"
                                          "1-2    10     --     --\n"
                                          "3-4    15     --     --\n"
                                          "5-6    20     40     80\n"
                                          "7-8    25     50     100\n"
                                          "9-10   30     60     120\n"
                                          "11+    35     70     140\n"
        },

    {"b12a7f63787a6bb08e683837a8ed3f18",  "DEMON ATTACK (c)1981 IMAGIC\n"
                                          " \n"  
                                          "Game Select:\n"
                                          "1  Demon Attack\n"
                                          "3  Tracer Shot Demon Attack\n"
                                          "5  Advanced Demon Attack\n"
                                          "7  Advanced Tracer Demon Attack\n"
                                          "L-DIFF:  Faster Demon Attack\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Wave  Demons  Split  Dive\n"
                                          "1-2    10     --     --\n"
                                          "3-4    15     --     --\n"
                                          "5-6    20     40     80\n"
                                          "7-8    25     50     100\n"
                                          "9-10   30     60     120\n"
                                          "11+    35     70     140\n"
        },
    
    {"ab5bf1ef5e463ad1cbb11b6a33797228",  "COSMIC ARK (c)1981 IMAGIC\n"
                                          " \n"  
                                          "Survive the Meteor Shower\n"
                                          "and then rescue both of the\n"  
                                          "beasties on the planet!\n"  
                                          " \n"  
                                          "Game Select:\n"
                                          "Game 1: Regular Mission\n"
                                          "Game 2: Meteor Shower\n"
                                          "Game 4: Advanced Mission\n"
                                          "Game 5: Advanced Meteor Shower\n"
                                          "L-DIFF: A=Ark Size Wider\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Meteor              10 pts\n"
                                          "Wave Meteor         30 pts\n"
                                          "Rescue Beasties:  1000 pts\n"
        },

    {"c1cb228470a87beb5f36e90ac745da26",  "CHOPPER COMMAND\n"
                                          "(c)1981 BY ACTIVISION\n"
                                          " \n"  
                                          "Knock out enemy aircraft\n"
                                          "while protecting trucks.\n"
                                          " \n"  
                                          "Game Select:\n"
                                          "Game 1: Cadet Level\n"
                                          "Game 3: Commander Level\n"
                                          "L-DIFF: A=Slower Chopper Fire\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Enemy Copter       100 pts\n"
                                          "Enemy Jet          200 pts\n"
                                          "Wave Bonus/Truck:  100 pts\n"
                                          "Bonus Ship every 10000 pts\n"
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
