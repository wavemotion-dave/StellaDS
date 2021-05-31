//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Cart.cxx,v 1.8 2005/02/13 19:17:02 stephena Exp $
//============================================================================
#include <nds.h>

#include <assert.h>
#include <string.h>
#include "Cart.hxx"
#include "Cart2K.hxx"
#include "Cart3E.hxx"
#include "Cart3F.hxx"
#include "Cart4K.hxx"
#include "CartAR.hxx"
#include "CartDPC.hxx"
#include "CartDPCPlus.hxx"
#include "CartE0.hxx"
#include "CartE7.hxx"
#include "CartF4.hxx"
#include "CartF4SC.hxx"
#include "CartF6.hxx"
#include "CartF6SC.hxx"
#include "CartF8.hxx"
#include "CartF8SC.hxx"
#include "CartFASC.hxx"
#include "CartFE.hxx"
#include "CartMC.hxx"
#include "CartMB.hxx"
#include "CartCV.hxx"
#include "CartUA.hxx"
#include "MD5.hxx"

extern void dsWarnIncompatibileCart(void);
extern void dsPrintCartType(char *);

// We can store up to 8k in the fast DTCM memory to give a speed boost... This helps 2k/4k and 8k carts... plus Starpath Supercharger "AR" carts
uInt8 fast_cart_buffer[8*1024] __attribute__((section(".dtcm")));
CartInfo myCartInfo __attribute__((section(".dtcm")));

static const CartInfo table[] = 
{
    {"DefaultCart_xxxxxxxxxxxxxxxxxxxx",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Default Cart is 4k, full-scale, L-Joy and nothing special...
    {"0db4f4150fecf77e4ce72ca4d04c052f",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // 3-D Tic-Tac-Toe (1980).bin
    {"b6d13da9e641b95352f090090e440ce4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Aardvark.bin
    {"b6c2b4ddc42ef5db21cfcc3f8be120d6",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  198,   0,  1},    // Aardvark (32k Demo).bin    
    {"ac7c2260378975614192ca2bc3d20e0b",  "FE",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Activision Decathlon (1983).bin
    {"525f2dfc8b21b0186cff2568e0509bfc",  "FE",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Activision Decathlon (1983) [fixed].bin
    {"a1bcbe0bfe6570da2661fc4de2f74e8a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Actionauts.bin
    {"157bddb7192754a45372be196797f284",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   0,  0,  ANA1_0,  210,   0,  1},    // Adventure (1980).bin
    {"ca4f8c5b4d6fb9d608bb96bc7ebd26c7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  0,  ANA1_0,  210,   0,  5},    // Adventures of TRON (1982).bin
    {"4d77f291dca1518d7d8e47838695f54b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Airlock (1982).bin
    {"a9cb638cd2cb2e8e0643d7a67db4281c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  5},    // Air Raiders (1982).bin
    {"16cb43492987d2f32b423817cdaaf7c4",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Air-Sea Battle (1977).bin
    {"0c7926d660f903a2d6910c254660c32c",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Air-Sea Battle (pal).bin
    {"1d1d2603ec139867c1d1f5ddf83093f1",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Air-Sea Battle (sears).bin
    {"f1a0a23e6464d954e3a9579c4ccd01c8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Alien (1982).bin
    {"3aa47765f6184e64c41d1fa2b0b16ddc",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Alligator32.bin
    {"e1a51690792838c5c687da80cd764d78",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Alligator People (1983).bin
    {"9e01f7f95cb8596765e03b9a36e8e33c",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Alpha Beam with Ernie (1983).bin
    {"acb7750b4d0c4bd34969802a7deb2990",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Amidar (1982).bin
    {"539f3c42c4e15f450ed93cb96ce93af5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  201,   0,  1},    // Amoeba Jump.bin
    {"0866e22f6f56f92ea1a14c8d8d01d29c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // AndroMan on the Moon (1984).bin
    {"428428097f85f74dc5cf0dbe07cf16e0",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Anguna-rc3.bin
    {"e73838c43040bcbc83e4204a3e72eef4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Apples and Dolls (CCE).bin
    {"f69d4fcf76942fcd9bdf3fd8fde790fb",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Aquaventure (CCE).bin
    {"a7b584937911d60c120677fe0d47f36f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Armor Ambush (1982).bin
    {"c77c35a6fc3c0f12bf9e8bae48cba54b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Artillery Duel (1983).bin
    {"de78b3a064d374390ac0710f95edde92",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  201,   0,  0},    // Assault (AKA Sky Alien) (1983).bin
    {"c50fbee08681f15d2d40dbc693d3a837",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // A Star.bin
    {"89a68746eff7f266bbf08de2483abe55",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Asterix (AKA Taz) (1983).bin
    {"dd7884b4f93cab423ac471aa1935e3df",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Asteroids (1981).bin
    {"ccbd36746ed4525821a8083b0d6d2c2c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Asteroids (1981) [no copyright].bin
    {"170e7589a48739cfb9cc782cbb0fe25a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -3},    // Astroblast (1982).bin
    {"8f53a3b925f0fd961d9b8c4d46ee6755",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Astrowar (Unknown).bin
    {"f0631c6675033428238408885d7e4fde",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Atari 2600 Test Cart.bin
    {"4edb251f5f287c22efc64b3a2d095504",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Atari VCS Point-of-Purchase ROM (1982).bin
    {"3f540a30fdee0b20aed7288e4a5ea528",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  206,   0,  3},    // Atari Video Cube (1982).bin
    {"cd5af682685cfecbc25a983e16b9d833",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // A-Team (AKA Saboteur) (1984).bin
    {"9ad36e699ef6f45d9eb6c4cf90475c9f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // Atlantis (1982).bin
    {"826481f6fc53ea47c9f272f7050eedf7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // Atlantis II (1982).bin
    {"14d8bf013eed9edd76e55b86a27709d8",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Ature (2010) (beoran).bin
    {"4eb7b733de3e61184341f46a24f8e489",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Avalanche.bin
    {"cb81972e2cd9b175ded45d7f0892da42",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // AVCSTec Challenge.bin
    {"274d17ccd825ef9c728d68394b4569d2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Bachelorette Party (1982).bin
    {"5b124850de9eea66781a50b2e9837000",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Bachelor Party (1982).bin
    {"0f34f9158b4b85707d465a06d9b270bf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // BackFire.bin
    {"8556b42aa05f94bc29ff39c39b11bff4",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Backgammon (1979).bin
    {"19a15ccf2fc4f81f6223150078978e0a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Balloon TripV3.1.bin
    {"00ce0bdd43aed84a983bef38fe7f5ee3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Bank Heist (1983).bin
    {"f8240e62d8c0a64a61e19388414e3104",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  3},    // Barnstorming (1982).bin
    {"819aeeb9a2e11deb54e6de334f843894",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Basic Math (1977).bin
    {"9f48eeb47836cf145a15771775f0767a",  "4K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // BASIC Programming (1979).bin
    {"ab4ac994865fb16ebb85738316309457",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Basketball (1978).bin
    {"41f252a66c6301f1e8ab3612c19bc5d4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  0},    // Battlezone (1983).bin
    {"79ab4123a83dc11d468fb2108ea09e2e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   4,  0},    // Beamrider (1984).bin
    {"d0b9df57bfea66378c0418ec68cfe37f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Beany Bopper (1982).bin
    {"59e96de9628e8373d1c685f5e57dcf10",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Beat 'Em & Eat 'Em (1982).bin
    {"0d22edcb05e5e52f45ddfe171e3c4b50",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Bee Ball.bin
    {"d655f3b4e327d2713f52b2e342bf25c6",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Bell Hopper (2011) (Tjoppen) (NTSC).bin    
    {"ee6665683ebdb539e89ba620981cb0f6",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Berenstain Bears (1983).bin
    {"b8ed78afdb1e6cfe44ef6e3428789d5f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Bermuda Triangle (1982).bin
    {"136f75c4dd02c29283752b7e5799f978",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Berzerk (1982).bin
    {"be41463cd918daef107d249f8cde3409",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Berzerk-VE.bin
    {"1badbf0d3cb5abf7cf29233120dc14cc",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // BiFrost (RC).bin
    {"1802cc46b879b229272501998c5de04f",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Big Bird's Egg Catch (1983).bin
    {"ffcb629e39e0455b37626ca1cf360db2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // BirdandbeansV2.1.bin
    {"a6239810564638de7e4c54e66b3014e4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Birthday Mania.bin
    {"0a981c03204ac2b278ba392674682560",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Blackjack (1977).bin
    {"42dda991eff238d26669fd33e353346d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Blinky Goes Up.bin
    {"33d68c3cd74e5bc4cf0df3716c5848bc",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Blueprint (1983).bin
    {"968efc79d500dce52a906870a97358ab",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // BMX Air Master (1990).bin
    {"521f4dd1eb84a09b2b19959a41839aad",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Bobby Is Going Home (1983).bin
    {"c59633dbebd926c150fb6d30b0576405",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  5},    // Bogey Blaster (1989).bin
    {"a5855d73d304d83ef07dde03e379619f",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Boggle (1978).bin
    {"14c2548712099c220964d7f044c59fd9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Boing! (1983).bin
    {"0ff357ea06b7f4f542cbfdd953eb159e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   4,  4},    // Bombs Away! (2010) (Steve Engelhardt).bin
    {"ab48c4af46c8b34c3613d210e1206132",  "3E",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Boulder Dash Demo V2.bin
    {"594dbc80b93fa5804e0f1368c037331d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Bouncin' Baby Bunnies (Prototype).bin
    {"c9b7afad3bfd922e006a6bfc1d4f3fe7",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Bowling (1979).bin
    {"c3ef5c4653212088eda54dc91d787870",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Boxing (1980).bin
    {"1cca2197d95c5a41f2add49a13738055",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Brain Games (1978).bin
    {"f34f08e5eb96e500e851a80be3277a56",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_1,  210,   0,  0},    // Breakout (1978).bin
    {"6c76fe09aa8b39ee52035e0da6d0808b",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_1,  210,   0,  0},    // Breakout (1978).bin
    {"4df6124093ccb4f0b6c26a719f4b7706",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_1,  210,   0,  0},    // Breakout (1978).bin
    {"9a25b3cfe2bbb847b66a97282200cca2",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_1,  210,   0,  0},    // Breakout (1978).bin    
    {"c5fe45f2734afd47e27ca3b04a90213c",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_1,  210,   0,  0},    // Breakout (1978).bin        
    {"413c925c5fdcea62842a63a4c671a5f2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Bridge (1980).bin
    {"cfd6a8b23d12b0462baf6a05ef347cd8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Bridge (1980).bin    
    {"1cf59fc7b11cdbcefe931e41641772f6",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  5},    // Buck Rogers (1983).bin
    {"68597264c8e57ada93be3a5be4565096",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Bugs (1982).bin
    {"fa4404fabc094e3a31fcd7b559cdd029",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Bugs Bunny (1983).bin
    {"aa1c41f86ec44c0a44eb64c332ce08af",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Bumper Bash (1983).bin
    {"76f53abbbf39a0063f24036d6ee0968a",  "E7",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Bump 'n' Jump (1983) [alt].bin
    {"ab2cfcaad3daaf673b2b14fdbb8dac33",  "E7",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Bump 'n' Jump (1983).bin
    {"0443cfa9872cdb49069186413275fa21",  "E7",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // BurgerTime (1983).bin
    {"19d6956ff17a959c48fcd8f4706a848d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Burning Desire (1982).bin
    {"466c9d4a93668ab6a052c268856cf4a5",  "F4SC", CTR_PADDLE0,   SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Circus Atari Age PRGE Demo.bin
    {"66fcf7643d554f5e15d4d06bab59fe70",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Cabbage Patch Kids (1984).bin
    {"7f6533386644c7d6358f871666c86e79",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Cakewalk (1983).bin
    {"9ab72d3fd2cc1a0c9adb504502579037",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  4},    // California Games (1988).bin
    {"9d652c50a0f101d8ea071753bddb8f53",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Candy Catcher (2011) (Grant Thienemann).bin
    {"79e151d8ca4fb96d46dfbc685323c0fe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Cannonhead Clash.bin
    {"feedcc20bc3ca34851cd5d9e38aa2ca6",  "2K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Canyon Bomber (1979).bin
    {"151c33a71b99e6bcffb34b43c6f0ec23",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Care Bears (1983).bin
    {"028024fb8e5e5f18ea586652f9799c96",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  200,   0,  3},    // Carnival (1982).bin
    {"b816296311019ab69a21cb9e9e235d12",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Casino (1979).bin
    {"9e192601829f5f5c2d3b51f8ae25dbe5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  9},    // Cathouse Blues (1982).bin
    {"d071d2ec86b9d52b585cc0382480b351",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  202,   0, -1},    // Cat Tracks.bin
    {"76f66ce3b83d7a104a899b4b3354a2f2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  202,   0, -1},    // Cat Trax (1983).bin
    {"3002e64a33a744487272be26d6069b3a",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Cave 1K (2004) (Thomas Jentzsch).bin    
    {"1cedebe83d781cc22e396383e028241a",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Cave In.bin
    {"049b33b03e7928af596c9d683f587475",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Caverns.bin
    {"91c2098e88a6b13f977af8c003e0bca5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Centipede (1982).bin
    {"5d799bfa9e1e7b6224877162accada0d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Challenge of.... Nexar (1982).bin
    {"73158ea51d77bf521e1369311d26c27b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  206,   0,  3},    // Challenge (Zellers).bin
    {"ce09df4f125e49a8239c954e22fe8adb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Chakdust - NTSC Cart.bin    
    {"ace319dc4f76548659876741a6690d57",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Championship Soccer (1980).bin
    {"3e33ac10dcf2dff014bc1decf8a9aea4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Chase the Chuckwagon (1983).bin
    {"3f5a43602f960ede330cd2f43a25139e",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   6,  2},    // Checkers (1980).bin
    {"749fec9918160921576f850b2375b516",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  190,   0, -2},    // China Syndrome (1982).bin
    {"c1cb228470a87beb5f36e90ac745da26",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  5},    // Chopper Command (1982).bin
    {"3f58f972276d1e4e0e09582521ed7a5b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Chuck Norris Superkicks (1983).bin
    {"a7b96a8150600b3e800a4689c3ec60a2",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA0_7,  210,   0,  0},    // Circus Atari (1980).bin
    {"5f383ce70c30c5059f13e89933b05c4a",  "F8",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA0_7,  210,   0,  0},    // Circus Atariage 2020.bin  
    {"13a11a95c9a9fb0465e419e4e2dbd50a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Climber5.bin
    {"1e587ca91518a47753a28217cd4fd586",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Coco Nuts (1982).bin
    {"5846b1d34c296bf7afc2fa05bbc16e98",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Codebreaker (1978).bin
    {"e3aa9681fab887a5cb964a55b4edd5d7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Colony 7.bin
    {"4c8832ed387bbafc055320c05205bc08",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  199,   0,  3},    // Combat (1977).bin
    {"5d2cc33ca798783dee435eb29debf6d6",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   5,  4},    // Commando (1988).bin
    {"f457674cef449cfd85f21db2b4f631a7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Commando Raid (1982).bin
    {"b98cc2c6f7a0f05176f74f0f62c45488",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // CompuMate.bin
    {"2c8835aed7f52a0da9ade5226ee5aa75",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   0,  1,  ANA1_0,  210,   4,  0},    // Communist Mutants from Space (1982).bin
    {"6a2c68f7a77736ba02c0f21a6ba0985b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  8},    // Computer Chess (1978).bin
    {"1f21666b8f78b65051b7a609f1d48608",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  200,   0, -2},    // Condor Attack (1982).bin
    {"f965cc981cbb0822f955641f8d84e774",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  192,   0, -1},    // Confrontation (1983).bin
    {"00b7b4cbec81570642283e7fc1ef17af",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  9},    // Congo Bongo (1983).bin
    {"0f604cd4c9d2795cf5746e8af7948064",  "F6",   CTR_LJOY,      SPEC_CONMARS,   MODE_NO,   1,  1,  ANA1_0,  210,   0,  7},    // Conquest of Mars.bin
    {"57c5b351d4de021785cf8ed8191a195c",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Cookie Monster Munch (1983).bin
    {"ab5bf1ef5e463ad1cbb11b6a33797228",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Cosmic Ark (1982).bin
    {"7d903411807704e725cf3fafbeb97255",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Cosmic Ark (1982) [selectable starfield].bin
    {"133b56de011d562cbab665968bde352b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Cosmic Commuter (1984).bin
    {"f367e58667a30e7482175809e3cec4d4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Cosmic Corridor (1983).bin
    {"3c853d864a1d5534ed0d4b325347f131",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Cosmic Creeps (1982).bin
    {"e5f17b3e62a21d0df1ca9aee1aa8c7c5",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -1},    // Cosmic Swarm (1982).bin
    {"fe67087f9c22655ce519616fc6c6ef4d",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   3,  1},    // Crack'ed (1988).bin
    {"a184846d8904396830951217b47d13d9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  7},    // Crackpots (1983).bin
    {"fb88c400d602fe759ae74ef1716ee84e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  203,   0, -1},    // Crash Dive (1983).bin
    {"9c53b60a7b439a01efa46ae1effa348e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Crazy Ballon.bin
    {"55ef7b65066428367844342ed59f956c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Crazy Climber (1982).bin
    {"4a7eee19c2dfb6aeb4d9d0a01d37e127",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Crazy Valet.bin
    {"8cd26dcf249456fe4aeb8db42d49df74",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  0},    // Crossbow (1987).bin
    {"c17bdc7d14a36e10837d039f43ee5fa3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Cross Force (1982).bin
    {"74f623833429d35341b7a84bc09793c0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Cruise Missile (1987).bin
    {"384f5fbf57b5e92ed708935ebf8a8610",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Crypts of Chaos (1982).bin
    {"1c6eb740d3c485766cade566abab8208",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Crystal Castles (1984).bin
    {"6fa0ac6943e33637d8e77df14962fbfc",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Cubicolor (1982).bin
    {"58513bae774360b96866a07ca0e8fd8e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Custer's Revenge (1982).bin
    {"a422194290c64ef9d444da9d6a207807",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   3,  3},    // Dark Cavern (1982).bin
    {"106855474c69d08c8ffa308d47337269",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Dark Chambers (1988).bin
    {"6333ef5b5cbb77acd47f558c8b7a95d3",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Dark Mage (8K) (PD).bin
    {"e4c00beb17fdc5881757855f2838c816",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Deadly Duck (1982).bin
    {"4e15ddfd48bca4f0bf999240c47b49f5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  195,   0,  2},    // Death Trap (1983).bin
    {"0f643c34e40e3f1daafd9c524d3ffe64",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Defender (1982).bin
    {"3a771876e4b61d42e3a3892ad885d889",  "F8SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Defender II (Stargate) (1988).bin
    {"d09935802d6760ae58253685ff649268",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Demolition Herby (1983).bin
    {"bcb2967b6a9254bcccaf906468a22241",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  6},    // Demon Attack (1981).bin
    {"f0e0addc07971561ab80d9abe1b8d333",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  6},    // Demon Attack (1981) [alt].bin
    {"b12a7f63787a6bb08e683837a8ed3f18",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  6},    // Demon Attack (1981) [fixed].bin
    {"f91fb8da3223b79f1c9a07b77ebfa0b2",  "4K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   1,  1,  ANA0_9,  210,   0,  0},    // Demons to Diamonds (1982).bin
    {"fd4f5536fd80f35c64d365df85873418",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -4},    // Desert Falcon (1987).bin
    {"9222b25a0875022b412e8da37e7f6887",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -3},    // Dice Puzzle (1983).bin
    {"6dda84fb8e442ecf34241ac0d1d91d69",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Dig Dug (1983).bin
    {"939ce554f5c0e74cc6e4e62810ec2111",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  6},    // Dishaster (1983).bin
    {"f1ae6305fa33a948e36deb0ef12af852",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Donkey Kong VCS.bin
    {"494cda91cc640551b4898c82be058dd9",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Donkey Kong VCS (pal).bin    
    {"c3472fa98c3b452fa2fd37d1c219fb6f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  8},    // Dodge 'Em (1980).bin
    {"83bdc819980db99bf89a7f2ed6a2de59",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  8},    // Dodge 'Em (1980) [fixed].bin
    {"c6124a6c82c5b965f6afcf01f2790697",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  198,   0,  8},    // Doggone_It!.bin
    {"ca09fa7406b7d2aea10d969b6fc90195",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  5},    // Dolphin (1983).bin
    {"937736d899337036de818391a87271e0",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  2},    // Donald Duck's Speedboat (1983).bin
    {"36b20c427975760cb9cf4a47e41369e4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Donkey Kong (1982).bin
    {"c8fa5d69d9e555eb16068ef87b1c9c45",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Donkey Kong Junior (1983).bin
    {"7386004f9a5a7daf7e50ac8547088337",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // DOT.bin
    {"7e2fe40a788e56765fe56a3576019968",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  6},    // Double Dragon (1989).bin
    {"368d88a6c071caba60b4f778615aae94",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Double Dunk (Super Basketball) (1989).bin
    {"41810dd94bd0de1110bedc5092bef5b0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Dragonfire (1982).bin
    {"90ccf4f30a5ad8c801090b388ddd5613",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   3,  0},    // Dragonstomper (Excalibur) (1982).bin
    {"77057d9d14b99e465ea9e29783af0ae3",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   0, -2},    // Dragster (1980).bin
    {"5de972bc653d2a83645ce0fc49e3f05c",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Duck Attack.BIN
    {"51de328e79d919d7234cf19c1cd77fbc",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Dukes of Hazzard (1983).bin
    {"3897744dd3c756ea4b1542e5e181e02a",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Dumbo's Flying Circus (1983).bin
    {"469473ff6fed8cc8d65f3c334f963aab",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -3},    // Dune (1984).bin
    {"d4f23c92392474b8bc79184f1deb1b4d",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Dungeon.bin
    {"e2e514c3176b8215dd1fbc8fa83fd8f2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Dungeon II.bin
    {"033e21521e0bf4e54e8816873943406d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  198,   0, -3},    // Earth Dies Screaming (1983).bin
    {"683dc64ef7316c13ba04ee4398e2b93a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Edtris (1995).bin
    {"db41f3ffc90cbb22a289386a85c524fe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Egg Drop (2011) (Mr. Podoboo).bin    
    {"42b2c3b4545f1499a083cfbc4a3b7640",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Eggomania (1982).bin
    {"71f8bacfbdca019113f3f0801849057e",  "F8SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Elevator Action (1983).bin
    {"7657b6373fcc9ad69850a687bee48aa1",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   0,  0,  ANA1_0,  210,   0,  1},    // Elevators Amiss.bin
    {"b6812eaf87127f043e78f91f2028f9f4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Eli's Ladder (1982).bin
    {"7eafc9827e8d5b1336905939e097aae7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Elk Attack (1987).bin
    {"dbc8829ef6f12db8f463e30f60af209f",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Encounter at L-5 (1982).bin
    {"94b92a882f6dbaa6993a46e2dcc58402",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4, -1},    // Enduro (1983).bin
    {"9f5096a6f1a5049df87798eb59707583",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  206,   0, -2},    // Entity (1983).bin
    {"6b683be69f92958abe0e2a9945157ad5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  204,   0,  2},    // Entombed (1982).bin
    {"7e6a1375ee356f5a682f643bb8b7090c",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Epic Adventure v28.bin
    {"8334075902fa9f3471905a30fc84e706",  "F8SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Escape It (2009) (Alan W. Smith).bin
    {"81f4f0285f651399a12ff2e2f35bab77",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   0,  1,  ANA1_0,  210,   0,  0},    // Escape from the Mindmaster (1982).bin
    {"f344ac1279152157d63e64aa39479599",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Espial (1984).bin
    {"615a3bf251a38eb6638cdc7ffbde5480",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // E.T. - The Extra-Terrestrial (1982).bin
    {"ebd2488dcace40474c1a78fa53ebfadf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Extra Terrestrials.bin   
    {"6205855cc848d1f6c4551391b9bfa279",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Euchre.bin
    {"e04c8ecae485b6970d680c202e58f843",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Evil Magician Returns.bin
    {"6362396c8344eec3e86731a700b13abf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Exocet (1983).bin
    {"76181e047c0507b2779b4bcbf032c9d5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Fall Down.bin
    {"b80d50ecee73919a507498d0a4d922ae",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Fantastic Voyage (1982).bin
    {"f7e07080ed8396b68f2e5788a5c245e2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Farmyard Fun.bin
    {"9de0d45731f90a0a922ab09228510393",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Fast Eddie (1982).bin
    {"665b8f8ead0eef220ed53886fbd61ec9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Fast Food (1982).bin
    {"85470dcb7989e5e856f36b962d815537",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  0},    // Fatal Run (1989).bin
    {"0b55399cf640a2a00ba72dd155a0c140",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Fathom (1983).bin
    {"e4b12deaafd1dbf5ac31afe4b8e9c233",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Fellowship of the Ring.bin
    {"211fbbdbbca1102dc5b43dc8157c09b3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Final Approach (1982).bin
    {"386ff28ac5e254ba1b1bac6916bcc93a",  "AR",   CTR_PADDLE0,   SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Fireball (Frantic) (1982).bin
    {"d09f1830fb316515b90694c45728d702",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Fire Fighter (1982).bin
    {"20dca534b997bf607d658e77fbb3c0ee",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  5},    // Fire Fly (1983).bin
    {"d3171407c3a8bb401a3a62eb578f48fb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Fire Spinner.bin
    {"3fe43915e5655cf69485364e9f464097",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  9},    // Fisher Price (1983).bin
    {"b8865f05676e64f3bec72b9defdacfa7",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   0,  0,  ANA1_0,  210,   0,  4},    // Fishing Derby (1980).bin
    {"db112399ab6d6402cc2b34f18ef449da",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Fixit.bin
    {"30512e0e83903fc05541d2f6a6a62654",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Flag Capture (1978).bin
    {"163ff70346c5f4ce4048453d3a2381db",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // FlapPing.bin
    {"8786c1e56ef221d946c64f6b65b697e9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Flash Gordon (1983).bin
    {"e549f1178e038fa88dc6d657dc441146",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Football (1979).bin
    {"e275cbe7d4e11e62c3bfcfb38fca3d49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   4,  2},    // Football (AKA Super Challenge Football) (1989).bin
    {"5926ab1a9d1d34fb7c3bfd5afff6bc80",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Four Play.bin
    {"15dd21c2608e0d7d9f54c0d3f08cca1f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Frankenstein's Monster (1983).bin
    {"16c909c16ce47e2429fe6e005551178d",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Frantic.bin
    {"8e0ab801b1705a740b476b7f588c6d16",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   5,  2},    // Freeway (1981).bin
    {"e80a4026d29777c3c7993fbfaee8920f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Frisco (Unknown).bin
    {"081e2c114c9c20b61acf25fc95c71bf4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -3},    // Frogger (1982).bin
    {"27c6a2ca16ad7d814626ceea62fa8fb4",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Frogger II - Threeedeep! (1984).bin
    {"f67181b3a01b9c9159840b15449b87b0",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Frog Pond (1982).bin
    {"dcc2956c7a39fdbf1e861fc5c595da0d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  7},    // Frogs and Flies (1982).bin
    {"e556e07cc06c803f2955986f53ef63ed",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Front Line (1984).bin
    {"4ca73eb959299471788f0b685c3ba0b5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  5},    // Frostbite (1983).bin
    {"d3bb42228a6cd452c111c1932503cc03",  "UA",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Funky Fish (1983).bin
    {"819aeeb9a2e11deb54e6de334f843894",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Fun with Numbers (1980).bin
    {"f38210ca3955a098c06a1e1c0004ef39",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Galactopus20141115.bin
    {"211774f4c5739042618be8ff67351177",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Galaxian (1983).bin
    {"102672bbd7e25cd79f4384dd7214c32b",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Game of Concentration (1980).bin
    {"31f4692ee2ca07a7ce1f7a6a1dab4ac9",  "4K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Game of Concentration (1980).bin       
    {"db971b6afc9d243f614ebf380af0ac60",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, 13},    // Gamma-Attack (1983).bin
    {"f16ef574d2042ed8fe877d6541f4dba4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  200,   0,  2},    // Gangster Alley (1982).bin
    {"20edcc3aa6c189259fa7e2f044a99c49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  200,   0,  2},    // Gangster Alley (1982) [fixed].bin
    {"dc13df8420ec69841a7c51e41b9fbba5",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Garfield (1984).bin
    {"728152f5ae6fdd0d3a9b88709bee6c7a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gas Hog (1983).bin
    {"5cbd7c31443fb9c308e9f0b54d94a395",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gas Hog (1983) [fixed].bin
    {"e9ecb4102242f662acbf6ea6e77fa940",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // GateRacerII.bin
    {"e64a8008812327853877a37befeb6465",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Gauntlet (1983).bin
    {"e314b42761cd13c03def744b4afc7b1b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Ghostbusters (1985).bin
    {"2bee7f226d506c217163bad4ab1768c0",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Ghost Manor (1983).bin
    {"1c8c42d1aee5010b30e7f1992d69216e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  9},    // Gigolo (1982).bin
    {"c1fdd44efda916414be3527a47752c75",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   0, -2},    // G.I. Joe - Cobra Strike.bin
    {"5e0c37f534ab5ccc4661768e2ddf0162",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Glacier Patrol (1989).bin
    {"2d9e5d8d083b6367eda880e80dfdfaeb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Glib - Video Word Game (1983).bin
    {"787a2faebadc670a887a0e2483b8f034",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Go Fish.bin
    {"0c4e5087d748c67a81d114b3a9b5a112",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Go Fish - Extended.bin
    {"4093382187f8387e6d011883e8ea519b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Go Go Home Monster (Unknown).bin
    {"2e663eaa0d6b723b645e643750b942fd",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   3,  0},    // Golf (1980).bin
    {"c16c79aad6272baffb8aae9a7fff0864",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gopher (1982).bin
    {"81b3bf17cf01039d311b4cd738ae608e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gorf (1982).bin
    {"2903896d88a341511586d69fcfc20f7d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Grand Prix (1982).bin
    {"7146dd477e019f81eac654a79be96cb5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gravitar (1983) [alt].bin
    {"8ac18076d01a6b63acf6e2cab4968940",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gravitar (1983).bin
    {"9245a84e9851565d565cb6c9fac5802b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Great Escape (1983).bin
    {"01cb3e8dfab7203a9c62ba3b94b4e59f",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gremlins (1984).bin
    {"66b89ba44e7ae0b51f9ef000ebba1eb7",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Grover's Music Maker (1983).bin
    {"7ab2f190d4e59e8742e76a6e870b567e",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Guardian (1982).bin
    {"f750b5d613796963acecab1690f554ae",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gunfight (Manuel Polik).bin
    {"b311ab95e85bc0162308390728a7361d",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Gyruss (1984).bin
    {"30516cfbaa1bc3b5335ee53ad811f17a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Halloween (1983).bin
    {"4afa7f377eae1cafb4265c68f73f2718",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Halo2600_Final.bin
    {"f16c709df0a6c52f47ff52b9d95b7d8d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Hangman (1978).bin
    {"a34560841e0878c7b14cc65f79f6967d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Harem (1982).bin
    {"f0a6e99f5875891246c3dbecbf2d2cea",  "4K",   CTR_LJOY,      SPEC_HAUNTED,   MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Haunted House (1982).bin
    {"2cc640f904e1034bf438075a139548d3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Heartbreak.bin
    {"fca4a5be1251927027f2c24774a02160",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  8},    // H.E.R.O. (1984).bin
    {"260c787e8925bf3649c8aeae5b97dcc0",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Hell Driver (ITT Family Games, Thomas Jentzsch) (NTSC).bin
    {"3d48b8b586a09bdbf49f1a016bf4d29a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Hole Hunter (AKA Topy).bin
    {"c52d9bbdc5530e1ef8e8ba7be692b01e",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Holey Moley (1984).bin
    {"0bfabf1e98bdb180643f35f2165995d0",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Home Run - Baseball (1978).bin
    {"9f901509f0474bf9760e6ebd80e629cd",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Home Run - Baseball (Sears 4K).bin
    {"a6fe2dd4b829ad267b2b3183e1650228",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Hugo Hunt stellaDemo8k_ntsc.bin    
    {"7972e5101fa548b952d852db24ad6060",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   3,  0},    // Human Cannonball (1979).bin
    {"102672bbd7e25cd79f4384dd7214c32b",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Hunt & Score (1978).bin
    {"ecdcd9bd0949bad0b5e371aefa3f6352",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Hunchy 1K (2005) (Chris Walton).bin
    {"9fa0c664b157a0c27d10319dbbca812c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Hunchy II (2005) (Chris Walton).bin
    {"a4c08c4994eb9d24fb78be1793e82e26",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Ice Hockey (1981).bin
    {"9a21fba9ee9794e0fadd7c7eb6be4e12",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Ikari Warriors (1989).bin
    {"c4bc8c2e130d76346ebf8eb544991b46",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Imagic Selector.bin
    {"75a303fd46ad12457ed8e853016815a0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Immies & Aggies (1983).bin
    {"47abfb993ff14f502f88cf988092e055",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Inca Gold (AKA Spider Kong).bin
    {"ad859c8d0c9513b47861f38d03bdead7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Incoming! [alt].bin
    {"331e3c95f7c5d2e7869301d070b7de76",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Incoming!.bin
    {"c5301f549d0722049bb0add6b10d1e09",  "2K",   CTR_DRIVING,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  198,   0, -2},    // Indy 500 (1977).bin
    {"afe88aae81d99e0947c0cfb687b16251",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Infiltrate (1981).bin
    {"b4030c38a720dd84b84178b6ce1fc749",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // International Soccer (1982).bin
    {"9ea8ed9dec03082973244a080941e58a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // INV+.bin
    {"4868a81e1b6031ed66ecd60547e6ec85",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Inv (V2.1) (1-3-98).bin
    {"4b9581c3100a1ef05eac1535d25385aa",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // I.Q. 180.bin
    {"ab301d3d7f2f4fe3fdd8a3540b7a74f5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // IQ-180.bin
    {"f6a282374441012b01714e19699fc62a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // I Want My Mommy (1983).bin
    {"47db521cc84aaa1a0a8eef9464685dc9",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // IRan_final.bin
    {"2f0546c4d238551c7d64d884b618100c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Ixion (1984).bin
    {"e51030251e440cffaab1ac63438b44ae",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // James Bond 007 (1983).bin
    {"04dfb4acac1d0909e4c360fd2ac04480",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Jammed.bin
    {"58a82e1da64a692fd727c25faef2ecc9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Jawbreaker (1982).bin
    {"718ae62c70af4e5fd8e932fee216948a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Journey Escape (1982).bin
    {"3276c777cbe97cdd2b4a63ffc16b7151",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Joust (1983).bin
    {"ec40d4b995a795650cf5979726da67df",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Joust Pong.bin
    {"89a923f6f6bec64f9b6fa6ff8ea37510",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Joyride.bin
    {"36c29ceee2c151b23a1ad7aa04bd529d",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -5},    // Jr. Pac-Man (1984).bin
    {"4364680de59e650a02348b901ca7202f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Jump.bin
    {"2cccc079c15e9af94246f867ffc7e9bf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Jungle Fever (1982).bin
    {"2bb9f4686f7e08c5fcc69ec1a1c66fe7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  1},    // Jungle Hunt (1983).bin
    {"90b647bfb6b18af35fcf613573ad2eec",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Juno First.bin
    {"b9d1e3be30b131324482345959aed5e5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  4},    // Kabobber (1983).bin
    {"5428cdfada281c569c74c7308c7f2c26",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Kaboom! (1981).bin
    {"af6ab88d3d7c7417db2b3b3c70b0da0a",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Kaboom! (1981).bin
    {"7b43c32e3d4ff5932f39afcb4c551627",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Kamikaze Saucers (1983).bin
    {"4326edb70ff20d0ee5ba58fa5cb09d60",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Kangaroo (1983).bin
    {"cedbd67d1ff321c996051eec843f8716",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Karate (1982).bin
    {"be929419902e21bd7830a7a7d746195d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   3,  3},    // Keystone Kapers (1983).bin
    {"248668b364514de590382a7eda2c9834",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Kick-Man (Prototype).bin
    {"7a7f6ab9215a3a6b5940b8737f116359",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   0,  1,  ANA1_0,  210,   0,  0},    // Killer Satellites (1983).bin
    {"e21ee3541ebd2c23e817ffb449939c37",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  192,   0, -4},    // King Kong (1982).bin
    {"2c29182edf0965a7f56fe0897d2f84ba",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Klax (1990).bin
    {"7fcd1766de75c614a3ccc31b25dd5b7a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Knight on the Town (1982).bin
    {"534e23210dd1993c828d944c6ac4d9fb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Kool-Aid Man (1983).bin
    {"4baada22435320d185c95b7dd2bcdb24",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Krull (1983).bin
    {"5b92a93b23523ff16e2789b820e2a4c5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  3},    // Kung-Fu Master (1987).bin
    {"7ad782952e5147b88b65a25cadcdf9e0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  5},    // Kwibble (1983).bin
    {"b86552198f52cfce721bafb496363099",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Kyphus (1982).bin
    {"adfbd2e8a38f96e03751717f7422851d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  203,   0,  4},    // Lady Bug.bin
    {"f1489e27a4539a0c6c8529262f9f7e18",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, 10},    // Lady Bug PAL.bin    
    {"95a89d1bf767d7cc9d0d5093d579ba61",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Lady in Wading (1982).bin
    {"d06b7fcc7640735d36c0b8d44fb5e765",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Lander (2008) (Bastian Framke).bin    
    {"931b91a8ea2d39fe4dca1a23832b591a",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Laser Blast (1981).bin
    {"105918c03562a262c56c145fa736465d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Laserblast (2007) (Nick Poirer, David Poore, Jenny Rainwater).bin
    {"1fab68fd67fe5a86b2c0a9227a59bb95",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  192,   0, -3},    // Lasercade (1983).bin
    {"1fa58679d4a39052bd9db059e8cda4ad",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Laser Gates (1983).bin
    {"31e518debba46df6226b535fa8bd2543",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  5},    // Last Starfighter (1984).bin
    {"021dbeb7417cac4e5f8e867393e742d6",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Lead (16k).bin
    {"c98ff002205095c8a40f7a537a7e8f01",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Lead (8k).bin
    {"3947eb7305b0c904256cdbc5c5956c0f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   4, -2},    // Lilly Adventure.bin
    {"86128001e69ab049937f265911ce7e8a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  9},    // Lochjaw (1981).bin
    {"71464c54da46adae9447926fdbfc1abe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Lock 'n' Chase (1982).bin
    {"b4e2fd27d3180f0f4eb1065afc0d7fc9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // London Blitz (1983).bin
    {"5babe0cad3ec99d76b0aa1d36a695d2f",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -3},    // Looping (1983).bin
    {"e24d7d879281ffec0641e9c3f52e505a",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  204,   0, -2},    // Lord of the Rings (1983).bin
    {"7c00e7a205d3fda98eb20da7c9c50a55",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Lost Luggage (1981).bin
    {"2d76c5d1aad506442b9e9fb67765e051",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Lost Luggage (1981) [no opening scene].bin
    {"593268a34f1e4d78d94729e1d7ee8367",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Lunokhod.bin
    {"393e41ca8bdd35b52bf6256a968a9b89",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  5},    // M.A.D. (Missile Intercept) (1982).bin
    {"cddabfd68363a76cd30bee4e8094c646",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // MagiCard (1981).bin
    {"ccb5fa954fb76f09caae9a8c66462190",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Malagai (1983).bin
    {"54a1c1255ed45eb8f71414dadb1cf669",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Mangia' (1983).bin
    {"402d876ec4a73f9e3133f8f7f7992a1e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Man Goes Down.bin
    {"9104ffc48c5eebd2164ec8aeb0927b91",  "DPCP", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Mappy.bin
    {"13895ef15610af0d0f89d588f376b3fe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Marauder (1982).bin
    {"b00e8217633e870bf39d948662a52aac",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Marine Wars (1983).bin
    {"e908611d99890733be31733a979c62d8",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Mario Bros. (1983).bin
    {"835759ff95c2cdc2324d7c1e7c5fa237",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  1},    // M.A.S.H (1983).bin
    {"ae4be3a36b285c1a1dff202157e2155d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Master Builder (1983).bin
    {"3b76242691730b2dd22ec0ceab351bc6",  "E7",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Masters of the Universe (1983).bin
    {"470878b9917ea0348d64b5750af149aa",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Math Gran Prix (1982).bin
    {"f825c538481f9a7a46d1e9bc06200aaf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Maze Craze (1980).bin
    {"35b43b54e83403bb3d71f519739a9549",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // McDonald's - Golden Arches (1983).bin
    {"09a03e0c85e667695bcd6c6394e47e5f",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Marble Craze ntsc_7_26_3.bin
    {"f7fac15cf54b55c5597718b6742dbec2",  "F4",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Medieval Mayhem.bin
    {"daeb54957875c50198a7e616f9cc8144",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Mega Force (1982).bin
    {"318a9d6dda791268df92d72679914ac3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   3,  4},    // MegaMania (1982).bin
    {"96e798995af6ed9d8601166d4350f276",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  201,   0, -1},    // Meltdown (1983).bin
    {"5f791d93ac95bdd8a691a65d665fb436",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  201,   0, -1},    // Meltdown.bin
    {"712924a2c7b692f6e7b009285c2169a7",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Meteoroids (1982).bin
    {"f1554569321dc933c87981cf5c239c43",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   0,  1,  ANA1_0,  210,   0,  0},    // Midnight Magic (1984).bin
    {"3c57748c8286cf9e821ecd064f21aaa9",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Millipede (1984).bin
    {"0e224ea74310da4e7e2103400eb1b4bf",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Mind Maze (1984).bin
    {"fa0570561aa80896f0ead05c46351389",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  189,  -1, -3},    // Miner 2049er (1982).bin
    {"3b040ed7d1ef8acb4efdeebebdaa2052",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  189,  -1, -3},    // Miner 2049er (1982) [fixed].bin
    {"2a1b454a5c3832b0240111e7fd73de8a",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  196,   0, -2},    // Miner 2049er Vol II (1983).bin
    {"4543b7691914dfd69c3755a5287a95e1",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Mines of Minos (1982).bin
    {"df62a658496ac98a3aa4a6ee5719c251",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  200,   0,  6},    // Miniature Golf (1979).bin
    {"bb6fc47aed82b3f65c4938bf668de767",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -3},    // Minotaur (2006) (Michael Biggs).bin
    {"3a2e2d0c6892aa14544083dfb7762782",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  5},    // Missile Command (1981).bin
    {"1a8204a2bcd793f539168773d9ad6230",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  5},    // Missile Command (1981) [no initials].bin
    {"e6e5bb0e4f4350da573023256268313d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Missile Control (AKA Raketen-Angriff) (Ariola, Thomas Jentzsch).bin
    {"4c6afb8a44adf8e28f49164c84144bfe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Mission 3000 A.D. (1983).bin
    {"4181087389a79c7f59611fb51c263137",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Miss Piggy's Wedding (1983).bin
    {"e13818a5c0cb2f84dd84368070e9f099",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   2,  1},    // Misterious Thief (1983).bin
    {"7af40c1485ce9f29b1a7b069a2eb04a7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4, -1},    // Mogul Maniac (1983).bin
    {"6913c90002636c1487538d4004f7cac2",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Monster Cise (1984).bin
    {"6913c90002636c1487538d4004f7cac2",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Monstercise.bin
    {"3347a6dd59049b15a38394aa2dafa585",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Montezuma's Revenge (1984).bin
    {"515046e3061b7b18aa3a551c3ae12673",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Moon Patrol (1983).bin
    {"203abb713c00b0884206dcc656caa48f",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Moonsweeper (1983).bin
    {"7d1034bcb38c9b746ea2c0ae37d9dff2",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Morse Code Tutor.bin
    {"de0173ed6be9de6fd049803811e5f1a8",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Motocross Racer (1983).bin
    {"378a62af6e9c12a760795ff4fc939656",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // MotoRodeo (1990).bin
    {"db4eb44bc5d652d9192451383d3249fc",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // Mountain King (1983).bin
    {"5678ebaa09ca3b699516dba4671643ed",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Mouse Trap (1982).bin
    {"0164f26f6b38a34208cd4a2d0212afc3",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Mr. Do! (1983).bin
    {"b7a7e34e304e4b7bc565ec01ba33ea27",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Mr. Do!'s Castle (1984) [alt].bin
    {"97184b263722748757cfdc41107ca5c0",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Mr. Do!'s Castle (1984).bin
    {"8644352b806985efde499ae6fc7b0fec",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Mr. Postman (1983).bin
    {"f0daaa966199ef2b49403e9a29d12c50",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Mr. Postman.bin
    {"87e79cd41ce136fd4f72cc6e2c161bee",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  208,   0, -1},    // Ms. Pac-Man (1982).bin
    {"ddf72763f88afa541f6b52f366c90e3a",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  206,   0,  1},    // Muncher.bin
    {"65b106eba3e45f3dab72ea907f39f8b4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Music Machine (1983).bin
    {"fcbbd0a407d3ff7bf857b8a399280ea1",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Mysterious Thief (1983).bin
    {"0546f4e6b946f38956799dd00caab3b1",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // My Golf (HES, Thomas Jentzsch) (NTSC).bin
    {"36306070f0c90a72461551a7a4f3a209",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Name This Game (1982).bin
    {"392f00fd1a074a3c15bc96b0a57d52a1",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Night Driver (1980).bin
    {"ed0ab909cf7b30aff6fc28c3a4660b8e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // Nightmare.bin
    {"27f9e2e1b92af9dc17c6155605c38e49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Nightmare (CCE).bin
    {"b6d52a0cf53ad4216feb04147301f87d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // No Escape! (1982).bin
    {"de7a64108074098ba333cc0c70eef18a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Nuts (Unknown).bin
    {"133a4234512e8c4e9e8c5651469d4a09",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Obelix (1983).bin
    {"a189f280521f4e5224d345efb4e75506",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Obelix (Atari, Thomas Jentzsch) (NTSC).bin
    {"e20fc6ba44176e9149ef6d3f99221ac5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Odin.bin
    {"67afbf02142c66861a1d682e4c0ea46e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Odin-joystick.bin
    {"c73ae5ba5a0a3f3ac77f0a9e14770e73",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   0,  1,  ANA1_0,  210,   0,  1},    // Official Frogger (1983).bin
    {"98f63949e656ff309cefa672146dc1b8",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Off the Wall (1989).bin
    {"b6166f15720fdf192932f1f76df5b65d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  203,   0, -2},    // Off Your Rocker (1983).bin
    {"c9c25fc536de9a7cdc5b9a916c459110",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  5},    // Oink! (1982).bin
    {"cca33ae30a58f39e3fc5d80f94dc0362",  "2K",   CTR_DRIVING,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Okie dokie (pd).bin
    {"9947f1ebabb56fd075a96c6d37351efa",  "FASC", CTR_BOOSTER,   SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Omega Race (1983).bin
    {"bc593f2284c67b7d8716d110f541953f",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Omicron 16k.bin
    {"2148917316ca5ce1672f6c49c0f89d0b",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Omicron 2k.bin
    {"52385334ac9e9b713e13ffa4cc5cb940",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  206,   0, -2},    // Open, Sesame! (1983).bin
    {"fa1b060fd8e0bca0c2a097dcffce93d3",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Oscar's Trash Race (1983).bin
    {"55949cb7884f9db0f8dfcf8707c7e5cb",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  203,   0,  2},    // Othello (1981).bin
    {"113cd09c9771ac278544b7e90efe7df2",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  203,   0,  2},    // Othello (1981) [no grid markers].bin
    {"890c13590e0d8d5d6149737d930e4d95",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Outlaw (1978).bin
    {"f97dee1aa2629911f30f225ca31789d4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Out of Control (1983).bin
    {"91f0a708eeb93c133e9672ad2c8e0429",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Oystron (V2.9) (PD).bin
    {"e959b5a2c882ccaacb43c32790957c2d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // p2p_ntsc_final.bin
    {"6e372f076fb9586aff416144f5cfe1cb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   1, -2},    // Pac-Man (1982).bin
    {"b36040a2f9ecafa73d835d804a572dbf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   1, -2},    // Pac Man (1983) (Digitel).bin
    {"880e45b99c785e9910450b88e69c49eb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Pac-Man 4k.bin
    {"6e88da2b704916eb04a998fed9e23a3e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // Pac-Man_4k (debro).bin
    {"98d41ef327c58812ecc75bf1611ddced",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  202,   0,  1},    // Pac-Man 8k.bin
    {"bf9a2045952d40e08711aa232a92eb78",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Panky the Panda [alt].bin
    {"25f69569b1defffcb64cb431edf3e093",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Panky the Panda.bin    
    {"012b8e6ef3b5fd5aabc94075c527709d",  "AR",   CTR_PADDLE0,   SPEC_AR,        MODE_NO,   0,  1,  ANA1_0,  210,   0,  0},    // Party Mix (1983).bin
    {"36c31bb5daeb103f488c66de67ac5075",  "AR",   CTR_PADDLE0,   SPEC_AR,        MODE_NO,   0,  1,  ANA1_0,  210,   0,  0},    // Party Mix - Bop a Buggy.bin
    {"b7300f7c94c8838b6b9a55e8d67f9fba",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Passthrough 2600 (2010) (Cliff Friedel).bin
    {"6ed5012793f5ddf4353a48c11ea9b8d3",  "AR",   CTR_PADDLE0,   SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Down on the Line, Handcar.bin
    {"aaea37b65db9e492798f0105a6915e96",  "AR",   CTR_PADDLE0,   SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Tug of War, Wizard's Keep.bin
    {"e40a818dac4dd851f3b4aafbe2f1e0c1",  "4K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Peek-A-Boo (1984).bin
    {"04014d563b094e79ac8974366f616308",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Pengo (1984).bin
    {"212d0b200ed8b45d8795ad899734d7d7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  200,   0,  1},    // Pepsi Invaders (1983).bin
    {"09388bf390cd9a86dc0849697b96c7dc",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Pete Rose Baseball (1988).bin
    {"e9034b41741dcee64ab6605aba9de455",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  203,   0, -1},    // Phantom Tank (Digivision).bin
    {"6a222c26bcece3a510ddda21398f72c6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  203,   0, -1},    // Phantom Tank (Digivision) [alt].bin        
    {"e959b5a2c882ccaacb43c32790957c2d",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Phantom II, Pirate (2006) (David Weavil).bin
    {"62f74a2736841191135514422b20382d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Pharaoh's Curse.bin
    {"7dcbfd2acc013e817f011309c7504daa",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   0,  1,  ANA1_0,  210,   0,  0},    // Phaser Patrol (1982).bin
    {"7e52a95074a66640fcfde124fffd491a",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Phoenix (1982).bin
    {"1d4e0a034ad1275bc4d75165ae236105",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Pick Up (1983).bin
    {"c4060a31d61ba857e756430a0a15ed2e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  200,   0, -4},    // Pick'n Pile (NTSC by Thomas Jentzsch).bin
    {"17c0a63f9a680e7a61beba81692d9297",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Picnic (1982).bin
    {"d3423d7600879174c038f53e5ebbf9d3",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Piece o' Cake (1982).bin
    {"8e4fa8c6ad8d8dce0db8c991c166cdaa",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Pigs in Space (1983).bin
    {"aff90d7fb05e8f43937fc655bfffe2ea",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Pirate (2006) (David Weavil).bin
    {"9b54c77b530582d013f0fa4334d785b7",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Pirate Special Edition (2007) (David Weavil).bin
    {"3e90cf23106f2e08b2781e41299de556",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  4},    // Pitfall! (1982).bin
    {"f73d2d0eff548e8fc66996f27acf2b4b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  4},    // Pitfall (1983) (CCE) (C-813).bin
    {"6d842c96d5a01967be9680080dd5be54",  "DPC",  CTR_LJOY,      SPEC_PITFALL2,  MODE_NO,   0,  1,  ANA1_0,  210,   5,  3},    // Pitfall II - Lost Caverns (1983).bin
    {"d9fbf1113114fb3a3c97550a0689f10f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Pizza Chef (1983).bin
    {"9efb4e1a15a6cdd286e4bcd7cd94b7b8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Planet of the Apes (1983).bin
    {"043f165f384fbea3ea89393597951512",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Planet Patrol (1982).bin
    {"da4e3396aa2db3bd667f83a1cb9e4a36",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Plaque Attack (1983).bin
    {"8bbfd951c89cc09c148bfabdefa08bec",  "UA",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -1},    // Pleiades (1983).bin
    {"860ae9177b882f3324ed561f7b797940",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // PlagueFinalNTSC 1.09.bas.bin    
    {"8c136e97c0a4af66da4a249561ed17db",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Poker_squares.bin
    {"44f71e70b89dcc7cf39dfd622cfb9a27",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Polaris (1983).bin
    {"5f39353f7c6925779b0169a87ff86f1e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  0},    // Pole Position (1983)[alt].bin
    {"a4ff39d513b993159911efe01ac12eba",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Pole Position (1983).bin    
    {"ee28424af389a7f3672182009472500c",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Polo (1978).bin
    {"a83b070b485cf1fb4d5a48da153fdf1a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Pompeii (1983).bin
    {"4799a40b6e889370b7ee55c17ba65141",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   0, -4},    // Pooyan (1983).bin
    {"c7f13ef38f61ee2367ada94fdcc6d206",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  206,   0,  8},    // Popeye (1983).bin
    {"f93d7fee92717e161e6763a88a293ffa",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Porky's (1983).bin
    {"97d079315c09796ff6d95a06e4b70171",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   3,  5},    // Pressure Cooker (1983).bin
    {"de1a636d098349be11bbc2d090f4e9cf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Pressure gauge.bin
    {"ef3a4f64b6494ba770862768caf04b86",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Private Eye (1983).bin
    {"ac53b83e1b57a601eeae9d3ce1b4a458",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // qb (2.15) (ntsc).bin
    {"34e37eaffc0d34e05e40ed883f848b40",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // qb (2.15) (stella).bin
    {"484b0076816a104875e00467d431c2d2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  10},   // Q-bert (1983).bin
    {"dcb406b54f1b69017227cfbe4052005e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  10},   // Q-bert Arcade Hack.bin
    {"517592e6e0c71731019c0cebc2ce044f",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  203,  -1,  7},    // Q-bert's Qubes (1984).bin
    {"024365007a87f213cbe8ef5f2e8e1333",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Quadrun (1983).bin
    {"a0675883f9b09a3595ddd66a6f5d3498",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Quest for Quintana Roo (1984).bin
    {"7eba20c2291a982214cc7cbe8d0b47cd",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Quick Step! (1983).bin
    {"fb4ca865abc02d66e39651bd9ade140a",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   4,  1},    // Rabbit Transit (1983).bin
    {"4df9d7352a56a458abb7961bf10aba4e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Racing Car (Unknown).bin
    {"a20d931a8fddcd6f6116ed21ff5c4832",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  1},    // Racquetball (1981).bin
    {"247fa1a29ad90e64069ee13d96fea6d6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Radar (1983).bin
    {"baf4ce885aa281fd31711da9b9795485",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   2,  1},    // Radar Lock (1989).bin
    {"92a1a605b7ad56d863a56373a866761b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Raft Rider (1982).bin
    {"f724d3dd2471ed4cf5f191dbb724b69f",  "F8",   CTR_RAIDERS,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Raiders of the Lost Ark (1982).bin
    {"1cafa9f3f9a2fce4af6e4b85a2bbd254",  "F8",   CTR_RAIDERS,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Raiders of the Lost Ark (pal).bin
    {"cb96b0cf90ab7777a2f6f05e8ad3f694",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Rainbow Invaders NTSC.bin
    {"7096a198531d3f16a99d518ac0d7519a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  203,   0,  1},    // Ram It (1982).bin
    {"5e1b4629426f4992cf3b2905a696e1a7",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Rampage! (1989).bin
    {"9f8fad4badcd7be61bbd2bcaeef3c58f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Reactor (1982).bin
    {"eb634650c3912132092b7aee540bbce3",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  206,   0, -2},    // RealSports Baseball (1982).bin
    {"3177cc5c04c1a4080a927dfa4099482b",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // RealSports Boxing (1987).bin
    {"7ad257833190bc60277c1ca475057051",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // RealSports Football (1982).bin
    {"08f853e8e01e711919e734d85349220d",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // RealSports Soccer (1983).bin
    {"dac5c0fe74531f077c105b396874a9f1",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // RealSports Tennis (1983).bin
    {"aed0b7bd64cc384f85fdea33e28daf3b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // RealSports Volleyball (1982).bin
    {"60a61da9b2f43dd7e13a5093ec41a53d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Rescue Terra I (1982).bin
    {"42249ec8043a9a0203dde0b5bb46d8c4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Resgate Espacial.bin
    {"2395021c633bb1890b546c6e23b6c3df",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Refraction_Beta_1.bin    
    {"0b01909ba84512fdaf224d3c3fd0cf8d",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // RevengeOfTheApes.bin
    {"4f64d6d0694d9b7a1ed7b0cb0b83e759",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Revenge of the Beefsteak Tomatoes (1982).bin
    {"a995b6cbdb1f0433abc74050808590e6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Riddle of the Sphinx (1982).bin
    {"31512cdfadfd82bfb6f196e3b0fd83cd",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // River Patrol (1984).bin
    {"393948436d1f4cc3192410bb918f9724",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -3},    // River Raid (1982).bin
    {"ab56f1b2542a05bebc4fbccfc4803a38",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // River Raid II (1988).bin
    {"4eb4fd544805babafc375dcdb8c2a597",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Red Sea Crossing.bin
    {"2bd00beefdb424fa39931a75e890695d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Road Runner (1989) [alt].bin   
    {"ce5cc62608be2cd3ed8abd844efb8919",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Road Runner (1989).bin   
    {"72a46e0c21f825518b7261c267ab886e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Robin Hood (1983).bin
    {"4f618c2429138e0280969193ed6c107e",  "FE",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  7},    // Robot Tank (1983).bin
    {"ec44dcf2ddb4319962fc43b725a902e8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  206,   0, -3},    // Robot City (RC8).bin
    {"d97fd5e6e1daacd909559a71f189f14b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Rocky & Bullwinkle (1983).bin
    {"65bd29e8ab1b847309775b0de6b2e4fe",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  201,   0, -1},    // Roc 'n Rope (1984).bin
    {"67931b0d37dc99af250dd06f1c095e8d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Room of Doom (1982).bin
    {"1f2ae0c70a04c980c838c2cdc412cf45",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  204,   0,  1},    // Rubik's Cube (1984).bin
    {"40b1832177c63ebf81e6c5b61aaffd3a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // RubiksCube3D.bin
    {"f3cd0f886201d1376f3abab2df53b1b9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  198,   0,  1},    // Rush Hour (1983).bin
    {"a4ecb54f877cd94515527b11e698608c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // Saboteur (1983).bin
    {"1ec57bbd27bdbd08b60c391c4895c1cf",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Saboteur.bin
    {"4d502d6fb5b992ee0591569144128f99",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Save Mary! (1989).bin
    {"ed1a784875538c7871d035b7a98c2433",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Save Our Ship.bin
    {"49571b26f46620a85f93448359324c28",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Save Our Ship (Unknown).bin
    {"e377c3af4f54a51b85efe37d4b7029e6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  200,   0,  2},    // Save the Whales (1983).bin
    {"a3fe5e6a744cd34a3c1466874a3e9d5f",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Scramble 0106.bin
    {"19e761e53e5ec8e9f2fceea62715ca06",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Scuba Diver (1983).bin
    {"1bc2427ac9b032a52fe527c7b26ce22c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sea Battle (1983).bin
    {"624e0a77f9ec67d628211aaf24d8aea6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   2,  2},    // Sea Hawk (1983).bin
    {"5dccf215fdb9bbf5d4a6d0139e5e8bcb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  9},    // Sea Hunt (1987).bin
    {"a8c48b4e0bf35fe97cc84fdd2c507f78",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  196,   0, -4},    // Seamonster (1982).bin
    {"dd45e370aceff765f1e72c619efd4399",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  196,   0, -4},    // Sea Monster (1982) (Bit Corporation) (PG201).bin
    {"240bfbac5163af4df5ae713985386f92",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Seaquest (1983).bin
    {"c880c659cdc0f84c4a66bc818f89618e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  202,   0, -4},    // Sesam, Oeffne Dich (AKA Open Sesame) (Bitcorp, TJ).bin
    {"3034532daf80997f752aee680d2e7fc3",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Seaweed Assault.bin
    {"dde55d9868911407fe8b3fefef396f00",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sea Wolf.bin
    {"605fd59bfef88901c8c4794193a4cbad",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Secret Agent (1983).bin
    {"fc24a94d4371c69bc58f5245ada43c44",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Secret Quest (1989).bin
    {"8da51e0c4b6b46f7619425119c7d018e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Sentinel (1990).bin
    {"258f8f1a6d9af8fc1980b22361738678",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Shadow Reflex (Beta 10-26-2020).bin
    {"54f7efa6428f14b9f610ad0ca757e26c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   1,  6},    // Shark Attack (1982).bin
    {"0248a33b414282ba2addf7dcb2a2e696",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // Shield Shifter (2009) (John A. Reder).bin
    {"8debebc61916692e2d66f2edf4bed29c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sheep It Up_ntsc.bin    
    {"b5a1a189601a785bdb2f02a424080412",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Shootin' Gallery (1982).bin
    {"15c11ab6e4502b2010b18366133fc322",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Shooting Arcade (1989).bin
    {"25b6dc012cdba63704ea9535c6987beb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Shuttle Orbiter (1983).bin
    {"1e85f8bccb4b866d4daa9fcf89306474",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Sinistar (1984).bin
    {"4c8970f6c294a0a54c9c45e5e8445f93",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Sir Lancelot (1983).bin
    {"f847fb8dba6c6d66d13724dbe5d95c4d",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Skate Boardin' (1987).bin
    {"39c78d682516d79130b379fa9deb8d1c",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Skeet Shoot (1981).bin
    {"eafe8b40313a65792e88ff9f2fe2655c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  198,   0,  0},    // SkelPlus.bin
    {"8654d7f0fb351960016e06646f639b02",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Ski Hunt.bin
    {"5305f69fbf772fac4760cdcf87f1ab1f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Ski Run (2600 Screen Search Console).bin
    {"b76fbadc8ffb1f83e2ca08b6fb4d6c9f",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   1,  1},    // Skiing (1980).bin
    {"46c021a3e9e2fd00919ca3dd1a6b76d8",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Sky Diver (1979).bin
    {"2a0ba55e56e7a596146fa729acf0e109",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  3},    // Sky Jinks (1982).bin
    {"4c9307de724c36fd487af6c99ca078f2",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Sky Patrol (1982).bin
    {"3b91c347d8e6427edbe942a7a405290d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Sky Skipper (1983).bin
    {"f90b5da189f24d7e1a2117d8c8abc952",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Slot Machine (1979).bin
    {"705fe719179e65b0af328644f3a04900",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Slot Machine (1979).bin
    {"81254ebce88fa46c4ff5a2f4d2bad538",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Slot Machine (1979).bin
    {"fc6052438f339aea373bbc999433388a",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Slot Machine (pal).bin
    {"aed82052f7589df05a3f417bb4e45f0c",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   0,  0},    // Slot Racers (1978).bin
    {"3d1e83afdb4265fa2fb84819c9cfd39c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Smurf - Rescue in Gargamel's Castle (1982).bin
    {"a204cd4fb1944c86e800120706512a64",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Smurfs Save the Day (1983).bin
    {"9c6faa4ff7f2ae549bbcb14f582b70e4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   1, -3},    // Sneak 'n Peek (1982).bin
    {"57939b326df86b74ca6404f64f89fce9",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  0},    // Snoopy and the Red Baron (1983).bin
    {"75ee371ccfc4f43e7d9b8f24e1266b55",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Snow White (1982) [alt].bin
    {"75028162bfc4cc8e74b04e320f9e6a3f",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Snow White (1982).bin
    {"3f6dbf448f25e2bd06dea44248eb122d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   3,  2},    // Soccer (1989).bin
    {"947317a89af38a49c4864d6bdd6a91fb",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // Solar Fox (1983).bin
    {"e72eb8d4410152bdcb69e7fba327b420",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Solaris (1986).bin
    {"b5be87b87fd38c61b1628e8e2d469cb5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Solar Plexus.bin
    {"97842fe847e8eb71263d6f92f7e122bd",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Solar Storm (1983).bin
    {"d2c4f8a4a98a905a9deef3ba7380ed64",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sorcerer (1983).bin
    {"5f7ae9a7f8d79a3b37e8fc841f65643a",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sorcerer's Apprentice (1983).bin
    {"17badbb3f54d1fc01ee68726882f26a6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Space Attack (1982).bin
    {"1d566002bbc51e5eee73de4c595fd545",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // SpaceBattleFinal4N.bin
    {"df6a28a89600affe36d94394ef597214",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Space Cavern (1981).bin
    {"ec5c861b487a5075876ab01155e74c6c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Spacechase (1981) .bin
    {"44ca1a88274ff55787ed1763296b8456",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // SpaceGame-Final.bin
    {"94255d5c05601723a58df61726bc2615",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // SpaceGame 2K.bin
    {"9f81edee8b4b5afbde0e49a6fe8da0de",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  5},    // Space Battle (NTSC).bin    
    {"72ffbef6504b75e69ee1045af9075f66",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  6},    // Space Invaders (1980).bin
    {"b2a6f31636b699aeda900f07152bab6e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  201,   0,  2},    // Space Invaders Deluxe.a26    
    {"6f2aaffaaf53d23a28bf6677b86ac0e3",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  4},    // Space Jockey (1982).bin
    {"45040679d72b101189c298a864a5b5ba",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  3},    // SpaceMaster X-7 (1983).bin
    {"cb3a9b32a01746621f5c268db48833b2",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Space Raid.bin
    {"3dfb7c1803f937fadc652a3e95ff7dc6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Space Robot (Dimax - Sinmax).bin
    {"fe395b292e802ea16b3b5782b21ee686",  "DPCP", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Space Rocks.bin
    {"5894c9c0c1e7e29f3ab86c6d3f673361",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   0,  1},    // Space Shuttle (1983).bin
    {"898143773824663efe88d0a3a0bb1ba4",  "FE",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   0,  1},    // Space Shuttle (1983) [FE Bankswitching].bin
    {"6c9a32ad83bcfde3774536e52be1cce7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Space Treat.bin
    {"9e135f5dce61e3435314f5cddb33752f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Space Treat Deluxe.bin
    {"be3f0e827e2f748819dac2a22d6ac823",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Space Tunnel (1982).bin
    {"df2745d585238780101df812d00b49f4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Space Tunnel (1982) (Bit Corporation) (PG202).bin
    {"a7ef44ccb5b9000caf02df3e6da71a92",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Space War (1978).bin
    {"25c97848ae6499e569b832b686a84bb2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  202,   0,  2},    // Sp+.bin
    {"8454ed9787c9d8211748ccddb673e920",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Spiderdroid (1987).bin
    {"24d018c4a6de7e5bd19a36f2b879b335",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Spider Fighter (1982).bin
    {"199eb0b8dce1408f3f7d46411b715ca9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Spider-Man (1982).bin
    {"21299c8c3ac1d54f8289d88702a738fd",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Spider Maze (1982).bin
    {"a4e885726af9d97b12bb5a36792eab63",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Spike's Peak (1983).bin
    {"d3171407c3a8bb401a3a62eb578f48fb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Spinning Fireball (1983).bin
    {"cef2287d5fd80216b2200fb2ef1adfa8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Spitfire Attack (1983).bin
    {"6216bef66edceb8a24841e6065bf233e",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Splatform 2600 (v1.01).bin
    {"4cd796b5911ed3f1062e805a3df33d98",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Springer (1982).bin
    {"5a8afe5422abbfb0a342fb15afd7415f",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sprint Master (1988).bin
    {"3105967f7222cc36a5ac6e5f6e89a0b4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Spy Hunter (1984).bin
    {"ba257438f8a78862a9e014d831143690",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Squeeze Box (1982).bin
    {"68878250e106eb6c7754bc2519d780a0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Squirrel (1983).bin
    {"aa8c75d6f99548309949916ad6cf33bc",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Squish-Em.bin
    {"34c808ad6577dbfa46169b73171585a3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Squoosh (1983).bin
    {"21a96301bb0df27fde2e7eefa49e0397",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sssnake (1982).bin
    {"21d7334e406c2407e69dbddd7cec3583",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Stampede (1981).bin
    {"d9c9cece2e769c7985494b1403a25721",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Star Castle.bin
    {"f526d0c519f5001adb1fc7948bfbb3ce",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Star Fox (1983).bin
    {"0c48e820301251fbb6bcdc89bd3555d9",  "F8SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Stargate (1984).bin
    {"a3c1c70024d7aabb41381adbfb6d3b25",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, 12},    // Stargunner (1982).bin
    {"d69559f9c9dc6ef528d841bf9d91b275",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // StarMaster (1982).bin
    {"cbd981a23c592fb9ab979223bb368cd5",  "F8",   CTR_STARRAID,  SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Star Raiders (1982).bin
    {"c1a83f44137ea914b495fc6ac036c493",  "F8",   CTR_STARRAID,  SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Star Raiders (PAL 1982).bin
    {"e363e467f605537f3777ad33e74e113a",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Star Ship (1977).bin
    {"7b938c7ddf18e8362949b62c7eaa660a",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Star Ship (1977).bin
    {"79e5338dbfa6b64008bb0d72a3179d3c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Star Strike (1983).bin
    {"03c3f7ba4585e349dd12bfa7b34b7729",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Star Trek - Strategic Operations Simulator (1983).bin
    {"813985a940aa739cc28df19e0edd4722",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Star Voyager (1982).bin
    {"5336f86f6b982cc925532f2e80aa1e17",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Star Wars - Death Star Battle (1983).bin
    {"d44d90e7c389165f5034b5844077777f",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  206,   0,  8},    // Star Wars - Ewok Adventure (1983).bin
    {"c9f6e521a49a2d15dac56b6ddb3fb4c7",  "4K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Star Wars - Jedi Arena (1983).bin
    {"6339d28c9a7f92054e70029eb0375837",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  2},    // Star Wars - The Arcade Game (1984).bin
    {"3c8e57a246742fa5d59e517134c0b4e6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Star Wars - The Empire Strikes Back (1982).bin
    {"c5bab953ac13dbb2cba03cd0684fb125",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  206,   0,  0},    // StayFrosty.bin
    {"541cac55ebcf7891d9d51c415922303f",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // StayFrosty2.bin
    {"656dc247db2871766dffd978c71da80c",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA2_5,  210,   0,  4},    // Steeplechase (1980).bin
    {"0b8d3002d8f744a753ba434a4d39249a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Stellar Track (1980).bin
    {"23fad5a125bcd4463701c8ad8a0043a9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Stone Age (1983).bin
    {"9333172e3c4992ecf548d3ac1f2553eb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Strategy X (1983).bin
    {"807a8ff6216b00d52aba2dfea5d8d860",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // StratOGemsDeluxe.bin
    {"e10d2c785aadb42c06390fae0d92f282",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  192,   0, -2},    // Strawberry Shortcake (1983).bin
    {"396f7bc90ab4fa4975f8c74abe4e81f0",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Street Racer (1977).bin
    {"8b2926c0716ecf062c27275467130573",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Strip Off (2009) (John A. Reder).bin
    {"7b3cf0256e1fa0fdc538caf3d5d86337",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Stronghold (1983).bin
    {"c3bbc673acf2701b5275e85d9372facf",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Stunt Cycle (1980).bin
    {"f3f5f72bfdd67f3d0e45d097e11b8091",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Submarine Commander (1982).bin
    {"5af9cd346266a1f2515e1fbc86f5186a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  8},    // Sub-Scan (1982).bin
    {"93c52141d3c4e1b5574d072f1afde6cd",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Subterranea (1983).bin
    {"cff578e5c60de8caecbee7f2c9bbb57b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Suicide Adventure.bin
    {"e4c666ca0c36928b95b13d33474dbb44",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_FF,   0,  1,  ANA1_0,  210,   0,  0},    // Suicide Mission (1982).bin
    {"45027dde2be5bdd0cab522b80632717d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Summer Games (1987).bin
    {"7adbcf78399b19596671edbffc3d34aa",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  204,   0, -2},    // Super Baseball (1988).bin
    {"8885d0ce11c5b40c3a8a8d9ed28cefef",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   0,  0,  ANA1_0,  210,   0,  0},    // Super Breakout (1982).bin
    {"9d37a1be4a6e898026414b8fee2fc826",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  4},    // Super Challenge Baseball (1982).bin
    {"e275cbe7d4e11e62c3bfcfb38fca3d49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   4,  2},    // Super Challenge Football (1982).bin
    {"c29f8db680990cb45ef7fef6ab57a2c2",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  6},    // Super Cobra (1982).bin
    {"841057f83ce3731e6bbfda1707cbca58",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Super Cobra.bin
    {"724613effaf7743cbcd695fab469c2a8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Super Ferrari (Quelle).bin
    {"09abfe9a312ce7c9f661582fdf12eab6",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Super Football (1988).bin
    {"a9531c763077464307086ec9a1fd057d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Superman (1979).bin
    {"5de8803a59c36725888346fdc6e7429d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Superman (1979) [fixed].bin
    {"aec9b885d0e8b24e871925630884095c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Surf's Up (1983).bin
    {"4d7517ae69f95cfbc053be01312b7dba",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Surround (1977).bin
    {"31d08cb465965f80d3541a57ec82c625",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Surround (4k).bin   
    {"c370c3268ad95b3266d6e36ff23d1f0c",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Surround (pal).bin  
    {"045035f995272eb2deb8820111745a07",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  195,   4, -2},    // Survival Island (1983).bin
    {"85e564dae5687e431955056fbda10978",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  194,   0, -4},    // Survival Run (1983).bin
    {"59e53894b3899ee164c91cfa7842da66",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, 10},    // Survival Run (Data Age).bin
    {"e51c23389e43ab328ccfb05be7d451da",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sweat! - The Decathlon (1983).bin
    {"278f14887d601b5e5b620f1870bc09f6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  192,   0, -4},    // Swoops! (v0.96).bin
    {"87662815bc4f3c3c86071dc994e3f30e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Swordfight (1983).bin
    {"528400fad9a77fd5ad7fc5fdc2b7d69d",  "AR",   CTR_LJOY,      SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Sword of Saros (1983).bin
    {"37f8ad4cbd23abf4fe8cbb499554c233",  "F4",   CTR_LJOY,      SPEC_AR,        MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Sword of Surtr.bin
    {"5aea9974b975a6a844e6df10d2b861c4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // SwordQuest - EarthWorld (1982).bin
    {"f9d51a4e5f8b48f68770c89ffd495ed1",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  1},    // SwordQuest - FireWorld (1982).bin
    {"bc5389839857612cfabeb810ba7effdc",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  204,   0,  6},    // SwordQuest - WaterWorld (1983).bin
    {"2c2aea31b01c6126c1a43e10cacbfd58",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Synth Cart.bin
    {"d45ebf130ed9070ea8ebd56176e48a38",  "4K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Tac-Scan (1982).bin
    {"c77d3b47f2293e69419b92522c6f6647",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  203,   0, -1},    // Tank Brigade (1983).bin
    {"fa6fe97a10efb9e74c0b5a816e6e1958",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  200,   0, -1},    // Tanks But No Tanks (1983).bin
    {"de3d0e37729d85afcb25a8d052a6e236",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Tapeworm (1982).bin
    {"c0d2434348de72fa6edcc6d8e40f28d7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  192,   0, -3},    // Tapper (1984).bin
    {"2d6741cda3000230f6bbdd5e31941c01",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Targ (1983).bin
    {"6cab04277e7cd552a3e40b3c0e6e1e3d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Targ (Universal Chaos Beta).bin
    {"0c35806ff0019a270a7acae68de89d28",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Task Force (1987).bin
    {"a1ead9c181d67859aa93c44e40f1709c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Tax Avoiders (1982).bin
    {"7574480ae2ab0d282c887e9015fdb54c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Taz (1983).bin
    {"3d7aad37c55692814211c8b590a0334c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  200,   3,  2},    // Telepathy (1983).bin
    {"c830f6ae7ee58bcc2a6712fb33e92d55",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Tempest (1984).bin
    {"42cdd6a9e42a3639e190722b8ea3fc51",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Tennis (1981).bin
    {"5eeb81292992e057b290a5cd196f155d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4,  6},    // Texas Chainsaw Massacre (1983).bin
    {"32ee2063bbec93a159d99b64db2285f9",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // The Stacks (2011) (Mike Mika, Kevin Wilson).bin
    {"5fb71cc60e293fe10a5023f11c734e55",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // This Planet Sucks.bin
    {"e63a87c231ee9a506f9599aa4ef7dfb9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  202,   0, -3},    // Threshold (1982).bin
    {"de7bca4e569ad9d3fd08ff1395e53d2d",  "F6",   CTR_BOOSTER,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Thrust (V1.22) (Thomas Jentzsch) (Booster Grip).bin
    {"7ded20e88b17c8149b4de0d55c795d37",  "F6",   CTR_BOOSTER,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Thrust v1.26 (PD).bin
    {"cf507910d6e74568a68ac949537bccf9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Thunderground (1983).bin
    {"c032c2bd7017fdfbba9a105ec50f800e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Thwocker (1984).bin
    {"fc2104dd2dadf9a6176c1c1c8f87ced9",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Time Pilot (1983).bin
    {"b879e13fd99382e09bcaf1d87ad84add",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -1},    // Time Warp (1982).bin
    {"40742a6770f0fdccceb9ffc7af32a1d7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -1},    // Tinkernut World (2010) (Tinkernut).bin
    {"953c45c3dd128a4bd5b78db956c455bb",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // TitanAxe.bin
    {"12123b534bdee79ed7563b9ad74f1cbd",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  3},    // Title Match Pro Wrestling (1987).bin
    {"8bc0d2052b4f259e7a50a7c771b45241",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Tomarc the Barbarian (1983) [alt].bin
    {"32dcd1b535f564ee38143a70a8146efe",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -2},    // Tomarc the Barbarian (1983).bin
    {"3ac6c50a8e62d4ce71595134cbd8035e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Tomcat - F-14 Fighter (1988).bin
    {"fa2be8125c3c60ab83e1c0fe56922fcb",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Tooth Protectors (1983).bin
    {"01abcc1d2d3cba87a3aa0eb97a9d7b9c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   4, -2},    // Topy (unknown).bin
    {"d45ba53d74811b7a12fcff4427777fbc",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Tower of Rubble 2600-NTSC-v1.1.bin    
    {"0aa208060d7c140f20571e3341f5a3f8",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0, -2},    // Towering Inferno (1982).bin
    {"f39e4bc99845edd8621b0f3c7b8c4fd9",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // ToyshopTrouble.bin
    {"6ae4dc6d7351dacd1012749ca82f9a56",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  207,   0,  4},    // Track and Field (1984).bin
    {"24d468d81ac91c521d01c05904dcad95",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Trashmania - Remix (2012) (Jonathon Bont).bin
    {"81414174f1816d5c1e583af427ac89fc",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   2,  2},    // Treasure Below (Video Gems, Thomas Jentzsch) (NTSC).bin
    {"6c878b6983558ce03facee8b37ea3f21",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Treasure Hunt (2012).bin
    {"24df052902aa9de21c2b2525eb84a255",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Trick Shot (1982).bin
    {"fb27afe896e7c928089307b32e5642ee",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // TRON - Deadly Discs (1982).bin
    {"b2737034f974535f5c0c6431ab8caf73",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Tunnel Runner (1983).bin
    {"7a5463545dfb2dcfdafa6074b2f2c15e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  7},    // Turmoil (1982).bin
    {"085322bae40d904f53bdcc56df0593fc",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Tutankham (1983).bin
    {"137373599e9b7bf2cf162a102eb5927f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Ultra SCSICide.bin
    {"b16cb64a306a56f2bc491cbe5fb50295",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Uppa Creek! (2010) (Jason Santuci).bin
    {"81a010abdba1a640f7adf7f84e13d307",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Universal Chaos (1989).bin
    {"ee681f566aad6c07c61bbbfc66d74a27",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // UnknownActivision1_NTSC.bin
    {"700a786471c8a91ec09e2f8e47f14a04",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Hard Head (Prototype).bin
    {"73e66e82ac22b305eb4d9578e866236e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Unknown Datatech Game.bin
    {"5f950a2d1eb331a1276819520705df94",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Unknown Universal Game (1983).bin
    {"a499d720e7ee35c62424de882a3351b6",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  200,   2,  0},    // Up 'n Down (1984).bin
    {"c6556e082aac04260596b4045bc122de",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  1},    // Vanguard (1982).bin
    {"787ebc2609a31eb5c57c4a18837d1aee",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Vault Assault.bin
    {"bf7389cbfee666b33b8a88cc6c830edd",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Vault (TE).bin
    {"d08fccfbebaa531c4a4fa7359393a0a9",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Venetian Blinds Demo (1982).bin
    {"3e899eba0ca8cd2972da1ae5479b4f0d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Venture (1982).bin
    {"0956285e24a18efa10c68a33846ca84d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Viagem Espacial.bin
    {"539d26b6e9df0da8e7465f0f5ad863b7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Video Checkers (1980).bin
    {"f0b7db930ca0e548c41a97160b9f6275",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Video Chess (1979).bin
    {"ed1492d4cafd7ebf064f0c933249f5b0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Video Cube (CCE).bin
    {"4191b671bcd8237fc8e297b4947f2990",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  4},    // Video Jogger (1983).bin
    {"4209e9dcdf05614e290167a1c033cfd2",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Video Life (1981).bin
    {"60e0ea3cbe0913d39803477945e9e5ec",  "2K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Video Olympics (1977).bin
    {"107cc025334211e6d29da0b6be46aec7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Video Pinball (1981).bin
    {"ee659ae50e9df886ac4f8d7ad10d046a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Video Reflex (1983).bin
    {"6c128bc950fcbdbcaf0d99935da70156",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Volleyball (1983).bin
    {"6041f400b45511aa3a69fab4b8fc8f41",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Wabbit (1982).bin
    {"d175258b2973b917a05b46df4e1cf15d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Walker.bin
    {"d3456b4cf1bd1a7b8fb907af1a80ee15",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0, -1},    // Wall Ball (1983).bin
    {"c16fbfdbfdf5590cc8179e4b0f5f5aeb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Wall-Defender (1983).bin
    {"03ff9e8a7af437f16447fe88cea3226c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Wall-Defender (1983)[alt].bin
    {"5da448a2e1a785d56bf4f04709678156",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Wall Jump Ninja.bin
    {"cbe5a166550a8129a5e6d374901dffad",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_FF,   1,  1,  ANA0_8,  210,   0,  0},    // Warlords (1981).bin
    {"679e910b27406c6a2072f9569ae35fc8",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Warplock (1982) [alt].bin
    {"a20bc456c3b5fd9335e110df6e000e12",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Warplock (1982).bin
    {"7e7c4c59d55494e66eef5e04ec1c6157",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Warring Worms.bin
    {"827a22b9dffee24e93ed0df09ff8414a",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  9},    // Wings_10-10-83.bin
    {"8e48ea6ea53709b98e6f4bd8aa018908",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  9},    // Wings (1983).bin
    {"0cdd9cc692e8b04ba8eb31fc31d72e5e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Wing War (Imagic, Thomas Jentzsch) (NTSC).bin
    {"83fafd7bd12e3335166c6314b3bde528",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Winter Games (1987).bin
    {"7b24bfe1b61864e758ada1fe9adaa098",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Wizard (1980).bin
    {"7e8aa18bc9502eb57daaf5e7c1e94da7",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Wizard of Wor (1982).bin
    {"663ef22eb399504d5204c543b8a86bcd",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Wizard of Wor (alternate)  
    {"ec3beb6d8b5689e867bafb5d5f507491",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Word Zapper (1982).bin
    {"e62e60a3e6cb5563f72982fcd83de25a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // World End (unknown).bin
    {"87f020daa98d0132e98e43db7d8fea7e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Worm War I (1982).bin
    {"eaf744185d5e8def899950ba7c6e7bb5",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Xenophobe (1990).bin
    {"c6688781f4ab844852f4e3352772289b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Xevious (1983).bin
    {"af6f3e9718bccfcd8afb421f96561a34",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // Xevious [alt] (1983).bin
    {"5961d259115e99c30b64fe7058256bcf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  201,   0,  0},    // X-Man (1983).bin
    {"c5930d0e8cdae3e037349bfa08e871be",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  210,   0,  0},    // Yars' Revenge (1982).bin
    {"eea0da9b987d661264cce69a7c13c3bd",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  202,   0,  8},    // Zaxxon (1982).bin
    {"a336beac1f0a835614200ecd9c41fd70",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // Zoo Keeper Sounds (1984).bin
    {"c1e6e4e7ef5f146388a090f1c469a2fa",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  200,   0, -2},    // Z-Tack (AKA Base Attack) (1983) (Bomb - Onbase).bin

    // Some Work in Progress Carts (these MD5s will change when released BIN is available)
    {"7412f6788087d7e912c33ba03b36dd1b",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   1,  1,  ANA1_0,  202,   0,  4},    // Venture Reloaded (RC3).bin
    {"2df8ea51bcc9f1b3b4c61a141b5a1405",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   1,  1,  ANA1_0,  210,   0,  2},    // NinjishGuy_prerelease.bin

    {"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",  "XX",   CTR_LJOY,      99,             MODE_NO,   1,  1,  ANA1_0,  210,   0,  0},    // End of list...
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge* Cartridge::create(const uInt8* image, uInt32 size)
{
  Cartridge* cartridge = 0;

   string type = autodetectType(image, size);
   
  // We should know the cart's type by now so let's create it
  if(type == "2K")
    cartridge = new Cartridge2K(image);
  else if(type == "3E")
    cartridge = new Cartridge3E(image, size);
  else if(type == "3F")
    cartridge = new Cartridge3F(image, size);
  else if(type == "4K")
    cartridge = new Cartridge4K(image);
  else if(type == "AR")
    cartridge = new CartridgeAR(image, size);
  else if(type == "DPC")
    cartridge = new CartridgeDPC(image, size);
  else if (type == "DPCP")
  {
      cartridge = new CartridgeDPCPlus(image, size);
      dsWarnIncompatibileCart();
  }
  else if(type == "E0")
    cartridge = new CartridgeE0(image);
  else if(type == "E7")
    cartridge = new CartridgeE7(image);
  else if(type == "F4")
    cartridge = new CartridgeF4(image);
  else if(type == "F4SC")
    cartridge = new CartridgeF4SC(image);
  else if(type == "F6")
    cartridge = new CartridgeF6(image);
  else if(type == "F6SC")
    cartridge = new CartridgeF6SC(image);
  else if(type == "F8")
    cartridge = new CartridgeF8(image);
  else if(type == "F8SC")
    cartridge = new CartridgeF8SC(image);
  else if(type == "FASC")
    cartridge = new CartridgeFASC(image);
  else if(type == "FE")
    cartridge = new CartridgeFE(image);
  else if(type == "MC")
    cartridge = new CartridgeMC(image, size);
  else if(type == "MB")
    cartridge = new CartridgeMB(image);
  else if(type == "CV")
    cartridge = new CartridgeCV(image, size);
  else if(type == "UA")
    cartridge = new CartridgeUA(image);
  else
  {
    // TODO: At some point this should be handled in a better way...
    assert(false);
  }

  return cartridge;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::~Cartridge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge::autodetectType(const uInt8* image, uInt32 size)
{
  bool bFound = false;
  extern void OutputCartInfo(string type, string md5);
    
  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  // Defaults for the selected cart... this may change up below...
  myCartInfo = table[0];

  // -----------------------------------------------------------------------
  // Take a closer look at the ROM image and try to figure out its type
  // -----------------------------------------------------------------------

  // First we'll see if it's type is listed in the table above
  for(const CartInfo* entry = table; (entry->special != 99); ++entry)
  {
    if(entry->md5 == md5)   // String compare...
    {
        myCartInfo = *entry;
        bFound = true;
        break;
    }
  }
    
  if (!isDSiMode()) // For older DS/DS-LITE, we turn off Flicker Free by default...
  {
      myCartInfo.mode = MODE_NO;    
  }
  
  // Handle special situations...
  if (myCartInfo.special == SPEC_HAUNTED)
  {
      uInt8* imageOffset = (uInt8*)(image+1103);
      // Haunted House needs a fix to work... original programming bug.
      if (*imageOffset == 0xE5) *imageOffset = 0xE9;
  }

  // If we didn't find the type in the table then guess it based on size
  if(!bFound)
  {
    myCartInfo.md5 = md5;
    if((size % 8448) == 0)
    {
      myCartInfo.type = "AR";
    }
    else if((size == 2048) || (size == 4096 && memcmp(image, image + 2048, 2048) == 0))
    {
      myCartInfo.type = "2K";
    }
    else if(size == 4096)
    {
      myCartInfo.type = "4K";
    }
    else if(size == 8192)  // 8K
    {
      // TODO - autodetect FE and UA, probably not possible
      if(isProbablySC(image, size))
        myCartInfo.type = "F8SC";
      else if(memcmp(image, image + 4096, 4096) == 0)
        myCartInfo.type = "4K";
      else if(isProbablyE0(image, size))
        myCartInfo.type = "E0";
      else if(isProbably3F(image, size))
        myCartInfo.type = isProbably3E(image, size) ? "3E" : "3F";
      else
        myCartInfo.type = "F8";
    }
    else if((size == 10495) || (size == 10496) || (size == 10240))  // 10K - Pitfall2
    {
      myCartInfo.type = "DPC";
    }
    else if(size == 12288)
    {
      // TODO - this should really be in a method that checks the first
      // 512 bytes of ROM and finds if either the lower 256 bytes or
      // higher 256 bytes are all the same.  For now, we assume that
      // all carts of 12K are CBS RAM Plus/FASC.
      myCartInfo.type = "FASC";
    }
    else if(size == 16384)  // 16K
    {    
      if(isProbablySC(image, size))
        myCartInfo.type = "F6SC";
      else if(isProbablyE7(image, size))
        myCartInfo.type = "E7";
      else if(isProbably3F(image, size))
        myCartInfo.type = isProbably3E(image, size) ? "3E" : "3F";
      else
        myCartInfo.type = "F6";
    }
    else if(size == 32768)  // 32K 
    {
      // Assume this is a 32K super-cart then check to see if it is
      if(isProbablySC(image, size))
        myCartInfo.type = "F4SC";
      else if (isProbablyDPCplus(image, size))
        myCartInfo.type = "DPCP";
      else if(isProbably3F(image, size))
        myCartInfo.type = isProbably3E(image, size) ? "3E" : "3F";
      else
        myCartInfo.type = "F4";
    }
    else if(size == 65536)
    {
      //ALEK type = isProbably3F(image, size) ? "3F" : "MB";
      // TODO - autodetect 4A50
      if(isProbably3F(image, size))
        myCartInfo.type = isProbably3E(image, size) ? "3E" : "3F";
      else
        myCartInfo.type = "MB";
    }
    else if(size == 131072)
    {
      //ALEK type = isProbably3F(image, size) ? "3F" : "MC";
      // TODO - autodetect 4A50
      if(isProbably3F(image, size))
        myCartInfo.type = isProbably3E(image, size) ? "3E" : "3F";
      else
        myCartInfo.type = "MC";
    }
    else  // what else can we do?
    {
      if(isProbably3F(image, size))
        myCartInfo.type = isProbably3E(image, size) ? "3E" : "3F";
      else
        myCartInfo.type = "4K";  // Most common bankswitching type
    }  
  }
    
  dsPrintCartType((char*)myCartInfo.type.c_str());
  
  return myCartInfo.type;
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge::searchForBytes(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 1; ++i)
  {
    if((image[i] == byte1) && (image[i + 1] == byte2))
    {
      ++count;
    }
  }

  return count;
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge::searchForBytes4(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2, uInt8 byte3, uInt8 byte4)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 3; ++i)
  {
    if((image[i] == byte1) && (image[i + 1] == byte2) && (image[i + 2] == byte3) && (image[i + 3] == byte4))
    {
      ++count;
    }
  }

  return count;
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablySC(const uInt8* image, uInt32 size)
{
  // We assume a Superchip cart contains the same bytes for its entire
  // RAM area; obviously this test will fail if it doesn't
  // The RAM area will be the first 256 bytes of each 4K bank
  uInt32 banks = size / 4096;
  for(uInt32 i = 0; i < banks; ++i)
  {
    uInt8 first = image[i*4096];
    for(uInt32 j = 0; j < 256; ++j)
    {
      if(image[i*4096+j] != first)
        return false;
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbably3F(const uInt8* image, uInt32 size)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 1; ++i)
  {
    if((image[i] == 0x85) && (image[i + 1] == 0x3F))
    {
      ++count;
    }
  }

  return (count > 2);
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbably3E(const uInt8* image, uInt32 size)
{
  return (searchForBytes(image, size, 0x85, 0x3E) > 2);
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablyE0(const uInt8* image, uInt32 size)
{
  // E0 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FF7 using absolute non-indexed addressing
  // So we search for the pattern 'LDA Fxxx' or 'STA Fxxx' in hex
  // using the regex (AD|8D, E0-F7, xF)
  // This must be present at least three times, since there are
  // three segments to be initialized (and a few more so that more
  // of the ROM is used)
  // Thanks to "stella@casperkitty.com" for this advice
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 2; ++i)
  {
    uInt8 b1 = image[i], b2 = image[i+1], b3 = image[i+2];
    if((b1 == 0xAD || b1 == 0x8D) &&
       (b2 >= 0xE0 && b2 <= 0xF7) &&
       ((b3 & 0xF) == 0xF))
    {
      if(++count > 4)  return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablyDPCplus(const uInt8* image, uInt32 size)
{
  // DPC+ ARM code has 2 occurrences of the string DPC+
  // Note: all Harmony/Melody custom drivers also contain the value
  // 0x10adab1e (LOADABLE) if needed for future improvement
  return searchForBytes4(image, size, 'D', 'P', 'C', '+');
}


//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablyE7(const uInt8* image, uInt32 size)
{
  // E7 carts map their second 1K block of RAM at addresses
  // $800 to $8FF.  However, since this occurs in the upper 2K address
  // space, and the last 2K in the cart always points to the last 2K of the
  // ROM image, the RAM area should fall in addresses $3800 to $38FF
  // Similar to the Superchip cart, we assume this RAM block contains
  // the same bytes for its entire area
  // Also, we want to distinguish between ROMs that have large blocks
  // of the same amount of (probably unused) data by making sure that
  // something differs in the previous 32 or next 32 bytes
  uInt8 first = image[0x3800];
  for(uInt32 i = 0x3800; i < 0x3A00; ++i)
  {
    if(first != image[i])
      return false;
  }

  // OK, now scan the surrounding 32 byte blocks
  uInt32 count1 = 0, count2 = 0;
  for(uInt32 i = 0x3800 - 32; i < 0x3800; ++i)
  {
    if(first != image[i])
      ++count1;
  }
  for(uInt32 i = 0x3A00; i < 0x3A00 + 32; ++i)
  {
    if(first != image[i])
      ++count2;
  }

  return (count1 > 0 || count2 > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge(const Cartridge&)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge& Cartridge::operator = (const Cartridge&)
{
  assert(false);
  return *this;
}

