# StellaDS - PHOENIX EDITION
StellaDS - An Atari 2600 VCS Emulator for the DS/DSi
To use this emulator, you must use compatibles rom with a26/bin format. 
Twilight Menu++ wants to see the file extension be .a26 (you can just rename .bin)

_**Have you played Atari today?!**_

Features :
-----------------------
High level of compatibility to make games playable. Most games you remember fondly run full speed on a DSi in native 133MHz mode. For the older DS-LITE and DS-PHAT, many of the simple 8k, 4k and 2k games will play perfectly More sophisticated bank-switching schemes really require the DSi for the best experience.
 
See compatibility.txt for a full list of the ROMS supported. Generally these are going to be NTSC No-Intro or ROMHUNTER roms (the good ones!). 

* Wide range of controllers including joystick, paddles, driving controller, booster grip, keypad, Genesis 2-button and more
* Savekey Support with backing 32K EEPROM
* FA2 and Cherity supported with backing EEPROM support
* Most popular bankswitching is supported up to 256K EF/DF/SB and 512K 3E and 3F schemes with up to 32K of RAM
* High score support with up to 10 scores for each game
* Manuals for more than 100 of the common games included
* Keypad overlay for Star Raiders

Copyright :
-----------------------
StellaDS Phoenix-Edition is Copyright (c) 2020-2022 Dave Bernazzani (wavemotion-dave)

As long as there is no commercial use (i.e. no profit is made), copying and distribution of this emulator, it's source code and associated readme files, with or without modification,  are permitted in any medium without royalty provided this copyright notice is used and wavemotion-dave (Phoenix-Edition), Alekmaul (original port) and the Stella Team are thanked profusely.

The StellaDS emulator is offered as-is, without any warranty.

Known Issues :
-----------------------
* DPC+ and CDFJ/CDFJ+ games can't be emulated - that requires a coprocessor running at 70MHz (more   than the speed of an original DS!).
* Pitfall II DPC support is incomplete and the background music will not play correctly (game is otherwise fine to play).
* Cherity music fetchers are incomplete and the background music will not play correctly (game is otherwise fine to play).
* The original DS-Lite and DS-Phat are running at half the speed (67MHz vs 134MHz) of the DSi/DSi-XL and as such you will find that only a subset of games run at full speed... Fortunately many of the best games will runfine - Space Invaders, Missile Command, Asteroids, Yars Revenge, Adventure, Phoenix, Chopper Command, Megamania, Pitfall!, Cosmic Ark, Demon Attack and lots more will be at your fingertips.

Installation:
-----------------------
Copy StellaDS.nds to your flash cart or SD card of choice. Most launchers will
auto patch the game to run so you should be good to go... press on the CART slot
too pick and load a game (.bin or .a26). For some older launchers or flash-carts
you might need to DLDI patch the StellaDS.nds for your system (but it's unlikely
as most launchers will auto-patch when you run the emulator).

Configuration:
-----------------------
The gear icon on the main screen allows you to set a number of configuration 
parameters related specifically to the game you are running. In addition to
the parameters shown, the difficulty switches (Left/Right A/B) are also saved
on a per-game basis.  If you change any configuration settings, you really 
should re-load the game after saving out your configuration - some settings
such as Bank Switching and NTSC vs PAL are only applied when the game is first
loaded.

Here is a description of the configuration items you can adjust. Be sure to 
press the START button to save out your configuration after making changes:

* Controller:      Most games use the Left-Joystick but you can change this.
* Bankswitch:      The system tries to auto-detect the correct bankswitching scheme.
* Frame Blend:     Normal is fastest. Flicker free is slowest but blends frames. You can also try Flicker Reduce Background/Black which is mid-ground.
* TV Type:         NTSC vs PAL. Reload your game after saving this setting.             
* Sound:           Normally ON. Can be turned OFF which will render screen faster.
* ABXY Button:     Default to FIRE button but you can change this.
* HBLANK Zero:     Whether system clears pixels on horizontal blanks. Turn off at your own risk (it will speed up emulation which helps with older DS-LITE)
* VBLANK Zero:     Whether system clears pixels on vertical blanks. Turn off at your own risk (it will speed up emulation which helps with older DS-LITE)
* Analog Sens:     Default is 10 (1.0x speed). You can ajust how the paddle emulation responds.
* Start Scanline:  Starting Scanline setting. When the TV first starts output of scanlines.
* Num Scanlines:   Number of scanlines to display. Don't touch if you don't understand this.
* Offset/Scale:    You can ajust the screen scaling and offset positions for the game. 

PAL vs NTSC:
-----------------------
StellaDS supports PAL games but be warned... in the very early days of video
games, 99% of all games were developed as NTSC for the American Market and
only later ported to a PAL system for our European friends. Some game makers
did a good job (Imagic). Some did an OK job (Atari). And some did not do well
(Activision). That means that some games just don't run at the right speed 
for the PAL market - since Activision did not optiize for PAL, the games run
17% slower due to the frame rate difference. To make matters worse, the main
benefit of PAL is that the programmer has more scanlines to work with but the
DS only has 192 pixel rows... so with PAL we must compress the image and
that results in missing pixel rows. All of this to say: while PAL is supported
you are generally going to get the most genuine Atari 2600 experience with
NTSC roms. 

Thanks :
-----------------------
* To Bradford W. Mott and Stephen Anthony and various contributors for Stella.
* To Alekmaul for porting Stella to the DS.
* To Robz for Twilight-Menu++ which rekindled emulation on the classic handheld.

How to use StellaDS :
---------------------
Put the a26/bin files where you want on your SD or flashcard. 

Place StellaDS.NDS on your SD or flashcard.

That's all, StellaDS can be use now :) !

When the emulator starts, click on the cartridge slot to choose a file. you are use Up/Down 
to select a file, then use A to load it.

Controls :
 * D-Pad   : the Joystick
 * A,B,X,Y : Fire button (configurable)
 * SELECT  : SELECT switch
 * START   : START switch
 * R-Trig + D-Pad  : Shift display offset in the D-PAD direction
 * L-Trig + D-Pad  : Change Scaling of the Y-Screen (UP/DOWN scaling only)
 * L-Trig + R-Trig + A:  Swap LCD top/bottom. 
 * Use stylus on buttons for other icon-based actions on bottom screen.

Older Credits and links:
----------------------------
Thanks Wintermute for devkitpro and libnds (http://www.devkitpro.org).
Thanks TheChucksters to understand how Stella emulator works with the release of
his source code conversion (http://www.charlesmoyes.com/drupal/)
Thanks Stella Team for this marvelous emulator (http://stella.sourceforge.net/).

Version History:
-----------------------

V5.2 : 30-Sep-2022 by Dave Bernazzani (wavemotion)
  * Fixed goof in F6, F4 and related F6SC and F4SC drivers! 
  * Another frame of increased performance
  * More accurate AR cart handling for Supercharger games

V5.1 : 29-Sep-2022 by Dave Bernazzani (wavemotion)
  * Improved all bankswitching for more speed and greater compatibility.
  * New F6 driver for DS-LITE to support 16K games - many at full speed!
  * Cleanups across the board.

V5.0 : 27-Sep-2022 by Dave Bernazzani (wavemotion)
  * Added new bankswitching scheme:  X07 (Stella's Stocking).
  * Improved 4K/2K driver to squeese out a couple more frames of performance.
  * New special F8 driver to support more 8K games on the DS-Lite.
  * Lots of cleanup across the board in the database to make more games playable.

V4.9 : 18-Sep-2022 by Dave Bernazzani (wavemotion)
  * Added new bankswitching schemes:  TVBOY, UASW and 0840 (Econobanking)
  * Fixed UA bankswitching scheme to allow more games to run

V4.8 : 11-Sep-2022 by Dave Bernazzani (wavemotion)
  * Fixed Super Bank (SB) scheme.

V4.7 : 24-Mar-2022 by Dave Bernazzani (wavemotion)
  * Added per-game configuration settings.
  * Fixed 3E/3F and UA bankswitching schemes.

V4.6 : 20-Mar-2022 by Dave Bernazzani (wavemotion)
  * Added SaveKey support - standard games will have a virtual SaveKey in the right controller 
    jack and will be auto-backed to a 32K /data/StellaDS.EE file.
  * Added EF, EFSC, DFSC and SB (SuperBank) support for games as large as 256K!
  
V4.5 : 05-Nov-2021 by Dave Bernazzani (wavemotion)
  * New sound output core - no more zingers!

V4.4 : 22-Aug-2021 by Dave Bernazzani (wavemotion)
  * Minor cleanups across the board. Improved AR cart speed by ~1FPS.
  * Added horizontal stretch - use the Left Shoulder + L/R D-Pad.
  * The Y button is now auto-fire.

V4.3 : 09-Aug-2021 by Dave Bernazzani (wavemotion)
  * Massive overhaul of the non-bank-switched engine so now the older DS-Lite
    and DS-Phat will run many games at full speed.

V4.2 : 02-Aug-2021 by Dave Bernazzani (wavemotion)
  * New overlay graphic for Star Raiders (plus manual)
  * A half dozen odd games got their controllers straightened out
  * About 100 graphical tweaks for various games to make them look as good as possible.

V4.1 : 28-July-2021 by Dave Bernazzani (wavemotion)
  * Found another 1-2% speed up in TIA rendering!
  * Bumper Bash now plays correctly (you can use the shoulder buttons too).
  * Kool-Aid-Man now plays correctly 
  * Meltdown graphics improvements (but not fixed... but playable).
  * Alpha Beam, Big Bird and Cookie Monster games all work with joystick controls now.

V4.0 : 24-July-2021 by Dave Bernazzani (wavemotion)
  * Found another 3-5% speed up in TIA rendering! Official Frogger now at 60FPS.
  * Press and hold L+R+A to swap LCD screens.
  * Other minor cleanups and polish.
  
V3.9 : 22-July-2021 by Dave Bernazzani (wavemotion)
  * Finally fixed graphical glitches in Rabbit Transit and Dragonstomper.
  * Many cleanups and improvements to the TIA handling.
  * Added UA and FE bankswitching detection schemes.
  * Another 100 games added to the compatibility database.
  
V3.8 : 18-July-2021 by Dave Bernazzani (wavemotion)
  * Massive update to database to support 50+ of the best Atari Hacks.
  * Support for WD bankswitching - Pursuit of the Pink Panther playable.
  * Genesis Controller support for all the awesome games that take advantage of it.
  * 120 Game Manuals!!

V3.7 : 12-July-2021 by Dave Bernazzani (wavemotion)
  * 100 Game Manuals!!
  * Fix for Thwocker prototypes.
  * Minor screen tweaks to tighten up some displays.
  * New info graphic.

V3.6 : 10-July-2021 by Dave Bernazzani (wavemotion)
  * PAL Support!  PAL games will render in proper 50Hz / 312 Scanlines.
  * Database of known games expanded from 850 to 1900.
  * Long rom filenames now scroll when selecting a game.
  * Total instruction manuals: 80!

V3.5 : 5-July-2021 by Dave Bernazzani (wavemotion)
  * Sound on Quadrun improved.
  * Total instruction manuals: 75!

V3.4 : 28-Jun-2021 by Dave Bernazzani (wavemotion)
  * Added new flicker reduction algorithms to help with games
    like Frogger, Asteroids, Astroblast, Pac-Man, etc.
  * Total instruction manuals: 50!

V3.3 : 26-Jun-2021 by Dave Bernazzani (wavemotion)
  * Added [TIME], [LOW] and [ALPHA] as high-score options.
  * Fixed small graphical glitches in F8, F6 and F4 games.
  * Another 5 instruction manuals added.

V3.2 : 24-Jun-2021 by Dave Bernazzani (wavemotion)
  * Vastly improved high score internals to support future enhancements.
  * Cleanup of paddle handling - games like Tac-Scan now work.

V3.1 : 22-Jun-2021 by Dave Bernazzani (wavemotion)
  * Up to 30 instruction manuals added.
  * Fix for difficulty switches (they reset on new game)
  * Fix for High Score initials so it recalls last known entry.

V3.0 : 21-Jun-2021 by Dave Bernazzani (wavemotion)
  * Reworked the page access and bank switching to eek out another 2-3% speed!
  * Cleanup for the CPU Core
  * More instruction manuals added (about 20 games total now)

V2.9 : 19-Jun-2021 by Dave Bernazzani (wavemotion)
  * High Score Support!
  * Preliminary Instruction Manual support (only a dozen games so far)

V2.8 : 11-Jun-2021 by Dave Bernazzani (wavemotion)
  * A couple dozen micro-tweaks to the sound driver to squeeze out a bit more
    performance. Elevators Amiss is now running at 60FPS! All of the Starpath
    Supercharger games are running in the high 50s and many are at 60FPS.

V2.7 : 4-Jun-2021 by Dave Bernazzani (wavemotion)
  * A few more games added to the compatibility list.
  * A minor optimization for fetching PC contents - yielding 1 or 2FPS in many games.

V2.6 : 3-Jun-2021 by Dave Bernazzani (wavemotion)
  * Fixed ability to scale/move screen with Paddle games.
  * Tweaked more than 100 game screen offsets to better center/show them by default.
  * Minor cleanups to the code and memory.

V2.5 : 1-Jun-2021 by Dave Bernazzani (wavemotion)
  * Restored PAL palette selection when choosing a game.
  * Eeked out 1 or 2 FPS on Starpath Supercharger games by limiting the number
    of vertical scanlines we will show. Elevators Amiss is now close to 54FPS!

V2.4 : 31-May-2021 by Dave Bernazzani (wavemotion)
  * A bit more juice squeezed out of the TIA. Added cart-specific options
    to bypass VerticalBlank zero (not all games need that memory cleared if
    we are dealing with a static screen... and this buys us CPU cycles!) and,
    somewhat more dangerously, HorizontalBlank clearing can be disabled for
    more speed. Only a few of the more stubborn games utilize these!
  * Removed PAL/NTSC option... only NTSC is supported (all games were released
    in NTSC except a dozen PAL exclusives which have long since been converted
    to run on NTSC - Search Atariage).
  * General cleanup and minor memory/code optimizations to get the most out 
    of the emulator.
  * Added 2 more lines of resolution before the top of the screen and 5 more
    below... this allows the games that utilize underscan and/or significant
    overscan to show properly.

V2.3 : 30-May-2021 by Dave Bernazzani (wavemotion)
  * And more improvement/stramline of TIA rendering to give a 10% boost
    for most games 0 this brings Elevators Amiss up to 51FPS and Phaser
    Patrol to 55 with gusts up to 59FPS!
  
V2.2 : 29-May-2021 by Dave Bernazzani (wavemotion)
  * Improved handling of TIA to trade off a little bit of speed on the fast
    games to gain 10-15% speed boost on the hard-to-render games. This makes
    games like Fantastic Voyage, Super Breakout, Elevators Amiss and many 
    other games run much closer to full speed!

V2.1 : 23-Mar-2021 by Dave Bernazzani (wavemotion)
  * Improved Screen Scaling (using Left/Right shoulder buttons + Arrow Keys)
  * Fix for file selection bug
  * Minor cleanup and improvements to squeeze out the last bit of juice from the emulation!

V2.0 : 26-Jan-2021 by Dave Bernazzani (wavemotion)
  * Faster AR processing rendering some of the more stubborn games playable.
  * Faster screen rendering for more complicated games. Simple games take a small
    performance hit (think: games with solid backgrounds) but those were already 
    running plenty fast enough - to we took the trade off to make some of the 
    more complicated games faster - this is an overall win for most games!
  * General cleanup - more games rendering properly (screen offset/scaling tweaks).

V1.9 : 24-Jan-2021 by Dave Bernazzani (wavemotion)
  * Screen tweaks to more than 400 games to make them look as good as possible.
  * More games added to the compatibility table - many homebrews.
  * Faster AR (Starpath Supercharger) processing so more of those games are playable.

V1.8 : 19-Jan-2021 by Dave Bernazzani (wavemotion)
  * Booster Grip Implemented (Omega Race and Thrust+ working)
  * Improved timing accuracy so games like Venture Reloaded work.
  * Implemented "AR" handling so Supercharger carts work.
  * Improved timing across the board... about 5% faster than 1.7
  * DPC+ Carts now recognized... and message will come up stating they are not supported.
  * L-TRIG+DPAD bumps screen offsets.   R-TRIG+DPAD changes Y-Scaling  

V1.7 : 31-Dec-2020 by Dave Bernazzani (wavemotion)
  * Very minor cleanup and polish.

V1.6 : 05-Dec-2020 by Dave Bernazzani (wavemotion)
  * Star Raiders working now.
  * All Tigervision (3F) games working.
  * Boulder Dash Demo V2 (3E game) works!
  * Minor graphical cleanups and better keypad touch input support.
  * Compatibility.txt list updated.

V1.5 : 05-Dec-2020 by Dave Bernazzani (wavemotion)
  * General cleanup. Ready for wider distribution.

V1.1k : 04-Dec-2020 by Dave Bernazzani (wavemotion)
  * Added Sound Toggle. Off gets you 10% speed boost.
  
V1.1j : 03-Dec-2020 by Dave Bernazzani (wavemotion)
  * Added info icon.
  * Better handling of touch screen.
  * Skip drawing frames that are off-screen.
  * Fixed sound gliches long present in the emulator.
  * Fixed color for Barnstorming bars (more red, less purple).
  * Other optimizations as time allowed.

V1.1h : 01-Dec-2020 by Dave Bernazzani (wavemotion)
  * Added full paddle emulation via new Paddle Icon
  * Added Keyboard / Keypad emulation via new Keypad Icon
  * Added Driver Controller emulation (for Indy500 only for now)
  * More core cleanups and minor timing improvements
  * Rebranding PHEONIX EDITION

V1.1g : 29-Nov-2020 by Dave Bernazzani (wavemotion)
  * Added paddle support - use the stylus on the bottom of screen.
  * New ROM mapping table for most acurate position of screen and will
    automatically enable the right controller and Flicker Free mode.
  * Better and more accurate timing (helps FPS measurement)
  * Switched to using unused VRAM to help with DMA copies.
  * Left/Right trigger to position screen now happens during Vertical Blank (cleaner).

V1.1f : 28-Nov-2020 by Dave Bernazzani (wavemotion)
  * Switched to dual-channel DMA memory copy for frame buffer. Slight speed improvement.
  * New button handling. See button map below the history section.
  * Added Flicker-Free switch... please use with caution as this takes significant CPU 
    power to make work as it ORs the previous two frames to help with 30Hz flicker that
    some games produce (Stellar Track, Yars Revenge, Asteroids, etc). 
  * Added ability to shift the screen up and down by 1 pixel per press of the shoulder buttons.

V1.1e : 27-Nov-2020 by Dave Bernazzani (wavemotion)
  * Slight improvement on TIA speed (1-2% speed improvement on many games).
  * Correctly identify games that use the right joystick (Home Run, Surround, Air-Sea Battle, 
    Wizard of Wor, Basketball, Star Ship and Slot Racers) and switch the DS controls
    to compensate (those games were unplayable before this).
  * Added Compatibility.txt for a list of games and their framerates.

V1.1d : 26-Nov-2020 by Dave Bernazzani (wavemotion)
  * Improved bank switching algorithm to speed it up. Makes F6/F8 games faster.
  * Improved speed in Pitfall II. Up to about 45 FPS - almost (but not quite) playable.
  * Faster display of ROMs list.
  * Emulator Version number now on ROM selection list.

V1.1c : 25-Nov-2020 by Dave Bernazzani (wavemotion)
  * Starting with RocketRobz 2-Apr-2020 baseline Stella DS codebase... 
  * Added graphical difficulty switches for both Left and Right players.
  * Used  a bit more of the DSi screen resolution so that the occasional missing graphic doesn't get 
    compressed away (e.g. if you fire the laser in Chopper Command you can see it sometimes disappears at 
    certain vertical positions of the Chopper... this is no longer an issue). Generally this results in a
    few missing rows of pixels at the very bottom of the screen which is a better trade-off for 95% of 
    games where almost no action takes place at the extremes.
  * Fixed all the on-screen touch button handling such that it's not so glitchy... press SELECT switch or
    START switch and it actually works every time (it's amazing what a little debounce code will do!). 
    I increased the hot-spots where you can press for each switch so that you can cleanly operate it with
    a finger or thumb... I hated getting out the Stylus for the small hit-boxes that were originally used.
  * Improved the emulator core to reorder a few operations, streamline a few others and one semi-major hack 
    that I'm not proud of to use a global integer for CPU cycles to avoid any structure/class overhead which
    yields a 5-15% speedup (depending on the game).  More games now run full speed and other games are elevated
    to "playable" with these speed improvements. 
  * Start and Select buttons now map to RESET (often used to start a game) and SELECT switches. That only seems logical. 
  * Remapped the FPS display to the right shoulder switch. I also only output the FPS once per second instead of 
    the wasteful 60x per second which means you can run with the FPS enabled with very little (but not zero!) impact on emulation.
  * Press (and hold) the left shoulder switch to run the game at max speed... some games run at 120FPS max and others can't even get to 60... but it's improving!
  * The Power Off button no longer makes that hideous screeching noise. And if you decide not to quit, it renders the screen back properly.
  * Fixed the ROM loading problems when the filenames were > 29 characters long.
  * Other minor improvements as time allowed...

V1.0 : 20/05/2011 (Alek Maul)
  * Initial release based on Stella 1.4.2 
  * Compiled with last version of Devkitpro/libnds, so DSi compatible \o/
  * Sound engine written from scratch
  * Video engine modified to refresh screen after each line
  * New palette (better colors) added to stella
  * New menu added to stella
  * Add support for colour / B&W TV switch (from Stella 2.2)
  * Add support for more cards recognition (3E)
  * Add support for PAL/NTSC palette
  
