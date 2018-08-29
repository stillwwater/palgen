#include <stdio.h>
#include <stdbool.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct {
    char *name;
    u32 *colors;
    i32 nrows;
    i32 ncols;
} Palette;

typedef struct {
    FILE *fp;
    size_t size;
    char *buffer;
} Reader;

i32 palette_count;

Palette load_palette(i32 pal);
void load_palettes_from_file(char *filename);
void free_palettes();
char * eat_token(Reader *reader, size_t *length);
bool isnumber(const char *s);
