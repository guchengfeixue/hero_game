#if !defined(HEROGAME_TILE_H)

struct tile_map_position
{
	uint32 AbsTileX;
	uint32 AbsTileY;

	real32 TileRelX;			// tilemapÖÐÏà¶ÔÓÚ¸ñ×ÓÄÚµÄÎ»ÖÃx
	real32 TileRelY;			// tilemapÖÐÏà¶ÔÓÚ¸ñ×ÓÄÚµÄÎ»ÖÃy
};

struct tile_chunk_position 
{
	uint32 TileChunkX;
	uint32 TileChunkY;

	uint32 RelTileX;
	uint32 RelTileY;
};

struct tile_chunk
{
	uint32 *Tiles;
};

struct tile_map
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


#define HEROGAME_TILE_H
#endif
