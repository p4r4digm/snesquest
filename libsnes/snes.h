#pragma once

#include "libutils/Defs.h"

#define SNES_SIZE_X 256
#define SNES_SIZE_Y 168
#define SNES_SCANLINE_WIDTH (SNES_SIZE_X * 2)
#define SNES_SCANLINE_COUNT (SNES_SIZE_Y)

#pragma pack(push, 1)


// color is 15-bit BGR, stored in 2 bytes
// keep in mind "BGR" is writen lef to right MSB to LSB
// but R is the LSB, and B is the MSB
// 0bbbbbgg gggrrrrr
typedef struct {
   byte2 r : 5, g : 5, b : 5;
} SNESColor;

/* stretch top 3 bits into lower 3 bits of target
   5bit :     43210
   target: 43210432*/
#define snesColorConverTo24Bit(IN) ((ColorRGBA){ .r = ((IN).r >> 2) | ((IN).r << 3), .g = ((IN).g >> 2) | ((IN).g << 3), .b = ((IN).b >> 2) | ((IN).b << 3), .a = 255})
#define snesColorConvertFrom24Bit(IN) ((SNESColor) { .r = (IN).r >> 3, .g = (IN).g >> 3, .b = (IN).b >> 3 })

typedef struct {
   union {
      //256-color bg palettes use all of cgram.  reminder that DCM doesnt use these
      SNESColor colors[256];

      //BG2 in mode7-extbg uses the first 128 colors
      struct {
         SNESColor colors[128];
      } bgPalette128;

      //most of the time, bgs use one of the first 8 16-color palettes in cgram
      struct {
         SNESColor colors[16];
      } bgPalette16s[8];

      //4-color bgs (NOT in mode 0) use the first 2 16-color bg pals split into 8 4-color pals
      struct {
         SNESColor colors[4];
      } bgPalette4s[8];

      //mode 0 splits the bg pals into 4 (one for each bg) sets of 8 4-color palettes
      struct {
         struct {
            SNESColor colors[4];
         } palette4s[8];
      } mode0BGPalettes[4];

      //objs always use one of 8 16-color palettes that sit in the 2nd half of cgram
      struct {
         SNESColor __unused[16][8];
         struct {
            SNESColor colors[16];
         } palette16s[8];
      } objPalettes;
   };   
}CGRAM;

//an 8x8 4-color character
typedef struct {
   struct {
      byte planes[2];
   } rows[8];
}Char4;

//an 8x8 16-color character
typedef struct {
   Char4 tiles[2];
}Char16;

//an 8x8 256-color character
typedef struct {
   Char16 tiles[2];
}Char256;

// Tile
typedef struct {
   union {

      //typical tile structure
      struct {
         byte2 character : 10, palette : 3, priority : 1, flipX : 1, flipY : 1;
      } tile;


      // For "Offsset-per-tile" modes (2, 4, and 6), BG3 reads its tiles differently
      // Rather than BG3 being a 32x32 tile map, in these modes it is effectively 32x2
      //    Row 0 governs the horizontal offset of the corresponding column in bgs 1/2
      //    Row 1 governs the vertical offset of the corresponding column in bgs 1/2
      // Additionally, tiles read for the column PRECEDING in BG3
      //    Tile1 in BG1 utilizes tile 0 in BG3 for horz offset and tile 16 for vert offset
      //    Tile0 is never affected by OPT
      // Mode 4 was unable to read both vertical and horizontal due to bandwidth constraints
      //    Thus, only row0 was read and bit15 "applyToVertical" determines whether to use offset for vert or horz
      // Finally, horizontal offset IGNORES THE LOWER 3 BITS when applying offset
      //    Thus, tiles can only be offset in 8-pixel increments
      //    The lower 3 bits of  x+bgnHorzOffs supplements this for smooth horizontal scrolling
      // Fun tip: bg's 1 and 3 can be set to have different tile size  and that works :S
      struct {
         byte2 
            //unsigned offset amount, for horizontal the 3LSBs are ignored (8-pixel steps)
            offset : 10, 

            //unused
            : 3, 

            //which BGs to apply OPT to, note that applytoBG2 only makes sense in Mode 2
            applyToBG1 : 1, applyToBG2 : 1, 

            //in mode4, determine whether offset 
            applyToVertical : 1;
      } opt; // offset per tile


      // in 256-color BG's, a Direct-Color-Mode (DCM) register can allow the tile to
      // repurpose the 13 bits for character and palette for direct BGR color.
      // getting the color out of dcm:
      // bgr are the LSBs of the colors from byte 0, organized as BBGGGRRR
      // Together, the 15bit color BBb00:RRRr0:GGGg0
      /* Color c = {0};
         c.r = (dcm.RRR << 2) | (dcm.r << 1);
         c.g = (dcm.GGG << 2) | (dcm.g << 1);
         c.b = (dcm.BB << 3) | (dcm.b << 2); */
      struct {
         union {
            byte RRR : 3, GGG : 3, BB : 2;
            byte colorRaw; // if 0 then transparent
         };         
         byte: 2, r : 1, g : 1, b : 1, priority : 1, flipX : 1, flipY : 1;
      } dcm; // direct color mode
   };
} Tile;

// Tilemaps consist of 32x32 tiles
typedef struct {
   Tile tiles[32 * 32];
}TileMap;

// vram is 64kb of tiles and characters
// where these sit in vram is set by registers
// obj characters are here as well.  They have two dedicated 16x16 character maps whose adresses are set by registers
//    these dont have to be adjecent in memory! there may be a gap, designated by another register
// tilemaps come in contigous blocks of 1, 2, or 4 depending on the size registers

// some helpful sizes
/*
      1 OBJ Character Map (16x16 8x8 16-color tiles) - 8kb
      1 BG Tilemap (32x32 2byte tiles) - 2kb
      1 4-color 8x8 tile - 16bytes
      1 16-color 8x8 tile - 32bytes
      1 256-color 8x8 tile - 64bytes

      Mode 1:
         BG0: 4 tilemaps: 8kb, 32byte tiles
         BG1: 4 tilemaps: 8kb, 32byte tiles
         BG2: 1 tilemap: 2kb, 16byte tiles
         2 OBJ Char Maps: 16kb
*/
typedef struct {
   union {
      byte raw[0x10000]; //64kb

      // a simple organization of vram for mode 1 is 4 tilemaps for each of bg1 and 2
      // followed by a full 1024-tile charmap and then the two obj charmaps
      // the bg3 tmap lives in the last 2kb of the big bg charmap
      // the 4-color characters for bg3 can be stored in 32-tile rows starting at any 8kb step
      // pretty simple to encode this into the beginning or middle of the big bgcmap
      // bgCMap has room for 960 tiles
      struct {

         //chip 1, 32kb
         TileMap bg1TMaps[4];
         TileMap bg2TMaps[4];

         struct {
            Char16 chars[16 * 16];
         }objCMaps[2];


         //chip 2, 32kb

         //the baseAdr for all 3 BGs is offsetOf(bgCMap)
         // but for bg1 and 2, skip the first 64
         struct {
            Char4 color4s[32 * 4]; //256 4-color characters to use with BG3
            Char16 color16s[16 * 56]; //64 rows minus 4 for bg3 chars and 4 at end for the bg3 tmap.  Starting char here would be Addr+64
         } bgCMap;         
         TileMap bg3TMap;         
      } mode1;

      // in mode 7, half of vram is taken over to  have a 128x128 tilemap (the 'playing field') next to 256 8x8 characters
      struct {
         union {            
            struct {
               byte tiles[128 * 128];

               //mode7 characters are linear, not bitplaned, 1 byte per pixel (256 colors)
               struct {
                  byte pixels[8 * 8];
               } characters[256];
            } BG1;

            // If the register for EXTBG is set, BG2 is enabled which reads the same VRAM data as BG1
            // but with lower color resolution due to the high bit becoming a priority bit.  
            // NOTE: BG2 utilizes the Mode7 Screen Scrolling Offset, NOT the BG2 offsets!
            struct {
               byte tiles[128 * 128];

               struct {
                  struct {
                     byte color : 7, priority : 1;
                  } pixels[8 * 8];
               } characters[256];
            } BG2;
         };

         byte raw[0x7FF];//32kb excess
      } mode7;
   };
}VRAM;

// Sprites (OBJs) are 4 bytes
typedef struct {
   byte 
      x, y, //screen coords
      character; //index into the character table

   byte 
      nameTable : 1, //objs have 2 characters maps, use 0 or 1
      palette : 3, 
      priority : 2, //objs have any of 4 priorities, up from bg's 2
      flipX : 1, flipY : 1; //these flip the whole sprite (even if it consists of multiple 8x8 tiles)
} Sprite;

// OAM holds up to 128 OBJs 
// Theres no concept of a 'null terminator'
typedef struct {
   //the primary table holds most data, outlined in the sprite struct
   Sprite primary[128];

   // a second table holds 2 bit per sprite
   // x9: MSB for X
   // sz: which of the two sprite sizes defined by registers to use
   struct {
      byte  
         x9_0 : 1, sz_0 : 1,
         x9_1 : 1, sz_1 : 1,
         x9_2 : 1, sz_2 : 1,
         x9_3 : 1, sz_3 : 1;
   } secondary[32];
} OAM;

enum OBJSIZE_TYPE {
   OBJSIZE_8x8_16x16 = 0,
   OBJSIZE_8x8_32x32 = 1,
   OBJSIZE_8x8_64x64 = 2,
   OBJSIZE_16x16_32x32 = 3,
   OBJSIZE_16x16_64x64 = 4,
   OBJSIZE_32x32_64x64 = 5,
   OBJSIZE_16x32_32x64 = 6,
   OBJSIZE_16x32_32x32 = 7
};

// Registers Struct
// Contains all register options for the PPU
// Excludes general-purpose read/write/operator registers
typedef struct {

   // 2101: objSizeAndBase
   // OBJ Size and Base
   // Base defines the character adress in VRAM
   struct {

      byte  baseAddr : 3, // Address in VRAM for base 0 (first of the two OBJ char tables)
                         // This value is in 16kb steps, so adress is &vram + (baseAddr << 14)
                         // vis a vi obj char maps can only start at 0, 1/4, 1/2, or 3/4 of the way through vram

            baseGap : 2, // the two OBJ 16x16 char tables dont need to be adjacent
                         // baseGap determined the distance between the end of base 0 and the start of base 1
                         // this value is in 8kb steps, the adress of base 1 is &vram + ((baseAdr) << 14) + ((baseGap + 1) << 13)
                         // vis a vi base 1 can start at 1/8, 1/4, 3/8, 1/2, 5/8, 3/4, or 7/8 depending on the location of base 0

            objSize : 3; // this defines which 2 sprite sizes are currently in use for OBJ rendering based on this table
                         // Value     Small  Big
                         // 000 (0) = 8 x8   16x16 
                         // 001 (1) = 8 x8   32x32 
                         // 010 (2) = 8 x8   64x64 
                         // 011 (3) = 16x16  32x32 
                         // 100 (4) = 16x16  64x64 
                         // 101 (5) = 32x32  64x64 
                         // 110 (6) = 16x32  32x64 
                         // 111 (7) = 16x32  32x32 
   } objSizeAndBase;


   // 2105: bgMode
   // BG Mode and Character Size
   struct {
      byte 
         //The video mode
         /* Mode     BG depth  OPT  Priorities
                     1 2 3 4        Front -> Back
            -=-------=-=-=-=----=---============---
             0       2 2 2 2    n    3AB2ab1CD0cd
             1       4 4 2      n    3AB2ab1C 0c
                        * if e set: C3AB2ab1  0c
             2       4 4        y    3A 2B 1a 0b
             3       8 4        n    3A 2B 1a 0b
             4       8 2        y    3A 2B 1a 0b
             5       4 2        n    3A 2B 1a 0b
             6       4          y    3A 2  1a 0
             7       8          n    3  2  1a 0
             7+EXTBG 8 7        n    3  2B 1a 0b    

             "OPT" means "Offset-per-tile mode". 
             For the priorities, numbers mean sprites with that priority. 
             Letters correspond to BGs (A=1, B=2, etc), with upper/lower case indicating tile priority 1/0. */
         mode : 3, 

         //this is a dedicated bit for modifying priority of BG3 while in Mode 1 (see table above)
         m1bg3pri : 1,

         //these sizes determine BG Character size (for tiles), 0 for 8x8, 1 for 16x16
         sizeBG1 : 1, sizeBG2 : 1, sizeBG3 : 1, sizeBG4 : 1;
   } bgMode;


   // 2106: mosaic
   // Mosaic Screen Pixelization
   // Causes BG pixels drawn to all use the color of the pixel in the top left of their "block"
   // This block is defined as a uniform grid of sizexsize
   struct {
      byte 
         // mosaic is turned on/off per-BG
         enableBG1 : 1, enableBG2 : 1, enableBG3 : 1, enableBG4 : 1, 
         
         // 0-15 for the mosaic block size
         size : 4;
   } mosaic;


   // 2107: bgSizeAndTileBase[4]
   // BG Size and Tile Base
   struct {
      byte 
         // instead of determining the size of tiles, this determines how many tilemaps to use.
         // sizeY will assume a 2nd 32x32 tielmap following the first in VRAM and will position it 'below' the first on the screen
         // sizeX will assume the same but position it to the left
         // sizeX AND sizeY together will assume 4 tilemaps in a row in order of (TopLeft, TopRight, BottomLeft, BottomRight)
         sizeY : 1, sizeX : 1, 

         // this value determines the address of the first tilemap inside VRAM
         // this value is in 2kb steps, so adress is &vram + (baseAddr << 11)
         baseAddr : 6;
   } bgSizeAndTileBase[4]; // one per layer (0:1, 1:2, etc)


   // 210b: bgCharBase
   // BG Character Base
   struct {
      // 4bits per BG for the base address of the characters in VRAM
      // This value is in 8kb steps, &vram + (baseAddr << 13)
      byte bg1 : 4, bg2 : 4;
      byte bg3 : 4, bg4 : 4;
   } bgCharBase;


   // 210d: bgScroll[4]
   // BG Scrolling
   // Each BG can specify 10 bits (unsigned 0-1024) for each of vertical and horizontal scrolling
   // In mode 7, bgScroll[0] uses 13-bit fixe dpoint offset vals and apply to both BG1 and BG2
   struct {
      union {
         struct {
            byte2 horzOffset : 10;
            byte2 vertOffset : 10;
         } BG;

         struct {
            //using 13-bit 2's-complement
            TwosComplement13 horzOffset, vertOffset;
         } M7; //only use this for BG1!    
      };
   } bgScroll[4];


   // 211a: mode7Settings
   // Mode 7 Options
   struct {
      byte
         // these bits flip the ENTIRE mode7 BG
         xFlip : 1, yFlip : 1,

         : 4, //unused

         // With mode7, its possible to get a tile index outside the 128x128 map
         // In that situation, these bits determine behavior:
         //    0 or 1: Wrap within the 128x128 tile area
         //    2: use transparent
         //    3: use tile 0
         screenOver: 2;

   } mode7Settings;


   // 211b: mode7Matrix
   // Mode 7 Rotation and Scaling Matrix
   struct {
      // using 16-bit fixed point (8frac, 7int, 1sign)
      FixedPoint a, b, c, d;
   } mode7Matrix;


   // 211f: mode7Origin
   // Mode 7 ROtation and Scaling Center Coordinates
   // Note: Horz/Vert scrolls for mode7 are set in bgScroll[0]
   struct {
      // using 13-bit 2's-complement
      TwosComplement13 x, y;
   } mode7Origin;


   // 2126: windowPosition
   // There are two windows defined as a horizontal range on the scanline
   // This range is INCLUSIVE of the pixels at both left and right
   struct {
      byte left, right;
   } windowPosition[2];


   // 2123: windowMaskSettings
   // Window Mask Settings
   // There are two windows which can define per-scaline masking
   // Each BG (as well as OBJ and "COLOR") can enable or invert each window
   // If enabled, pixels that fall outside the left and right values of the window (see below) will be ignored
   // If enabled and inverted, windows ignore every pixel BETWEEN left and right
   // for information on the "Color Window" see colorMathControl
   struct {
      byte  
         win1InvertBG1 : 1, win1EnableBG1 : 1, win2InvertBG1 : 1, win2EnableBG1 : 1,
         win1InvertBG2 : 1, win1EnableBG2 : 1, win2InvertBG2 : 1, win2EnableBG2 : 1;

      byte  
         win1InvertBG3 : 1, win1EnableBG3 : 1, win2InvertBG3 : 1, win2EnableBG3 : 1,
         win1InvertBG4 : 1, win1EnableBG4 : 1, win2InvertBG4 : 1, win2EnableBG4 : 1;

      byte  
         win1InvertOBJ : 1, win1EnableOBJ : 1, win2InvertOBJ : 1, win2EnableOBJ : 1,
         win1InvertCol : 1, win1EnableCol : 1, win2InvertCol : 1, win2EnableCol : 1;
   } windowMaskSettings;


   // 212a: windowMaskLogic
   // Window 1/2 Mask Logic
   // If both windows are enabled on a BG (or OBJ/Color), this allows them both to merge together
   // they are merged per-pixel with one of four operations (2 bits)
   // 00: OR, 01: AND, 10: XOR, 11: XNOR (inverse of XOR)
   struct {
      byte bg1 : 2, bg2 : 2, bg3 : 2, bg4 : 2;
      byte obj : 2, color : 2;
   } windowMaskLogic;


   // 212e: mainScreenMasking
   // Main Screen masking: Enable/Disable masking for the given BG/OBJ on the main screen
   struct {
      byte bg1 : 1, bg2 : 1, bg3 : 1, bg4 : 1, obj : 1;
   } mainScreenMasking;


   // 212f: subScreenMasking
   // Sub Screen masking: Enable/Disable masking for the given BG/OBJ on the sub screen
   struct {
      byte bg1 : 1, bg2 : 1, bg3 : 1, bg4 : 1, obj : 1;
   } subScreenMasking;


   // 212c: mainScreenDesignation
   // Main Screen Designation: Enable/Disable drawing of a given BG/OBJ to the main screen
   struct {
      byte bg1 : 1, bg2 : 1, bg3 : 1, bg4 : 1, obj : 1;
   } mainScreenDesignation;


   // 212d: subScreenDesignation
   // Sub Screen Designation: Enable/Disable drawing of a given BG/OBJ to the sub screen
   struct {
      byte bg1 : 1, bg2 : 1, bg3 : 1, bg4 : 1, obj : 1;
   } subScreenDesignation;


   // 2130: colorMathControl
   // Controls various settings for how color math is applied
   // First a subscreen pixel is determined based on what is designated toward it and whether or 
   // not the subscreen is set to draw layers
   // Then, the final pixel, if using color math, will have the sub pixel applied to it.
   // OBJs can participate in Color Math but ONLY for palettes 4-7.  OBJs using palettes 0-3 will never use color math
   struct {
      byte
         // In Modes with 256-color BGs, those BG tiles will now ignore 
         // their palettes and user their character data to generate a BGR color (see Tile::dcm)
         directColorMode : 1, 

         // If 1, Color Math will check BGs and OBJs for overlapping the backdrop
         // if 0, the backdrop color will always be used instead
         enableBGOBJ : 1, 

         //unused
         : 2, 

         // The "Color Window" refers to the portions of the scanline affected by color math
         // This can be modified from its default of being the full scanline byt having it utilize window masks (see window masks)

         // This allows 1 of 4 options for enabling/disabling color math
         //    0: Always enable
         //    1: Only enable when inside the color window
         //    2: Only enable when outside the color window
         //    3: Never enable
         colorMathEnable : 2, 

         // This allows 1 of 4 options for forcing the screen to black
         //    0: Always force
         //    1: Only force when inside the color window
         //    2: Only force when outside the color window
         //    3: Never force
         forceScreenBlack : 2;

      // This byte controls what layers partiicpate in Color Math and how it is applied
      byte 
         // Each BG, OBJs, or the backdrop can indiviually enable/disable colormath
         // note the inclusion of the backdrop which only applies when all layers above it at that pixel turn out transparent
         // again enabling for OBJs will only work for OBJs not utilizing palettes 0-3
         bg1 : 1, bg2 : 1, bg3 : 1, bg4 : 1, obj : 1, backDrop : 1, 
         
         // if set to 1, the final color value is halved
         halve : 1, 
         
         // 0: main and are added. if 1, subtracted
         addSubtract : 1;

   } colorMathControl;


   // 2132: Color Math Subscreen Backdrop Color
   // This is the color used for the subscreen pixel if all BJ/OBJs are transparent at that pixel (or if enableBGOBJ is 0)
   SNESColor fixedColorData;


   // 2133: Some special screen functionalities, we only realy care about extbg and hires
   struct {
      byte screenInterlace : 1, objInterlace : 1, overscanMode : 1, 
         
         // Modes 4, 5, and 6 utilize a hires mode where the subscreen is shifted a halfdot to the right
         // and interleaved with the main screen
         // this bit enables it in all modes,
         pseudoHiResMode : 1, 
         
         : 2, 
         
         // In mode 7, this bit enables the use of BG2 which takes on some interesting properties, see VRAM::mode7::BG2
         mode7EXTBG : 1,          
         
         externalSync : 1;
   } screenSettings;


}Registers;

typedef struct SNES_t{
   CGRAM cgram;
   VRAM vram;
   OAM oam;
   Registers reg;
} SNES;

//output is 512x168 32-bit color RGBA
enum {
   SNES_RENDER_DEBUG_WHITE = 1<<0
};
void snesRender(SNES *self, ColorRGBA *out, int flags);

#pragma pack(pop)

// A character map inside VRAM
typedef struct CMap CMap;

// 'rows' are sets of 32 4-color characters (16 bytes each)
// baseAddr follows the cmaps baseaddr scheme of 8kb steps (vram + (baseAddr << 13))
CMap *cMapCreate(VRAM *vram, byte baseAddr, byte rowOffset, byte rowCount);
void cMapDestroy(CMap *self);
void cMapCommit(CMap *self);// push all blocks to vram

// this is an arbitrarily-sized grid of characters
// allocated by a CMap.  The CMap will defreg and reorganize this block to make sense in vram
typedef struct CMapBlock CMapBlock;

// allocates a block of a cmap for writing and use
// if there is no room, cmap will attempt to defrag to make room
// if it still cant find space for it it will return null
// colorDepth is the bitcount for tile color, (2, 4, 8)->(4,16,256)
CMapBlock *cMapAlloc(CMap *cmap, byte colorDepth, byte2 width, byte2 height, byte tileWidth, byte tileHeight);
void cMapFree(CMap *cmap, CMapBlock *block);

//pushes bitplaned chardata to the block.  The assumption is that data is correctly sized!
void cMapBlockSetCharacters(CMapBlock *block, Char4 *data);

// this will take an x,y  for coordinates into the full block and
// return the correct character index inside vram based on how it is organized
// x/y can remain consistent regardless of how the CMap reorganizes or splits the blocks
byte2 cMapBlockGetCharacter(CMapBlock *block, byte2 x, byte2 y);
