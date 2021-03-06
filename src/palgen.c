#define PIXEL_SIZE 4

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <parser.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
    BITMAPINFO bitmap_info;
    i32 width;
    i32 height;
    void *bitmap;
} BackBuffer;

bool running;
i32 palette_index;
BackBuffer back_buffer;

void shade_pixel(u32 *pixel, Palette *palette, i32 x, i32 y, i32 row_size, i32 col_size) {
    for (i32 i = 0; i < palette->ncols; i++) {
        for (i32 j = 0; j < palette->nrows; j++) {
            if (x >= col_size * i && y >= row_size * j) {
                *pixel = palette->colors[i + j * palette->ncols];
            }
        }
    }
}

void render_palette() {
    Palette palette = load_palette(palette_index);
    u8 *row = (u8 *)back_buffer.bitmap;

    i32 pixel_inc = PIXEL_SIZE * back_buffer.width;
    i32 col_size  = back_buffer.width / (palette.ncols);
    i32 row_size  = back_buffer.height / (palette.nrows);

    for (i32 y = 0; y < back_buffer.height; y++) {
        u32 *pixel = (u32 *)row;

        for (i32 x = 0; x < back_buffer.width; x++) {
            shade_pixel(pixel, &palette, x, y, row_size, col_size);
            *pixel++;
        }

        row += pixel_inc;
    }
}

void init_DIB(HWND window_handle, i32 width, i32 height) {
    if (back_buffer.bitmap) {
        VirtualFree(back_buffer.bitmap, 0, MEM_RELEASE);
    }

    back_buffer.width  = width;
    back_buffer.height = height;

    back_buffer.bitmap_info.bmiHeader.biSize        = sizeof(back_buffer.bitmap_info.bmiHeader);
    back_buffer.bitmap_info.bmiHeader.biWidth       = width;
    back_buffer.bitmap_info.bmiHeader.biHeight      = -height; // 0,0 in top left
    back_buffer.bitmap_info.bmiHeader.biPlanes      = 1;
    back_buffer.bitmap_info.bmiHeader.biBitCount    = PIXEL_SIZE * 8;
    back_buffer.bitmap_info.bmiHeader.biCompression = BI_RGB;

    i32 bitmap_size = (width * height) * PIXEL_SIZE;
    back_buffer.bitmap = VirtualAlloc(0, bitmap_size, MEM_COMMIT, PAGE_READWRITE);
    render_palette();

    char *palette_name = load_palette(palette_index).name;
    char *name_buffer  = malloc(strlen(palette_name) + 16);

    sprintf(name_buffer, "%s (W: %d H: %d)", palette_name, width, height);
    SetWindowTextA(window_handle, name_buffer);
}

void draw_window(HDC device_context, RECT *window_rect, i32 width, i32 height) {
    StretchDIBits(
        device_context,
        0, 0, width, height,
        0, 0, back_buffer.width, back_buffer.height,
        back_buffer.bitmap,
        &back_buffer.bitmap_info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

void redraw_window(HWND window_handle, RECT *client_rect) {
    i32 width  = client_rect->right - client_rect->left;
    i32 height = client_rect->bottom - client_rect->top;

    init_DIB(window_handle, width, height);
    InvalidateRect(window_handle, client_rect, 0);
}

void resize_window(HWND window_handle, RECT *client_rect, RECT *window_rect) {
    i32 x_off = (window_rect->right - window_rect->left) - client_rect->right;
    i32 y_off = (window_rect->bottom - window_rect->top) - client_rect->bottom;
    SetWindowPos(window_handle, 0, 0, 0, texture.width + x_off, texture.height + y_off, SWP_NOMOVE | SWP_NOZORDER);
}

LRESULT CALLBACK window_callback(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    switch (message) {
        case WM_ACTIVATEAPP: {
            break;
        }
        case WM_SIZE: {
            RECT client_rect;
            GetClientRect(window_handle, &client_rect);

            i32 width  = client_rect.right - client_rect.left;
            i32 height = client_rect.bottom - client_rect.top;

            init_DIB(window_handle, width, height);
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT paint;
            RECT client_rect, window_rect;
            GetClientRect(window_handle, &client_rect);

            HDC device_context = BeginPaint(window_handle, &paint);

            i32 width  = paint.rcPaint.right - paint.rcPaint.left;
            i32 height = paint.rcPaint.bottom - paint.rcPaint.top;

            draw_window(device_context, &client_rect, width, height);
            EndPaint(window_handle, &paint);
            break;
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            u32 vk_key = wparam;

            RECT client_rect, window_rect;
            GetClientRect(window_handle, &client_rect);
            GetWindowRect(window_handle, &window_rect);

            switch (vk_key) {
                case VK_LEFT: {
                    palette_index--;

                    if (palette_index < 0) {
                        palette_index = palette_count - 1;
                    }

                    redraw_window(window_handle, &client_rect);
                    break;
                }
                case VK_RIGHT: {
                    palette_index++;

                    if (palette_index >= palette_count) {
                        palette_index = 0;
                    }

                    redraw_window(window_handle, &client_rect);
                    break;
                }
            }
            break;
        }
        case WM_CLOSE:
        case WM_DESTROY: {
            running = false;
            break;
        }
        default: {
            result = DefWindowProc(window_handle, message, wparam, lparam);
            break;
        }
    }
    return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd, int cmd_show) {
    load_palettes_from_file("../test.pal"); // @Temporary
    WNDCLASS window_class = {0};
    window_class.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = window_callback;
    window_class.hInstance     = instance;
    window_class.lpszClassName = "PALGEN";

    if (!RegisterClass(&window_class)) {
        return -1;
    }

    HWND window_handle = CreateWindowEx(
        0,
        window_class.lpszClassName,
        load_palette(palette_index).name,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        texture.width,
        texture.height,
        0,
        0,
        instance,
        0
    );

    RECT client_rect, window_rect;
    GetClientRect(window_handle, &client_rect);
    GetWindowRect(window_handle, &window_rect);

    resize_window(window_handle, &client_rect, &window_rect);

    if (window_handle) {
        MSG message;
        running = true;

        while (running) {
            BOOL message_success = GetMessage(&message, 0, 0, 0);

            if (message_success <= 0) {
                break;
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return 0;
}
