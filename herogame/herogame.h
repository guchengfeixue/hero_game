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

struct tile_chunk_position 
{
	uint32 TileChunkX;
	uint32 TileChunkY;

	uint32 RelTileX;
	uint32 RelTileY;
};

struct world_position
{
	uint32 AbsTileX;
	uint32 AbsTileY;

	real32 TileRelX;			// tilemap中相对于格子内的位置x
	real32 TileRelY;			// tilemap中相对于格子内的位置y
};

struct tile_chunk
{
	uint32 *Tiles;
};

struct world
{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	real32 TilesSideInMeters;
	int32 TileSideInPixels;
	real32 MetersToPixels;

	int32 TileChunkCountX;
	int32 TileChunkCountY;

	tile_chunk *TileChunks;
};

struct game_state
{
	world_position PlayerP;
};

#define HEROGAME_H

#endif