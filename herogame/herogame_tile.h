#if !defined(HEROGAME_TILE_H)

struct tile_map_difference
{
	v2 dXY;
	real32 dZ;
};

struct tile_map_position
{
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;

	v2 Offset_;
};

struct tile_chunk_position 
{
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;

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

	real32 TileSideInMeters;

	uint32 TileChunkCountX;
	uint32 TileChunkCountY;
	uint32 TileChunkCountZ;

	tile_chunk *TileChunks;
};


#define HEROGAME_TILE_H
#endif
