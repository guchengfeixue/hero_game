#include <windows.h>
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

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

win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return (Result);
}

internal void RenderWeirdGradient(int XOffset, int YOffset)
{
	int Width = GlobalBackbuffer.Width;
	int Height = GlobalBackbuffer.Height;

	int BytesPerPixel = 4;

	int Pitch = Width * BytesPerPixel;
	uint8 *Row = (uint8 *)GlobalBackbuffer.Memory;
	for (int Y = 0; Y < GlobalBackbuffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < GlobalBackbuffer.Width; ++X)
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
		Row += Pitch;
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
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer)
{
	StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer.Width, Buffer.Height,
		Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;

	switch (uMsg)
	{
		case WM_SIZE:
		{
			//win32_window_dimension Dimension = Win32GetWindowDimension(hwnd);
			//Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
			//OutputDebugStringA("WM_SIZE\n");

		}break;

		case WM_DESTROY:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_DESTROY\n");
		}break;

		case WM_CLOSE:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_QUIT\n");
		}break;

		case WM_ACTIVATEAPP:
		{
		}break;

		case WM_PAINT:
		{
			OutputDebugStringA("WM_PAINT\n");
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(hwnd, &Paint);

			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			win32_window_dimension Dimension = Win32GetWindowDimension(hwnd);
			//Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
			Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);

			EndPaint(hwnd, &Paint);
		}break;

		default:
		{
			Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		}break;
	}
	return (Result);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int)
{
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

			MSG Message;
			GlobalRunning = true;
			while(GlobalRunning)
			{
				while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
						GlobalRunning = false;
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				RenderWeirdGradient(XOffset, YOffset);
				win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);
				Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);
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