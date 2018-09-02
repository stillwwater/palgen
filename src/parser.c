#define MAX_PALETTES 512

#include <stdbool.h>
#include <stdint.h>
#include <parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

Palette *palettes;
i32 palette_count;
Texture texture;

Palette load_palette(i32 pal) {
    if (!palettes) {
        texture.width  = 512;
        texture.height = 512;

        // Default palette
        palettes = VirtualAlloc(0, sizeof(Palette), MEM_COMMIT, PAGE_READWRITE);

        palettes[0].ncols  = 2;
        palettes[0].nrows  = 2;
        palettes[0].colors = malloc(4 * sizeof(u32));

        palettes[0].colors[0] = ((255 << 16) | (255 << 8) | 255);
        palettes[0].colors[1] = ((255 << 16) | (0 << 8) | 255);
        palettes[0].colors[2] = ((0 << 16) | (255 << 8) | 255);
        palettes[0].colors[3] = ((255 << 16) | (255 << 8) | 0);
    }
    return palettes[pal];
}

void load_palettes_from_file(char *filename) {
    FILE *fp;
    char *token;
    size_t buffer_length;
    Reader reader = {0};

    fp = fopen(filename, "r");

    if (!fp) {
        return;
    }

    reader.fp = fp;

    i32 p_length = -1;
    size_t p_color_index = 0;

    if (palettes) {
        free_palettes();
    }

    palettes = VirtualAlloc(0, MAX_PALETTES * sizeof(Palette), MEM_COMMIT, PAGE_READWRITE);

    while (token = eat_token(&reader, &buffer_length)) {
        if (token[0] == 0) {
            continue;
        }

        if (token[0] == '#') {
             if (palettes[p_length].nrows < 0) {
                 // Error: no size given
                 continue;
             }

             if (palettes[p_length].ncols < 0) {
                // No column size given, use row size
                palettes[p_length].ncols = palettes[p_length].nrows;
             }

             if (!palettes[p_length].colors) {
                palettes[p_length].colors = malloc(palettes[p_length].nrows * palettes[p_length].ncols * sizeof(u32));
             }

             palettes[p_length].colors[p_color_index] = strtol(++token, 0, 16);
             p_color_index++;
        } else if (isnumber(token)) {
            i32 token_int = atoi(token);

            if (p_length < 0) {
                if (texture.width <= 0) {
                    texture.width = token_int;
                } else {
                    texture.height = token_int;
                }
                continue;
            }

            if (palettes[p_length].nrows == -1) {
                palettes[p_length].nrows = token_int;
            } else {
                palettes[p_length].ncols = token_int;
            }
        } else {
            p_length++;
            palettes[p_length].ncols = -1;
            palettes[p_length].nrows = -1;
            palettes[p_length].name = malloc(buffer_length);
            memcpy(palettes[p_length].name, token, buffer_length);
            p_color_index = 0;
        }
    }

    palette_count = p_length + 1;

    if (texture.width <= 0) {
        texture.width = 512;
    }

    if (texture.height <= 0) {
        texture.height = texture.width;
    }

    free(reader.buffer);
    fclose(fp);
}

void free_palettes() {
    for (i32 i = 0; i < palette_count; i++) {
        free(palettes[i].colors);
        free(palettes[i].name);
    }

    VirtualFree(palettes, 0, MEM_RELEASE);
}

char * eat_token(Reader *reader, size_t *length) {
    *length = 0;

    while (1) {
        i32 c = fgetc(reader->fp);

        if (ferror(reader->fp)) {
            return NULL;
        }

        if (*length == reader->size) {
            reader->size = reader->size + 128;
            reader->buffer  = realloc(reader->buffer, reader->size);
        }

        if (c == EOF) {
            if (*length == 0) {
                return NULL;
            }

            reader->buffer[(*length)++] = '\0';
            return reader->buffer;
        }

        switch (c) {
            // Legal separators
            case '\n':
            case '\r':
            case ' ':
            case ',':
            case ';':
            case ':':
            case '(':
            case ')':
            case '[':
            case ']': {
                reader->buffer[(*length)++] = '\0';
                return reader->buffer;
            }
        }

        reader->buffer[(*length)++] = c;
    }
}

bool isnumber(const char *s) {
    for (i32 i = 0; s[i] != 0; i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
}


