
//{{BLOCK(bg)

//======================================================================
//
//	bg, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 677 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x32 
//	Total size: 512 + 21664 + 2048 = 24224
//
//	Time-stamp: 2026-03-29, 10:51:47
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_BG_H
#define GRIT_BG_H

#define bgTilesLen 21664
extern const unsigned int bgTiles[5416];

#define bgMapLen 2048
extern const unsigned short bgMap[1024];

#define bgPalLen 512
extern const unsigned short bgPal[256];

#endif // GRIT_BG_H

//}}BLOCK(bg)
