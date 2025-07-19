# Background

While reverse engineering AlphaWaves/Continuum (https://www.mobygames.com/game/939/continuum/) i have found out that the game files were compressed.
I disassembled the algorithm and found other games of **Infogrames Europe SA** that uses the same compression method.
There is no real use for gamers but maybe that helps people like me have an easier start in reversing/modding some of these games

* i created a ModdingWiki-Page with some CC0/CC1 infos: https://moddingwiki.shikadi.net/wiki/Alpha_Waves, https://moddingwiki.shikadi.net/wiki/CC1_Format
* My testproject for reversing findings https://github.com/LowLevelMahn/alpha_waves_loader
* ported the original 16bit DOS uncompression code (https://github.com/LowLevelMahn/alpha_waves_loader/blob/fc3269c686726227f3f5c7b044d4bc631ad2e87f/ae.asm#L970)
by creating my own simple emulator to convert the asm semantics directly to C/C++: https://github.com/LowLevelMahn/alpha_waves_loader/blob/main/read_some_file_sub_4/original_port.cpp
* did a cleanup and created a command line tool for uncompressing these files: https://github.com/LowLevelMahn/alpha_waves_loader/tree/main/tools/uncompress_cc

# How did i validate the uncompression code

* converted the orginal assembler code to semantical identical asm-like functions and then ported step by step - very slow :)
* tested my uncompression by hooking into Dosbox - overwriting the original code - doing regression tests and even run the game itself with my code
* i tried to keep the algorithm as picky as possible to detected any fails or loss of data etc. (any wrong change will normaly leads to endless-recursion or other fails very fast)

# What is this repo?

* Its a unit-test to test my reversed algorithm to uncompression CC0/CC1 files
* the unit-test gets compressed and uncompressed reference data from the included `test_data.bin` file (given by commandline) and just uncompress and compare the result
* the `test_data.bin` file contains compressed game data but is useless without knowing which part is from what game, unuseable for anything beside this test
* the unit-test does not contain the file-handling stuff for real CC0/CC1 files but only the uncompression-algorithm
* C++14, CMake - compiles/runs on Windows/Linux with clang,gcc,cl,CLion,QtCreator,MSVC,...

# Games with CC0 or CC1 packed files

* Call of Cthulhu: Shadow of the Comet (ES): https://www.mobygames.com/game/132/call-of-cthulhu-shadow-of-the-comet/, released 1994 on DOS
* Alone in the Dark: https://www.mobygames.com/game/325/alone-in-the-dark/, released 1992 on DOS
* Eternam: https://www.mobygames.com/game/1368/eternam, release 1992 on DOS
* Disney's Beauty and the Beast: https://www.mobygames.com/game/46990/disneys-beauty-and-the-beast/, release 1992 on DOS  
* 7 Colors: https://www.mobygames.com/game/11468/7-colors/, release 1991 on DOS
* Advantage Tennis: https://www.mobygames.com/game/4961/advantage-tennis/, release 1991 on DOS 
* Neptune: https://www.mobygames.com/game/4314/operation-neptune/, release 1991 on DOS
* Alpha Waves: https://www.mobygames.com/game/939/continuum/, released 1990 on DOS
* Mystical: https://www.mobygames.com/game/33025/mystical/, release 1990 on DOS
* The Light Corridor: https://www.mobygames.com/game/6904/the-light-corridor/, release 1990 on DOS
* North & South: https://www.mobygames.com/game/6926/north-south/, release 1990 on DOS
* Murders in Space: https://www.mobygames.com/game/4803/murders-in-space/, release 1990 on DOS
* Full Metal Planet: https://www.mobygames.com/game/6538/full-metal-planet/, release 1990 on DOS
* Wild Life: https://www.mobygames.com/game/54191/wild-life/, release 1990 on DOS
* Murders in Venice: https://www.mobygames.com/game/4793/murders-in-venice/, release 1990 on DOS
* Cool Ball, Pop-Up: https://www.mobygames.com/game/54514/cool-ball/, release 1990 on DOS
* Extase: https://www.mobygames.com/game/23277/extase/, release 1990 on DOS 
* Hostage: Rescue Mission, Operation Jupiter: https://www.mobygames.com/game/6939/hostage-rescue-mission/, released 1989 on DOS
* Drakkhen: https://www.mobygames.com/game/1969/drakkhen/, release 1989 on DOS
* The Quest for the Time-Bird: https://www.mobygames.com/game/6931/the-quest-for-the-time-bird/, release 1989 on DOS
* Tintin On The Moon: https://www.mobygames.com/game/9047/tintin-on-the-moon/, release 1989 on DOS
* Safari Guns: https://www.mobygames.com/game/14723/safari-guns/, release 1989 on DOS
* Shufflepuck Cafe: https://www.mobygames.com/game/3507/shufflepuck-cafe/, release 1989 on DOS
* Chicago 90: https://www.mobygames.com/game/1720/chicago-90/, release 1989 on DOS 
* Fire!: https://www.mobygames.com/game/54100/fire/, release 1989 on DOS
* Chamber of the Sci-Mutant Priestess, KULT: The Temple of Flying Saucers, Kult: https://www.mobygames.com/game/562/chamber-of-the-sci-mutant-priestess/, * released 1989 on DOS
* Teenage Queen: https://www.mobygames.com/game/6224/teenage-queen/, release 1988 on DOS
* Highway Patrol II: https://www.mobygames.com/game/14208/highway-patrol-ii/, release 1988 on DOS
maybe more...

Datailed file list: https://github.com/LowLevelMahn/alpha_waves_loader/blob/main/tools/uncompress_cc/known_files.txt

# How to find files with this compression?

search for byte pattern of the uncompression code

```
BF 01 03  mov di, 0x301
33 C0     xor ax, ax
F3 AB     rep stosw word ptr es:[di], ax
5F        pop di
07        pop es
```

use ripgrep

https://github.com/BurntSushi/ripgrep/releases - ripgrep-13.0.0-x86_64-pc-windows-msvc.zip no installation needed
inside the zip is a rg.exe file, a grep like search program that can also search for byte patterns

use it on the command line to find files with the relevant pattern

prints on console
```
rg.exe -l --binary -uuu (?-u:\xBF\x01\x03\x33\xC0\xF3\xAB\x5F\x07) "c:\all_your_games_folder"
```
redirects the output to c:\temp\out.txt

```
rg.exe -l --binary -uuu (?-u:\xBF\x01\x03\x33\xC0\xF3\xAB\x5F\x07) "c:\all_your_games_folder" > c:\temp\out.txt
```
