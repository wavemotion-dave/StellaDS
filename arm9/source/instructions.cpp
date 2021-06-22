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
    {"157bddb7192754a45372be196797f284",    "Adventure (c)1980 Atari\n"
                                            "by Warren Robinett\n"
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
    
    {"ca4f8c5b4d6fb9d608bb96bc7ebd26c7",    "Adventures of Tron (c)1982"
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
    
    {"dd7884b4f93cab423ac471aa1935e3df",  "Asteroids (c)1981 Atari\n"
                                            "GAME 1: Slow, Extra Life AT  5K\n"
                                            "GAME 2: Fast, Extra Life AT  5K\n"
                                            "GAME 3: Slow, Extra Life AT 10K\n"
                                            "GAME 4: Fast, Extra Life AT 10K\n"
                                            "GAME 5: Slow, Extra Life AT 20K\n"
                                            "GAME 6: Fast, Extra Life AT 20K\n"
                                            "GAME 9: Slow, Shields, Life 5K\n"
                                            "GAME 10:Fast, Shields, Life 5K\n"
                                            "GAME 12:Fast, Shields, Life 10K\n"
                                            "GAME 14:Fast, Shields, Life 20K\n"
                                            "L-Diff:  A=UFOs + SATELLITES\n"
                                            " \n"
                                            "SMALL ASTEROIDS   100 PTS\n"
                                            "MEDIUM ASTEROIDS   50 PTS\n"
                                            "LARGE ASTEROIDS    20 PTS\n"
                                            "SATELLITES        200 PTS\n"
                                            "UFOs             1000 PTS\n"
    },
    
    {"ccbd36746ed4525821a8083b0d6d2c2c",  "Asteroids (c)1981 Atari\n"
                                            "GAME 1: Slow, Extra Life AT  5K\n"
                                            "GAME 2: Fast, Extra Life AT  5K\n"
                                            "GAME 3: Slow, Extra Life AT 10K\n"
                                            "GAME 4: Fast, Extra Life AT 10K\n"
                                            "GAME 5: Slow, Extra Life AT 20K\n"
                                            "GAME 6: Fast, Extra Life AT 20K\n"
                                            "GAME 9: Slow, Shields, Life 5K\n"
                                            "GAME 10:Fast, Shields, Life 5K\n"
                                            "GAME 12:Fast, Shields, Life 10K\n"
                                            "GAME 14:Fast, Shields, Life 20K\n"
                                            "L-Diff:  A=UFOs + SATELLITES\n"
                                            " \n"
                                            "SMALL ASTEROIDS   100 PTS\n"
                                            "MEDIUM ASTEROIDS   50 PTS\n"
                                            "LARGE ASTEROIDS    20 PTS\n"
                                            "SATELLITES        200 PTS\n"
                                            "UFOs             1000 PTS\n"
    },

    {"9ad36e699ef6f45d9eb6c4cf90475c9f",  "ATLANTIS (c)1982 IMAGIC"
                                            " \n"
                                            "Defend Atlantis! Blast Gorgons\n"
                                            "before they demolish Atlantis!\n"
                                            " \n"
                                            "GAME 1: Normal Game\n"
                                            "GAME 2: No Command Post!\n"
                                            "GAME 4: Easy Game\n"
                                            " \n"
                                            "SCORING:\n"
                                            "Large Gorgon       100/200 PTS\n"
                                            "Small Gorgon     1000/2000 PTS\n"
                                            "BONUS 500 PTS FOR EACH AREA\n"
                                            "THAT SURVIVIES GORGON ASSAULT\n"
                                            "EXTRA LIFE EVERY 10,000 PTS\n"
                                            " \n"
    },
    

    {"79ab4123a83dc11d468fb2108ea09e2e",  "BEAMRIDER (c)1984 Activision\n"
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
    
    {"c3472fa98c3b452fa2fd37d1c219fb6f",  "DODGE-EM (c)1980 Atari\n"
                                          " \n"
                                          "L-Diff: A=Computer 2x SPEED\n"
                                          "R-Diff: A=Computer Rand Start\n"
                                          " \n"
                                          "Each dot removed is 1 point.\n"
                                          "Bonus of 8 PTS per dot if\n"
                                          "you clear the entire screen.\n"
    },
 
    {"83bdc819980db99bf89a7f2ed6a2de59",  "DODGE-EM (c)1980 Atari\n"
                                          " \n"
                                          "L-Diff: A=Computer 2x SPEED\n"
                                          "R-Diff: A=Computer Rand Start\n"
                                          " \n"
                                          "Each dot removed is 1 point.\n"
                                          "Bonus of 8 PTS per dot if\n"
                                          "you clear the entire screen.\n"
    },
    
    {"0f643c34e40e3f1daafd9c524d3ffe64",  "DEFENDER (c)1982 Atari\n"
                                          " \n"
                                          "Gm 1: NORMAL SPEED\n"
                                          "Gm 2: FASTER SPEED\n"
                                          "Gm 4: Same as GM 1 - Wave 3\n"
                                          "Gm 7: Same as GM 1 - Wave 5\n"
                                          " \n"
                                          "L-Diff:  A=Slower Defender\n"
                                          "Pod               1000 PTS\n"
                                          "Swarmer            500 PTS\n"
                                          "Bomber             250 PTS\n"
                                          "Baiter             200 PTS\n"
                                          "Lander             150 PTS\n"
                                          "Mutant             150 PTS\n"
                                          "Save Human: 250/500/1000 PTS\n"
                                          "BONUS DEFENDER EVERY 10K PTS\n"
    },
    
    {"133b56de011d562cbab665968bde352b",  "COSMIC COMMUTER\n"
                                          "(c)1984 by ACTIVISION\n"
                                          " \n"
                                          "LAND YOUR ROCKET MODULE\n"
                                          "GET COMMUTERS AT BUS STOPS\n"
                                          "BLAST OBSTACLES IN YOUR WAY\n"
                                          "RETURN TO ROCKET MODULE\n"
                                          " \n"
                                          "GAME 1:    NORMAL\n"
                                          "GAME 2:    HARDER\n"
                                          " \n"
                                          "BONUS BUS EVERY 10000 PTS\n"
    },
    
    {"3a2e2d0c6892aa14544083dfb7762782",  "MISSILE COMMAND (c)1981 Atari\n"
                                          " \n"
                                          "SAVE ZARDON CITIES FROM ATTACK\n"
                                          "Gm 1: SLOW TARGET,DUMB CRUISE\n"
                                          "Gm 2: FAST TARGET,DUMB CRUISE\n"
                                          "Gm 3: SLOW TARGET,SMART CRUISE\n"
                                          "Gm 4: FAST TARGET,SMART CRUISE\n"
                                          "Gm 5: SAME AS 1 - START WAVE 7\n"
                                          "Gm 9: SAME AS 1 - START WAVE 11\n"
                                          " \n"
                                          "L-Diff:  A=Slower ABMs\n"
                                          "IBMS                25 PTS\n"
                                          "CRUISE MISSILE     125 PTS\n"
                                          "UNUSED ABMS          5 PTS\n"
                                          "SAVED CITIES       100 PTS\n"
                                          "EVERY ODD WAVE ALL SCORING\n"
                                          "MULTIPLIES (2X,3X,4X,5X,ETC)\n"
                                          "BONUS CITY EVERY 10,000 PTS\n"
    },
    
    {"1a8204a2bcd793f539168773d9ad6230",  "MISSILE COMMAND (c)1981 Atari\n"
                                          " \n"
                                          "SAVE ZARDON CITIES FROM ATTACK\n"
                                          "Gm 1: SLOW TARGET,DUMB CRUISE\n"
                                          "Gm 2: FAST TARGET,DUMB CRUISE\n"
                                          "Gm 3: SLOW TARGET,SMART CRUISE\n"
                                          "Gm 4: FAST TARGET,SMART CRUISE\n"
                                          "Gm 5: SAME AS 1 - START WAVE 7\n"
                                          "Gm 9: SAME AS 1 - START WAVE 11\n"
                                          "L-Diff:  A=Slower ABMs\n"
                                          " \n"            
                                          "IBMS                25 PTS\n"
                                          "CRUISE MISSILE     125 PTS\n"
                                          "UNUSED ABMS          5 PTS\n"
                                          "SAVED CITIES       100 PTS\n"
                                          "EVERY ODD WAVE ALL SCORING\n"
                                          "MULTIPLIES (2X,3X,4X,5X,ETC)\n"
                                          "BONUS CITY EVERY 10,000 PTS\n"
    },
    
    {"72ffbef6504b75e69ee1045af9075f66",  "SPACE INVADERS (c)1980 Atari\n"
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
    
    {"136f75c4dd02c29283752b7e5799f978",  "BERZERK (c)1982 Atari\n"
                                          " \n"  
                                          "GAME SELECT:\n"
                                          "GAME 1: Extra Life 1K. No Otto\n"
                                          "GAME 2: Extra Life 1K. Nor Otto\n"
                                          "GAME 3: Extra Life 1K. Inv Otto\n"
                                          "GAME 4: Extra Life 2K. No Otto\n"
                                          "GAME 5: Extra Life 2K. Nor Otto\n"
                                          "GAME 6: Extra Life 2K. Inv Otto\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Each Robot      50 PTS EACH\n"
                                          "All Robot Bonus 10 PTS EACH\n"
        },
        
    {"91c2098e88a6b13f977af8c003e0bca5",  "CENTIPEDE (c)1982 Atari\n"
                                          " \n"  
                                          "GAME SELECT:\n"
                                          "GAME 1: Easy Play\n"
                                          "GAME 2: Standard Play\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Centipede Body    10 PTS\n"
                                          "Centipede Head   100 PTS\n"
                                          "Spider (distant) 300 PTS\n"
                                          "Spider (medium)  600 PTS\n"
                                          "Spider (close!)  900 PTS\n"
                                          "Flea             200 PTS\n"
                                          "Scorpion        1000 PTS\n"
                                          "Mushroom (dest)    1 PTS\n"
                                          "Mushroom (wound)   5 PTS\n"
                                          "Bonus Wand Every 10,000 PTS\n"
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
    
    {"7657b6373fcc9ad69850a687bee48aa1",  "ELEVATORS AMISS\n"
                                          "(c)2007 BY BMONTGOMERY\n"
                                          " \n"  
                                          "Guide Maid Maria to the top\n"
                                          "floor while avoiding Elevators\n"  
                                          " \n"  
                                          "GAME     ELEV  BONUS\n"
                                          "NORMAL:   7     40 pts\n"
                                          "EXPERT:   7     60 pts\n"
                                          "CHILD:    2      0 pts\n"
                                          "NOVICE:   4     20 pts\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Get Across Floor:  Timer pts\n"
                                          "Get to top Floor:  Bonus pts\n"
                                          " \n"  
                                          "Press Joy Button for SPEED!\n"  
        },
    
    {"fb4ca865abc02d66e39651bd9ade140a",  "RABBIT TRANSIT (c)1983 ARCADIA\n"
                                          " \n"  
                                          "Guide Your Bunny to the\n"
                                          "Bunny Bushes!\n"
                                          " \n"  
                                          "L-DIFF: A=Turtle Submerges\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Each Hop Scores Points\n"
                                          "Bonus Points for Time Remain\n"
                                          " \n"  
                                          "Extra Bunny at 10,000 PTS\n"  
                                          " \n"
        
        },
        
    {"240bfbac5163af4df5ae713985386f92",  "SEA QUEST (c)1983 ACTIVISION\n"
                                          " \n"  
                                          "Rescue Divers from Subs and\n"
                                          "Sharks in the Deep Ocean.\n"
                                          "Don't run out of Oxygen!\n"
                                          " \n"  
                                          "L-DIFF: A=Slower Fire\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Subs/Sharks      10 PTS\n"
                                          "Rescued Divers   50 PTS\n"
                                          "Points Increase when you\n"  
                                          "Bring 6 Divers to Surface\n"  
                                          "Bonus Points for OXYGEN Remain\n"  
        },
        
    {"393948436d1f4cc3192410bb918f9724",  "RIVER RAID (c)1982 ACTIVISION\n"
                                          " \n"  
                                          "DESTROY ENEMIES AS YOU SEEK\n"  
                                          "BRIDGE AT END OF EVERY SECTION\n"
                                          "WATCH YOUR FUEL CONSUMPTION!\n"
                                          " \n"  
                                          "L-DIFF: A=Unguided Shots\n"
                                          " \n"  
                                          "SCORING:\n"
                                          "Tanker       30 PTS\n"
                                          "Helicopter   60 PTS\n"
                                          "Fuel Depot   80 PTS\n"
                                          "Jet         100 PTS\n"
                                          "Bridge      500 PTS\n"
                                          " \n"
                                          "BONUS JET EVERY 10,000 PTS\n"
    },
        
    {"5f383ce70c30c5059f13e89933b05c4a",  "CIRCUS ATARI (c)1980 ATARI\n"
                                          " \n"  
                                          "Pop  balloons to score!\n"  
                                          " \n"  
                                          "L-DIFF: A=FASTER CLOWN BOUNCE\n"
                                          " \n"  
                                          "GAME 1: NORMAL BOUNCE\n"
                                          "GAME 2: EXTRA WALL\n"
                                          "GAME 3: BREAKTHRU (EASIER)\n"
                                          "GAME 4: BREAKTHRU + WALL\n"
                                          "GAME 5: MUST POP ALL BALOONS\n"
                                          "SCORING:\n"
                                          "TRAMPOLINE BOUNCE:   1 PTS\n"
                                          "RED BALOON POP:     10 PTS\n"
                                          "BLUE BALOON POP:     5 PTS\n"
                                          "WHITE BALOON POP:    2 PTS\n"
                                          "CLEAR ROW BONUS 100/50/20 PTS\n"
                                          " \n"
    },
    
    {"c9b7afad3bfd922e006a6bfc1d4f3fe7",  "BOWLING (c)1980 ATARI\n"
                                          " \n"  
                                          "MAX SCORE IS 300 PTS!\n"  
                                          " \n"  
                                          "L-DIFF: A=HARDER TO STRIKE\n"
                                          " \n"  
                                          "SELECT:\n"
                                          "GAME 1: Curve Ball\n"
                                          "GAME 3: Steer Ball\n"
                                          "GAME 5: Straight Ball\n"
                                          " \n"
    },
    
    {"515046e3061b7b18aa3a551c3ae12673",  "MOON PATROL (c)1983 ATARI\n"
                                          " \n"  
                                          "L-DIFF: A=MUSIC OFF\n"
                                          "R-DIFF: A=TWO BUGGY SPEEDS\n"
                                          "CHOOSE EASY, MED, HARD GAME\n"
                                          " \n"
                                          " JUMPING   OBSTACLE    SHOOTING\n"
                                          " 100 PTS   SM CRATER   --- PTS\n"
                                          " 200 PTS   LG CRATER   --- PTS\n"
                                          "  50 PTS   LAND MINE   --- PTS\n"
                                          "  80 PTS   ROCK        100 PTS\n"
                                          " 100 PTS   TANK        200 PTS\n"
                                          " --- PTS   REGULAR UFO 100 PTS\n"
                                          " --- PTS   CRATER UFO  200 PTS\n"
                                          " --- PTS   ENEMY CAR 500/800/1K\n"
                                          " \n"
                                          "BONUS PTS FOR COMPLETE COURSE\n"
                                          "BONUS BUGGY AT 10k,30k,50k\n"
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
      dsPrintValue(1,5,0, (char *)"Many more comming soon!");
  }
}
