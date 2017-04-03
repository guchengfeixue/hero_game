#if !defined(HEROGAME_H)
#include "herogame_platform.h"

/*
HEROGAME_INTERNAL:
0 - Build for public release
1 - Build for developer only

HEROGAME_SLOW:
0 - Not slow code allowed!
1 - Slow code welcom.
*/

#define internal static 
#define local_persist static 
#define global_variable static

#define Pi32 3.14159265359f

#if HEROGAME_SLOW
#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32 SafeTruncateUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFF);
	uint32 Result = (uint32)Value;
	return (Result);
}

inline game_controller_input *GetController(game_input *Input, unsigned int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return (Result);
}

#include "herogame_intrinsics.h"
#include "herogame_tile.h"

struct world
{
	tile_map *TileMap;
};

struct game_state
{
	tile_map_position PlayerP;
};

#define HEROGAME_H

#endif