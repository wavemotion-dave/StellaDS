StellaDS
--------------------------------------------------------------------------------
StellaDS is an Atari 2600 console emulator.
To use this emulator, you must use compatibles rom with a26/bin format. 
Do not ask me about such files, I don't have them. A search with Google will certainly 
help you. 

Features :
----------
 Most things you should expect from an emulator.

Missing :
---------
 All that is not yet emulated ;)
 Need to improve speed

Check updates on my web site : 
http://www.portabledev.com

--------------------------------------------------------------------------------
History :
--------------------------------------------------------------------------------
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


V1.0 : 20/05/2011
  * Initial release based on Stella 1.4.2 
  * Compiled with last version of Devkitpro/libnds, so DSi compatible \o/
  * Sound engine written from scratch
  * Video engine modified to refresh screen after each line
  * New palette (better colors) added to stella
  * New menu added to stella
  * Add support for colour / B&W TV switch (from Stella 2.2)
  * Add support for more cards recognition (3E)
  * Add support for PAL/NTSC palette
  
--------------------------------------------------------------------------------
How to use StellaDS :
--------------------------------------------------------------------------------
YOU NEED PERHAPS TO PATCH THE NDS FILE WITH THE DLDI PATCH BEFORE USING IT. 
Unzip StellaDS.nds from the StellaDS.zip archive in a directory of your flash / (micro) SD 
/ MMC card.
Put the a26/bin files where you want on your flashcard. 

That's all, StellaDS can be use now :) !

When the emulator starts, click on the cartridge slot to choose a file. you are use Up/Down 
to select a file, then use A to load it.

Controls :
 * D-Pad   : the Joystick
 * A       : Fire button
 * SELECT  : SELECT switch
 * START   : START switch
 * R-Trig  : FPS Display
 * L-Trig  : Press and Hold for MAX Speed
 
 Use stylus on buttons for other actions on bottom screen.
--------------------------------------------------------------------------------
Credits:
--------------------------------------------------------------------------------
Thanks Wintermute for devkitpro and libnds (http://www.devkitpro.org).
Thanks TheChucksters to understand how Stella emulator works with the release of
his source code conversion (http://www.charlesmoyes.com/drupal/)
Thanks Stella Team for this marvelous emulator (http://stella.sourceforge.net/).

--------------------------------------------------------------------------------
Alekmaul
alekmaul@portabledev.com
http://www.portabledev.com
--------------------------------------------------------------------------------
