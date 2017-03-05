#include "herogame.h"
#include "herogame.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <Xinput.h>
#include <dsound.h>

#include "win32_herogame.h"

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

/*
typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
global_variable x_input_get_state *XInputGetState_;
global_variable x_input_set_state *XInputSetState_;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
*/

// Note: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);		//设备没有连上
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//Note: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					//NOTE: File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{

			}
		}
		else
		{

		}
		CloseHandle(FileHandle);
	}
	else
	{

	}
	return (Result);
}

internal void DEBUGPlatformFreeFileMemory(void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory)
{
	bool32 Result = false;
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			Result = (BytesWritten == MemorySize);
		}
		else
		{

		}
		CloseHandle(FileHandle);
	}
	else
	{

	}
	return (Result);
}

internal void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState) { XInputGetState = XInputGetStateStub; }
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
	}
	else
	{
		//TODO: Diagnostic
	}
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)
			GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				//NOTO: "create" a primary buffer
				//TODO: DSBCAPS_GLOBALFOCUS?
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if (SUCCEEDED(Error))
					{
						//NOTO: we have finally set the format!
						OutputDebugStringA("create primary buffer\n");
					}
					else
					{
						//TODO: diagnostic
					}
				}
				else
				{
					//TODO: diagnostic
				}
			}
			else
			{
				//TODO: diagnostic
			}
			//TODO: DSBCAPS_GETCURRENTPOSITION2
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				OutputDebugStringA("create secondary buffer\n");
			}
			else
			{
				//TODO: Diagnostic
			}
		}
		else
		{
			//TODO: Diagnostic
		}
	}
	else
	{
		//TODO: Diagnostic
	}
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return (Result);
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if (Buffer->Memory)
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);

	int BytesPerPixel = 4;
	Buffer->BytesPerPixel = BytesPerPixel;

	Buffer->Width = Width;
	Buffer->Height = Height;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Width * BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal void Win32ClearBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		uint8 *DestSample = (uint8 *)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
			*DestSample++ = 0;
		DestSample = (uint8 *)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
			*DestSample++ = 0;
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32FillSoundBuffer(
	win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		//TODO assert that Region1Size/Region2Size is valid
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16 *DestSample = (int16 *)Region1;
		int16 *SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16 *)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

LRESULT CALLBACK Win32MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM Wparam, LPARAM Lparam)
{
	LRESULT Result = 0;

	switch (uMsg)
	{
		case WM_CLOSE:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_QUIT\n");
		}break;

		//case WM_SIZE:
		//{
		//	//win32_window_dimension Dimension = Win32GetWindowDimension(hwnd);
		//	//Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
		//	OutputDebugStringA("WM_SIZE\n");

		//}break;

		case WM_ACTIVATEAPP:
		{
		}break;

		case WM_DESTROY:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_DESTROY\n");
		}break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Assert(!"Keyboard input came in through");
		}break;
		case WM_PAINT:
		{
			//OutputDebugStringA("WM_PAINT\n");
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(hwnd, &Paint);
			win32_window_dimension Dimension = Win32GetWindowDimension(hwnd);
			//Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
			Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(hwnd, &Paint);
		}break;

		default:
		{
			Result = DefWindowProc(hwnd, uMsg, Wparam, Lparam);
		}break;
	}
	return (Result);
}

internal void Win32ProcessKeyBoardMessage(game_button_state *NewState, bool32 IsDown)
{
	Assert(NewState->EndedDown != IsDown);
	NewState->EndedDown = IsDown;
	++NewState->HalfTransitionCount;
}

internal void Win32ProcessXInputDigitalButton(
	DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
	real32 Result = 0;

	if (Value < -DeadZoneThreshold)
	{
		Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
	}
	else if (Value > DeadZoneThreshold)
	{
		Result = (real32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
	}
	return (Result);
}

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
	MSG Message;
	while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch (Message.message)
		{
			case WM_QUIT:
			{
				GlobalRunning = false;
			}break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32 VKCode = (uint32)Message.wParam;
				bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
				if (WasDown != IsDown)
				{
					if (VKCode == 'W')
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->MoveUp, IsDown);
					}
					else if (VKCode == 'A')
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->MoveLeft, IsDown);
					}
					else if (VKCode == 'S')
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->MoveDown, IsDown);
					}
					else if (VKCode == 'D')
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->MoveRight, IsDown);
					}
					else if (VKCode == 'Q')
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if (VKCode == 'E')
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->RightShoulder, IsDown);
					}
					else if (VKCode == VK_UP)
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->ActionUp, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->ActionLeft, IsDown);
					}													
					else if (VKCode == VK_DOWN)							
					{													 
						Win32ProcessKeyBoardMessage(&KeyboardController->ActionDown, IsDown);
					}													 
					else if (VKCode == VK_RIGHT)						 
					{													 
						Win32ProcessKeyBoardMessage(&KeyboardController->ActionRight, IsDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
						GlobalRunning = false;
						Win32ProcessKeyBoardMessage(&KeyboardController->Start, IsDown);
					}
					else if (VKCode == VK_SPACE)
					{
						Win32ProcessKeyBoardMessage(&KeyboardController->Back, IsDown);
					}
				}
				bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
				if ((VKCode == VK_F4) && AltKeyWasDown)
					GlobalRunning = false;
			}break;
			default:
			{
				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			}
		}
	}
}

inline LARGE_INTEGER Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return (Result);
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);
	return (Result);
}


internal void Win32DebugDrawVertical(win32_offscreen_buffer *Backbuffer, int X, int Top, int Bottom, uint32 Color)
{
	uint8 *Pixel = ((uint8 *)Backbuffer->Memory + X * Backbuffer->BytesPerPixel + Top * Backbuffer->Pitch);
	for (int Y = Top; Y < Bottom; ++Y)
	{
		*(uint32 *)Pixel = Color;
		Pixel += Backbuffer->Pitch;
	}
}


inline void Win32DrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer, win32_sound_output *SoundOutput, real32 C, int PadX, int Top, int Bottom,
	DWORD Value, uint32 Color)
{
	Assert(Value < SoundOutput->SecondaryBufferSize);
	real32 XReal32 = (C * (real32)Value);
	int X = PadX + (int)(XReal32);
	Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}
internal void Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer, int MarkerCount, win32_debug_time_marker *Markers,
	win32_sound_output *SoundOutput, real32 TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int Top = PadY;
	int Bottom = Backbuffer->Height - PadY;

	real32 C = (real32)(Backbuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;
	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->PlayCursor, 0xFFFFFFFF);
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFFFF0000);
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int)
{
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGrannular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
	
	Win32LoadXInput();

	WNDCLASS WindowClass = {};
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc		= Win32MainWindowCallback;
	WindowClass.hInstance		= hInstance;
	//WindowClass.hInstance		= GetModuleHandle(0);
	WindowClass.lpszClassName	= "HeroGameWindowClass";

#define FramesOfAudioLatency 4
#define MonitorRefeshHz 60
#define GameUpdateHz (MonitorRefeshHz / 2)
	real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Hero Game",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			hInstance,
			0);

		if (Window)
		{
			//NOTE: 使用了CS_OWNER标志后，保持一个device context 并一直使用，不与其他共享
			HDC DeviceContext = GetDC(Window);

			win32_sound_output SoundOutput = {};

			//NOTE: Sound test
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = FramesOfAudioLatency * (SoundOutput.SamplesPerSecond / GameUpdateHz);

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample);
			Win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			//while (GlobalRunning)
			//{
			//	DWORD PlayCursorT;
			//	DWORD WriteCursorT;
			//	GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursorT, &WriteCursorT);
			//	char TextBuffer[256];
			//	_snprintf_s(TextBuffer, sizeof(TextBuffer),
			//		"PC:%u WC:%u\n", PlayCursorT, WriteCursorT);
			//	OutputDebugStringA(TextBuffer);
			//}

			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			//int16 *Samples = (int16 *)_alloca(SoundOutput.SecondaryBufferSize);

#if HEROGAME_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
			LPVOID BaseAddress = 0;
#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes((uint64)4);

			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{

				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				LARGE_INTEGER LastCounter = Win32GetWallClock();

				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = { 0 };

				DWORD LastPlayCursor = 0;
				bool32 SoundIsValid = false;

				uint64 LastCycleCount = __rdtsc();

				while (GlobalRunning)
				{
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;

					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}

					Win32ProcessPendingMessages(NewKeyboardController);

					DWORD MaxControllerCount = XUSER_MAX_COUNT;
					if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
					{
						MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
					}

					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
					{
						DWORD OurControllerIndex = ControllerIndex + 1;
						game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
						game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

						XINPUT_STATE ControllerState;
						if (XInputGetState(OurControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							NewController->IsConnected = true;
							//NOTE: This controller is plugged in
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

							NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							NewController->StickAverageY= Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							if ((NewController->StickAverageX != 0.0f) || (NewController->StickAverageY != 0.0f))
							{
								NewController->IsAnalog = true;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
							{
								NewController->StickAverageY = 1.0f;
								NewController->IsAnalog = false;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
							{
								NewController->StickAverageY = -1.0f;
								NewController->IsAnalog = false;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
							{
								NewController->StickAverageX = -1.0f;
								NewController->IsAnalog = false;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
							{
								NewController->StickAverageX = 1.0f;
								NewController->IsAnalog = false;
							}

							real32 Threshold = 0.5f;
							Win32ProcessXInputDigitalButton(
								(NewController->StickAverageX < -Threshold) ? 1 : 0, &OldController->MoveLeft, 1, &NewController->MoveLeft);

							Win32ProcessXInputDigitalButton(
								(NewController->StickAverageX > Threshold) ? 1 : 0, &OldController->MoveRight, 1, &NewController->MoveRight);

							Win32ProcessXInputDigitalButton(
								(NewController->StickAverageY < -Threshold) ? 1 : 0, &OldController->MoveDown, 1, &NewController->MoveDown);

							Win32ProcessXInputDigitalButton(
								(NewController->StickAverageY > Threshold) ? 1 : 0, &OldController->MoveUp, 1, &NewController->MoveUp);

							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A, &NewController->ActionDown);
							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X, &NewController->ActionLeft);
							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y, &NewController->ActionUp);
							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);
							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);
							Win32ProcessXInputDigitalButton(
								Pad->wButtons, &OldController->Back, XINPUT_GAMEPAD_BACK ,&NewController->Back);

						}
						else
						{
							NewController->IsConnected = false;
							//NOTE: The controller is not available
						}
					}

					DWORD ByteToLock = 0;
					DWORD TargetCursor = 0;
					DWORD BytesToWrite = 0;

					if (SoundIsValid)
					{
						ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
						TargetCursor = ((LastPlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
						if (ByteToLock > TargetCursor)
						{
							BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}
						char TextBuffer[256];
						_snprintf_s(TextBuffer, sizeof(TextBuffer), 
							"PC:%u BTL:%u TC:%u BTW:%u\n", LastPlayCursor, ByteToLock, TargetCursor, BytesToWrite);
						OutputDebugStringA(TextBuffer);
					}

					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackbuffer.Memory;
					Buffer.Width = GlobalBackbuffer.Width;
					Buffer.Height = GlobalBackbuffer.Height;
					Buffer.Pitch = GlobalBackbuffer.Pitch;

					GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);
					if (SoundIsValid)
					{
#if HEROGAME_INTERNAL
						DWORD PlayCursorT;
						DWORD WriteCursorT;
						GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursorT, &WriteCursorT);
						char TextBuffer[256];
						_snprintf_s(TextBuffer, sizeof(TextBuffer),
							"LPC:%u BTL:%u TC:%u BTW:%u - PC:%u WC:%u\n", 
							LastPlayCursor, ByteToLock, TargetCursor, BytesToWrite, PlayCursorT, WriteCursorT);
						OutputDebugStringA(TextBuffer);
#endif

						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}


					LARGE_INTEGER WorkCounter = Win32GetWallClock();
					real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
					real32 SecondsElapsedForFrame = WorkSecondsElapsed;
					if (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						if (SleepIsGrannular)
						{
							DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
							if (SleepMS > 0)
							{
								Sleep(SleepMS);
							}
						}
						real32 TestSecondElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						//Assert(TestSecondElapsedForFrame < TargetSecondsPerFrame);

						while (SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						}
					}
					else
					{
						//TODO: missed frame rate!
						//TODO: logging
					}
					LARGE_INTEGER EndCounter = Win32GetWallClock();
					real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
					LastCounter = EndCounter;

					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
#if HEROGAME_INTERNAL
					Win32DebugSyncDisplay(&GlobalBackbuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, &SoundOutput, TargetSecondsPerFrame);
#endif
					Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);

					DWORD PlayCursor;
					DWORD WriteCursor;
					if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
					{
						LastPlayCursor = PlayCursor;
						if (!SoundIsValid)
						{
							SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
							SoundIsValid = true;
						}
					}
					else
					{
						SoundIsValid = false;
					}

#if HEROGAME_INTERNAL
					{
						win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex++];
						if (DebugTimeMarkerIndex > ArrayCount(DebugTimeMarkers))
						{
							DebugTimeMarkerIndex = 0;
						}
						Marker->PlayCursor = PlayCursor;
						Marker->WriteCursor = WriteCursor;
					}
#endif
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;


					uint64 EndCycleCount = __rdtsc();
					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					LastCycleCount = EndCycleCount;

					real64 FPS = 0.0f;
					real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

					char FPSBuffer[256];
					_snprintf_s(FPSBuffer,sizeof(FPSBuffer), "%.02fms/f, %.02ff/s, %.02fmc/f\n", MSPerFrame, FPS, MCPF);
					OutputDebugStringA(FPSBuffer);

				}
				//ReleaseDC(Window, DeviceContext);
			}
		}
		else
		{
			//TODO Logging
		}
	}
	else
	{
		//TODO Logging
	}
	return (0);
}