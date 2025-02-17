#include "Game.h"
#include "libutils/CheckedMemory.h"
#include "LogSpud.h"
#include "App.h"
#include "AppData.h"
#include "snes.h"
#include "DBAssets.h"

struct Game_t {
   int UNUSED;
};

static void _setupTestSNES(SNES *snes, AppData *data) {

   snes->reg.bgMode.mode = 1;
   snes->reg.bgMode.m1bg3pri = 1;

   snes->reg.bgSizeAndTileBase[0].baseAddr = 0;
   snes->reg.bgSizeAndTileBase[0].sizeX = 0;
   snes->reg.bgSizeAndTileBase[0].sizeY = 0;

   snes->reg.bgSizeAndTileBase[1].baseAddr = 4;
   snes->reg.bgSizeAndTileBase[1].sizeX = 0;
   snes->reg.bgSizeAndTileBase[1].sizeY = 0;

   snes->reg.bgSizeAndTileBase[2].baseAddr = 31;
   snes->reg.bgSizeAndTileBase[2].sizeX = 0;
   snes->reg.bgSizeAndTileBase[2].sizeY = 0;

   snes->reg.objSizeAndBase.baseAddr = 1;
   snes->reg.objSizeAndBase.baseGap = 0;
   snes->reg.objSizeAndBase.objSize = OBJSIZE_32x32_64x64;

   snes->reg.bgCharBase.bg1 = 4;
   snes->reg.bgCharBase.bg2 = 4;
   snes->reg.bgCharBase.bg3 = 4;

   //snes->reg.bgMode.sizeBG1 = 1;

   snes->reg.colorMathControl.enableBGOBJ = 1;
   //snes->reg.colorMathControl.halve = 1;
   //snes->reg.colorMathControl.addSubtract = 1;
   snes->reg.colorMathControl.bg1 = 1;
   snes->reg.mainScreenDesignation.bg1 = 1;   
   snes->reg.mainScreenDesignation.bg3 = 1;
   snes->reg.mainScreenDesignation.obj = 0;
   snes->reg.subScreenDesignation.obj = 1;
   snes->reg.mosaic.enableBG1 = 1;




   DBCharacterMaps hades = dbCharacterMapsSelectFirstByid(data->db, 25);
   CMap *hmap = cMapCreate(&snes->vram, 2, 2, 32);
   CMapBlock *hblock = cMapAlloc(hmap, 4, hades.width, hades.height, 8, 8);
   cMapBlockSetCharacters(hblock, hades.data);
   cMapCommit(hmap);
   snes->oam.primary[0].character = cMapBlockGetCharacter(hblock, 0, 0);

   data->testX = 28;
   data->testY = 58;

   //memcpy(data->snes->vram.mode1.objCMaps, hades.data, hades.dataSize);   

   vec(DBCharacterEncodePalette) *pals = dbCharacterEncodePaletteSelectBycharacterMapId(data->db, hades.id);
   vecForEach(DBCharacterEncodePalette, p, pals, {
      DBPalettes dbp = dbPalettesSelectFirstByid(data->db, p->paletteId);
      memcpy(&data->snes->cgram.objPalettes.palette16s[p->index], dbp.colors, dbp.colorsSize);
      dbPalettesDestroy(&dbp);
   });

   memcpy(&data->snes->cgram.objPalettes.palette16s[1], &data->snes->cgram.objPalettes.palette16s[0], sizeof(data->snes->cgram.objPalettes.palette16s[0]));

   vecDestroy(DBCharacterEncodePalette)(pals);
   dbCharacterMapsDestroy(&hades);

   DBCharacterMaps bg = dbCharacterMapsSelectFirstByid(data->db, 29);

   CMap *map = cMapCreate(&snes->vram, 4, 4, 60);
   CMapBlock *block = cMapAlloc(map, 4, 30, 19, 8, 8);
   cMapBlockSetCharacters(block, bg.data);
   cMapCommit(map);


   //memcpy(&data->snes->vram.mode1.bgCMap, bg.data, bg.dataSize);

   byte2 x = 0, y = 0;
   for (y = 0; y < bg.height; ++y) {
      for (x = 0; x < bg.width; ++x) {
         int i = y * 32 + x;
         Tile *t = &snes->vram.mode1.bg1TMaps[0].tiles[i];


         t->tile.palette = *((byte*)bg.tilePaletteMap + (y*bg.width +x));
         t->tile.character = cMapBlockGetCharacter(block, x, y);
         t->tile.priority = 1;

         //byte2 foo = cMapBlockGetCharacter(block, x, y);
         //byte2 foo2 = (y * bg.width + x);

         //t->tile.character = (y * bg.width + x);
      }
   }

   DBCharacterMaps txt = dbCharacterMapsSelectFirstByid(data->db, 28);
   CMap *map2 = cMapCreate(&snes->vram, 4, 0, 4);
   cMapAlloc(map2, 2, 1, 1, 8, 8);
   CMapBlock *block2 = cMapAlloc(map2, 2, 16, 4, 8, 8);
   cMapBlockSetCharacters(block2, txt.data);
   cMapCommit(map2);

   for (y = 0; y < 4; ++y) {
      for (x = 0; x < txt.width; ++x) {
         int i = y * 32 + x;
         Tile *t = &snes->vram.mode1.bg3TMap.tiles[i];

         t->tile.palette = 3;
         t->tile.character = cMapBlockGetCharacter(block2, x, y);
         t->tile.priority = 1;
      }
   }
   

   pals = dbCharacterEncodePaletteSelectBycharacterMapId(data->db, bg.id);
   vecForEach(DBCharacterEncodePalette, p, pals, {
      DBPalettes dbp = dbPalettesSelectFirstByid(data->db, p->paletteId);
      memcpy(&data->snes->cgram.bgPalette16s[p->index], dbp.colors, dbp.colorsSize);
      dbPalettesDestroy(&dbp);
   });

   vecDestroy(DBCharacterEncodePalette)(pals);

   pals = dbCharacterEncodePaletteSelectBycharacterMapId(data->db, txt.id);
   vecForEach(DBCharacterEncodePalette, p, pals, {
      DBPalettes dbp = dbPalettesSelectFirstByid(data->db, p->paletteId);
      memcpy(&data->snes->cgram.bgPalette16s[p->index + 3], dbp.colors, dbp.colorsSize);
      dbPalettesDestroy(&dbp);
   });

   vecDestroy(DBCharacterEncodePalette)(pals);

   dbCharacterMapsDestroy(&hades);


   //int i = 0;

   //SNESColor *pColor = &snes->cgram.objPalettes.palette16s[0].colors[1];
   //pColor->r = 31;

   //pColor = &snes->cgram.objPalettes.palette16s[0].colors[2];
   //pColor->b = 31;

   //pColor = &snes->cgram.objPalettes.palette16s[0].colors[3];
   //pColor->g = 31;

   //Char16 *testChar = (Char16*)&snes->vram;

   //for (i = 0; i < 8; ++i) {
   //   testChar->tiles[0].rows[i].planes[i % 2] = 255;

   //   (testChar + 1)->tiles[0].rows[i].planes[i % 2] = 255;
   //   (testChar + 1)->tiles[0].rows[i].planes[1] = 255;

   //   (testChar + 16)->tiles[0].rows[i].planes[i % 2] = 255;
   //   (testChar + 16)->tiles[0].rows[i].planes[1] = 255;

   //   (testChar + 17)->tiles[0].rows[i].planes[!(i % 2)] = 255;
   //}

   //snes->reg.objSizeAndBase.objSize = OBJSIZE_32x32_64x64;
   //snes->oam.objCount = 128;

   
   return;

}

Game *gameCreate(AppData *data) {
   Game *out = checkedCalloc(1, sizeof(Game));

   

   return out;
}
void gameDestroy(Game *self) {
   checkedFree(self);
}

void gameStart(Game *self, AppData *data) {
   _setupTestSNES(data->snes, data);
}

static int counter = 0, counter2 = 0;

void gameUpdate(Game *self, AppData *data) {
   int x = 0, y = 0;

   Int2 n = data->window->nativeResolution;
   const Recti nativeViewport = { 0, 0, n.x, n.y };

   int xCount = 2;
   int yCount = 2;
   int spacing = 64;

   data->snes->reg.bgScroll[0].BG.horzOffset = (byte2)data->testBGX;
   data->snes->reg.bgScroll[0].BG.vertOffset = (byte2)data->testBGY;

   data->snes->reg.mosaic.size = (byte)(MAX(data->testMosaic, 0));

   for (y = 0; y < yCount; ++y) {
      for (x = 0; x < xCount; ++x) {
         int idx = y * xCount + x;

         TwosComplement9 testX = { data->testX + x*spacing };
         if (testX.raw >= 256) {
            testX.raw -= 512;
         }

         switch (idx % 4) {
         case 0:
            data->snes->oam.secondary[idx / 4].x9_0 = testX.twos.sign;
            data->snes->oam.secondary[idx / 4].sz_0 = 1;
            break;
         case 1:
            data->snes->oam.secondary[idx / 4].x9_1 = testX.twos.sign;
            data->snes->oam.secondary[idx / 4].sz_1 = 1;
            break;
         case 2:
            data->snes->oam.secondary[idx / 4].x9_2 = testX.twos.sign;
            data->snes->oam.secondary[idx / 4].sz_2 = 1;
            break;
         case 3:
            data->snes->oam.secondary[idx / 4].x9_3 = testX.twos.sign;
            data->snes->oam.secondary[idx / 4].sz_3 = 1;
            break;
         }
         //self->snes.oam.secondary[idx].x9_0 = testX.twos.sign;
         data->snes->oam.primary[idx].x = testX.twos.value;
         data->snes->oam.primary[idx].y = data->testY + y*spacing;
         data->snes->oam.primary[idx].priority = 3;
         data->snes->oam.primary[idx].palette = 1;

         if (x % 2) {
            data->snes->oam.primary[idx].flipX = 1;
         }

         if (y % 2) {
            data->snes->oam.primary[idx].flipY = 1;
         }
      }
   }


   
   memcpy(&data->snes->cgram.objPalettes.palette16s[1], &data->snes->cgram.objPalettes.palette16s[0], sizeof(data->snes->cgram.objPalettes.palette16s[0]));
   byte c = 0;
   for (c = 0; c < 16; ++c) {
      int amt = (counter % 62);
      
      SNESColor *col = &data->snes->cgram.objPalettes.palette16s[1].colors[c];

      if (amt >= 31) {
         data->snes->reg.mainScreenDesignation.obj = 1;
         data->snes->reg.subScreenDesignation.obj = 0;
         
         //amt -= 31;

         //col->b = MIN(31, MAX(0, col->b - amt));
         //col->g = MIN(31, MAX(0, col->g - amt));
      }
      else {
         data->snes->reg.mainScreenDesignation.obj = 0;
         data->snes->reg.subScreenDesignation.obj = 1;

         col->r = MIN(31, MAX(0, col->r - amt));
         col->b = col->r;
         col->g = 0;
      }


         

      

   }
   if (++counter2 % 4 == 0) {
      ++counter;
   }
   
}
