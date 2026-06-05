#define UNICODE
#define STRICT
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <Windows.h>
#include <GL/gl.h>
#include <wglext.h>
#pragma clang diagnostic pop

#include "common.h"

#ifndef APP_NAME
#  define APP_NAME "Noap"
#endif
#define WIDE_APP_NAME CONCAT(L, APP_NAME)

#if !defined(NDEBUG) && !defined(USE_OPENGL_DEBUG_CONTEXT)
#define USE_OPENGL_DEBUG_CONTEXT 1
#endif

static bool
LoadWGLFunctions(String* required_extensions, umm required_extensions_len, PFNWGLCHOOSEPIXELFORMATARBPROC* wglChoosePixelFormatARB, PFNWGLCREATECONTEXTATTRIBSARBPROC* wglCreateContextAttribsARB)
{
	bool succeeded = false;

	HWND dummy_window   = 0;
	HDC dummy_dc        = 0;
	HGLRC dummy_context = 0;

	do
	{
		dummy_window = CreateWindowW(L"STATIC", 0, WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
		BREAK_IF(dummy_window == 0);

		dummy_dc = GetDC(dummy_window);
		BREAK_IF(dummy_dc == 0);

		PIXELFORMATDESCRIPTOR dummy_format_descriptor = {
			.nSize      = sizeof(dummy_format_descriptor),
			.nVersion   = 1,
			.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			.iPixelType = PFD_TYPE_RGBA,
			.cColorBits = 24,
			.iLayerType = PFD_MAIN_PLANE,
		};

		int dummy_format_idx = ChoosePixelFormat(dummy_dc, &dummy_format_descriptor);
		BREAK_IF(dummy_format_idx == 0);

		BREAK_IF(!DescribePixelFormat(dummy_dc, dummy_format_idx, sizeof(dummy_format_descriptor), &dummy_format_descriptor));
		BREAK_IF(!SetPixelFormat(dummy_dc, dummy_format_idx, &dummy_format_descriptor));

		dummy_context = wglCreateContext(dummy_dc);
		BREAK_IF(dummy_context == 0);

		BREAK_IF(!wglMakeCurrent(dummy_dc, dummy_context));

		PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)(void*)wglGetProcAddress("wglGetExtensionsStringARB");
		BREAK_IF(wglGetExtensionsStringARB == 0);

		u8* extension_string = (u8*)wglGetExtensionsStringARB(dummy_dc);

		u64 required_extension_mask = 0;
		ASSERT(required_extensions_len < sizeof(required_extension_mask)*8 &&  "too many required extensions for 64 bit mask");

		u8* scan = extension_string;
		while (true)
		{
			while (*scan == ' ') ++scan;

			if (*scan == 0) break;

			u8* start_of_extension_name = scan;

			while (*scan != ' ' && *scan != 0) ++scan;

			String extension_name = {
				.data = (u8*)start_of_extension_name,
				.len  = (u64)(scan - start_of_extension_name),
			};

			for (umm i = 0; i < required_extensions_len; ++i)
			{
				if (String_Equal(extension_name, required_extensions[i])) required_extension_mask |= (1ULL << i);
			}
		}

		if (required_extension_mask != (1ULL << required_extensions_len) - 1)
		{
			//// ERROR: Missing required extensions
			break;
		}

		*wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)(void*)wglGetProcAddress("wglChoosePixelFormatARB");
		BREAK_IF(wglChoosePixelFormatARB != 0);

		*wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)(void*)wglGetProcAddress("wglCreateContextAttribsARB");
		BREAK_IF(wglCreateContextAttribsARB != 0);

		succeeded = true;

	} while (0);

	wglMakeCurrent(0, 0);
	if (dummy_context != 0) wglDeleteContext(dummy_context);
	if (dummy_dc      != 0) ReleaseDC(dummy_window, dummy_dc);
	if (dummy_window  != 0) DestroyWindow(dummy_window);

	return succeeded;
}

static bool
CreateGLContext(HWND window, HDC dc, PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB, PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB, HGLRC* context)
{
	bool succeeded = false;

	*context = 0;

	do
	{
		int format_attribs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			0 // null terminator
		};

		int format_idx = 0;
		BREAK_IF(!wglChoosePixelFormatARB(dc, format_attribs, 0, 1, &format_idx, &(UINT){0}));
		BREAK_IF(!SetPixelFormat(dc, format_idx, 0));

		int context_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,

#if USE_OPENGL_DEBUG_CONTEXT
			WGL_CONTEXT_DEBUG_BIT_ARB, GL_TRUE,
#endif

			0 // null terminator
		};

		*context = wglCreateContextAttribsARB(dc, 0, context_attribs);
		BREAK_IF(*context == 0);

		BREAK_IF(!wglMakeCurrent(dc, *context));

		succeeded = true;

	} while (0);

	return succeeded;
}

static bool
InitOpenGL(HWND window, HGLRC* context)
{
	bool succeeded = false;

	*context = 0;

	String required_extensions[] = {
		STRING("WGL_ARB_pixel_format"),
	};

	HDC dc = 0;

	do
	{
		dc = GetDC(window);
		BREAK_IF(dc == 0);

		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB       = 0;
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
		BREAK_IF(!LoadWGLFunctions(required_extensions, ARRAY_LEN(required_extensions), &wglChoosePixelFormatARB, &wglCreateContextAttribsARB));

		BREAK_IF(!CreateGLContext(window, dc, wglChoosePixelFormatARB, wglCreateContextAttribsARB, context));

		succeeded = true;
	} while (0);

	if (dc != 0) ReleaseDC(window, dc);

	return succeeded;
}

bool IsRunning = false;

LRESULT
WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_CLOSE)
	{
		IsRunning = false;
		return 0;
	}

	return DefWindowProcW(window, msg, wparam, lparam);
}

int
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR command_line, int command_show)
{
	WNDCLASSW window_class = {
		.style         = CS_OWNDC,
		.lpfnWndProc   = WndProc,
		.hInstance     = instance,
		.lpszClassName = WIDE_APP_NAME,
	};

	if (!RegisterClassW(&window_class))
	{
		//// ERROR
		NOT_IMPLEMENTED;
		return -1;
	}

	HWND window = CreateWindowW(WIDE_APP_NAME, WIDE_APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

	if (window == 0)
	{
		//// ERROR
		NOT_IMPLEMENTED;
		return -1;
	}


	ShowWindow(window, SW_SHOW);

	IsRunning = true;
	while (IsRunning)
	{
		for (MSG msg; PeekMessageW(&msg, window, 0, 0, PM_REMOVE); )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}
