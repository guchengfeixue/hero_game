#include "herogame.h"
#include "herogame_tile.cpp"
#include "herogame_random.h"

void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHZ)
{
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHZ;
	
	int16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
#if 0
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
		int16 SampleValue = 0;
#endif

		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
#if 0
		GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
		if (GameState->tSine > 2.0f * Pi32)
		{
			GameState->tSine -= 2.0f * Pi32;
		}
#endif
	}
}

internal void DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax ,real32 R, real32 G, real32 B)
{
	int32 MinX = RoundReal32ToInt32(vMin.X);
	int32 MinY = RoundReal32ToInt32(vMin.Y);
	int32 MaxX = RoundReal32ToInt32(vMax.X);
	int32 MaxY = RoundReal32ToInt32(vMax.Y);

	if (MinX < 0)
		MinX = 0;
	if (MinY < 0)
		MinY = 0;
	if (MaxX > Buffer->Width)
		MaxX = Buffer->Width;
	if (MaxY > Buffer->Height)
		MaxY = Buffer->Height;

	uint32 Color = (uint32)((RoundReal32ToUInt32(R * 255.0f) << 16) |
							(RoundReal32ToUInt32(G * 255.0f) << 8) |
							(RoundReal32ToUInt32(B * 255.0f) << 0));

	uint8 *Row = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

internal void DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, real32 RealX, real32 RealY,
						int32 AlignX = 0, int32 AlignY = 0, real32 CAlpha = 1.0f)
{

	RealX -= (real32)AlignX;
	RealY -= (real32)AlignY;

	int32 MinX = RoundReal32ToInt32(RealX);
	int32 MinY = RoundReal32ToInt32(RealY);
	int32 MaxX = MinX + Bitmap->Width;
	int32 MaxY = MinY + Bitmap->Height;

	int32 SourceOffsetX = 0;
	if (MinX < 0)
	{ 
		SourceOffsetX = -MinX;
		MinX = 0;
	}

	int32 SourceOffsetY = 0;
	if (MinY < 0)
	{
		SourceOffsetY = -MinY;
		MinY = 0;
	}
	if (MaxX > Buffer->Width)
		MaxX = Buffer->Width;
	if (MaxY > Buffer->Height)
		MaxY = Buffer->Height;

	uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
	SourceRow += -Bitmap->Width * SourceOffsetY + SourceOffsetX;
	uint8 *DestRow = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

	for (int Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Dest = (uint32 *)DestRow;
		uint32 *Source = SourceRow;
		for (int X = MinX; X < MaxX; ++X)
		{
			real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
			A *= CAlpha;

			real32 SR = (real32)((*Source >> 16) & 0xFF);
			real32 SG = (real32)((*Source >> 8) & 0xFF);
			real32 SB = (real32)((*Source >> 0) & 0xFF);

			real32 DR = (real32)((*Dest >> 16) & 0xFF);
			real32 DG = (real32)((*Dest >> 8) & 0xFF);
			real32 DB = (real32)((*Dest >> 0) & 0xFF);

			real32 R = (1.0f - A) * DR + A * SR;
			real32 G = (1.0f - A) * DG + A * SG;
			real32 B = (1.0f - A) * DB + A * SB;

			*Dest = (((uint32)(R + 0.5f) << 16) |
				((uint32)(G + 0.5f) << 8) |
				((uint32)(B + 0.5f) << 0));

			++Dest;
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow -= Bitmap->Width;
	}
}

#pragma pack(push, 1)
struct bitmap_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Plances;
	uint16 BitsPerPixel;

	uint32 Compression;
	uint32 SizeOfBitmap;
	int32 HorzResolution;
	int32 VertResolution;
	uint32 ColorsUser;
	uint32 ColorsImportant;

	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
{
	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
	if (ReadResult.ContentsSize != 0)
	{
		bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
		uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		Assert(Header->Compression == 3);

		uint32 RedMask = Header->RedMask;
		uint32 GreenMask = Header->GreenMask;
		uint32 BlueMask = Header->BlueMask;
		uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

		bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

		Assert(RedScan.Found);
		Assert(GreenScan.Found);
		Assert(BlueScan.Found);
		Assert(AlphaScan.Found);

		int32 RedShift = 16 - (int32)RedScan.Index;
		int32 GreenShift = 8 - (int32)GreenScan.Index;
		int32 BlueShift = 0 - (int32)BlueScan.Index;
		int32 AlphaShift = 24 - (int32)AlphaScan.Index;

		uint32 *SourceDest = Pixels;
		for (int32 Y = 0; Y < Header->Height; Y++)
		{
			for (int32 X = 0; X < Header->Width; X++)
			{
				uint32 C = *SourceDest;
				*SourceDest++ = (RotateLeft(C & RedMask, RedShift) |
								 RotateLeft(C & GreenMask, GreenShift) |
								 RotateLeft(C & BlueMask, BlueShift) |
								 RotateLeft(C & AlphaMask, AlphaShift));
			}
		}
	}
	return (Result);
}

internal void ChangeEntityResidence(game_state *GameState, uint32 EntityIndex, entity_residence Residence)
{
	if (Residence == EntityResidence_High)
	{
		if (GameState->EntityResidence[EntityIndex] != EntityResidence_High)
		{
			high_entity *EntityHigh = &GameState->HighEntities[EntityIndex];
			dormant_entity *EntityDormant = &GameState->DormantEntities[EntityIndex];

			tile_map_difference Diff = Subtract(GameState->World->TileMap, &EntityDormant->P, &GameState->CameraP);
			EntityHigh->P = Diff.dXY;
			EntityHigh->dP = V2(0, 0);
			EntityHigh->AbsTileZ = EntityDormant->P.AbsTileZ;
			EntityHigh->FacingDirection = 0;
		}
	}
	GameState->EntityResidence[EntityIndex] = Residence;
}

inline entity GetEntity(game_state *GameState, entity_residence Residence, uint32 Index)
{
	entity Entity = {};
	if ((Index > 0) && (Index < GameState->EntityCount))
	{
		if (GameState->EntityResidence[Index] < Residence)
		{
			ChangeEntityResidence(GameState, Index, Residence);
			Assert(GameState->EntityResidence[Index] >= Residence);
		}
		Entity.Residence = Residence;
		Entity.Dormant = &GameState->DormantEntities[Index];
		Entity.Low = &GameState->LowEntities[Index];
		Entity.High = &GameState->HighEntities[Index];
	}
	return (Entity);
}

internal uint32 AddEntity(game_state *GameState, entity_type Type)
{
	uint32 EntityIndex = GameState->EntityCount++;
	Assert(GameState->EntityCount < ArrayCount(GameState->DormantEntities));
	Assert(GameState->EntityCount < ArrayCount(GameState->LowEntities));
	Assert(GameState->EntityCount < ArrayCount(GameState->HighEntities));

	GameState->EntityResidence[EntityIndex] = EnttiyResidence_Dormant;
	GameState->DormantEntities[EntityIndex] = {};
	GameState->LowEntities[EntityIndex] = {};
	GameState->HighEntities[EntityIndex] = {};
	GameState->DormantEntities[EntityIndex].Type = Type;

	return (EntityIndex);
}

internal uint32 AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	uint32 EntityIndex = AddEntity(GameState, EntityType_Wall);
	entity Entity = GetEntity(GameState, EnttiyResidence_Dormant, EntityIndex);

	Entity.Dormant->P.AbsTileX = AbsTileX;
	Entity.Dormant->P.AbsTileY = AbsTileY;
	Entity.Dormant->P.AbsTileZ = AbsTileZ;
	Entity.Dormant->Height = GameState->World->TileMap->TileSideInMeters;
	Entity.Dormant->Width = Entity.Dormant->Height;
	Entity.Dormant->Collides = true;

	//ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);
	return (EntityIndex);
}

internal uint32 AddPlayer(game_state *GameState)
{
	uint32 EntityIndex = AddEntity(GameState, EntityType_Hero);
	entity Entity = GetEntity(GameState, EnttiyResidence_Dormant, EntityIndex);

	Entity.Dormant->P.AbsTileX = 1;
	Entity.Dormant->P.AbsTileY = 3;
	Entity.Dormant->P.Offset_.X = .0f;
	Entity.Dormant->P.Offset_.Y = .0f;
	Entity.Dormant->Height = 0.5f;
	Entity.Dormant->Width = 1.0f;
	Entity.Dormant->Collides = true;

	ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);

	if (GetEntity(GameState, EnttiyResidence_Dormant,GameState->CameraFollowingEntityIndex).Residence == EntityResidence_Nonexistent)
		GameState->CameraFollowingEntityIndex = EntityIndex;
	return (EntityIndex);
}

internal bool32 TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY, real32 *tMin, real32 MinY, real32 MaxY)
{
	bool32 Hit = false;

	real32 tEpsilon = 0.001f;
	if (PlayerDeltaX != 0.0f)
	{
		real32 tResult = (WallX - RelX) / PlayerDeltaX;
		real32 Y = RelY + tResult * PlayerDeltaY;
		if ((tResult >= 0.0f) && (*tMin > tResult))
		{
			if ((Y >= MinY) && (Y <= MaxY))
			{
				*tMin = Maximum(0.0f, tResult - tEpsilon);
				Hit = true;
			}
		}
	}
	return Hit;
}

internal void MovePlayer(game_state *GameState, entity Entity, real32 dt, v2 ddP)
{
	tile_map *TileMap = GameState->World->TileMap;

	real32 ddPLength = LengthSq(ddP);
	if (ddPLength > 1.0f)
		ddP *= (1.0f / SquareRoot(ddPLength));

	real32 PlayerSpeed = 50.0f;
	ddP *= PlayerSpeed;
	ddP += -8.0f * Entity.High->dP;

	v2 OldPlayerP = Entity.High->P;
	v2 PlayerDelta = (0.5f * ddP * Square(dt) + Entity.High->dP* dt);
	Entity.High->dP = ddP * dt + Entity.High->dP;
	v2 NewPlayerP = OldPlayerP + PlayerDelta;
	/*
	uint32 MinTileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32 MinTileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

	uint32 MaxTileX = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32 MaxTileY = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

	//NOTE: Minkowski sum
	uint32 EntityTileWidth = CeilReal32ToInt32(Entity.High->Width / TileMap->TileSideInMeters);
	uint32 EntityTileHeight = CeilReal32ToInt32(Entity.High->Height / TileMap->TileSideInMeters);
	MinTileX -= EntityTileWidth;
	MinTileY -= EntityTileHeight;
	MaxTileX += EntityTileWidth;
	MaxTileY += EntityTileHeight;

	uint32 AbsTileZ = Entity.High->P.AbsTileZ;
	*/
	for (uint32 Iteration = 0; Iteration < 4; ++Iteration)
	{
		real32 tMin = 1.0f;
		v2 WallNormal = {};
		uint32 HitEntityIndex = 0;

		v2 DesiredPosition = Entity.High->P + PlayerDelta;
		for (uint32 EntityIndex = 1; EntityIndex < GameState->EntityCount; ++EntityIndex)
		{
			entity TestEntity = GetEntity(GameState, EntityResidence_High, EntityIndex);
			if (TestEntity.High != Entity.High)
			{
				if (TestEntity.Dormant->Collides)
				{
					real32 DiameterW = TestEntity.Dormant->Width + Entity.Dormant->Width;
					real32 DiameterH = TestEntity.Dormant->Height + Entity.Dormant->Height;

					v2 MinCorner = -0.5f * v2{ DiameterW, DiameterH };
					v2 MaxCorner = 0.5f * v2{ DiameterW, DiameterH };

					v2 Rel = Entity.High->P - TestEntity.High->P;
					if (TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = v2{ -1, 0 };
						HitEntityIndex = EntityIndex;
					}
					if (TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = v2{ 1, 0 };
						HitEntityIndex = EntityIndex;
					}
					if (TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
					{
						WallNormal = v2{ 0, -1 };
						HitEntityIndex = EntityIndex;
					}
					if (TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
					{
						WallNormal = v2{ 0, 1 };
						HitEntityIndex = EntityIndex;
					}
				}
			}
		}
		Entity.High->P += tMin * PlayerDelta;
		if (HitEntityIndex)
		{
			Entity.High->dP = Entity.High->dP - 1 * Inner(Entity.High->dP, WallNormal) * WallNormal;
			PlayerDelta = DesiredPosition - Entity.High->P;
			PlayerDelta = PlayerDelta - 1 * Inner(PlayerDelta, WallNormal) * WallNormal;

			entity HitEntity = GetEntity(GameState, EnttiyResidence_Dormant, HitEntityIndex);
			Entity.High->AbsTileZ += HitEntity.Dormant->dAbsTileZ;
		}
		else
			break;
	}
	if ((Entity.High->dP.X == 0.0f) && (Entity.High->dP.Y == 0.0f))
	{
	}
	else if (AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y))
	{
		if (Entity.High->dP.X > 0)
			Entity.High->FacingDirection = 0;
		else
			Entity.High->FacingDirection = 2;
	}
	else
	{
		if (Entity.High->dP.Y > 0)
			Entity.High->FacingDirection = 1;
		else
			Entity.High->FacingDirection = 3;
	}
	Entity.Dormant->P = MapIntoTileSpace(GameState->World->TileMap, GameState->CameraP, Entity.High->P);
}

internal void SetCamera(game_state *GameState, tile_map_position NewCameraP)
{
	tile_map *TileMap = GameState->World->TileMap;
	tile_map_difference dCameraP = Subtract(TileMap, &NewCameraP, &GameState->CameraP);
	GameState->CameraP = NewCameraP;

	uint32 TileSpanX = 17 * 3;
	uint32 TileSpanY = 9 * 3;
	rectangle2 CameraBounds = RectCenterDim(V2(0, 0), TileMap->TileSideInMeters * V2((real32)TileSpanX, (real32)TileSpanY));

	v2 EntityOffsetForFrame = -dCameraP.dXY;
	for (uint32 EntityIndex = 1; EntityIndex < ArrayCount(GameState->HighEntities); ++EntityIndex)
	{
		if (GameState->EntityResidence[EntityIndex] == EntityResidence_High)
		{
			high_entity *High = GameState->HighEntities + EntityIndex;
			High->P += EntityOffsetForFrame;
			if (!IsInRectangle(CameraBounds, High->P))
			{
				ChangeEntityResidence(GameState, EntityIndex, EnttiyResidence_Dormant);
			}
		}
	}

	uint32 MinTileX = NewCameraP.AbsTileX - TileSpanX / 2;
	uint32 MinTileY = NewCameraP.AbsTileY - TileSpanY / 2;
	uint32 MaxTileX = NewCameraP.AbsTileX + TileSpanX / 2;
	uint32 MaxTileY = NewCameraP.AbsTileY + TileSpanY / 2;
	for (uint32 EntityIndex = 1; EntityIndex < ArrayCount(GameState->DormantEntities); ++EntityIndex)
	{
		if (GameState->EntityResidence[EntityIndex] == EnttiyResidence_Dormant)
		{
			dormant_entity *Dormant = GameState->DormantEntities + EntityIndex;
			if ((Dormant->P.AbsTileZ == NewCameraP.AbsTileX) &&
				(Dormant->P.AbsTileX >= MinTileX) &&
				(Dormant->P.AbsTileX < MaxTileX) &&
				(Dormant->P.AbsTileY >= MinTileY) &&
				(Dormant->P.AbsTileY < MaxTileY))
			{
				ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);
			}
		}
	}
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)))
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if (!Memory->IsInitialized)
	{
		//NOTE: Reserve entity slot 0 for null entity
		AddEntity(GameState, EntityType_Null);
		GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
		GameState->Shadow = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_shadow.bmp");

		hero_bitmaps *Bitmap;
		Bitmap = GameState->HeroBitmaps;
		Bitmap->Head= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
			(uint8 *)Memory->PermanentStorage + sizeof(game_state));

		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena, tile_map);
		tile_map *TileMap = World->TileMap;

		TileMap->ChunkShift = 4;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
		TileMap->ChunkDim = (1 << TileMap->ChunkShift);

		TileMap->TileChunkCountX = 128;
		TileMap->TileChunkCountY = 128;
		TileMap->TileChunkCountZ = 2;

		TileMap->TileChunks = PushArray(
			&GameState->WorldArena, TileMap->TileChunkCountX * TileMap->TileChunkCountY * TileMap->TileChunkCountZ, tile_chunk);
		TileMap->TileSideInMeters = 1.4f;

		uint32 RandomNumberIndex = 0;

		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenX = 0;
		uint32 ScreenY = 0;
		uint32 AbsTileZ = 0;

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 2; ++ScreenIndex)
		{
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice;
			if (true)
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
			else
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;

			bool32 CreatedZDoor = false;
			if (RandomChoice == 2)
			{
				CreatedZDoor = true;
				if (AbsTileZ == 0)
					DoorUp = true;
				else
					DoorDown = true;
			}
			else if (RandomChoice == 1)
				DoorRight = true;
			else
				DoorTop = true;

			for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
			{
				for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

					uint32 TileValue = 1;
					if ((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight / 2))))
						TileValue = 2;
					if ((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight / 2))))
						TileValue = 2;
					if ((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth / 2))))
						TileValue = 2;
					if ((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth / 2))))
						TileValue = 2;
					if ((TileX == 10) && (TileY == 6))
					{
						if (DoorUp)
							TileValue = 3;
						if (DoorDown)
							TileValue = 4;
					}
					SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
					if (TileValue == 2)
						AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
				}
			}

			DoorLeft = DoorRight;
			DoorBottom = DoorTop;

			if (CreatedZDoor)
			{
				DoorDown = !DoorDown;
				DoorUp = !DoorUp;
			}
			else
			{
				DoorUp = false;
				DoorDown = false;
			}

			DoorRight = false;
			DoorTop = false;
			if (RandomChoice == 2)
			{
				if (AbsTileZ == 0)
					AbsTileZ = 1;
				else
					AbsTileZ = 0;
			}
			else if (RandomChoice == 1)
				ScreenX += 1;
			else
				ScreenY += 1;
		}
		tile_map_position NewCameraP = {};
		NewCameraP.AbsTileX = 17 / 2;
		NewCameraP.AbsTileY = 9 / 2;
		SetCamera(GameState, NewCameraP);

		Memory->IsInitialized = true;
	}

	world *World = GameState->World;
	tile_map *TileMap = World->TileMap;

	int32 TileSideInPixels = 60;
	real32 MetersToPixels = (real32)TileSideInPixels / (real32)TileMap->TileSideInMeters;
	real32 LowerLeftX = -(real32)TileSideInPixels / 2;
	real32 LowerLeftY = (real32)Buffer->Height;


	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		entity ControllingEntity = GetEntity(GameState, EntityResidence_High, GameState->PlayerIndexForController[ControllerIndex]);
		if (ControllingEntity.Residence != EntityResidence_Nonexistent)
		{
			v2 ddP = {};
			if (Controller->IsAnalog)
			{
				ddP = v2{ Controller->StickAverageX, Controller->StickAverageY };
			}
			else
			{
				if (Controller->MoveUp.EndedDown)
					ddP.Y = 1.0f;
				if (Controller->MoveDown.EndedDown)
					ddP.Y = -1.0f;
				if (Controller->MoveLeft.EndedDown)
					ddP.X = -1.0f;
				if (Controller->MoveRight.EndedDown)
					ddP.X = 1.0f;
			}

			if (Controller->ActionUp.EndedDown)
				ControllingEntity.High->dZ = 3.0f;

			MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddP);
		}
		else
		{
			if (Controller->Start.EndedDown)
			{
				uint32 EntityIndex = AddPlayer(GameState);
				GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
			}
		}
	}

	// camera move
	entity CameraFollowingEntity = GetEntity(GameState, EntityResidence_High, GameState->CameraFollowingEntityIndex);
	if (CameraFollowingEntity.Residence != EntityResidence_Nonexistent)
	{
		tile_map_position NewCameraP = GameState->CameraP;
		NewCameraP.AbsTileZ = CameraFollowingEntity.Dormant->P.AbsTileZ;
#if 0
		if (CameraFollowingEntity.High->P.X > (9.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileX += 17;
		if (CameraFollowingEntity.High->P.X < -(9.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileX -= 17;
		if (CameraFollowingEntity.High->P.Y > (5.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileY += 9;
		if (CameraFollowingEntity.High->P.Y < -(5.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileY -= 9;
#else
		if (CameraFollowingEntity.High->P.X > (1.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileX += 1;
		if (CameraFollowingEntity.High->P.X < -(1.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileX -= 1;
		if (CameraFollowingEntity.High->P.Y >(1.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileY += 1;
		if (CameraFollowingEntity.High->P.Y < -(1.0f * TileMap->TileSideInMeters))
			NewCameraP.AbsTileY -= 1;
#endif

		SetCamera(GameState, NewCameraP);
	}

	DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;
#if 0
	for (int32 RelRow = -10; RelRow < 10; ++RelRow)
	{
		for (int32 RelColumn = -20; RelColumn < 20; ++RelColumn)
		{
			uint32 Column = GameState->CameraP.AbsTileX + RelColumn;
			uint32 Row = GameState->CameraP.AbsTileY + RelRow;
			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
			if (TileID > 1)
			{
				real32 Gray = 0.5f;
				if (TileID == 2)
					Gray = 1.0f;

				if (TileID > 2)
					Gray = 0.25f;

				if ((Column == GameState->CameraP.AbsTileX) && (Row == GameState->CameraP.AbsTileY))
					Gray = 0.0f;
				v2 TileSide = { 0.5f * TileSideInPixels, 0.5f * TileSideInPixels };
				v2 Cen = { ScreenCenterX - MetersToPixels * GameState->CameraP.Offset_.X + ((real32)RelColumn) * TileSideInPixels ,
					ScreenCenterY + MetersToPixels * GameState->CameraP.Offset_.Y - ((real32)RelRow) * TileSideInPixels};
				v2 Min = Cen - 0.9f * TileSide;
				v2 Max = Cen + TileSide;
				DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
			}
		}
	}
#endif
	for (uint32 EntityIndex=0;EntityIndex < GameState->EntityCount;++EntityIndex)
	{
		if(GameState->EntityResidence[EntityIndex] == EntityResidence_High)
		{
			high_entity *HighEntity = &GameState->HighEntities[EntityIndex];
			low_entity *LowEntity = &GameState->LowEntities[EntityIndex];
			dormant_entity *DormantEntity = &GameState->DormantEntities[EntityIndex];

			real32 dt = Input->dtForFrame;
			real32 ddZ = -9.8f;
			HighEntity->Z = 0.5f * ddZ * Square(dt) + HighEntity->dZ * dt + HighEntity->Z;
			HighEntity->dZ = ddZ * dt + HighEntity->dZ;
			if (HighEntity->Z < 0)
				HighEntity->Z = 0;

			real32 CAlpha = 1.0f - 0.5f * HighEntity->Z;
			if (CAlpha < 0.0f)
				CAlpha = 0.0f;

			real32 PlayerR = 1.0f;
			real32 PlayerG = 1.0f;
			real32 PlayerB = 0.0f;

			real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels * HighEntity->P.X;
			real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels * HighEntity->P.Y;
			real32 Z = -MetersToPixels * HighEntity->Z;

			v2 PlayerLeftTop = { PlayerGroundPointX - 0.5f * MetersToPixels * DormantEntity->Width,
				PlayerGroundPointY - 0.5f * MetersToPixels * DormantEntity->Height };
			v2 EntityWidthHeight = { DormantEntity->Width, DormantEntity->Height };
			if (DormantEntity->Type == EntityType_Hero)
			{
				hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[HighEntity->FacingDirection];
				DrawBitmap(Buffer, &GameState->Shadow, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY, CAlpha);
				DrawBitmap(Buffer, &HeroBitmaps->Torso, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
				DrawBitmap(Buffer, &HeroBitmaps->Cape, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
				DrawBitmap(Buffer, &HeroBitmaps->Head, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
			}
			else
			{
				DrawRectangle(Buffer, PlayerLeftTop,
					PlayerLeftTop + MetersToPixels * EntityWidthHeight,
					PlayerR, PlayerG, PlayerB);
			}
		}
	}
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, 400);
}
