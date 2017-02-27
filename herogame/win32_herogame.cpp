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
			BufferDescription.dwFlags = 0;
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
	int BytesPerPixel = 4;

	if (Buffer->Memory)
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);

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

		case WM_SIZE:
		{
			//win32_window_dimension Dimension = Win32GetWindowDimension(hwnd);
			//Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
			OutputDebugStringA("WM_SIZE\n");

		}break;

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
			uint32 VKCode = Wparam;
			bool WasDown = ((Lparam & (1 << 30)) != 0);
			bool IsDown = ((Lparam & (1 << 31)) == 0);
			if (WasDown != IsDown)
			{
				if (VKCode == 'W')
				{
				}
				else if (VKCode == 'A')
				{
				}
				else if (VKCode == 'S')
				{
				}
				else if (VKCode == 'D')
				{
				}
				else if (VKCode == 'Q')
				{
				}
				else if (VKCode == 'E')
				{
				}
				else if (VKCode == VK_UP)
				{
				}
				else if (VKCode == VK_LEFT)
				{
				}
				else if (VKCode == VK_DOWN)
				{
				}
				else if (VKCode == VK_RIGHT)
				{
				}
				else if (VKCode == VK_RIGHT)
				{
				}
				else if (VKCode == VK_ESCAPE)
				{
					OutputDebugStringA("ESCAPE: ");
					if (IsDown)
						OutputDebugStringA("IsDown ");
					if (WasDown)
						OutputDebugStringA("WasDown");
					OutputDebugStringA("\n");
					GlobalRunning = false;
				}
				else if (VKCode == VK_SPACE)
				{

				}
			}
			bool32 AltKeyWasDown = (Lparam & (1 << 29));
			if ((VKCode == VK_F4) && AltKeyWasDown)
				GlobalRunning = false;

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

internal void Win32ProcessXInputDigitalButton(
	DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int)
{
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	Win32LoadXInput();

	WNDCLASS WindowClass = {};
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc		= Win32MainWindowCallback;
	WindowClass.hInstance		= hInstance;
	//WindowClass.hInstance		= GetModuleHandle(0);
	WindowClass.lpszClassName	= "HeroGameWindowClass";

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

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample);
			Win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;
			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			//int16 *Samples = (int16 *)_alloca(SoundOutput.SecondaryBufferSize);
			
			game_input Input[2] = {};
			game_input *NewInput = &Input[0];
			game_input *OldInput = &Input[1];

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			uint64 LastCycleCount = __rdtsc();

			while(GlobalRunning)
			{
				MSG Message;

				while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
						GlobalRunning = false;
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				int MaxControllerCount = XUSER_MAX_COUNT;
				if (MaxControllerCount > ArrayCount(NewInput->Controllers))
				{
					MaxControllerCount = ArrayCount(NewInput->Controllers);
				}

				for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
				{

					game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
					game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						//NOTE: This controller is plugged in
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						bool Up				= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

						NewController->IsAnalog = true;
						NewController->StartX = OldController->EndX;
						NewController->StartY = OldController->EndY;

						real32 X;
						if (Pad->sThumbLX < 0)
						{
							X = (real32)Pad->sThumbLX / 32768.0f;
						}
						else
						{
							X = (real32)Pad->sThumbLX / 32767.0f;
						}
						NewController->MinX = NewController->MaxX = NewController->EndX = X;


						real32 Y;
						if (Pad->sThumbLY < 0)
						{
							Y = (real32)Pad->sThumbLY / 32768.0f;
						}
						else
						{
							Y = (real32)Pad->sThumbLY / 32767.0f;
						}
						NewController->MinY = NewController->MaxY = NewController->EndY = Y;

						//int16 StickX = Pad->sThumbLX;
						//int16 StickY = Pad->sThumbLY;

						Win32ProcessXInputDigitalButton(
							Pad->wButtons, &OldController->Down, XINPUT_GAMEPAD_A, &NewController->Down);
						Win32ProcessXInputDigitalButton(
							Pad->wButtons, &OldController->Right, XINPUT_GAMEPAD_B, &NewController->Right);
						Win32ProcessXInputDigitalButton(
							Pad->wButtons, &OldController->Left, XINPUT_GAMEPAD_X, &NewController->Left);
						Win32ProcessXInputDigitalButton(
							Pad->wButtons, &OldController->Up, XINPUT_GAMEPAD_Y, &NewController->Up);
						Win32ProcessXInputDigitalButton(
							Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
						Win32ProcessXInputDigitalButton(
							Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

						bool Start			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder	= (bool)(Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder	= (bool)(Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_Y);

					}
					else
					{
						//NOTE: The controller is not available
					}
				}

				DWORD ByteToLock = 0;
				DWORD TargetCursor = 0;
				DWORD BytesToWrite = 0;
				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0;
				bool32 SoundIsValid = false;

				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
					TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					SoundIsValid = true;
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

				GameUpdateAndRender(NewInput, &Buffer, &SoundBuffer);
				if (SoundIsValid)
				{
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
				}

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
				//++XOffset; 
				//++YOffset;

				uint64 EndCycleCount = __rdtsc();

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				real64 MSPerFrame = ((1000.0f * (real64)CounterElapsed) / (real64)PerfCountFrequency);
				real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
				real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

				//char Buffer[256];
				//sprintf(Buffer, "%.02fms/f, %.02ff/s, %.02fmc/f\n", MSPerFrame, FPS, MCPF);
				//OutputDebugStringA(Buffer);

				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;

				game_input *Temp = NewInput;
				NewInput = OldInput;
				OldInput = Temp;

			}
			//ReleaseDC(Window, DeviceContext);
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