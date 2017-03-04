#include "herogame.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHZ)
{
	local_persist real32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHZ;
	
	int16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
	}
}

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 Blue = (uint8)(X + XOffset);
			uint8 Green = (uint8)(Y + YOffset);
			/*
			Memory:		BB GG RR xx
			Register:	xx RR GG BB
			Pixel (32-bits)
			*/
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Buffer->Pitch;
	}
}

void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer)
{
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)))
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if (!Memory->IsInitialized)
	{
		char *Filename = __FILE__;
		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if (File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}
		//NOTE: virtualalloc cleared to 0
		GameState->ToneHZ = 256;
		//GameState->GreenOffset = 0;
		//GameState->BlueOffset = 0;
		Memory->IsInitialized = true;
	}

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if (Controller->IsAnalog)
		{
			GameState->BlueOffset += (int)(4.0f * Controller->StickAverageX);
			GameState->ToneHZ = 256 + (int)(128.0f * Controller->StickAverageY);
		}
		else
		{
			//NOTE: Use digital movement tuning
			if (Controller->MoveLeft.EndedDown)
			{
				GameState->BlueOffset -= 1;
			}
			
			if (Controller->MoveRight.EndedDown)
			{
				GameState->BlueOffset += 1;
			}

			if (Controller->MoveUp.EndedDown)
			{
				GameState->GreenOffset -= 1;
			}

			if (Controller->MoveDown.EndedDown)
			{
				GameState->GreenOffset += 1;
			}
		}
		if (Controller->ActionDown.EndedDown)
		{
			GameState->GreenOffset += 1;
		}
	}

	GameOutputSound(SoundBuffer, GameState->ToneHZ);
	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}