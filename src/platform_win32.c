#define UNICODE
#define STRICT
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <Windows.h>
#include <GL/gl.h>
#include <glext.h>
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

#define GL_FUNC_LIST()                                     \
	X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT)         \
	X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)     \
	X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback) \
	X(PFNGLCREATETEXTURESPROC, glCreateTextures)             \
	X(PFNGLTEXTUREPARAMETERIPROC, glTextureParameteri)       \
	X(PFNGLTEXTURESTORAGE2DPROC, glTextureStorage2D)         \
	X(PFNGLTEXTURESUBIMAGE2DPROC, glTextureSubImage2D)       \
	X(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv) \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                 \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)       \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)       \
	X(PFNGLGENPROGRAMPIPELINESPROC, glGenProgramPipelines)   \
	X(PFNGLUSEPROGRAMSTAGESPROC, glUseProgramStages)         \
	X(PFNGLBINDPROGRAMPIPELINEPROC, glBindProgramPipeline)   \
	X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)           \
	X(PFNGLBINDTEXTUREUNITPROC, glBindTextureUnit)

#define X(T, N) static T N;
GL_FUNC_LIST()
#undef X

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
		BREAK_IF(wglChoosePixelFormatARB == 0);

		*wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)(void*)wglGetProcAddress("wglCreateContextAttribsARB");
		BREAK_IF(wglCreateContextAttribsARB == 0);

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

		PIXELFORMATDESCRIPTOR format_descriptor = { .nSize = sizeof(format_descriptor), .nVersion = 1 };
		BREAK_IF(!DescribePixelFormat(dc, format_idx, sizeof(format_descriptor), &format_descriptor));
		BREAK_IF(!SetPixelFormat(dc, format_idx, &format_descriptor));

		int context_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 5,
			WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,

#if USE_OPENGL_DEBUG_CONTEXT
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
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

static void
GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
{
	//if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		OutputDebugStringA(message);
		OutputDebugStringA("\n");
	}
}

typedef struct GL_State
{
	HGLRC context;
	GLuint vao;
	GLuint texture;
	GLuint pipeline;
} GL_State;

// The opengl context example code by Mārtiņš Možeiko (mmozeiko) was used as reference when implementing the opengl initialization
// link: https://gist.github.com/mmozeiko/ed2ad27f75edf9c26053ce332a1f6647
static bool
InitOpenGL(HWND window, HDC dc, GL_State* gl_state)
{
	bool succeeded = false;

	*gl_state = (GL_State){0};

	String required_extensions[] = {
		STRING("WGL_ARB_pixel_format"),
		STRING("WGL_ARB_create_context"),
		STRING("WGL_ARB_create_context_profile"),
		STRING("WGL_EXT_swap_control"),
	};

	do
	{
		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB       = 0;
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
		BREAK_IF(!LoadWGLFunctions(required_extensions, ARRAY_LEN(required_extensions), &wglChoosePixelFormatARB, &wglCreateContextAttribsARB));

		BREAK_IF(!CreateGLContext(window, dc, wglChoosePixelFormatARB, wglCreateContextAttribsARB, &gl_state->context));

#define X(T, N)                                                                         \
			N = (T)(void*)wglGetProcAddress(STRINGIFY(N));                                    \
			if (!N)                                                                           \
			{                                                                                 \
				OutputDebugStringA("ERROR: Failed to load GL function \"" STRINGIFY(N) "\"\n"); \
				break;                                                                          \
			}

			GL_FUNC_LIST()
#undef X

		wglSwapIntervalEXT(1);

#if USE_OPENGL_DEBUG_CONTEXT
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(GLDebugProc, 0);
#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glCreateVertexArrays(1, &gl_state->vao);
		glCreateTextures(GL_TEXTURE_2D, 1, &gl_state->texture);
		glTextureParameteri(gl_state->texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(gl_state->texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(gl_state->texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(gl_state->texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTextureStorage2D(gl_state->texture, 1, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT);

		const char* vertex_shader_code =
			"#version 450 core                                                                    \n"
			"                                                                                     \n"
			"layout(location=0) out vec2 uv;                                                      \n"
			"                                                                                     \n"
			"out gl_PerVertex { vec4 gl_Position; };                                              \n"
			"                                                                                     \n"
			"void                                                                                 \n"
			"main()                                                                               \n"
			"{                                                                                    \n"
			"  gl_Position = vec4((gl_VertexID < 2 ? -1 : 3), (gl_VertexID == 1 ? -3 : 1), 0, 1); \n"
			"  uv          = vec2((gl_VertexID < 2 ?  0 : 2), (gl_VertexID == 1 ?  2 : 0));       \n"
			"}                                                                                    \n"
		;

		const char* fragment_shader_code =
			"#version 450 core                          \n"
			"                                           \n"
			"layout(binding=0) uniform sampler2D image; \n"
			"                                           \n"
			"layout(location=0) in vec2 uv;             \n"
			"                                           \n"
			"layout(location=0) out vec4 color;         \n"
			"                                           \n"
			"void                                       \n"
			"main()                                     \n"
			"{                                          \n"
			"  color = texture(image, uv);              \n"
			"}                                          \n"
		;

		GLuint vertex_shader   = glCreateShaderProgramv(GL_VERTEX_SHADER,   1, &vertex_shader_code);
		GLuint fragment_shader = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fragment_shader_code);

		GLint linked;
		glGetProgramiv(vertex_shader, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			//// ERROR

			char m[1024];
			glGetProgramInfoLog(vertex_shader, sizeof(m), 0, m);
			OutputDebugStringA(m);

			break;
		}

		glGetProgramiv(fragment_shader, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			//// ERROR

			char m[1024];
			glGetProgramInfoLog(fragment_shader, sizeof(m), 0, m);
			OutputDebugStringA(m);

			break;
		}

		glGenProgramPipelines(1, &gl_state->pipeline);
		glUseProgramStages(gl_state->pipeline, GL_VERTEX_SHADER_BIT,   vertex_shader);
		glUseProgramStages(gl_state->pipeline, GL_FRAGMENT_SHADER_BIT, fragment_shader);

		succeeded = true;
	} while (0);

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

	HDC dc = GetDC(window);

	if (dc == 0)
	{
		//// ERROR
		NOT_IMPLEMENTED;
		return -1;
	}

	GL_State gl_state = {0};
	if (!InitOpenGL(window, dc, &gl_state))
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

		u32 pixels[SCREEN_WIDTH*SCREEN_HEIGHT] = {0};
		for (umm i = 0; i < ARRAY_LEN(pixels); ++i) pixels[i] = (((i / SCREEN_WIDTH)/10 + (i % SCREEN_WIDTH)/10) % 2 == 0 ? 0xFF881188 : 0xFF888811);
		glTextureSubImage2D(gl_state.texture, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		{
			RECT client_rect;
			GetClientRect(window, &client_rect);

			s32 client_width  = client_rect.right - client_rect.left;
			s32 client_height = client_rect.bottom - client_rect.top;

			
			s32 width_first  = (client_width  * SCREEN_ASPECT_Y) / SCREEN_ASPECT_X;
			s32 height_first = (client_height * SCREEN_ASPECT_X) / SCREEN_ASPECT_Y;

			s32 width  = client_width;
			s32 height = width_first;
			if (height > client_height)
			{
				width  = height_first;
				height = client_height;
			}

			s32 offset_x = (client_width  - width) / 2;
			s32 offset_y = (client_height - height) / 2;

			glViewport(offset_x, offset_y, width, height);
		}

		glBindProgramPipeline(gl_state.pipeline);
		glBindVertexArray(gl_state.vao);
		glBindTextureUnit(0, gl_state.texture);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		SwapBuffers(dc);
	}

	return 0;
}
