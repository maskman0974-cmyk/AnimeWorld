
//{{BLOCK(shibuya)

//======================================================================
//
//	shibuya, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 907 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x32 
//	Total size: 512 + 29024 + 2048 = 31584
//
//	Time-stamp: 2026-03-30, 01:19:18
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_SHIBUYA_H
#define GRIT_SHIBUYA_H

#define shibuyaTilesLen 29024
extern const unsigned int shibuyaTiles[7256];

#define shibuyaMapLen 2048
extern const unsigned short shibuyaMap[1024];

#define shibuyaPalLen 512
extern const unsigned short shibuyaPal[256];

#endif // GRIT_SHIBUYA_H

//}}BLOCK(shibuya)
