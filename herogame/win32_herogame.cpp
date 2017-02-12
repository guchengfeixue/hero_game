#include <windows.h>
#include <stdint.h>
#include <Xinput.h>

#define internal static 
#define local_persist static 
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

// Chandler Carruth: LLVM compiler optimisition

struct win32_window_dimension
{
	int Width;
	int Height;
};

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

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 Blue = (X + XOffset);
			uint8 Green = (Y + YOffset);
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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
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

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int)
{
	Win32LoadXInput();

	WNDCLASS WindowClass = {};

	//TODO : check CS_HREDRAW CS_VREDRAW OWNDC still matter
	WindowClass.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc		= Win32MainWindowCallback;
	//WindowClass.hInstance		= hInstance;
	WindowClass.hInstance		= GetModuleHandle(0);
	WindowClass.lpszClassName	= "HeroGameWindowClass";
	
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	if (RegisterClassA(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(
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

		if (WindowHandle)
		{
			//NOTE: 使用了CS_OWNER标志后，保持一个device context 并一直使用，不与其他共享
			HDC DeviceContext = GetDC(WindowHandle);

			int XOffset = 0;
			int YOffset = 0;

			GlobalRunning = true;
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

				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						//NOTE: This controller is plugged in
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool Up				= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back			= (bool)(Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder	= (bool)(Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder	= (bool)(Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton		= (bool)(Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

						XOffset += StickX >> 12;
						YOffset += StickY >> 12;
					}
					else
					{
						//NOTE: The controller is not available
						//OutputDebugStringA("xinput device not available\n");
					}
				}

				//XINPUT_VIBRATION Vibration;
				//Vibration.wLeftMotorSpeed = 60000;
				//Vibration.wRightMotorSpeed = 60000;
				//XInputSetState(0, &Vibration);

				RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
				win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);
				Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
				++XOffset;
				++YOffset;
			}
			ReleaseDC(WindowHandle, DeviceContext);
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