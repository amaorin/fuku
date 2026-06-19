#include <stdint.h>
#include <stdbool.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define S8_MIN  ( (s8)0x80)
#define S16_MIN ((s16)0x8000)
#define S32_MIN ((s32)0x80000000)
#define S64_MIN ((s32)0x8000000000000000LL)

#define S8_MAX  ( (s8)0x7F)
#define S16_MAX ((s16)0x7FFF)
#define S32_MAX ((s32)0x7FFFFFFF)
#define S64_MAX ((s32)0x7FFFFFFFFFFFFFFFLL)

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define U8_MAX  ( (u8)0xFFU)
#define U16_MAX ((u16)0xFFFFU)
#define U32_MAX ((u32)0xFFFFFFFFU)
#define U64_MAX ((u64)0xFFFFFFFFFFFFFFFFULL)

typedef s64 smm;
typedef u64 umm;

#define SMM_MIN S64_MIN
#define SMM_MAX S64_MAX
#define UMM_MAX U64_MAX

typedef float f32;
typedef double f64;

typedef struct String
{
	u8* data;
	u64 len;
} String;

#define STRING(S) (String){ .data = (u8*)(S), .len = sizeof(S)-1 }

#define ARRAY_LEN(A) (sizeof(A)/sizeof(0[A]))

#define CONCAT__(A, B) A##B
#define CONCAT_(A, B) CONCAT__(A, B)
#define CONCAT(A, B) CONCAT_(A, B)

#define STRINGIFY__(A) #A
#define STRINGIFY_(A) STRINGIFY__(A)
#define STRINGIFY(A) STRINGIFY_(A)

#define ASSERT(EX) ((EX) ? (1) : ((*(volatile int*)0 = 0), 0))
#define STATIC_ASSERT(EX, MSG) static_assert((EX), MSG)

#define NOT_IMPLEMENTED ASSERT("NOT_IMPLEMENTED" != 0)

#define BREAK_IF(E) { if (E) break; }

inline bool
String_Equal(String a, String b)
{
	bool result = (a.len == b.len);

	for (umm i = 0; i < a.len && result; ++i) result = (a.data[i] == b.data[i]);

	return result;
}

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_ASPECT_X 4
#define SCREEN_ASPECT_Y 3
