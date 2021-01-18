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

// Analog Sensitivity... 10 = 1.0 and normal... 1.1 is faster and 0.9 is slower
#define ANA0_7   7
#define ANA0_8   8
#define ANA0_9   9
#define ANA1_0  10
#define ANA1_1  11
#define ANA1_2  12
#define ANA1_3  13
#define ANA1_4  14
#define ANA1_5  15
#define ANA2_0  20
#define ANA2_5  25

extern void dsWarnIncompatibileCart(void);

// We can store up to 10k in the fast DTCM memory to give a speed boost...
uInt8 fast_cart_buffer[10*1024] __attribute__((section(".dtcm")));
CartInfo myCartInfo = {"","",0,0,0,0};

static CartInfo table[] = 
{
    {"DefaultCart_xxxxxxxxxxxxxxxxxxxx",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Default Cart is 4k, L-Joy and nothing special...
    {"0db4f4150fecf77e4ce72ca4d04c052f",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // 3-D Tic-Tac-Toe (1980).bin
    {"b6d13da9e641b95352f090090e440ce4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Aardvark.bin
    {"b6c2b4ddc42ef5db21cfcc3f8be120d6",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  194,   0,  0},    // Aardvark (32k Demo).bin    
    {"ac7c2260378975614192ca2bc3d20e0b",  "FE",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Activision Decathlon (1983).bin
    {"157bddb7192754a45372be196797f284",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Adventure (1980).bin
    {"ca4f8c5b4d6fb9d608bb96bc7ebd26c7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Adventures of TRON (1982).bin
    {"4d77f291dca1518d7d8e47838695f54b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Airlock (1982).bin
    {"a9cb638cd2cb2e8e0643d7a67db4281c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Air Raiders (1982).bin
    {"16cb43492987d2f32b423817cdaaf7c4",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Air-Sea Battle (1977).bin
    {"0c7926d660f903a2d6910c254660c32c",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Air-Sea Battle (pal).bin
    {"1d1d2603ec139867c1d1f5ddf83093f1",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Air-Sea Battle (sears).bin
    {"f1a0a23e6464d954e3a9579c4ccd01c8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Alien (1982).bin
    {"3aa47765f6184e64c41d1fa2b0b16ddc",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Alligator32.bin
    {"e1a51690792838c5c687da80cd764d78",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Alligator People (1983).bin
    {"9e01f7f95cb8596765e03b9a36e8e33c",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Alpha Beam with Ernie (1983).bin
    {"acb7750b4d0c4bd34969802a7deb2990",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Amidar (1982).bin
    {"539f3c42c4e15f450ed93cb96ce93af5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Amoeba Jump.bin
    {"0866e22f6f56f92ea1a14c8d8d01d29c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // AndroMan on the Moon (1984).bin
    {"428428097f85f74dc5cf0dbe07cf16e0",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Anguna-rc3.bin
    {"e73838c43040bcbc83e4204a3e72eef4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Apples and Dolls (CCE).bin
    {"f69d4fcf76942fcd9bdf3fd8fde790fb",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Aquaventure (CCE).bin
    {"a7b584937911d60c120677fe0d47f36f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Armor Ambush (1982).bin
    {"c77c35a6fc3c0f12bf9e8bae48cba54b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Artillery Duel (1983).bin
    {"de78b3a064d374390ac0710f95edde92",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Assault (AKA Sky Alien) (1983).bin
    {"c50fbee08681f15d2d40dbc693d3a837",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // A Star.bin
    {"89a68746eff7f266bbf08de2483abe55",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Asterix (AKA Taz) (1983).bin
    {"dd7884b4f93cab423ac471aa1935e3df",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  1},    // Asteroids (1981).bin
    {"170e7589a48739cfb9cc782cbb0fe25a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Astroblast (1982).bin
    {"8f53a3b925f0fd961d9b8c4d46ee6755",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Astrowar (Unknown).bin
    {"4edb251f5f287c22efc64b3a2d095504",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Atari VCS Point-of-Purchase ROM (1982).bin
    {"3f540a30fdee0b20aed7288e4a5ea528",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Atari Video Cube (1982).bin
    {"cd5af682685cfecbc25a983e16b9d833",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // A-Team (AKA Saboteur) (1984).bin
    {"9ad36e699ef6f45d9eb6c4cf90475c9f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Atlantis (1982).bin
    {"826481f6fc53ea47c9f272f7050eedf7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Atlantis II (1982).bin
    {"4eb7b733de3e61184341f46a24f8e489",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Avalanche.bin
    {"cb81972e2cd9b175ded45d7f0892da42",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // AVCSTec Challenge.bin
    {"274d17ccd825ef9c728d68394b4569d2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bachelorette Party (1982).bin
    {"5b124850de9eea66781a50b2e9837000",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bachelor Party (1982).bin
    {"0f34f9158b4b85707d465a06d9b270bf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // BackFire.bin
    {"8556b42aa05f94bc29ff39c39b11bff4",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Backgammon (1979).bin
    {"00ce0bdd43aed84a983bef38fe7f5ee3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bank Heist (1983).bin
    {"f8240e62d8c0a64a61e19388414e3104",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // Barnstorming (1982).bin
    {"9d37a1be4a6e898026414b8fee2fc826",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Baseball (AKA Super Challenge Baseball) (1989).bin
    {"819aeeb9a2e11deb54e6de334f843894",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Basic Math (1977).bin
    {"9f48eeb47836cf145a15771775f0767a",  "4K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // BASIC Programming (1979).bin
    {"ab4ac994865fb16ebb85738316309457",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Basketball (1978).bin
    {"41f252a66c6301f1e8ab3612c19bc5d4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Battlezone (1983).bin
    {"79ab4123a83dc11d468fb2108ea09e2e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Beamrider (1984).bin
    {"d0b9df57bfea66378c0418ec68cfe37f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Beany Bopper (1982).bin
    {"59e96de9628e8373d1c685f5e57dcf10",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Beat 'Em & Eat 'Em (1982).bin
    {"0d22edcb05e5e52f45ddfe171e3c4b50",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bee Ball.bin
    {"ee6665683ebdb539e89ba620981cb0f6",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Berenstain Bears (1983).bin
    {"b8ed78afdb1e6cfe44ef6e3428789d5f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bermuda Triangle (1982).bin
    {"136f75c4dd02c29283752b7e5799f978",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Berzerk (1982).bin
    {"be41463cd918daef107d249f8cde3409",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Berzerk-VE.bin
    {"1badbf0d3cb5abf7cf29233120dc14cc",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // BiFrost (RC).bin
    {"1802cc46b879b229272501998c5de04f",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Big Bird's Egg Catch (1983).bin
    {"0a981c03204ac2b278ba392674682560",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Blackjack (1977).bin
    {"42dda991eff238d26669fd33e353346d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Blinky Goes Up.bin
    {"33d68c3cd74e5bc4cf0df3716c5848bc",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Blueprint (1983).bin
    {"968efc79d500dce52a906870a97358ab",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // BMX Air Master (1990).bin
    {"521f4dd1eb84a09b2b19959a41839aad",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bobby Is Going Home (1983).bin
    {"c59633dbebd926c150fb6d30b0576405",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bogey Blaster (1989).bin
    {"a5855d73d304d83ef07dde03e379619f",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Boggle (1978).bin
    {"14c2548712099c220964d7f044c59fd9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Boing! (1983).bin
    {"ab48c4af46c8b34c3613d210e1206132",  "3E",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Boulder Dash Demo V2.bin
    {"c9b7afad3bfd922e006a6bfc1d4f3fe7",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bowling (1979).bin
    {"25f2e760cd7f56b88aac88d63757d41b",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, 10},    // Boxing (1980).bin
    {"1cca2197d95c5a41f2add49a13738055",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Brain Games (1978).bin
    {"f34f08e5eb96e500e851a80be3277a56",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_1,  205,   0,  0},    // Breakout (1978).bin
    {"6c76fe09aa8b39ee52035e0da6d0808b",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_1,  205,   0,  0},    // Breakout (1978).bin
    {"4df6124093ccb4f0b6c26a719f4b7706",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_1,  205,   0,  0},    // Breakout (1978).bin
    {"9a25b3cfe2bbb847b66a97282200cca2",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_1,  205,   0,  0},    // Breakout (1978).bin    
    {"c5fe45f2734afd47e27ca3b04a90213c",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_1,  205,   0,  0},    // Breakout (1978).bin        
    {"413c925c5fdcea62842a63a4c671a5f2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bridge (1980).bin
    {"1cf59fc7b11cdbcefe931e41641772f6",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  6},    // Buck Rogers (1983).bin
    {"68597264c8e57ada93be3a5be4565096",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bugs (1982).bin
    {"fa4404fabc094e3a31fcd7b559cdd029",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Bugs Bunny (1983).bin
    {"aa1c41f86ec44c0a44eb64c332ce08af",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bumper Bash (1983).bin
    {"76f53abbbf39a0063f24036d6ee0968a",  "E7",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Bump 'n' Jump (1983).bin
    {"0443cfa9872cdb49069186413275fa21",  "E7",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // BurgerTime (1983).bin
    {"19d6956ff17a959c48fcd8f4706a848d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Burning Desire (1982).bin
    {"466c9d4a93668ab6a052c268856cf4a5",  "F4SC", CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Circus Atari Age PRGE Demo.bin
    {"66fcf7643d554f5e15d4d06bab59fe70",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cabbage Patch Kids (1984).bin
    {"7f6533386644c7d6358f871666c86e79",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cakewalk (1983).bin
    {"9ab72d3fd2cc1a0c9adb504502579037",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // California Games (1988).bin
    {"feedcc20bc3ca34851cd5d9e38aa2ca6",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Canyon Bomber (1979).bin
    {"151c33a71b99e6bcffb34b43c6f0ec23",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Care Bears (1983).bin
    {"028024fb8e5e5f18ea586652f9799c96",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  9},    // Carnival (1982).bin
    {"b816296311019ab69a21cb9e9e235d12",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Casino (1979).bin
    {"9e192601829f5f5c2d3b51f8ae25dbe5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cathouse Blues (1982).bin
    {"d071d2ec86b9d52b585cc0382480b351",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cat Tracks.bin
    {"76f66ce3b83d7a104a899b4b3354a2f2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cat Trax (1983).bin
    {"1cedebe83d781cc22e396383e028241a",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Cave In.bin
    {"91c2098e88a6b13f977af8c003e0bca5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Centipede (1982).bin
    {"5d799bfa9e1e7b6224877162accada0d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Challenge of.... Nexar (1982).bin
    {"73158ea51d77bf521e1369311d26c27b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Challenge (Zellers).bin
    {"ace319dc4f76548659876741a6690d57",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Championship Soccer (1980).bin
    {"3e33ac10dcf2dff014bc1decf8a9aea4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Chase the Chuckwagon (1983).bin
    {"3f5a43602f960ede330cd2f43a25139e",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Checkers (1980).bin
    {"749fec9918160921576f850b2375b516",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // China Syndrome (1982).bin
    {"c1cb228470a87beb5f36e90ac745da26",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Chopper Command (1982).bin
    {"3f58f972276d1e4e0e09582521ed7a5b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Chuck Norris Superkicks (1983).bin
    {"a7b96a8150600b3e800a4689c3ec60a2",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA0_7,  205,   0,  0},    // Circus Atari (1980).bin
    {"5f383ce70c30c5059f13e89933b05c4a",  "F8",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA0_7,  205,   0,  0},    // Circus Atariage 2020.bin  
    {"13a11a95c9a9fb0465e419e4e2dbd50a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Climber5.bin
    {"1e587ca91518a47753a28217cd4fd586",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Coco Nuts (1982).bin
    {"5846b1d34c296bf7afc2fa05bbc16e98",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -2},    // Codebreaker (1978).bin
    {"e3aa9681fab887a5cb964a55b4edd5d7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Colony 7.bin
    {"4c8832ed387bbafc055320c05205bc08",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Combat (1977).bin
    {"5d2cc33ca798783dee435eb29debf6d6",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Commando (1988).bin
    {"f457674cef449cfd85f21db2b4f631a7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Commando Raid (1982).bin
    {"2c8835aed7f52a0da9ade5226ee5aa75",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Communist Mutants from Space (1982).bin
    {"6a2c68f7a77736ba02c0f21a6ba0985b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Computer Chess (1978).bin
    {"1f21666b8f78b65051b7a609f1d48608",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Condor Attack (1982).bin
    {"f965cc981cbb0822f955641f8d84e774",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Confrontation (1983).bin
    {"00b7b4cbec81570642283e7fc1ef17af",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Congo Bongo (1983).bin
    {"0f604cd4c9d2795cf5746e8af7948064",  "F6",   CTR_LJOY,      SPEC_CONMARS,   MODE_NO,   ANA1_0,  205,   0,  7},    // Conquest of Mars.bin
    {"57c5b351d4de021785cf8ed8191a195c",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cookie Monster Munch (1983).bin
    {"ab5bf1ef5e463ad1cbb11b6a33797228",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cosmic Ark (1982).bin
    {"133b56de011d562cbab665968bde352b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cosmic Commuter (1984).bin
    {"f367e58667a30e7482175809e3cec4d4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cosmic Corridor (1983).bin
    {"3c853d864a1d5534ed0d4b325347f131",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cosmic Creeps (1982).bin
    {"e5f17b3e62a21d0df1ca9aee1aa8c7c5",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cosmic Swarm (1982).bin
    {"fe67087f9c22655ce519616fc6c6ef4d",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crack'ed (1988).bin
    {"a184846d8904396830951217b47d13d9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  7},    // Crackpots (1983).bin
    {"fb88c400d602fe759ae74ef1716ee84e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crash Dive (1983).bin
    {"9c53b60a7b439a01efa46ae1effa348e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crazy Ballon.bin
    {"55ef7b65066428367844342ed59f956c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crazy Climber (1982).bin
    {"4a7eee19c2dfb6aeb4d9d0a01d37e127",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crazy Valet.bin
    {"8cd26dcf249456fe4aeb8db42d49df74",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crossbow (1987).bin
    {"c17bdc7d14a36e10837d039f43ee5fa3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cross Force (1982).bin
    {"74f623833429d35341b7a84bc09793c0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cruise Missile (1987).bin
    {"384f5fbf57b5e92ed708935ebf8a8610",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crypts of Chaos (1982).bin
    {"1c6eb740d3c485766cade566abab8208",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Crystal Castles (1984).bin
    {"6fa0ac6943e33637d8e77df14962fbfc",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Cubicolor (1982).bin
    {"58513bae774360b96866a07ca0e8fd8e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Custer's Revenge (1982).bin
    {"a422194290c64ef9d444da9d6a207807",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dark Cavern (1982).bin
    {"106855474c69d08c8ffa308d47337269",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dark Chambers (1988).bin
    {"6333ef5b5cbb77acd47f558c8b7a95d3",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dark Mage (8K) (PD).bin
    {"e4c00beb17fdc5881757855f2838c816",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Deadly Duck (1982).bin
    {"4e15ddfd48bca4f0bf999240c47b49f5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Death Trap (1983).bin
    {"0f643c34e40e3f1daafd9c524d3ffe64",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Defender (1982).bin
    {"3a771876e4b61d42e3a3892ad885d889",  "F8SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Defender II (Stargate) (1988).bin
    {"d09935802d6760ae58253685ff649268",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Demolition Herby (1983).bin
    {"bcb2967b6a9254bcccaf906468a22241",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Demon Attack (1981).bin
    {"f91fb8da3223b79f1c9a07b77ebfa0b2",  "4K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   ANA0_9,  205,   0,  0},    // Demons to Diamonds (1982).bin
    {"fd4f5536fd80f35c64d365df85873418",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Desert Falcon (1987).bin
    {"9222b25a0875022b412e8da37e7f6887",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dice Puzzle (1983).bin
    {"6dda84fb8e442ecf34241ac0d1d91d69",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dig Dug (1983).bin
    {"939ce554f5c0e74cc6e4e62810ec2111",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dishaster (1983).bin
    {"f1ae6305fa33a948e36deb0ef12af852",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  2},    // Donkey Kong VCS.bin
    {"494cda91cc640551b4898c82be058dd9",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Donkey Kong VCS (pal).bin    
    {"c3472fa98c3b452fa2fd37d1c219fb6f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dodge 'Em (1980).bin
    {"ca09fa7406b7d2aea10d969b6fc90195",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dolphin (1983).bin
    {"937736d899337036de818391a87271e0",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Donald Duck's Speedboat (1983).bin
    {"36b20c427975760cb9cf4a47e41369e4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Donkey Kong (1982).bin
    {"c8fa5d69d9e555eb16068ef87b1c9c45",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  2},    // Donkey Kong Junior (1983).bin
    {"7386004f9a5a7daf7e50ac8547088337",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // DOT.bin
    {"7e2fe40a788e56765fe56a3576019968",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Double Dragon (1989).bin
    {"368d88a6c071caba60b4f778615aae94",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Double Dunk (Super Basketball) (1989).bin
    {"05215b73ec33b502449ee726ac6b201f",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Draconian.bin
    {"41810dd94bd0de1110bedc5092bef5b0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  2},    // Dragonfire (1982).bin
    {"90ccf4f30a5ad8c801090b388ddd5613",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Dragonstomper (Excalibur) (1982).bin
    {"77057d9d14b99e465ea9e29783af0ae3",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dragster (1980).bin
    {"51de328e79d919d7234cf19c1cd77fbc",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dukes of Hazzard (1983).bin
    {"3897744dd3c756ea4b1542e5e181e02a",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dumbo's Flying Circus (1983).bin
    {"469473ff6fed8cc8d65f3c334f963aab",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dune (1984).bin
    {"469473ff6fed8cc8d65f3c334f963aab",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dune.bin
    {"d4f23c92392474b8bc79184f1deb1b4d",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Dungeon.bin
    {"033e21521e0bf4e54e8816873943406d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Earth Dies Screaming (1983).bin
    {"683dc64ef7316c13ba04ee4398e2b93a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Edtris (1995).bin
    {"42b2c3b4545f1499a083cfbc4a3b7640",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Eggomania (1982).bin
    {"71f8bacfbdca019113f3f0801849057e",  "F8SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Elevator Action (1983).bin
    {"7657b6373fcc9ad69850a687bee48aa1",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Elevators Amiss.bin
    {"b6812eaf87127f043e78f91f2028f9f4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Eli's Ladder (1982).bin
    {"7eafc9827e8d5b1336905939e097aae7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Elk Attack (1987).bin
    {"dbc8829ef6f12db8f463e30f60af209f",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Encounter at L-5 (1982).bin
    {"94b92a882f6dbaa6993a46e2dcc58402",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Enduro (1983).bin
    {"9f5096a6f1a5049df87798eb59707583",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Entity (1983).bin
    {"6b683be69f92958abe0e2a9945157ad5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Entombed (1982).bin
    {"7e6a1375ee356f5a682f643bb8b7090c",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Epicadv28.bin
    {"81f4f0285f651399a12ff2e2f35bab77",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Escape from the Mindmaster (1982).bin
    {"f344ac1279152157d63e64aa39479599",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Espial (1984).bin
    {"615a3bf251a38eb6638cdc7ffbde5480",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // E.T. - The Extra-Terrestrial (1982).bin
    {"6205855cc848d1f6c4551391b9bfa279",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Euchre.bin
    {"e04c8ecae485b6970d680c202e58f843",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Evil Magician Returns.bin
    {"d44d90e7c389165f5034b5844077777f",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ewok Adventure.bin
    {"6362396c8344eec3e86731a700b13abf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Exocet (1983).bin
    {"76181e047c0507b2779b4bcbf032c9d5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fall Down.bin
    {"b80d50ecee73919a507498d0a4d922ae",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fantastic Voyage (1982).bin
    {"f7e07080ed8396b68f2e5788a5c245e2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Farmyard Fun.bin
    {"9de0d45731f90a0a922ab09228510393",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fast Eddie (1982).bin
    {"665b8f8ead0eef220ed53886fbd61ec9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fast Food (1982).bin
    {"85470dcb7989e5e856f36b962d815537",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fatal Run (1989).bin
    {"0b55399cf640a2a00ba72dd155a0c140",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fathom (1983).bin
    {"e4b12deaafd1dbf5ac31afe4b8e9c233",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fellowship of the Ring.bin
    {"211fbbdbbca1102dc5b43dc8157c09b3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Final Approach (1982).bin
    {"386ff28ac5e254ba1b1bac6916bcc93a",  "AR",   CTR_PADDLE0,   SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Fireball (Frantic) (1982).bin
    {"d09f1830fb316515b90694c45728d702",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fire Fighter (1982).bin
    {"20dca534b997bf607d658e77fbb3c0ee",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fire Fly (1983).bin
    {"d3171407c3a8bb401a3a62eb578f48fb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fire Spinner.bin
    {"3fe43915e5655cf69485364e9f464097",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fisher Price (1983).bin
    {"b8865f05676e64f3bec72b9defdacfa7",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Fishing Derby (1980).bin
    {"db112399ab6d6402cc2b34f18ef449da",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fixit.bin
    {"30512e0e83903fc05541d2f6a6a62654",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Flag Capture (1978).bin
    {"163ff70346c5f4ce4048453d3a2381db",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // FlapPing.bin
    {"8786c1e56ef221d946c64f6b65b697e9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Flash Gordon (1983).bin
    {"e549f1178e038fa88dc6d657dc441146",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Football (1979).bin
    {"e275cbe7d4e11e62c3bfcfb38fca3d49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Football (AKA Super Challenge Football) (1989).bin
    {"5926ab1a9d1d34fb7c3bfd5afff6bc80",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Four Play.bin
    {"15dd21c2608e0d7d9f54c0d3f08cca1f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Frankenstein's Monster (1983).bin
    {"16c909c16ce47e2429fe6e005551178d",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Frantic.bin
    {"8e0ab801b1705a740b476b7f588c6d16",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   2,  2},    // Freeway (1981).bin
    {"e80a4026d29777c3c7993fbfaee8920f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Frisco (Unknown).bin
    {"081e2c114c9c20b61acf25fc95c71bf4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0, -3},    // Frogger (1982).bin
    {"27c6a2ca16ad7d814626ceea62fa8fb4",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Frogger II - Threeedeep! (1984).bin
    {"f67181b3a01b9c9159840b15449b87b0",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Frog Pond (1982).bin
    {"dcc2956c7a39fdbf1e861fc5c595da0d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Frogs and Flies (1982).bin
    {"e556e07cc06c803f2955986f53ef63ed",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Front Line (1984).bin
    {"4ca73eb959299471788f0b685c3ba0b5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Frostbite (1983).bin
    {"d3bb42228a6cd452c111c1932503cc03",  "UA",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Funky Fish (1983).bin
    {"d3bb42228a6cd452c111c1932503cc03",  "UA",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Funky Fish.bin
    {"819aeeb9a2e11deb54e6de334f843894",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Fun with Numbers (1980).bin
    {"f38210ca3955a098c06a1e1c0004ef39",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Galactopus20141115.bin
    {"476d8d236085f8b1a6892dad3a898a62",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Galaga.bin
    {"211774f4c5739042618be8ff67351177",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Galaxian (1983).bin
    {"102672bbd7e25cd79f4384dd7214c32b",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Game of Concentration (1980).bin
    {"31f4692ee2ca07a7ce1f7a6a1dab4ac9",  "4K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Game of Concentration (1980).bin       
    {"db971b6afc9d243f614ebf380af0ac60",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gamma-Attack (1983).bin
    {"20edcc3aa6c189259fa7e2f044a99c49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gangster Alley (1982).bin
    {"dc13df8420ec69841a7c51e41b9fbba5",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Garfield (1984).bin
    {"5cbd7c31443fb9c308e9f0b54d94a395",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gas Hog (1983).bin
    {"e9ecb4102242f662acbf6ea6e77fa940",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // GateRacerII.bin
    {"e64a8008812327853877a37befeb6465",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gauntlet (1983).bin
    {"e314b42761cd13c03def744b4afc7b1b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ghostbusters (1985).bin
    {"2bee7f226d506c217163bad4ab1768c0",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ghost Manor (1983).bin
    {"1c8c42d1aee5010b30e7f1992d69216e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gigolo (1982).bin
    {"5e0c37f534ab5ccc4661768e2ddf0162",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Glacier Patrol (1989).bin
    {"5e0c37f534ab5ccc4661768e2ddf0162",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Glacier Patrol.bin
    {"2d9e5d8d083b6367eda880e80dfdfaeb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Glib - Video Word Game (1983).bin
    {"787a2faebadc670a887a0e2483b8f034",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // Go Fish.bin
    {"4093382187f8387e6d011883e8ea519b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Go Go Home Monster (Unknown).bin
    {"2e663eaa0d6b723b645e643750b942fd",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Golf (1980).bin
    {"c16c79aad6272baffb8aae9a7fff0864",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gopher (1982).bin
    {"81b3bf17cf01039d311b4cd738ae608e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gorf (1982).bin
    {"2903896d88a341511586d69fcfc20f7d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  6},    // Grand Prix (1982).bin
    {"7146dd477e019f81eac654a79be96cb5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gravitar (1983).bin
    {"9245a84e9851565d565cb6c9fac5802b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Great Escape (1983).bin
    {"01cb3e8dfab7203a9c62ba3b94b4e59f",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gremlins (1984).bin
    {"66b89ba44e7ae0b51f9ef000ebba1eb7",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Grover's Music Maker (1983).bin
    {"7ab2f190d4e59e8742e76a6e870b567e",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Guardian (1982).bin
    {"f750b5d613796963acecab1690f554ae",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gunfight (Manuel Polik).bin
    {"b311ab95e85bc0162308390728a7361d",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Gyruss (1984).bin
    {"30516cfbaa1bc3b5335ee53ad811f17a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Halloween (1983).bin
    {"4afa7f377eae1cafb4265c68f73f2718",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Halo2600_Final.bin
    {"f16c709df0a6c52f47ff52b9d95b7d8d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Hangman (1978).bin
    {"a34560841e0878c7b14cc65f79f6967d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Harem (1982).bin
    {"f0a6e99f5875891246c3dbecbf2d2cea",  "4K",   CTR_LJOY,      SPEC_HAUNTED,   MODE_FF,   ANA1_0,  205,   0,  2},    // Haunted House (1982).bin
    {"2cc640f904e1034bf438075a139548d3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Heartbreak.bin
    {"fca4a5be1251927027f2c24774a02160",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  8},    // H.E.R.O. (1984).bin
    {"3d48b8b586a09bdbf49f1a016bf4d29a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Hole Hunter (AKA Topy).bin
    {"3d48b8b586a09bdbf49f1a016bf4d29a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Hole Hunter.bin
    {"c52d9bbdc5530e1ef8e8ba7be692b01e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Holey Moley (1984).bin
    {"0bfabf1e98bdb180643f35f2165995d0",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Home Run - Baseball (1978).bin
    {"9f901509f0474bf9760e6ebd80e629cd",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Home Run - Baseball (Sears 4K).bin
    {"7972e5101fa548b952d852db24ad6060",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Human Cannonball (1979).bin
    {"102672bbd7e25cd79f4384dd7214c32b",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Hunt & Score (1978).bin
    {"a4c08c4994eb9d24fb78be1793e82e26",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ice Hockey (1981).bin
    {"9a21fba9ee9794e0fadd7c7eb6be4e12",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ikari Warriors (1989).bin
    {"c4bc8c2e130d76346ebf8eb544991b46",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Imagic Selector.bin
    {"75a303fd46ad12457ed8e853016815a0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Immies & Aggies (1983).bin
    {"47abfb993ff14f502f88cf988092e055",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Inca Gold (AKA Spider Kong).bin
    {"ad859c8d0c9513b47861f38d03bdead7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Incoming.bin
    {"c5301f549d0722049bb0add6b10d1e09",  "2K",   CTR_DRIVING,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Indy 500 (1977).bin
    {"afe88aae81d99e0947c0cfb687b16251",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Infiltrate (1981).bin
    {"b4030c38a720dd84b84178b6ce1fc749",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // International Soccer (1982).bin
    {"9ea8ed9dec03082973244a080941e58a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // INV+.bin
    {"4868a81e1b6031ed66ecd60547e6ec85",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Inv (V2.1) (1-3-98).bin
    {"4b9581c3100a1ef05eac1535d25385aa",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // I.Q. 180.bin
    {"ab301d3d7f2f4fe3fdd8a3540b7a74f5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // IQ-180.bin
    {"f6a282374441012b01714e19699fc62a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // I Want My Mommy (1983).bin
    {"2f0546c4d238551c7d64d884b618100c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ixion (1984).bin
    {"2f0546c4d238551c7d64d884b618100c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ixion.bin
    {"e51030251e440cffaab1ac63438b44ae",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // James Bond 007 (1983).bin
    {"04dfb4acac1d0909e4c360fd2ac04480",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Jammed.bin
    {"58a82e1da64a692fd727c25faef2ecc9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Jawbreaker (1982).bin
    {"718ae62c70af4e5fd8e932fee216948a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Journey Escape (1982).bin
    {"3276c777cbe97cdd2b4a63ffc16b7151",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Joust (1983).bin
    {"ec40d4b995a795650cf5979726da67df",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Joust Pong.bin
    {"89a923f6f6bec64f9b6fa6ff8ea37510",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Joyride.bin
    {"36c29ceee2c151b23a1ad7aa04bd529d",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -6},    // Jr. Pac-Man (1984).bin
    {"4364680de59e650a02348b901ca7202f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Jump.bin
    {"2cccc079c15e9af94246f867ffc7e9bf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Jungle Fever (1982).bin
    {"2bb9f4686f7e08c5fcc69ec1a1c66fe7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Jungle Hunt (1983).bin
    {"90b647bfb6b18af35fcf613573ad2eec",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Juno First.bin
    {"b9d1e3be30b131324482345959aed5e5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kabobber (1983).bin
    {"b9d1e3be30b131324482345959aed5e5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kabobber (Activision).bin
    {"5428cdfada281c569c74c7308c7f2c26",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Kaboom! (1981).bin
    {"af6ab88d3d7c7417db2b3b3c70b0da0a",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Kaboom! (1981).bin
    {"7b43c32e3d4ff5932f39afcb4c551627",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kamikaze Saucers (1983).bin
    {"7b43c32e3d4ff5932f39afcb4c551627",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kamikaze Saucers.bin
    {"4326edb70ff20d0ee5ba58fa5cb09d60",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kangaroo (1983).bin
    {"cedbd67d1ff321c996051eec843f8716",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Karate (1982).bin
    {"cedbd67d1ff321c996051eec843f8716",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Karate (1987) (Froggo).bin
    {"be929419902e21bd7830a7a7d746195d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Keystone Kapers (1983).bin
    {"7a7f6ab9215a3a6b5940b8737f116359",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Killer Satellites (1983).bin
    {"e21ee3541ebd2c23e817ffb449939c37",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // King Kong (1982).bin
    {"2c29182edf0965a7f56fe0897d2f84ba",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Klax (1990).bin
    {"7fcd1766de75c614a3ccc31b25dd5b7a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Knight on the Town (1982).bin
    {"534e23210dd1993c828d944c6ac4d9fb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kool-Aid Man (1983).bin
    {"4baada22435320d185c95b7dd2bcdb24",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Krull (1983).bin
    {"5b92a93b23523ff16e2789b820e2a4c5",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kung-Fu Master (1987).bin
    {"3f58f972276d1e4e0e09582521ed7a5b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kung Fu Superkicks (1989).bin
    {"7ad782952e5147b88b65a25cadcdf9e0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kwibble (1983).bin
    {"b86552198f52cfce721bafb496363099",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Kyphus (1982).bin
    {"adfbd2e8a38f96e03751717f7422851d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0, 10},    // Lady Bug.bin
    {"f1489e27a4539a0c6c8529262f9f7e18",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0, 10},    // Lady Bug PAL.bin    
    {"95a89d1bf767d7cc9d0d5093d579ba61",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lady in Wading (1982).bin
    {"931b91a8ea2d39fe4dca1a23832b591a",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Laser Blast (1981).bin
    {"1fab68fd67fe5a86b2c0a9227a59bb95",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lasercade (1983).bin
    {"1fa58679d4a39052bd9db059e8cda4ad",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // Laser Gates (1983).bin
    {"31e518debba46df6226b535fa8bd2543",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Last Starfighter (1984).bin
    {"021dbeb7417cac4e5f8e867393e742d6",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Lead (16k).bin
    {"c98ff002205095c8a40f7a537a7e8f01",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Lead (8k).bin
    {"3947eb7305b0c904256cdbc5c5956c0f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lilly Adventure.bin
    {"86128001e69ab049937f265911ce7e8a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lochjaw (1981).bin
    {"71464c54da46adae9447926fdbfc1abe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lock 'n' Chase (1982).bin
    {"b4e2fd27d3180f0f4eb1065afc0d7fc9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // London Blitz (1983).bin
    {"5babe0cad3ec99d76b0aa1d36a695d2f",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Looping (1983).bin
    {"e24d7d879281ffec0641e9c3f52e505a",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lord of the Rings (1983).bin
    {"7c00e7a205d3fda98eb20da7c9c50a55",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lost Luggage (1981).bin
    {"593268a34f1e4d78d94729e1d7ee8367",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Lunokhod.bin
    {"393e41ca8bdd35b52bf6256a968a9b89",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // M.A.D. (Missile Intercept) (1982).bin
    {"cddabfd68363a76cd30bee4e8094c646",  "2K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // MagiCard (1981).bin
    {"ccb5fa954fb76f09caae9a8c66462190",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Malagai (1983).bin
    {"54a1c1255ed45eb8f71414dadb1cf669",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mangia' (1983).bin
    {"402d876ec4a73f9e3133f8f7f7992a1e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  6},    // Man Goes Down.bin
    {"9104ffc48c5eebd2164ec8aeb0927b91",  "DPCP", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mappy.bin
    {"13895ef15610af0d0f89d588f376b3fe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Marauder (1982).bin
    {"b00e8217633e870bf39d948662a52aac",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Marine Wars (1983).bin
    {"e908611d99890733be31733a979c62d8",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mario Bros. (1983).bin
    {"835759ff95c2cdc2324d7c1e7c5fa237",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // M.A.S.H (1983).bin
    {"ae4be3a36b285c1a1dff202157e2155d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Master Builder (1983).bin
    {"3b76242691730b2dd22ec0ceab351bc6",  "E7",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Masters of the Universe (1983).bin
    {"470878b9917ea0348d64b5750af149aa",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Math Gran Prix (1982).bin
    {"f825c538481f9a7a46d1e9bc06200aaf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Maze Craze (1980).bin
    {"35b43b54e83403bb3d71f519739a9549",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // McDonald's - Golden Arches (1983).bin
    {"09a03e0c85e667695bcd6c6394e47e5f",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // MC_ntsc_7_26_3.bin
    {"f7fac15cf54b55c5597718b6742dbec2",  "F4",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Medieval Mayhem.bin
    {"daeb54957875c50198a7e616f9cc8144",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mega Force (1982).bin
    {"318a9d6dda791268df92d72679914ac3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // MegaMania (1982).bin
    {"96e798995af6ed9d8601166d4350f276",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Meltdown (1983).bin
    {"5f791d93ac95bdd8a691a65d665fb436",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Meltdown.bin
    {"712924a2c7b692f6e7b009285c2169a7",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Meteoroids (1982).bin
    {"f1554569321dc933c87981cf5c239c43",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Midnight Magic (1984).bin
    {"3c57748c8286cf9e821ecd064f21aaa9",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Millipede (1984).bin
    {"0e224ea74310da4e7e2103400eb1b4bf",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mind Maze (1984).bin
    {"0e224ea74310da4e7e2103400eb1b4bf",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // MindMaze.bin
    {"fa0570561aa80896f0ead05c46351389",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Miner 2049er (1982).bin
    {"2a1b454a5c3832b0240111e7fd73de8a",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Miner 2049er Vol II (1983).bin
    {"4543b7691914dfd69c3755a5287a95e1",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mines of Minos (1982).bin
    {"df62a658496ac98a3aa4a6ee5719c251",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Miniature Golf (1979).bin
    {"3a2e2d0c6892aa14544083dfb7762782",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Missile Command (1981).bin
    {"4c6afb8a44adf8e28f49164c84144bfe",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mission 3000 A.D. (1983).bin
    {"4181087389a79c7f59611fb51c263137",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Miss Piggy's Wedding (1983).bin
    {"4181087389a79c7f59611fb51c263137",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Miss Piggy's Wedding [!].bin
    {"e13818a5c0cb2f84dd84368070e9f099",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Misterious Thief (1983).bin
    {"7af40c1485ce9f29b1a7b069a2eb04a7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mogul Maniac (1983).bin
    {"6913c90002636c1487538d4004f7cac2",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Monster Cise (1984).bin
    {"6913c90002636c1487538d4004f7cac2",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Monstercise.bin
    {"3347a6dd59049b15a38394aa2dafa585",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Montezuma's Revenge (1984).bin
    {"515046e3061b7b18aa3a551c3ae12673",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Moon Patrol (1983).bin
    {"203abb713c00b0884206dcc656caa48f",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Moonsweeper (1983).bin
    {"de0173ed6be9de6fd049803811e5f1a8",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Motocross Racer (1983).bin
    {"378a62af6e9c12a760795ff4fc939656",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // MotoRodeo (1990).bin
    {"db4eb44bc5d652d9192451383d3249fc",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Mountain King (1983).bin
    {"5678ebaa09ca3b699516dba4671643ed",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mouse Trap (1982).bin
    {"0164f26f6b38a34208cd4a2d0212afc3",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mr. Do! (1983).bin
    {"b7a7e34e304e4b7bc565ec01ba33ea27",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mr. Do!'s Castle (1984).bin
    {"8644352b806985efde499ae6fc7b0fec",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mr. Postman (1983).bin
    {"f0daaa966199ef2b49403e9a29d12c50",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mr. Postman.bin
    {"87e79cd41ce136fd4f72cc6e2c161bee",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ms. Pac-Man (1982).bin
    {"ddf72763f88afa541f6b52f366c90e3a",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Muncher.bin
    {"65b106eba3e45f3dab72ea907f39f8b4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Music Machine (1983).bin
    {"fcbbd0a407d3ff7bf857b8a399280ea1",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Mysterious Thief (1983).bin
    {"36306070f0c90a72461551a7a4f3a209",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Name This Game (1982).bin
    {"392f00fd1a074a3c15bc96b0a57d52a1",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Night Driver (1980).bin
    {"ed0ab909cf7b30aff6fc28c3a4660b8e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Nightmare.bin
    {"27f9e2e1b92af9dc17c6155605c38e49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Nightmare (CCE).bin
    {"b6d52a0cf53ad4216feb04147301f87d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // No Escape! (1982).bin
    {"de7a64108074098ba333cc0c70eef18a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Nuts (Unknown).bin
    {"133a4234512e8c4e9e8c5651469d4a09",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Obelix (1983).bin
    {"e20fc6ba44176e9149ef6d3f99221ac5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Odin.bin
    {"67afbf02142c66861a1d682e4c0ea46e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Odin-joystick.bin
    {"c73ae5ba5a0a3f3ac77f0a9e14770e73",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  1},    // Official Frogger (1983).bin
    {"98f63949e656ff309cefa672146dc1b8",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Off the Wall (1989).bin
    {"b6166f15720fdf192932f1f76df5b65d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Off Your Rocker (1983).bin
    {"c9c25fc536de9a7cdc5b9a916c459110",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Oink! (1982).bin
    {"cca33ae30a58f39e3fc5d80f94dc0362",  "2K",   CTR_DRIVING,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Okie dokie (pd).bin
    {"9947f1ebabb56fd075a96c6d37351efa",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Omega Race (1983).bin
    {"bc593f2284c67b7d8716d110f541953f",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Omicron 16k.bin
    {"2148917316ca5ce1672f6c49c0f89d0b",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Omicron 2k.bin
    {"52385334ac9e9b713e13ffa4cc5cb940",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Open, Sesame! (1983).bin
    {"fa1b060fd8e0bca0c2a097dcffce93d3",  "F8",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Oscar's Trash Race (1983).bin
    {"55949cb7884f9db0f8dfcf8707c7e5cb",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Othello (1981).bin
    {"890c13590e0d8d5d6149737d930e4d95",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Outlaw (1978).bin
    {"f97dee1aa2629911f30f225ca31789d4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Out of Control (1983).bin
    {"91f0a708eeb93c133e9672ad2c8e0429",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Oystron (V2.9) (PD).bin
    {"e959b5a2c882ccaacb43c32790957c2d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // p2p_ntsc_final.bin
    {"6e372f076fb9586aff416144f5cfe1cb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Pac-Man (1982).bin
    {"b36040a2f9ecafa73d835d804a572dbf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pac Man (1983) (Digitel).bin
    {"880e45b99c785e9910450b88e69c49eb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pac-Man 4k.bin
    {"6e88da2b704916eb04a998fed9e23a3e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pac-Man_4k (debro).bin
    {"98d41ef327c58812ecc75bf1611ddced",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pac-Man 8k.bin
    {"bf9a2045952d40e08711aa232a92eb78",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Panky the Panda.bin
    {"012b8e6ef3b5fd5aabc94075c527709d",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Party Mix (1983).bin
    {"e40a818dac4dd851f3b4aafbe2f1e0c1",  "4K",   CTR_KEYBOARD,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Peek-A-Boo (1984).bin
    {"04014d563b094e79ac8974366f616308",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pengo (1984).bin
    {"212d0b200ed8b45d8795ad899734d7d7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pepsi Invaders (1983).bin
    {"09388bf390cd9a86dc0849697b96c7dc",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pete Rose Baseball (1988).bin
    {"e9034b41741dcee64ab6605aba9de455",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Phanton Tank (Digivision).bin
    {"62f74a2736841191135514422b20382d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pharaoh's Curse.bin
    {"7dcbfd2acc013e817f011309c7504daa",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Phaser Patrol (1982).bin
    {"7e52a95074a66640fcfde124fffd491a",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Phoenix (1982).bin
    {"1d4e0a034ad1275bc4d75165ae236105",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pick Up (1983).bin
    {"17c0a63f9a680e7a61beba81692d9297",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Picnic (1982).bin
    {"d3423d7600879174c038f53e5ebbf9d3",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Piece o' Cake (1982).bin
    {"8e4fa8c6ad8d8dce0db8c991c166cdaa",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pigs in Space (1983).bin
    {"9b54c77b530582d013f0fa4334d785b7",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pirate_se.bin
    {"3e90cf23106f2e08b2781e41299de556",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Pitfall! (1982).bin
    {"f73d2d0eff548e8fc66996f27acf2b4b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Pitfall (1983) (CCE) (C-813).bin
    {"6d842c96d5a01967be9680080dd5be54",  "DPC",  CTR_LJOY,      SPEC_PITFALL2,  MODE_NO,   ANA1_0,  205,   0,  2},    // Pitfall II - Lost Caverns (1983).bin
    {"d9fbf1113114fb3a3c97550a0689f10f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pizza Chef (1983).bin
    {"9efb4e1a15a6cdd286e4bcd7cd94b7b8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Planet of the Apes (1983).bin
    {"043f165f384fbea3ea89393597951512",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Planet Patrol (1982).bin
    {"da4e3396aa2db3bd667f83a1cb9e4a36",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Plaque Attack (1983).bin
    {"8bbfd951c89cc09c148bfabdefa08bec",  "UA",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pleiades (1983).bin
    {"8bbfd951c89cc09c148bfabdefa08bec",  "UA",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pleiades.bin
    {"8c136e97c0a4af66da4a249561ed17db",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Poker_squares.bin
    {"44f71e70b89dcc7cf39dfd622cfb9a27",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Polaris (1983).bin
    {"44f71e70b89dcc7cf39dfd622cfb9a27",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Polaris_ntsc.bin
    {"5f39353f7c6925779b0169a87ff86f1e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pole Position (1983).bin
    {"ee28424af389a7f3672182009472500c",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Polo (1978).bin
    {"a83b070b485cf1fb4d5a48da153fdf1a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pompeii (1983).bin
    {"4799a40b6e889370b7ee55c17ba65141",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pooyan (1983).bin
    {"c7f13ef38f61ee2367ada94fdcc6d206",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Popeye (1983).bin
    {"f93d7fee92717e161e6763a88a293ffa",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Porky's (1983).bin
    {"97d079315c09796ff6d95a06e4b70171",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pressure Cooker (1983).bin
    {"de1a636d098349be11bbc2d090f4e9cf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Pressure gauge.bin
    {"ef3a4f64b6494ba770862768caf04b86",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Private Eye (1983).bin
    {"ac53b83e1b57a601eeae9d3ce1b4a458",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // qb (2.15) (ntsc).bin
    {"34e37eaffc0d34e05e40ed883f848b40",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // qb (2.15) (stella).bin
    {"484b0076816a104875e00467d431c2d2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  10},   // Q-bert (1983).bin
    {"dcb406b54f1b69017227cfbe4052005e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  10},   // Q-bert Arcade Hack.bin
    {"517592e6e0c71731019c0cebc2ce044f",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Q-bert's Qubes (1984).bin
    {"024365007a87f213cbe8ef5f2e8e1333",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Quadrun (1983).bin
    {"a0675883f9b09a3595ddd66a6f5d3498",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Quest for Quintana Roo (1984).bin
    {"7eba20c2291a982214cc7cbe8d0b47cd",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Quick Step! (1983).bin
    {"fb4ca865abc02d66e39651bd9ade140a",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Rabbit Transit (1983).bin
    {"4df9d7352a56a458abb7961bf10aba4e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Racing Car (Unknown).bin
    {"a20d931a8fddcd6f6116ed21ff5c4832",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Racquetball (1981).bin
    {"247fa1a29ad90e64069ee13d96fea6d6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Radar (1983).bin
    {"baf4ce885aa281fd31711da9b9795485",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Radar Lock (1989).bin
    {"92a1a605b7ad56d863a56373a866761b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Raft Rider (1982).bin
    {"f724d3dd2471ed4cf5f191dbb724b69f",  "F8",   CTR_RAIDERS,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Raiders of the Lost Ark (1982).bin
    {"1cafa9f3f9a2fce4af6e4b85a2bbd254",  "F8",   CTR_RAIDERS,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Raiders of the Lost Ark (pal).bin
    {"cb96b0cf90ab7777a2f6f05e8ad3f694",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Rainbow Invaders NTSC.bin
    {"7096a198531d3f16a99d518ac0d7519a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ram It (1982).bin
    {"5e1b4629426f4992cf3b2905a696e1a7",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Rampage! (1989).bin
    {"97cd63c483fe3c68b7ce939ab8f7a318",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RC021.bin
    {"9f8fad4badcd7be61bbd2bcaeef3c58f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Reactor (1982).bin
    {"eb634650c3912132092b7aee540bbce3",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RealSports Baseball (1982).bin
    {"3177cc5c04c1a4080a927dfa4099482b",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RealSports Boxing (1987).bin
    {"7ad257833190bc60277c1ca475057051",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RealSports Football (1982).bin
    {"08f853e8e01e711919e734d85349220d",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RealSports Soccer (1983).bin
    {"dac5c0fe74531f077c105b396874a9f1",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RealSports Tennis (1983).bin
    {"aed0b7bd64cc384f85fdea33e28daf3b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RealSports Volleyball (1982).bin
    {"60a61da9b2f43dd7e13a5093ec41a53d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Rescue Terra I (1982).bin
    {"42249ec8043a9a0203dde0b5bb46d8c4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Resgate Espacial.bin
    {"0b01909ba84512fdaf224d3c3fd0cf8d",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RevengeOfTheApes.bin
    {"4f64d6d0694d9b7a1ed7b0cb0b83e759",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Revenge of the Beefsteak Tomatoes (1982).bin
    {"a995b6cbdb1f0433abc74050808590e6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Riddle of the Sphinx (1982).bin
    {"31512cdfadfd82bfb6f196e3b0fd83cd",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // River Patrol (1984).bin
    {"393948436d1f4cc3192410bb918f9724",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -3},    // River Raid (1982).bin
    {"ab56f1b2542a05bebc4fbccfc4803a38",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // River Raid II (1988).bin
    {"2bd00beefdb424fa39931a75e890695d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Road Runner (1989).bin
    {"72a46e0c21f825518b7261c267ab886e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Robin Hood (1983).bin
    {"4f618c2429138e0280969193ed6c107e",  "FE",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  6},    // Robot Tank (1983).bin
    {"ec44dcf2ddb4319962fc43b725a902e8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -1},    // Robot City (RC8).bin
    {"d97fd5e6e1daacd909559a71f189f14b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Rocky & Bullwinkle (1983).bin
    {"65bd29e8ab1b847309775b0de6b2e4fe",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Roc 'n Rope (1984).bin
    {"67931b0d37dc99af250dd06f1c095e8d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Room of Doom (1982).bin
    {"1f2ae0c70a04c980c838c2cdc412cf45",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Rubik's Cube (1984).bin
    {"40b1832177c63ebf81e6c5b61aaffd3a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // RubiksCube3D.bin
    {"f3cd0f886201d1376f3abab2df53b1b9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Rush Hour (1983).bin
    {"a4ecb54f877cd94515527b11e698608c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Saboteur (1983).bin
    {"1ec57bbd27bdbd08b60c391c4895c1cf",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Saboteur.bin
    {"4d502d6fb5b992ee0591569144128f99",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Save Mary! (1989).bin
    {"4d502d6fb5b992ee0591569144128f99",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // SaveMary_NTSC.bin
    {"ed1a784875538c7871d035b7a98c2433",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Save Our Ship.bin
    {"49571b26f46620a85f93448359324c28",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Save Our Ship (Unknown).bin
    {"e377c3af4f54a51b85efe37d4b7029e6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Save the Whales (1983).bin
    {"a3fe5e6a744cd34a3c1466874a3e9d5f",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Scramble 0106.bin
    {"137373599e9b7bf2cf162a102eb5927f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // scsi200_NTSC.bin
    {"19e761e53e5ec8e9f2fceea62715ca06",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Scuba Diver (1983).bin
    {"1bc2427ac9b032a52fe527c7b26ce22c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sea Battle (1983).bin
    {"624e0a77f9ec67d628211aaf24d8aea6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sea Hawk (1983).bin
    {"5dccf215fdb9bbf5d4a6d0139e5e8bcb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sea Hunt (1987).bin
    {"a8c48b4e0bf35fe97cc84fdd2c507f78",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Seamonster (1982).bin
    {"dde55d9868911407fe8b3fefef396f00",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // seantsc.bin
    {"240bfbac5163af4df5ae713985386f92",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Seaquest (1983).bin
    {"3034532daf80997f752aee680d2e7fc3",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Seaweed Assault.bin
    {"dde55d9868911407fe8b3fefef396f00",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sea Wolf.bin
    {"605fd59bfef88901c8c4794193a4cbad",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Secret Agent (1983).bin
    {"fc24a94d4371c69bc58f5245ada43c44",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Secret Quest (1989).bin
    {"8da51e0c4b6b46f7619425119c7d018e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Sentinel (1990).bin
    {"54f7efa6428f14b9f610ad0ca757e26c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Shark Attack (1982).bin
    {"b5a1a189601a785bdb2f02a424080412",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Shootin' Gallery (1982).bin
    {"15c11ab6e4502b2010b18366133fc322",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Shooting Arcade (1989).bin
    {"25b6dc012cdba63704ea9535c6987beb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Shuttle Orbiter (1983).bin
    {"1e85f8bccb4b866d4daa9fcf89306474",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sinistar (1984).bin
    {"4c8970f6c294a0a54c9c45e5e8445f93",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sir Lancelot (1983).bin
    {"f847fb8dba6c6d66d13724dbe5d95c4d",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Skate Boardin' (1987).bin
    {"39c78d682516d79130b379fa9deb8d1c",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Skeet Shoot (1981).bin
    {"eafe8b40313a65792e88ff9f2fe2655c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // SkelPlus.bin
    {"8654d7f0fb351960016e06646f639b02",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ski Hunt.bin
    {"b76fbadc8ffb1f83e2ca08b6fb4d6c9f",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Skiing (1980).bin
    {"46c021a3e9e2fd00919ca3dd1a6b76d8",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Sky Diver (1979).bin
    {"2a0ba55e56e7a596146fa729acf0e109",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // Sky Jinks (1982).bin
    {"4c9307de724c36fd487af6c99ca078f2",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // Sky Patrol (1982).bin
    {"3b91c347d8e6427edbe942a7a405290d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sky Skipper (1983).bin
    {"f90b5da189f24d7e1a2117d8c8abc952",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Slot Machine (1979).bin
    {"705fe719179e65b0af328644f3a04900",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Slot Machine (1979).bin
    {"81254ebce88fa46c4ff5a2f4d2bad538",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Slot Machine (1979).bin
    {"fc6052438f339aea373bbc999433388a",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Slot Machine (pal).bin
    {"aed82052f7589df05a3f417bb4e45f0c",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Slot Racers (1978).bin
    {"3d1e83afdb4265fa2fb84819c9cfd39c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Smurf - Rescue in Gargamel's Castle (1982).bin
    {"a204cd4fb1944c86e800120706512a64",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Smurfs Save the Day (1983).bin
    {"9c6faa4ff7f2ae549bbcb14f582b70e4",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sneak 'n Peek (1982).bin
    {"57939b326df86b74ca6404f64f89fce9",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Snoopy and the Red Baron (1983).bin
    {"75ee371ccfc4f43e7d9b8f24e1266b55",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Snow White (1982).bin
    {"75ee371ccfc4f43e7d9b8f24e1266b55",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Snow White.bin
    {"3f6dbf448f25e2bd06dea44248eb122d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Soccer (1989).bin
    {"947317a89af38a49c4864d6bdd6a91fb",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Solar Fox (1983).bin
    {"e72eb8d4410152bdcb69e7fba327b420",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // Solaris (1986).bin
    {"b5be87b87fd38c61b1628e8e2d469cb5",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Solar Plexus.bin
    {"97842fe847e8eb71263d6f92f7e122bd",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Solar Storm (1983).bin
    {"d2c4f8a4a98a905a9deef3ba7380ed64",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sorcerer (1983).bin
    {"5f7ae9a7f8d79a3b37e8fc841f65643a",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sorcerer's Apprentice (1983).bin
    {"17badbb3f54d1fc01ee68726882f26a6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Attack (1982).bin
    {"1d566002bbc51e5eee73de4c595fd545",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // SpaceBattleFinal4N.bin
    {"df6a28a89600affe36d94394ef597214",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Cavern (1981).bin
    {"ec5c861b487a5075876ab01155e74c6c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spacechase (1981) .bin
    {"44ca1a88274ff55787ed1763296b8456",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // SpaceGame-Final.bin
    {"94255d5c05601723a58df61726bc2615",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // SpaceGame 2K.bin
    {"72ffbef6504b75e69ee1045af9075f66",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Space Invaders (1980).bin
    {"e074af84dcd5bd21fb48ca7f36845e61",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Space Invaders Deluxe.a26    
    {"6f2aaffaaf53d23a28bf6677b86ac0e3",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Jockey (1982).bin
    {"45040679d72b101189c298a864a5b5ba",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // SpaceMaster X-7 (1983).bin
    {"cb3a9b32a01746621f5c268db48833b2",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Raid.bin
    {"3dfb7c1803f937fadc652a3e95ff7dc6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Robot (Dimax - Sinmax).bin
    {"fe395b292e802ea16b3b5782b21ee686",  "DPCP", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Rocks.bin
    {"5894c9c0c1e7e29f3ab86c6d3f673361",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Shuttle (1983).bin
    {"6c9a32ad83bcfde3774536e52be1cce7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Space Treat.bin
    {"9e135f5dce61e3435314f5cddb33752f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Space Treat Deluxe.bin
    {"be3f0e827e2f748819dac2a22d6ac823",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space Tunnel (1982).bin
    {"a7ef44ccb5b9000caf02df3e6da71a92",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Space War (1978).bin
    {"25c97848ae6499e569b832b686a84bb2",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sp+.bin
    {"8454ed9787c9d8211748ccddb673e920",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spiderdroid (1987).bin
    {"24d018c4a6de7e5bd19a36f2b879b335",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -1},    // Spider Fighter (1982).bin
    {"199eb0b8dce1408f3f7d46411b715ca9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spider-Man (1982).bin
    {"21299c8c3ac1d54f8289d88702a738fd",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spider Maze (1982).bin
    {"a4e885726af9d97b12bb5a36792eab63",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spike's Peak (1983).bin
    {"d3171407c3a8bb401a3a62eb578f48fb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spinning Fireball (1983).bin
    {"cef2287d5fd80216b2200fb2ef1adfa8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spitfire Attack (1983).bin
    {"6216bef66edceb8a24841e6065bf233e",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Splatform 2600 (v1.01).bin
    {"4cd796b5911ed3f1062e805a3df33d98",  "3F",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Springer (1982).bin
    {"5a8afe5422abbfb0a342fb15afd7415f",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sprint Master (1988).bin
    {"3105967f7222cc36a5ac6e5f6e89a0b4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Spy Hunter (1984).bin
    {"ba257438f8a78862a9e014d831143690",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Squeeze Box (1982).bin
    {"68878250e106eb6c7754bc2519d780a0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Squirrel (1983).bin
    {"aa8c75d6f99548309949916ad6cf33bc",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Squish-Em.bin
    {"34c808ad6577dbfa46169b73171585a3",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Squoosh (1983).bin
    {"21a96301bb0df27fde2e7eefa49e0397",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sssnake (1982).bin
    {"21d7334e406c2407e69dbddd7cec3583",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Stampede (1981).bin
    {"d9c9cece2e769c7985494b1403a25721",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Castle.bin
    {"f526d0c519f5001adb1fc7948bfbb3ce",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Fox (1983).bin
    {"0c48e820301251fbb6bcdc89bd3555d9",  "F8SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Stargate (1984).bin
    {"a3c1c70024d7aabb41381adbfb6d3b25",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, 10},    // Stargunner (1982).bin
    {"d69559f9c9dc6ef528d841bf9d91b275",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // StarMaster (1982).bin
    {"cbd981a23c592fb9ab979223bb368cd5",  "F8",   CTR_STARRAID,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Raiders (1982).bin
    {"c1a83f44137ea914b495fc6ac036c493",  "F8",   CTR_STARRAID,  SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Raiders (PAL 1982).bin
    {"e363e467f605537f3777ad33e74e113a",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Ship (1977).bin
    {"7b938c7ddf18e8362949b62c7eaa660a",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Ship (1977).bin
    {"79e5338dbfa6b64008bb0d72a3179d3c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Strike (1983).bin
    {"03c3f7ba4585e349dd12bfa7b34b7729",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Trek - Strategic Operations Simulator (1983).bin
    {"813985a940aa739cc28df19e0edd4722",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Voyager (1982).bin
    {"5336f86f6b982cc925532f2e80aa1e17",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Wars - Death Star Battle (1983).bin
    {"d44d90e7c389165f5034b5844077777f",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Wars - Ewok Adventure (1983).bin
    {"c9f6e521a49a2d15dac56b6ddb3fb4c7",  "4K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Wars - Jedi Arena (1983).bin
    {"6339d28c9a7f92054e70029eb0375837",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Wars - The Arcade Game (1984).bin
    {"3c8e57a246742fa5d59e517134c0b4e6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Star Wars - The Empire Strikes Back (1982).bin
    {"541cac55ebcf7891d9d51c415922303f",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // StayFrosty2.bin
    {"c5bab953ac13dbb2cba03cd0684fb125",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // StayFrosty.bin
    {"656dc247db2871766dffd978c71da80c",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA2_5,  205,   0,  4},    // Steeplechase (1980).bin
    {"0b8d3002d8f744a753ba434a4d39249a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Stellar Track (1980).bin
    {"23fad5a125bcd4463701c8ad8a0043a9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -2},    // Stone Age (1983).bin
    {"9333172e3c4992ecf548d3ac1f2553eb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -2},    // Strategy X (1983).bin
    {"807a8ff6216b00d52aba2dfea5d8d860",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // StratOGemsDeluxe.bin
    {"e10d2c785aadb42c06390fae0d92f282",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Strawberry Shortcake (1983).bin
    {"396f7bc90ab4fa4975f8c74abe4e81f0",  "2K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Street Racer (1977).bin
    {"7b3cf0256e1fa0fdc538caf3d5d86337",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -2},    // Stronghold (1983).bin
    {"c3bbc673acf2701b5275e85d9372facf",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Stunt Cycle (1980).bin
    {"f3f5f72bfdd67f3d0e45d097e11b8091",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Submarine Commander (1982).bin
    {"5af9cd346266a1f2515e1fbc86f5186a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Sub-Scan (1982).bin
    {"93c52141d3c4e1b5574d072f1afde6cd",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Subterranea (1983).bin
    {"cff578e5c60de8caecbee7f2c9bbb57b",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Suicide Adventure.bin
    {"e4c666ca0c36928b95b13d33474dbb44",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Suicide Mission (1982).bin
    {"45027dde2be5bdd0cab522b80632717d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Summer Games (1987).bin
    {"7adbcf78399b19596671edbffc3d34aa",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Super Baseball (1988).bin
    {"8885d0ce11c5b40c3a8a8d9ed28cefef",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Super Breakout (1982).bin
    {"9d37a1be4a6e898026414b8fee2fc826",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Super Challenge Baseball (1982).bin
    {"e275cbe7d4e11e62c3bfcfb38fca3d49",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Super Challenge Football (1982).bin
    {"4565c1a7abce773e53c75b35414adefd",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Supercharger BIOS (1982).bin
    {"c29f8db680990cb45ef7fef6ab57a2c2",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Super Cobra (1982).bin
    {"841057f83ce3731e6bbfda1707cbca58",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // SuperCobra.bin
    {"724613effaf7743cbcd695fab469c2a8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Super Ferrari (Quelle).bin
    {"09abfe9a312ce7c9f661582fdf12eab6",  "F6SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Super Football (1988).bin
    {"5de8803a59c36725888346fdc6e7429d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  3},    // Superman (1979).bin
    {"aec9b885d0e8b24e871925630884095c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Surf's Up (1983).bin
    {"4d7517ae69f95cfbc053be01312b7dba",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Surround (1977).bin
    {"31d08cb465965f80d3541a57ec82c625",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Surround (4k).bin   
    {"c370c3268ad95b3266d6e36ff23d1f0c",  "2K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Surround (pal).bin  
    {"045035f995272eb2deb8820111745a07",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Survival Island (1983).bin
    {"85e564dae5687e431955056fbda10978",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Survival Run (1983).bin
    {"59e53894b3899ee164c91cfa7842da66",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Survival Run (Data Age).bin
    {"e51c23389e43ab328ccfb05be7d451da",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Sweat! - The Decathlon (1983).bin
    {"278f14887d601b5e5b620f1870bc09f6",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  4},    // Swoops! (v0.96).bin
    {"87662815bc4f3c3c86071dc994e3f30e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Swordfight (1983).bin
    {"528400fad9a77fd5ad7fc5fdc2b7d69d",  "AR",   CTR_LJOY,      SPEC_DISTADDR,  MODE_NO,   ANA1_0,  205,   0,  0},    // Sword of Saros (1983).bin
    {"5aea9974b975a6a844e6df10d2b861c4",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // SwordQuest - EarthWorld (1982).bin
    {"f9d51a4e5f8b48f68770c89ffd495ed1",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // SwordQuest - FireWorld (1982).bin
    {"bc5389839857612cfabeb810ba7effdc",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // SwordQuest - WaterWorld (1983).bin
    {"2c2aea31b01c6126c1a43e10cacbfd58",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Synth Cart.bin
    {"d45ebf130ed9070ea8ebd56176e48a38",  "4K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tac-Scan (1982).bin
    {"c77d3b47f2293e69419b92522c6f6647",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tank Brigade (1983).bin
    {"fa6fe97a10efb9e74c0b5a816e6e1958",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tanks But No Tanks (1983).bin
    {"de3d0e37729d85afcb25a8d052a6e236",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tapeworm (1982).bin
    {"c0d2434348de72fa6edcc6d8e40f28d7",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -2},    // Tapper (1984).bin
    {"2d6741cda3000230f6bbdd5e31941c01",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Targ (1983).bin
    {"2d6741cda3000230f6bbdd5e31941c01",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Targ (CBS).bin
    {"6cab04277e7cd552a3e40b3c0e6e1e3d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Targ (Universal Chaos Beta).bin
    {"0c35806ff0019a270a7acae68de89d28",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Task Force (1987).bin
    {"a1ead9c181d67859aa93c44e40f1709c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tax Avoiders (1982).bin
    {"7574480ae2ab0d282c887e9015fdb54c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Taz (1983).bin
    {"3d7aad37c55692814211c8b590a0334c",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Telepathy (1983).bin
    {"c830f6ae7ee58bcc2a6712fb33e92d55",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tempest (1984).bin
    {"42cdd6a9e42a3639e190722b8ea3fc51",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -1},    // Tennis (1981).bin
    {"5eeb81292992e057b290a5cd196f155d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Texas Chainsaw Massacre (1983).bin
    {"5fb71cc60e293fe10a5023f11c734e55",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // This Planet Sucks.bin
    {"e63a87c231ee9a506f9599aa4ef7dfb9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Threshold (1982).bin
    {"de7bca4e569ad9d3fd08ff1395e53d2d",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Thrust (V1.22) (Thomas Jentzsch) (Booster Grip).bin
    {"7ded20e88b17c8149b4de0d55c795d37",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Thrust v1.26 (PD).bin
    {"cf507910d6e74568a68ac949537bccf9",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  2},    // Thunderground (1983).bin
    {"c032c2bd7017fdfbba9a105ec50f800e",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Thwocker (1984).bin
    {"fc2104dd2dadf9a6176c1c1c8f87ced9",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Time Pilot (1983).bin
    {"b879e13fd99382e09bcaf1d87ad84add",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Time Warp (1982).bin
    {"953c45c3dd128a4bd5b78db956c455bb",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // TitanAxe.bin
    {"12123b534bdee79ed7563b9ad74f1cbd",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Title Match Pro Wrestling (1987).bin
    {"8bc0d2052b4f259e7a50a7c771b45241",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tomarc the Barbarian (1983).bin
    {"3ac6c50a8e62d4ce71595134cbd8035e",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tomcat - F-14 Fighter (1988).bin
    {"fa2be8125c3c60ab83e1c0fe56922fcb",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tooth Protectors (1983).bin
    {"01abcc1d2d3cba87a3aa0eb97a9d7b9c",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Topy (unknown).bin
    {"0aa208060d7c140f20571e3341f5a3f8",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0, -1},    // Towering Inferno (1982).bin
    {"f39e4bc99845edd8621b0f3c7b8c4fd9",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // ToyshopTrouble.bin
    {"6ae4dc6d7351dacd1012749ca82f9a56",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Track and Field (1984).bin
    {"24df052902aa9de21c2b2525eb84a255",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Trick Shot (1982).bin
    {"fb27afe896e7c928089307b32e5642ee",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // TRON - Deadly Discs (1982).bin
    {"b2737034f974535f5c0c6431ab8caf73",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tunnel Runner (1983).bin
    {"7a5463545dfb2dcfdafa6074b2f2c15e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  5},    // Turmoil (1982).bin
    {"085322bae40d904f53bdcc56df0593fc",  "E0",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Tutankham (1983).bin
    {"137373599e9b7bf2cf162a102eb5927f",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Ultra SCSICide.bin
    {"81a010abdba1a640f7adf7f84e13d307",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Universal Chaos (1989).bin
    {"ee681f566aad6c07c61bbbfc66d74a27",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // UnknownActivision1_NTSC.bin
    {"700a786471c8a91ec09e2f8e47f14a04",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // UnknownActivision2_NTSC.bin
    {"73e66e82ac22b305eb4d9578e866236e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Unknown Datatech Game.bin
    {"5f950a2d1eb331a1276819520705df94",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Unknown Universal Game (1983).bin
    {"a499d720e7ee35c62424de882a3351b6",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Up 'n Down (1984).bin
    {"c6556e082aac04260596b4045bc122de",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  1},    // Vanguard (1982).bin
    {"787ebc2609a31eb5c57c4a18837d1aee",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Vault Assault.bin
    {"bf7389cbfee666b33b8a88cc6c830edd",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Vault (TE).bin
    {"d08fccfbebaa531c4a4fa7359393a0a9",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Venetian Blinds Demo (1982).bin
    {"3e899eba0ca8cd2972da1ae5479b4f0d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Venture (1982).bin
    {"0956285e24a18efa10c68a33846ca84d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Viagem Espacial.bin
    {"539d26b6e9df0da8e7465f0f5ad863b7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Checkers (1980).bin
    {"f0b7db930ca0e548c41a97160b9f6275",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Chess (1979).bin
    {"ed1492d4cafd7ebf064f0c933249f5b0",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Cube (CCE).bin
    {"4191b671bcd8237fc8e297b4947f2990",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Jogger (1983).bin
    {"4209e9dcdf05614e290167a1c033cfd2",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Life (1981).bin
    {"60e0ea3cbe0913d39803477945e9e5ec",  "2K",   CTR_PADDLE1,   SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Olympics (1977).bin
    {"107cc025334211e6d29da0b6be46aec7",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Pinball (1981).bin
    {"ee659ae50e9df886ac4f8d7ad10d046a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Video Reflex (1983).bin
    {"6c128bc950fcbdbcaf0d99935da70156",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Volleyball (1983).bin
    {"1f21666b8f78b65051b7a609f1d48608",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Vulture Attack (1982).bin
    {"6041f400b45511aa3a69fab4b8fc8f41",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Wabbit (1982).bin
    {"d175258b2973b917a05b46df4e1cf15d",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Walker.bin
    {"d3456b4cf1bd1a7b8fb907af1a80ee15",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Wall Ball (1983).bin
    {"c16fbfdbfdf5590cc8179e4b0f5f5aeb",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Wall-Defender (1983).bin
    {"5da448a2e1a785d56bf4f04709678156",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Wall Jump Ninja.bin
    {"cbe5a166550a8129a5e6d374901dffad",  "4K",   CTR_PADDLE0,   SPEC_NONE,      MODE_NO,   ANA0_8,  205,   0,  0},    // Warlords (1981).bin
    {"679e910b27406c6a2072f9569ae35fc8",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Warplock (1982).bin
    {"7e7c4c59d55494e66eef5e04ec1c6157",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Warring Worms.bin
    {"827a22b9dffee24e93ed0df09ff8414a",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  9},    // Wings_10-10-83.bin
    {"8e48ea6ea53709b98e6f4bd8aa018908",  "FASC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  9},    // Wings (1983).bin
    {"83fafd7bd12e3335166c6314b3bde528",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Winter Games (1987).bin
    {"7b24bfe1b61864e758ada1fe9adaa098",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Wizard (1980).bin
    {"7e8aa18bc9502eb57daaf5e7c1e94da7",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Wizard of Wor (1982).bin
    {"663ef22eb399504d5204c543b8a86bcd",  "4K",   CTR_RJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Wizard of Wor (alternate)  
    {"ec3beb6d8b5689e867bafb5d5f507491",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Word Zapper (1982).bin
    {"e62e60a3e6cb5563f72982fcd83de25a",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // World End (unknown).bin
    {"87f020daa98d0132e98e43db7d8fea7e",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Worm War I (1982).bin
    {"eaf744185d5e8def899950ba7c6e7bb5",  "F6",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Xenophobe (1990).bin
    {"c6688781f4ab844852f4e3352772289b",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Xevious (1983).bin
    {"5961d259115e99c30b64fe7058256bcf",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // X-Man (1983).bin
    {"c5930d0e8cdae3e037349bfa08e871be",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Yars' Revenge (1982).bin
    {"eea0da9b987d661264cce69a7c13c3bd",  "F8",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Zaxxon (1982).bin
    {"a336beac1f0a835614200ecd9c41fd70",  "2K",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Zoo Keeper Sounds (1984).bin
    // Some Work in Progress Carts (these MD5s will change when released BIN is available)
//    {"ede752f2a5bfaa4827f741962fb2c608",  "F4",   CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  205,   0,  0},    // Wizard of Wor Arcade 20190812_demo_NTSC.bin
    {"7412f6788087d7e912c33ba03b36dd1b",  "F4SC", CTR_LJOY,      SPEC_NONE,      MODE_NO,   ANA1_0,  200,   0,  4},    // Venture Reloaded (RC3).bin
    {"258f8f1a6d9af8fc1980b22361738678",  "4K",   CTR_LJOY,      SPEC_NONE,      MODE_FF,   ANA1_0,  205,   0,  0},    // Shadow Reflex (Beta 10-26-2020).bin
    
// fcbdf405f0fc2027b0ea45bb5af94c1a  3-D Ghost Attack (Prototype).bin
// b2ca848aceeaae0a85c9ce12ff88ce89  3-D Havoc (Prototype).bin
// 0db4f4150fecf77e4ce72ca4d04c052f  3-D Tic-Tac-Toe.bin
// 4f32b24869d8c1310fecf039c6424db6  3-D Zapper (Prototype).bin
// 17ee23e5da931be82f733917adcb6386  Acid Drop (PAL).bin
// a1bcbe0bfe6570da2661fc4de2f74e8a  Actionauts.bin
// ac7c2260378975614192ca2bc3d20e0b  Activision Decathlon, The.bin
// 525f2dfc8b21b0186cff2568e0509bfc  Activision Decathlon, The [fixed].bin
// 157bddb7192754a45372be196797f284  Adventure.bin
// ca4f8c5b4d6fb9d608bb96bc7ebd26c7  Adventures of TRON.bin
// 4d77f291dca1518d7d8e47838695f54b  Airlock.bin
// a9cb638cd2cb2e8e0643d7a67db4281c  Air Raiders.bin
// 35be55426c1fec32dfb503b4f0651572  Air Raid (PAL).bin
// 16cb43492987d2f32b423817cdaaf7c4  Air-Sea Battle - Target Fun.bin
// f1a0a23e6464d954e3a9579c4ccd01c8  Alien.bin
// e1a51690792838c5c687da80cd764d78  Alligator People (Prototype).bin
// 9e01f7f95cb8596765e03b9a36e8e33c  Alpha Beam with Ernie.bin
// acb7750b4d0c4bd34969802a7deb2990  Amidar.bin
// 0866e22f6f56f92ea1a14c8d8d01d29c  AndroMan on the Moon (Prototype).bin
// 038e1e79c3d4410defde4bfe0b99cc32  Aquaventure (Prototype).bin
// 2434102f30eeb47792cf0825e368229b  Arkyology (Prototype).bin
// a7b584937911d60c120677fe0d47f36f  Armor Ambush.bin
// c77c35a6fc3c0f12bf9e8bae48cba54b  Artillery Duel.bin
// 18f299edb5ba709a64c80c8c9cec24f2  Asteroid Fire (PAL).bin
// dd7884b4f93cab423ac471aa1935e3df  Asteroids.bin
// ccbd36746ed4525821a8083b0d6d2c2c  Asteroids [no copyright].bin
// 75169c08b56e4e6c36681e599c4d8cc5  Astroblast.bin
// 170e7589a48739cfb9cc782cbb0fe25a  Astroblast [fixed].bin
// 317a4cdbab090dcc996833d07cb40165  Astrowar (PAL).bin
// 3f540a30fdee0b20aed7288e4a5ea528  Atari Video Cube.bin
// 9ad36e699ef6f45d9eb6c4cf90475c9f  Atlantis.bin
// 2cc3049b7feb8e92f1870f1972629757  Atom Smasher (Prototype).bin
// 5b124850de9eea66781a50b2e9837000  Bachelor Party.bin
// 8556b42aa05f94bc29ff39c39b11bff4  Backgammon.bin
// 00ce0bdd43aed84a983bef38fe7f5ee3  Bank Heist.bin
// f8240e62d8c0a64a61e19388414e3104  Barnstorming.bin
// d6dc9b4508da407e2437bfa4de53d1b2  Base Attack (PAL).bin
// 819aeeb9a2e11deb54e6de334f843894  Basic Math - Math.bin
// 9f48eeb47836cf145a15771775f0767a  BASIC Programming.bin
// ab4ac994865fb16ebb85738316309457  Basketball.bin
// 41f252a66c6301f1e8ab3612c19bc5d4  Battlezone.bin
// 79ab4123a83dc11d468fb2108ea09e2e  Beamrider.bin
// d0b9df57bfea66378c0418ec68cfe37f  Beany Bopper.bin
// e30f3a37032da52d7815b5a409f6d4b4  Bear Game Demo.bin
// 59e96de9628e8373d1c685f5e57dcf10  Beat 'Em & Eat 'Em.bin
// ee6665683ebdb539e89ba620981cb0f6  Berenstain Bears.bin
// b8ed78afdb1e6cfe44ef6e3428789d5f  Bermuda Triangle.bin
// 136f75c4dd02c29283752b7e5799f978  Berzerk.bin
// 1802cc46b879b229272501998c5de04f  Big Bird's Egg Catch.bin
// f0541d2f7cda5ec7bab6d62b6128b823  Bionic Breakthrough (Prototype).bin
// a6239810564638de7e4c54e66b3014e4  Birthday Mania.bin
// 0a981c03204ac2b278ba392674682560  Blackjack - Black Jack.bin
// 33d68c3cd74e5bc4cf0df3716c5848bc  Blueprint.bin
// 968efc79d500dce52a906870a97358ab  BMX Air Master.bin
// 2823364702595feea24a3fbee138a243  Bobby Is Going Home (PAL).bin
// a5855d73d304d83ef07dde03e379619f  Boggle (Prototype).bin
// 14c2548712099c220964d7f044c59fd9  Boing!.bin
// 594dbc80b93fa5804e0f1368c037331d  Bouncin' Baby Bunnies (Prototype).bin
// c9b7afad3bfd922e006a6bfc1d4f3fe7  Bowling.bin
// c3ef5c4653212088eda54dc91d787870  Boxing.bin
// 1cca2197d95c5a41f2add49a13738055  Brain Games.bin
// f34f08e5eb96e500e851a80be3277a56  Breakout - Breakaway IV.bin
// cfd6a8b23d12b0462baf6a05ef347cd8  Bridge.bin
// 413c925c5fdcea62842a63a4c671a5f2  Bridge [fixed].bin
// 1cf59fc7b11cdbcefe931e41641772f6  Buck Rogers - Planet of Zoom.bin
// 68597264c8e57ada93be3a5be4565096  Bugs.bin
// a3486c0b8110d9d4b1db5d8a280723c6  Bugs Bunny (Prototype).bin
// aa1c41f86ec44c0a44eb64c332ce08af  Bumper Bash.bin
// ab2cfcaad3daaf673b2b14fdbb8dac33  Bump 'n' Jump.bin
// 0443cfa9872cdb49069186413275fa21  BurgerTime.bin
// b42df8d92e3118dc594cecd575f515d7  Burning Desire (PAL).bin
// 66fcf7643d554f5e15d4d06bab59fe70  Cabbage Patch Kids - Adventures in the Park (Prototype).bin
// 7f6533386644c7d6358f871666c86e79  Cakewalk.bin
// 9ab72d3fd2cc1a0c9adb504502579037  California Games.bin
// feedcc20bc3ca34851cd5d9e38aa2ca6  Canyon Bomber.bin
// 151c33a71b99e6bcffb34b43c6f0ec23  Care Bears (Prototype).bin
// 028024fb8e5e5f18ea586652f9799c96  Carnival.bin
// b816296311019ab69a21cb9e9e235d12  Casino - Poker Plus.bin
// 76f66ce3b83d7a104a899b4b3354a2f2  Cat Trax.bin
// 91c2098e88a6b13f977af8c003e0bca5  Centipede.bin
// 5d799bfa9e1e7b6224877162accada0d  Challenge of.... Nexar, The.bin
// ace319dc4f76548659876741a6690d57  Championship Soccer - Soccer.bin
// 3e33ac10dcf2dff014bc1decf8a9aea4  Chase the Chuck Wagon.bin
// 3f5a43602f960ede330cd2f43a25139e  Checkers.bin
// 749fec9918160921576f850b2375b516  China Syndrome.bin
// c1cb228470a87beb5f36e90ac745da26  Chopper Command.bin
// 3f58f972276d1e4e0e09582521ed7a5b  Chuck Norris Superkicks.bin
// a7b96a8150600b3e800a4689c3ec60a2  Circus Atari - Circus.bin
// 1e587ca91518a47753a28217cd4fd586  Coco Nuts.bin
// 5846b1d34c296bf7afc2fa05bbc16e98  Codebreaker - Code Breaker.bin
// 76a9bf05a6de8418a3ebc7fc254b71b4  Color Bar Generator.bin
// 97a9bb5c3679d67f5c2cd17f30b85d95  Colors (Prototype) (PAL).bin
// 4c8832ed387bbafc055320c05205bc08  Combat - Tank-Plus.bin
// b0c9cf89a6d4e612524f4fd48b5bb562  Combat Two (Prototype).bin
// 5d2cc33ca798783dee435eb29debf6d6  Commando.bin
// 61631c2f96221527e7da9802b4704f93  Commando [different logo].bin
// f457674cef449cfd85f21db2b4f631a7  Commando Raid.bin
// 2c8835aed7f52a0da9ade5226ee5aa75  Communist Mutants from Space.bin
// b98cc2c6f7a0f05176f74f0f62c45488  CompuMate.bin
// 1f21666b8f78b65051b7a609f1d48608  Condor Attack.bin
// f965cc981cbb0822f955641f8d84e774  Confrontation (Prototype).bin
// 00b7b4cbec81570642283e7fc1ef17af  Congo Bongo.bin
// 57c5b351d4de021785cf8ed8191a195c  Cookie Monster Munch.bin
// ab5bf1ef5e463ad1cbb11b6a33797228  Cosmic Ark.bin
// 7d903411807704e725cf3fafbeb97255  Cosmic Ark [selectable starfield].bin
// 133b56de011d562cbab665968bde352b  Cosmic Commuter.bin
// 3c853d864a1d5534ed0d4b325347f131  Cosmic Creeps.bin
// e5f17b3e62a21d0df1ca9aee1aa8c7c5  Cosmic Swarm.bin
// fe67087f9c22655ce519616fc6c6ef4d  Crack'ed (Prototype).bin
// a184846d8904396830951217b47d13d9  Crackpots.bin
// fb88c400d602fe759ae74ef1716ee84e  Crash Dive.bin
// 55ef7b65066428367844342ed59f956c  Crazy Climber.bin
// 8cd26dcf249456fe4aeb8db42d49df74  Crossbow.bin
// c17bdc7d14a36e10837d039f43ee5fa3  Cross Force.bin
// 384f5fbf57b5e92ed708935ebf8a8610  Crypts of Chaos.bin
// 1c6eb740d3c485766cade566abab8208  Crystal Castles.bin
// 6fa0ac6943e33637d8e77df14962fbfc  Cubicolor (Prototype).bin
// 58513bae774360b96866a07ca0e8fd8e  Custer's Revenge.bin
// 929e8a84ed50601d9af8c49b0425c7ea  Dancing Plate (PAL).bin
// a422194290c64ef9d444da9d6a207807  Dark Cavern.bin
// 106855474c69d08c8ffa308d47337269  Dark Chambers.bin
// e4c00beb17fdc5881757855f2838c816  Deadly Duck.bin
// 4e15ddfd48bca4f0bf999240c47b49f5  Death Trap.bin
// 0f643c34e40e3f1daafd9c524d3ffe64  Defender.bin
// d09935802d6760ae58253685ff649268  Demolition Herby.bin
// f0e0addc07971561ab80d9abe1b8d333  Demon Attack.bin
// b12a7f63787a6bb08e683837a8ed3f18  Demon Attack [fixed].bin
// f91fb8da3223b79f1c9a07b77ebfa0b2  Demons to Diamonds.bin
// 519f007c0e14fb90208dbb5199dfb604  Depth Charge (Prototype).bin
// fd4f5536fd80f35c64d365df85873418  Desert Falcon.bin
// 626d67918f4b5e3f961e4b2af2f41f1d  Diagnostic Test Cartridge 2.0 (Prototype).bin
// 38bd172da8b2a3a176e517c213fcd5a6  Diagnostic Test Cartridge 2.6.bin
// 93e276172b521c4491097f8b1393eea7  Diagnostic Test Cartridge 4.2.bin
// e02156294393818ff872d4314fc2f38e  Dice Puzzle (PAL).bin
// 6dda84fb8e442ecf34241ac0d1d91d69  Dig Dug.bin
// c3472fa98c3b452fa2fd37d1c219fb6f  Dodge 'Em - Dodger Cars.bin
// 83bdc819980db99bf89a7f2ed6a2de59  Dodge 'Em - Dodger Cars [fixed].bin
// ca09fa7406b7d2aea10d969b6fc90195  Dolphin.bin
// 937736d899337036de818391a87271e0  Donald Duck's Speedboat (Prototype).bin
// 36b20c427975760cb9cf4a47e41369e4  Donkey Kong.bin
// c8fa5d69d9e555eb16068ef87b1c9c45  Donkey Kong Junior.bin
// 7e2fe40a788e56765fe56a3576019968  Double Dragon.bin
// 368d88a6c071caba60b4f778615aae94  Double Dunk.bin
// 95e542a7467c94b1e4ab24a3ebe907f1  Dragon Defender (PAL).bin
// 41810dd94bd0de1110bedc5092bef5b0  Dragonfire.bin
// 90ccf4f30a5ad8c801090b388ddd5613  Dragonstomper.bin
// 77057d9d14b99e465ea9e29783af0ae3  Dragster.bin
// 51de328e79d919d7234cf19c1cd77fbc  Dukes of Hazzard.bin
// 3897744dd3c756ea4b1542e5e181e02a  Dumbo's Flying Circus (Prototype).bin
// 469473ff6fed8cc8d65f3c334f963aab  Dune (Prototype).bin
// 033e21521e0bf4e54e8816873943406d  Earth Dies Screaming, The.bin
// 42b2c3b4545f1499a083cfbc4a3b7640  Eggomania.bin
// 71f8bacfbdca019113f3f0801849057e  Elevator Action (Prototype).bin
// 7d483b702c44ee65cd2df22cbcc8b7ed  Elf Adventure (Prototype).bin
// b6812eaf87127f043e78f91f2028f9f4  Eli's Ladder.bin
// 7eafc9827e8d5b1336905939e097aae7  Elk Attack (Prototype).bin
// dbc8829ef6f12db8f463e30f60af209f  Encounter at L-5.bin
// 94b92a882f6dbaa6993a46e2dcc58402  Enduro.bin
// 9f5096a6f1a5049df87798eb59707583  Entity, The (Prototype).bin
// 6b683be69f92958abe0e2a9945157ad5  Entombed.bin
// 81f4f0285f651399a12ff2e2f35bab77  Escape from the Mindmaster.bin
// f344ac1279152157d63e64aa39479599  Espial.bin
// 615a3bf251a38eb6638cdc7ffbde5480  E.T. - The Extra-Terrestrial.bin
// 62d1f50219edf9a429a9f004c19f31b3  Euro Gen (PAL).bin
// 7ac4f4fb425db38288fa07fb8ff4b21d  Exocet (PAL).bin
// ebd2488dcace40474c1a78fa53ebfadf  Extra Terrestrials.bin
// b80d50ecee73919a507498d0a4d922ae  Fantastic Voyage.bin
// 9de0d45731f90a0a922ab09228510393  Fast Eddie.bin
// 665b8f8ead0eef220ed53886fbd61ec9  Fast Food.bin
// 074ec425ec20579e64a7ded592155d48  Fatal Run (PAL).bin
// 0b55399cf640a2a00ba72dd155a0c140  Fathom.bin
// 211fbbdbbca1102dc5b43dc8157c09b3  Final Approach.bin
// 386ff28ac5e254ba1b1bac6916bcc93a  Fireball.bin
// d09f1830fb316515b90694c45728d702  Fire Fighter.bin
// 20dca534b997bf607d658e77fbb3c0ee  Fire Fly.bin
// 554fd5775ca6d544818c96825032cf0d  Firefox (Prototype).bin
// b8865f05676e64f3bec72b9defdacfa7  Fishing Derby.bin
// 30512e0e83903fc05541d2f6a6a62654  Flag Capture - Capture.bin
// 8786c1e56ef221d946c64f6b65b697e9  Flash Gordon.bin
// e549f1178e038fa88dc6d657dc441146  Football.bin
// dd1422ffd538e2e33b339ebeef4f259d  Football Demo.bin
// 213e5e82ecb42af237cfed8612c128ac  Forest (PAL).bin
// 15dd21c2608e0d7d9f54c0d3f08cca1f  Frankenstein's Monster.bin
// 8e0ab801b1705a740b476b7f588c6d16  Freeway.bin
// cb4a7b507372c24f8b9390d22d54a918  Frisco (PAL).bin
// 45a4f55bb9a5083d470ad479afd8bca2  Frog Demo (PAL).bin
// 081e2c114c9c20b61acf25fc95c71bf4  Frogger.bin
// 27c6a2ca16ad7d814626ceea62fa8fb4  Frogger II - Threeedeep!.bin
// f67181b3a01b9c9159840b15449b87b0  Frog Pond (Prototype).bin
// dcc2956c7a39fdbf1e861fc5c595da0d  Frogs and Flies.bin
// e556e07cc06c803f2955986f53ef63ed  Front Line.bin
// 4ca73eb959299471788f0b685c3ba0b5  Frostbite.bin
// d3bb42228a6cd452c111c1932503cc03  Funky Fish (Prototype).bin
// 211774f4c5739042618be8ff67351177  Galaxian.bin
// f539e32bf6ce39c8ca47cb0cdd2c5cb8  GameLine Master Module ROM.bin
// db971b6afc9d243f614ebf380af0ac60  Gamma-Attack.bin
// f16ef574d2042ed8fe877d6541f4dba4  Gangster Alley.bin
// 20edcc3aa6c189259fa7e2f044a99c49  Gangster Alley [fixed].bin
// dc13df8420ec69841a7c51e41b9fbba5  Garfield (Prototype).bin
// 728152f5ae6fdd0d3a9b88709bee6c7a  Gas Hog.bin
// 5cbd7c31443fb9c308e9f0b54d94a395  Gas Hog [fixed].bin
// e64a8008812327853877a37befeb6465  Gauntlet.bin
// e314b42761cd13c03def744b4afc7b1b  Ghostbusters.bin
// c2b5c50ccb59816867036d7cf730bf75  Ghostbusters II (PAL).bin
// 643e6451eb6b8ab793eb60ba9c02e000  Ghostbusters II (PAL) [different tune].bin
// 2bee7f226d506c217163bad4ab1768c0  Ghost Manor.bin
// c1fdd44efda916414be3527a47752c75  G.I. Joe - Cobra Strike.bin
// 5e0c37f534ab5ccc4661768e2ddf0162  Glacier Patrol.bin
// 2d9e5d8d083b6367eda880e80dfdfaeb  Glib - Video Word Game.bin
// cfb83a3b0513acaf8be4cae1512281dc  Going-Up (Prototype).bin
// 2e663eaa0d6b723b645e643750b942fd  Golf.bin
// b16cd9784589219391c839cb68c47b9c  Golf Diagnostic (Prototype).bin
// 32ae78abbb5e677e2aabae5cc86cec29  Good Luck, Charlie Brown (Prototype).bin
// c16c79aad6272baffb8aae9a7fff0864  Gopher.bin
// 81b3bf17cf01039d311b4cd738ae608e  Gorf.bin
// 2903896d88a341511586d69fcfc20f7d  Grand Prix.bin
// 8ac18076d01a6b63acf6e2cab4968940  Gravitar.bin
// 01cb3e8dfab7203a9c62ba3b94b4e59f  Gremlins.bin
// 66b89ba44e7ae0b51f9ef000ebba1eb7  Grover's Music Maker (Prototype).bin
// 7ab2f190d4e59e8742e76a6e870b567e  Guardian.bin
// b311ab95e85bc0162308390728a7361d  Gyruss.bin
// 30516cfbaa1bc3b5335ee53ad811f17a  Halloween.bin
// f16c709df0a6c52f47ff52b9d95b7d8d  Hangman - Spelling.bin
// 700a786471c8a91ec09e2f8e47f14a04  Hard-Head (Prototype).bin
// a34560841e0878c7b14cc65f79f6967d  Harem.bin
// f0a6e99f5875891246c3dbecbf2d2cea  Haunted House.bin
// 5f950a2d1eb331a1276819520705df94  Heart Like a Wheel (Prototype).bin
// fca4a5be1251927027f2c24774a02160  H.E.R.O..bin
// c52d9bbdc5530e1ef8e8ba7be692b01e  Holey Moley (Prototype).bin
// 0bfabf1e98bdb180643f35f2165995d0  Home Run - Baseball.bin
// 7972e5101fa548b952d852db24ad6060  Human Cannonball - Cannon Man.bin
// 102672bbd7e25cd79f4384dd7214c32b  Hunt & Score - Memory Match.bin
// a4c08c4994eb9d24fb78be1793e82e26  Ice Hockey.bin
// 9a21fba9ee9794e0fadd7c7eb6be4e12  Ikari Warriors.bin
// c4bc8c2e130d76346ebf8eb544991b46  Imagic Selector ROM.bin
// 75a303fd46ad12457ed8e853016815a0  Immies & Aggies (Prototype).bin
// d39e29b03af3c28641084dd1528aae05  Inca Gold (PAL).bin
// c5301f549d0722049bb0add6b10d1e09  Indy 500 - Race.bin
// afe88aae81d99e0947c0cfb687b16251  Infiltrate.bin
// b4030c38a720dd84b84178b6ce1fc749  International Soccer.bin
// 2f0546c4d238551c7d64d884b618100c  Ixion (Prototype).bin
// e51030251e440cffaab1ac63438b44ae  James Bond 007.bin
// 58a82e1da64a692fd727c25faef2ecc9  Jawbreaker.bin
// 718ae62c70af4e5fd8e932fee216948a  Journey Escape.bin
// 3276c777cbe97cdd2b4a63ffc16b7151  Joust.bin
// 36c29ceee2c151b23a1ad7aa04bd529d  Jr. Pac-Man.bin
// 2bb9f4686f7e08c5fcc69ec1a1c66fe7  Jungle Hunt.bin
// b9d1e3be30b131324482345959aed5e5  Kabobber (Prototype).bin
// 5428cdfada281c569c74c7308c7f2c26  Kaboom!.bin
// 7b43c32e3d4ff5932f39afcb4c551627  Kamikaze Saucers (Prototype).bin
// 4326edb70ff20d0ee5ba58fa5cb09d60  Kangaroo.bin
// cedbd67d1ff321c996051eec843f8716  Karate.bin
// be929419902e21bd7830a7a7d746195d  Keystone Kapers.bin
// 248668b364514de590382a7eda2c9834  Kick-Man (Prototype).bin
// 7a7f6ab9215a3a6b5940b8737f116359  Killer Satellites.bin
// e21ee3541ebd2c23e817ffb449939c37  King Kong.bin
// eed9eaf1a0b6a2b9bc4c8032cb43e3fb  Klax (PAL).bin
// 534e23210dd1993c828d944c6ac4d9fb  Kool-Aid Man.bin
// 4baada22435320d185c95b7dd2bcdb24  Krull.bin
// 5b92a93b23523ff16e2789b820e2a4c5  Kung-Fu Master.bin
// b86552198f52cfce721bafb496363099  Kyphus (Prototype).bin
// 931b91a8ea2d39fe4dca1a23832b591a  Laser Blast.bin
// 1fab68fd67fe5a86b2c0a9227a59bb95  Lasercade (Prototype).bin
// 1fa58679d4a39052bd9db059e8cda4ad  Laser Gates.bin
// ab10f2974dee73dab4579f0cab35fca6  Lilly Adventure (PAL).bin
// 86128001e69ab049937f265911ce7e8a  Lochjaw.bin
// 71464c54da46adae9447926fdbfc1abe  Lock 'n' Chase.bin
// b4e2fd27d3180f0f4eb1065afc0d7fc9  London Blitz.bin
// 5babe0cad3ec99d76b0aa1d36a695d2f  Looping (Prototype).bin
// e24d7d879281ffec0641e9c3f52e505a  Lord of the Rings, The - Journey to Rivendell (Prototype).bin
// 7c00e7a205d3fda98eb20da7c9c50a55  Lost Luggage.bin
// 2d76c5d1aad506442b9e9fb67765e051  Lost Luggage [no opening scene].bin
// 393e41ca8bdd35b52bf6256a968a9b89  M.A.D..bin
// cddabfd68363a76cd30bee4e8094c646  MagiCard.bin
// ccb5fa954fb76f09caae9a8c66462190  Malagai.bin
// 54a1c1255ed45eb8f71414dadb1cf669  Mangia'.bin
// 13895ef15610af0d0f89d588f376b3fe  Marauder.bin
// b00e8217633e870bf39d948662a52aac  Marine Wars.bin
// e908611d99890733be31733a979c62d8  Mario Bros..bin
// 835759ff95c2cdc2324d7c1e7c5fa237  M.A.S.H.bin
// ae4be3a36b285c1a1dff202157e2155d  Master Builder.bin
// 3b76242691730b2dd22ec0ceab351bc6  Masters of the Universe - The Power of He-Man.bin
// 470878b9917ea0348d64b5750af149aa  Math Gran Prix.bin
// f825c538481f9a7a46d1e9bc06200aaf  Maze Craze - A Game of Cops 'n Robbers - Maze Mania - A Game of Cops 'n Robbers.bin
// 35b43b54e83403bb3d71f519739a9549  McDonald's - Golden Arches Adventure (Prototype).bin
// b65d4a38d6047735824ee99684f3515e  MegaBoy.bin
// daeb54957875c50198a7e616f9cc8144  Mega Force.bin
// 318a9d6dda791268df92d72679914ac3  MegaMania - A Space Nightmare.bin
// 96e798995af6ed9d8601166d4350f276  Meltdown (Prototype).bin
// f1554569321dc933c87981cf5c239c43  Midnight Magic.bin
// 3c57748c8286cf9e821ecd064f21aaa9  Millipede.bin
// 0e224ea74310da4e7e2103400eb1b4bf  Mind Maze (Prototype).bin
// fa0570561aa80896f0ead05c46351389  Miner 2049er - Starring Bounty Bob.bin
// 3b040ed7d1ef8acb4efdeebebdaa2052  Miner 2049er - Starring Bounty Bob [fixed].bin
// 2a1b454a5c3832b0240111e7fd73de8a  Miner 2049er Volume II.bin
// 4543b7691914dfd69c3755a5287a95e1  Mines of Minos.bin
// df62a658496ac98a3aa4a6ee5719c251  Miniature Golf - Arcade Golf.bin
// 3a2e2d0c6892aa14544083dfb7762782  Missile Command.bin
// 1a8204a2bcd793f539168773d9ad6230  Missile Command [no initials].bin
// cb24210dc86d92df97b38cf2a51782da  Missile Control (PAL).bin
// 6efe876168e2d45d4719b6a61355e5fe  Mission 3,000 A.D. (PAL).bin
// cf9069f92a43f719974ee712c50cd932  Mission Survive (PAL).bin
// 4181087389a79c7f59611fb51c263137  Miss Piggy's Wedding (Prototype).bin
// 7af40c1485ce9f29b1a7b069a2eb04a7  Mogul Maniac.bin
// 6913c90002636c1487538d4004f7cac2  Monster Cise (Prototype).bin
// 3347a6dd59049b15a38394aa2dafa585  Montezuma's Revenge - Featuring Panama Joe.bin
// 515046e3061b7b18aa3a551c3ae12673  Moon Patrol.bin
// 203abb713c00b0884206dcc656caa48f  Moonsweeper.bin
// 7d1034bcb38c9b746ea2c0ae37d9dff2  Morse Code Tutor.bin
// f5a2f6efa33a3e5541bc680e9dc31d5b  Motocross (PAL).bin
// de0173ed6be9de6fd049803811e5f1a8  Motocross Racer.bin
// 378a62af6e9c12a760795ff4fc939656  MotoRodeo.bin
// db4eb44bc5d652d9192451383d3249fc  Mountain King.bin
// 5678ebaa09ca3b699516dba4671643ed  Mouse Trap.bin
// 0164f26f6b38a34208cd4a2d0212afc3  Mr. Do!.bin
// 97184b263722748757cfdc41107ca5c0  Mr. Do!'s Castle.bin
// 603c7a0d12c935df5810f400f3971b67  Mr. Postman (PAL).bin
// 87e79cd41ce136fd4f72cc6e2c161bee  Ms. Pac-Man.bin
// aaac0d277eda054861e613c59c2e4ff2  Music Demo.bin
// 65b106eba3e45f3dab72ea907f39f8b4  Music Machine, The.bin
// 04fccc7735155a6c1373d453b110c640  My Golf (PAL).bin
// dfad86dd85a11c80259f3ddb6151f48f  My Golf (PAL) [fixed].bin
// fcbbd0a407d3ff7bf857b8a399280ea1  Mysterious Thief, A (Prototype).bin
// 36306070f0c90a72461551a7a4f3a209  Name This Game.bin
// 392f00fd1a074a3c15bc96b0a57d52a1  Night Driver.bin
// ead60451c28635b55ca8fea198444e16  Nightmare (PAL).bin
// b6d52a0cf53ad4216feb04147301f87d  No Escape!.bin
// e3c35eac234537396a865d23bafb1c84  Nuts (PAL).bin
// 133a4234512e8c4e9e8c5651469d4a09  Obelix.bin
// c73ae5ba5a0a3f3ac77f0a9e14770e73  Official Frogger, The.bin
// 98f63949e656ff309cefa672146dc1b8  Off the Wall.bin
// b6166f15720fdf192932f1f76df5b65d  Off Your Rocker (Prototype).bin
// c9c25fc536de9a7cdc5b9a916c459110  Oink!.bin
// 9947f1ebabb56fd075a96c6d37351efa  Omega Race.bin
// 28d5df3ed036ed63d33a31d0d8b85c47  Open, Sesame! (PAL).bin
// fa1b060fd8e0bca0c2a097dcffce93d3  Oscar's Trash Race.bin
// 55949cb7884f9db0f8dfcf8707c7e5cb  Othello.bin
// 113cd09c9771ac278544b7e90efe7df2  Othello [no grid markers].bin
// 890c13590e0d8d5d6149737d930e4d95  Outlaw - Gunslinger.bin
// f97dee1aa2629911f30f225ca31789d4  Out of Control.bin
// 6e372f076fb9586aff416144f5cfe1cb  Pac-Man.bin
// 0e713d4e272ea7322c5b27d645f56dd0  Panda Chase (PAL).bin
// 714e13c08508ee9a7785ceac908ae831  Parachute (PAL).bin
// 012b8e6ef3b5fd5aabc94075c527709d  Party Mix - Bop a Buggy, Tug of War, Wizard's Keep, Down on the Line, Handcar.bin
// e40a818dac4dd851f3b4aafbe2f1e0c1  Peek-A-Boo (Prototype).bin
// 04014d563b094e79ac8974366f616308  Pengo.bin
// 09388bf390cd9a86dc0849697b96c7dc  Pete Rose Baseball.bin
// 6b1fc959e28bd71aed7b89014574bdc2  Phantom Tank (PAL).bin
// 3577e19714921912685bb0e32ddf943c  Pharaoh's Curse (PAL).bin
// 7dcbfd2acc013e817f011309c7504daa  Phaser Patrol.bin
// 7e52a95074a66640fcfde124fffd491a  Phoenix.bin
// da79aad11572c80a96e261e4ac6392d0  Pick 'n' Pile (PAL).bin
// 1d4e0a034ad1275bc4d75165ae236105  Pick Up (Prototype).bin
// 17c0a63f9a680e7a61beba81692d9297  Picnic.bin
// d3423d7600879174c038f53e5ebbf9d3  Piece o' Cake.bin
// 8e4fa8c6ad8d8dce0db8c991c166cdaa  Pigs in Space - Starring Miss Piggy.bin
// 6d842c96d5a01967be9680080dd5be54  Pitfall II - Lost Caverns.bin
// 3e90cf23106f2e08b2781e41299de556  Pitfall! - Pitfall Harry's Jungle Adventure.bin
// d9fbf1113114fb3a3c97550a0689f10f  Pizza Chef (Prototype).bin
// 9efb4e1a15a6cdd286e4bcd7cd94b7b8  Planet of the Apes (Prototype).bin
// 043f165f384fbea3ea89393597951512  Planet Patrol.bin
// da4e3396aa2db3bd667f83a1cb9e4a36  Plaque Attack.bin
// 8bbfd951c89cc09c148bfabdefa08bec  Pleiades (Prototype).bin
// 44f71e70b89dcc7cf39dfd622cfb9a27  Polaris.bin
// a4ff39d513b993159911efe01ac12eba  Pole Position.bin
// ee28424af389a7f3672182009472500c  Polo (Prototype).bin
// a83b070b485cf1fb4d5a48da153fdf1a  Pompeii (Prototype).bin
// 4799a40b6e889370b7ee55c17ba65141  Pooyan.bin
// c7f13ef38f61ee2367ada94fdcc6d206  Popeye.bin
// f93d7fee92717e161e6763a88a293ffa  Porky's.bin
// 3ad3dc799211ccd424d7c6d454401436  Power Lords (Prototype).bin
// 97d079315c09796ff6d95a06e4b70171  Pressure Cooker.bin
// ef3a4f64b6494ba770862768caf04b86  Private Eye.bin
// 56210a3b9ea6d5dd8f417a357ed8ca92  Pursuit of the Pink Panther (Prototype).bin
// 484b0076816a104875e00467d431c2d2  Q-bert.bin
// 517592e6e0c71731019c0cebc2ce044f  Q-bert's Qubes.bin
// 024365007a87f213cbe8ef5f2e8e1333  Quadrun.bin
// a0675883f9b09a3595ddd66a6f5d3498  Quest for Quintana Roo.bin
// 7eba20c2291a982214cc7cbe8d0b47cd  Quick Step!.bin
// fb4ca865abc02d66e39651bd9ade140a  Rabbit Transit.bin
// 3c7a7b3a0a7e6319b2fa0f923ef6c9af  Racer (Prototype).bin
// aab840db22075aa0f6a6b83a597f8890  Racing Car (PAL).bin
// a20d931a8fddcd6f6116ed21ff5c4832  Racquetball.bin
// baf4ce885aa281fd31711da9b9795485  Radar Lock.bin
// 92a1a605b7ad56d863a56373a866761b  Raft Rider.bin
// f724d3dd2471ed4cf5f191dbb724b69f  Raiders of the Lost Ark.bin
// 7096a198531d3f16a99d518ac0d7519a  Ram It.bin
// 5e1b4629426f4992cf3b2905a696e1a7  Rampage!.bin
// 9f8fad4badcd7be61bbd2bcaeef3c58f  Reactor.bin
// eb634650c3912132092b7aee540bbce3  RealSports Baseball.bin
// 8a183b6357987db5170c5cf9f4a113e5  RealSports Basketball (Prototype) (PAL).bin
// 3177cc5c04c1a4080a927dfa4099482b  RealSports Boxing.bin
// 7ad257833190bc60277c1ca475057051  RealSports Football.bin
// 08f853e8e01e711919e734d85349220d  RealSports Soccer.bin
// f7856e324bc56f45b9c8e6ff062ec033  RealSports Soccer [no opening tune].bin
// dac5c0fe74531f077c105b396874a9f1  RealSports Tennis.bin
// aed0b7bd64cc384f85fdea33e28daf3b  RealSports Volleyball.bin
// 4eb4fd544805babafc375dcdb8c2a597  Red Sea Crossing.bin
// 60a61da9b2f43dd7e13a5093ec41a53d  Rescue Terra I.bin
// 4f64d6d0694d9b7a1ed7b0cb0b83e759  Revenge of the Beefsteak Tomatoes.bin
// a995b6cbdb1f0433abc74050808590e6  Riddle of the Sphinx.bin
// 31512cdfadfd82bfb6f196e3b0fd83cd  River Patrol.bin
// 393948436d1f4cc3192410bb918f9724  River Raid.bin
// ab56f1b2542a05bebc4fbccfc4803a38  River Raid II.bin
// ce5cc62608be2cd3ed8abd844efb8919  Road Runner.bin
// 72a46e0c21f825518b7261c267ab886e  Robin Hood.bin
// 4f618c2429138e0280969193ed6c107e  Robot Tank.bin
// d97fd5e6e1daacd909559a71f189f14b  Rocky & Bullwinkle (Prototype).bin
// 65bd29e8ab1b847309775b0de6b2e4fe  Roc 'n Rope.bin
// 67931b0d37dc99af250dd06f1c095e8d  Room of Doom.bin
// 40b1832177c63ebf81e6c5b61aaffd3a  Rubik's Cube 3-D (Prototype).bin
// b9b4612358a0b2c1b4d66bb146767306  Rush Hour (Prototype).bin
// a4ecb54f877cd94515527b11e698608c  Saboteur (Prototype).bin
// 715dbf2e39ba8a52c5fe5cdd927b37e0  S.A.C. Alert (Prototype).bin
// 4d502d6fb5b992ee0591569144128f99  Save Mary! (Prototype).bin
// 01297d9b450455dd716db9658efb2fae  Save Our Ship (PAL).bin
// e377c3af4f54a51b85efe37d4b7029e6  Save the Whales (Prototype).bin
// 1bc2427ac9b032a52fe527c7b26ce22c  Sea Battle.bin
// 74d072e8a34560c36cacbc57b2462360  Seahawk (PAL).bin
// 68489e60268a5e6e052bad9c62681635  Sea Monster (PAL).bin
// 240bfbac5163af4df5ae713985386f92  Seaquest.bin
// 605fd59bfef88901c8c4794193a4cbad  Secret Agent (Prototype).bin
// fc24a94d4371c69bc58f5245ada43c44  Secret Quest.bin
// 8da51e0c4b6b46f7619425119c7d018e  Sentinel.bin
// b5a1a189601a785bdb2f02a424080412  Shootin' Gallery.bin
// 15c11ab6e4502b2010b18366133fc322  Shooting Arcade (Prototype).bin
// 25b6dc012cdba63704ea9535c6987beb  Shuttle Orbiter.bin
// 1e85f8bccb4b866d4daa9fcf89306474  Sinistar (Prototype).bin
// 4c8970f6c294a0a54c9c45e5e8445f93  Sir Lancelot.bin
// f847fb8dba6c6d66d13724dbe5d95c4d  Skate Boardin'.bin
// 39c78d682516d79130b379fa9deb8d1c  Skeet Shoot.bin
// 8654d7f0fb351960016e06646f639b02  Ski Hunt (PAL).bin
// b76fbadc8ffb1f83e2ca08b6fb4d6c9f  Skiing.bin
// 340f546d59e72fb358c49ac2ca8482bb  Skindiver (PAL).bin
// f10e3f45fb01416c87e5835ab270b53a  Ski Run (PAL).bin
// c31a17942d162b80962cb1f7571cd1d5  Sky Alien (PAL).bin
// 46c021a3e9e2fd00919ca3dd1a6b76d8  Sky Diver - Dare Diver.bin
// 2a0ba55e56e7a596146fa729acf0e109  Sky Jinks.bin
// 4c9307de724c36fd487af6c99ca078f2  Sky Patrol (Prototype).bin
// 3b91c347d8e6427edbe942a7a405290d  Sky Skipper.bin
// f90b5da189f24d7e1a2117d8c8abc952  Slot Machine - Slots.bin
// aed82052f7589df05a3f417bb4e45f0c  Slot Racers - Maze.bin
// 3d1e83afdb4265fa2fb84819c9cfd39c  Smurf - Rescue in Gargamel's Castle.bin
// a204cd4fb1944c86e800120706512a64  Smurfs Save the Day.bin
// 898b5467551d32af48a604802407b6e8  Snail Against Squirrel (PAL).bin
// 9c6faa4ff7f2ae549bbcb14f582b70e4  Sneak 'n Peek.bin
// 57939b326df86b74ca6404f64f89fce9  Snoopy and the Red Baron.bin
// 75028162bfc4cc8e74b04e320f9e6a3f  Snow White (Prototype).bin
// 947317a89af38a49c4864d6bdd6a91fb  Solar Fox.bin
// e72eb8d4410152bdcb69e7fba327b420  Solaris.bin
// 97842fe847e8eb71263d6f92f7e122bd  Solar Storm.bin
// d2c4f8a4a98a905a9deef3ba7380ed64  Sorcerer.bin
// 5f7ae9a7f8d79a3b37e8fc841f65643a  Sorcerer's Apprentice.bin
// 17badbb3f54d1fc01ee68726882f26a6  Space Attack.bin
// df6a28a89600affe36d94394ef597214  Space Cavern.bin
// ec5c861b487a5075876ab01155e74c6c  Spacechase.bin
// 72ffbef6504b75e69ee1045af9075f66  Space Invaders.bin
// 6f2aaffaaf53d23a28bf6677b86ac0e3  Space Jockey.bin
// 45040679d72b101189c298a864a5b5ba  SpaceMaster X-7.bin
// 5894c9c0c1e7e29f3ab86c6d3f673361  Space Shuttle - A Journey Into Space.bin
// 898143773824663efe88d0a3a0bb1ba4  Space Shuttle - A Journey Into Space [FE bankswitching].bin
// c5387fc1aa71f11d2fa82459e189a5f0  Space Tunnel (PAL).bin
// a7ef44ccb5b9000caf02df3e6da71a92  Space War - Space Combat.bin
// 24d018c4a6de7e5bd19a36f2b879b335  Spider Fighter.bin
// 199eb0b8dce1408f3f7d46411b715ca9  Spider-Man.bin
// a4e885726af9d97b12bb5a36792eab63  Spike's Peak.bin
// 542c6dd5f7280179b51917a4cba4faff  Spinning Fireball (Prototype).bin
// cef2287d5fd80216b2200fb2ef1adfa8  Spitfire Attack.bin
// 4cd796b5911ed3f1062e805a3df33d98  Springer.bin
// 5a8afe5422abbfb0a342fb15afd7415f  Sprint Master.bin
// 3105967f7222cc36a5ac6e5f6e89a0b4  Spy Hunter.bin
// ba257438f8a78862a9e014d831143690  Squeeze Box.bin
// 34c808ad6577dbfa46169b73171585a3  Squoosh (Prototype).bin
// 21a96301bb0df27fde2e7eefa49e0397  Sssnake.bin
// 21d7334e406c2407e69dbddd7cec3583  Stampede.bin
// f526d0c519f5001adb1fc7948bfbb3ce  Star Fox.bin
// 0c48e820301251fbb6bcdc89bd3555d9  Stargate.bin
// a3c1c70024d7aabb41381adbfb6d3b25  Stargunner.bin
// d69559f9c9dc6ef528d841bf9d91b275  StarMaster.bin
// cbd981a23c592fb9ab979223bb368cd5  Star Raiders.bin
// e363e467f605537f3777ad33e74e113a  Star Ship - Outer Space.bin
// 79e5338dbfa6b64008bb0d72a3179d3c  Star Strike.bin
// 03c3f7ba4585e349dd12bfa7b34b7729  Star Trek - Strategic Operations Simulator.bin
// 813985a940aa739cc28df19e0edd4722  Star Voyager.bin
// c9f6e521a49a2d15dac56b6ddb3fb4c7  Star Wars - Jedi Arena.bin
// 5336f86f6b982cc925532f2e80aa1e17  Star Wars - Return of the Jedi - Death Star Battle.bin
// d44d90e7c389165f5034b5844077777f  Star Wars - Return of the Jedi - Ewok Adventure (Prototype).bin
// 6339d28c9a7f92054e70029eb0375837  Star Wars - The Arcade Game.bin
// 3c8e57a246742fa5d59e517134c0b4e6  Star Wars - The Empire Strikes Back.bin
// 656dc247db2871766dffd978c71da80c  Steeplechase.bin
// f1eeeccc4bba6999345a2575ae96508e  Steeplechase (PAL).bin
// 0b8d3002d8f744a753ba434a4d39249a  Stellar Track.bin
// 23fad5a125bcd4463701c8ad8a0043a9  Stone Age.bin
// 9333172e3c4992ecf548d3ac1f2553eb  Strategy X.bin
// e10d2c785aadb42c06390fae0d92f282  Strawberry Shortcake - Musical Match-Ups.bin
// 396f7bc90ab4fa4975f8c74abe4e81f0  Street Racer - Speedway II.bin
// 7b3cf0256e1fa0fdc538caf3d5d86337  Stronghold.bin
// c3bbc673acf2701b5275e85d9372facf  Stunt Cycle (Prototype).bin
// f3f5f72bfdd67f3d0e45d097e11b8091  Submarine Commander.bin
// 5af9cd346266a1f2515e1fbc86f5186a  Sub-Scan.bin
// 93c52141d3c4e1b5574d072f1afde6cd  Subterranea.bin
// e4c666ca0c36928b95b13d33474dbb44  Suicide Mission.bin
// 45027dde2be5bdd0cab522b80632717d  Summer Games.bin
// 8885d0ce11c5b40c3a8a8d9ed28cefef  Super Breakout.bin
// 9d37a1be4a6e898026414b8fee2fc826  Super Challenge Baseball.bin
// e275cbe7d4e11e62c3bfcfb38fca3d49  Super Challenge Football.bin
// 4565c1a7abce773e53c75b35414adefd  Supercharger BIOS.bin
// c29f8db680990cb45ef7fef6ab57a2c2  Super Cobra.bin
// 09abfe9a312ce7c9f661582fdf12eab6  Super Football.bin
// a9531c763077464307086ec9a1fd057d  Superman.bin
// 5de8803a59c36725888346fdc6e7429d  Superman [fixed].bin
// c20f15282a1aa8724d70c117e5c9709e  Surfer's Paradise - But Danger Below! (PAL).bin
// aec9b885d0e8b24e871925630884095c  Surf's Up (Prototype).bin
// 4d7517ae69f95cfbc053be01312b7dba  Surround - Chase.bin
// 045035f995272eb2deb8820111745a07  Survival Island.bin
// 85e564dae5687e431955056fbda10978  Survival Run.bin
// 59e53894b3899ee164c91cfa7842da66  Survival Run (Prototype).bin
// c7600d72247c5dfa1ec1a88d23e6c85e  Sweat! - The Decathlon Game (Prototype).bin
// 87662815bc4f3c3c86071dc994e3f30e  Swordfight.bin
// 528400fad9a77fd5ad7fc5fdc2b7d69d  Sword of Saros.bin
// 5aea9974b975a6a844e6df10d2b861c4  SwordQuest - EarthWorld.bin
// f9d51a4e5f8b48f68770c89ffd495ed1  SwordQuest - FireWorld.bin
// bc5389839857612cfabeb810ba7effdc  SwordQuest - WaterWorld.bin
// d45ebf130ed9070ea8ebd56176e48a38  Tac-Scan.bin
// de3d0e37729d85afcb25a8d052a6e236  Tapeworm.bin
// c0d2434348de72fa6edcc6d8e40f28d7  Tapper.bin
// a1ead9c181d67859aa93c44e40f1709c  Tax Avoiders.bin
// 7574480ae2ab0d282c887e9015fdb54c  Taz.bin
// 3d7aad37c55692814211c8b590a0334c  Telepathy (Prototype).bin
// c830f6ae7ee58bcc2a6712fb33e92d55  Tempest (Prototype).bin
// 42cdd6a9e42a3639e190722b8ea3fc51  Tennis.bin
// 5eeb81292992e057b290a5cd196f155d  Texas Chainsaw Massacre, The.bin
// 2d15b092e8350912ec4b2e5e750fa1c6  Texas Chainsaw Massacre, The (Prototype).bin
// e63a87c231ee9a506f9599aa4ef7dfb9  Threshold.bin
// cf507910d6e74568a68ac949537bccf9  Thunderground.bin
// c032c2bd7017fdfbba9a105ec50f800e  Thwocker (Prototype).bin
// fc2104dd2dadf9a6176c1c1c8f87ced9  Time Pilot.bin
// d6d1ddd21e9d17ea5f325fa09305069c  Time Warp (PAL).bin
// 12123b534bdee79ed7563b9ad74f1cbd  Title Match Pro Wrestling.bin
// 32dcd1b535f564ee38143a70a8146efe  Tomarc the Barbarian.bin
// 3ac6c50a8e62d4ce71595134cbd8035e  Tomcat - The F-14 Fighter Simulator.bin
// fa2be8125c3c60ab83e1c0fe56922fcb  Tooth Protectors.bin
// 0aa208060d7c140f20571e3341f5a3f8  Towering Inferno.bin
// 6ae4dc6d7351dacd1012749ca82f9a56  Track and Field.bin
// 66706459e62514d0c39c3797cbf73ff1  Treasure Below (PAL).bin
// 24df052902aa9de21c2b2525eb84a255  Trick Shot.bin
// fb27afe896e7c928089307b32e5642ee  TRON - Deadly Discs.bin
// b2737034f974535f5c0c6431ab8caf73  Tunnel Runner.bin
// 807841df228ee8aab0a06ee639ce5a8a  Turbo (Prototype).bin
// 7a5463545dfb2dcfdafa6074b2f2c15e  Turmoil.bin
// 085322bae40d904f53bdcc56df0593fc  Tutankham.bin
// 81a010abdba1a640f7adf7f84e13d307  Universal Chaos.bin
// ee681f566aad6c07c61bbbfc66d74a27  Unknown Activision Game (Prototype).bin
// 73e66e82ac22b305eb4d9578e866236e  Unknown Datatech Game.bin
// a499d720e7ee35c62424de882a3351b6  Up 'n Down.bin
// c6556e082aac04260596b4045bc122de  Vanguard.bin
// 19b3b807507653516985ba95da92499d  VCS Draw Demo.bin
// d08fccfbebaa531c4a4fa7359393a0a9  Venetian Blinds Demo.bin
// 3e899eba0ca8cd2972da1ae5479b4f0d  Venture.bin
// 539d26b6e9df0da8e7465f0f5ad863b7  Video Checkers - Checkers.bin
// f0b7db930ca0e548c41a97160b9f6275  Video Chess.bin
// 4191b671bcd8237fc8e297b4947f2990  Video Jogger.bin
// 497f3d2970c43e5224be99f75e97cbbb  Video Life.bin
// 4209e9dcdf05614e290167a1c033cfd2  Video Life [higher sounds].bin
// 60e0ea3cbe0913d39803477945e9e5ec  Video Olympics - Pong Sports.bin
// 107cc025334211e6d29da0b6be46aec7  Video Pinball - Arcade Pinball.bin
// ee659ae50e9df886ac4f8d7ad10d046a  Video Reflex.bin
// 297236cb9156be35679f83c4e38ee169  Video Reflex [no roman numbers].bin
// 6041f400b45511aa3a69fab4b8fc8f41  Wabbit.bin
// 7ff53f6922708119e7bf478d7d618c86  Walker (PAL).bin
// d3456b4cf1bd1a7b8fb907af1a80ee15  Wall Ball.bin
// 372bddf113d088bc572f94e98d8249f5  Wall Break (PAL).bin
// cbe5a166550a8129a5e6d374901dffad  Warlords.bin
// a20bc456c3b5fd9335e110df6e000e12  Warplock.bin
// 8e48ea6ea53709b98e6f4bd8aa018908  Wings (Prototype).bin
// 4e02880beeb8dbd4da724a3f33f0971f  Wing War (PAL).bin
// 83fafd7bd12e3335166c6314b3bde528  Winter Games.bin
// 7e8aa18bc9502eb57daaf5e7c1e94da7  Wizard of Wor.bin
// 7b24bfe1b61864e758ada1fe9adaa098  Wizard (Prototype).bin
// 2facd460a6828e0e476d3ac4b8c5f4f7  Words-Attack (Prototype) (PAL).bin
// ec3beb6d8b5689e867bafb5d5f507491  Word Zapper.bin
// 130c5742cd6cbe4877704d733d5b08ca  World End (PAL).bin
// 87f020daa98d0132e98e43db7d8fea7e  Worm War I.bin
// eaf744185d5e8def899950ba7c6e7bb5  Xenophobe.bin
// af6f3e9718bccfcd8afb421f96561a34  Xevious (Prototype).bin
// 5961d259115e99c30b64fe7058256bcf  X-Man.bin
// c5930d0e8cdae3e037349bfa08e871be  Yars' Revenge.bin
// eea0da9b987d661264cce69a7c13c3bd  Zaxxon.bin
// a336beac1f0a835614200ecd9c41fd70  Zoo Keeper Sounds (Prototype).bin
    {(char*)0,                        (char*)0,   0,0,0,0}
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
  extern void OutputCartInfo(string type, string md5);
    
  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  // Defaults for the selected cart... this may change up below...
  memcpy(&myCartInfo, &table[0], sizeof(myCartInfo));

  // Take a closer look at the ROM image and try to figure out its type
  myCartInfo.type = 0;

  // First we'll see if it's type is listed in the table above
  for(CartInfo* entry = table; (entry->md5 != 0); ++entry)
  {
    if(entry->md5 == md5)
    {
      memcpy(&myCartInfo, entry, sizeof(myCartInfo));
      break;
    }
  }
  
  // Handle special situations...
  if (myCartInfo.special == SPEC_HAUNTED)
  {
      uInt8* imageOffset = (uInt8*)(image+1103);
      // Haunted House needs a fix to work... original programming bug.
      if (*imageOffset == 0xE5) *imageOffset = 0xE9;
  }

  // If we didn't find the type in the table then guess it based on size
  if(myCartInfo.type == 0)
  {
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

