/* Non-empty translation unit marker — ISO C forbids empty TUs under -Wpedantic.
 * On non-Windows platforms the entire backend below compiles to nothing. */
typedef int swatch_window_win32_unit_;

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "window.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile LONG s_stop = 0;
static int           s_width = 0;
static int           s_height = 0;
static uint8_t      *s_pixels = NULL;
static BITMAPINFO    s_bmi;

static uint64_t seed_state(void) {
    uint64_t s = ((uint64_t)time(NULL) << 32)
               ^ (uint64_t)GetCurrentProcessId()
               ^ (uint64_t)(uintptr_t)&s;
    if (s == 0) s = 0x9E3779B97F4A7C15ULL;
    return s;
}

static BOOL WINAPI on_ctrl(DWORD type) {
    (void)type;
    InterlockedExchange(&s_stop, 1);
    return TRUE;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CLOSE:
        InterlockedExchange(&s_stop, 1);
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE: {
        int nw = LOWORD(lparam);
        int nh = HIWORD(lparam);
        if (nw > 0 && nh > 0 && (nw != s_width || nh != s_height)) {
            size_t bytes = (size_t)nw * (size_t)nh * 4u;
            uint8_t *np = (uint8_t *)realloc(s_pixels, bytes);
            if (np != NULL) {
                s_pixels = np;
                s_width  = nw;
                s_height = nh;
                s_bmi.bmiHeader.biWidth  = nw;
                s_bmi.bmiHeader.biHeight = -nh;
            }
        }
        return 0;
    }
    case WM_ERASEBKGND:
        /* Skip background erase — we paint every pixel each frame. */
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (s_pixels != NULL && s_width > 0 && s_height > 0) {
            SetDIBitsToDevice(
                hdc,
                0, 0,
                (DWORD)s_width, (DWORD)s_height,
                0, 0,
                0, (UINT)s_height,
                s_pixels,
                &s_bmi,
                DIB_RGB_COLORS);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

int swatch_window_run(swatch_window_opts_t opts) {
    int width  = opts.width  > 0 ? opts.width  : 640;
    int height = opts.height > 0 ? opts.height : 480;
    if (width < 64 || width > 3840 || height < 64 || height > 2160) {
        fprintf(stderr,
                "swatch window: --size out of range "
                "(width 64..3840, height 64..2160; got %dx%d)\n",
                width, height);
        return 64;
    }

    int fps = opts.fps;
    if (fps <= 0) fps = 15;
    if (fps > 120) fps = 120;

    int duration = opts.duration_seconds;
    if (duration < 0) duration = 0;
    if (duration > 3600) duration = 3600;

    HINSTANCE hinst = GetModuleHandleW(NULL);

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = window_proc;
    wc.hInstance     = hinst;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = L"SwatchNoise";
    if (!RegisterClassW(&wc)) {
        fprintf(stderr, "swatch window: RegisterClassW failed (%lu)\n",
                (unsigned long)GetLastError());
        return 1;
    }

    /* Use the standard overlapped window style (includes resize border,
     * minimize, and maximize boxes) so the user can full-screen / maximize. */
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, style, FALSE);
    int outer_w = rect.right - rect.left;
    int outer_h = rect.bottom - rect.top;

    HWND hwnd = CreateWindowExW(
        0, L"SwatchNoise", L"swatch noise",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        outer_w, outer_h,
        NULL, NULL, hinst, NULL);
    if (hwnd == NULL) {
        fprintf(stderr, "swatch window: CreateWindowExW failed (%lu)\n",
                (unsigned long)GetLastError());
        UnregisterClassW(L"SwatchNoise", hinst);
        return 1;
    }

    s_width  = width;
    s_height = height;
    size_t pixel_bytes = (size_t)width * (size_t)height * 4u;
    s_pixels = (uint8_t *)malloc(pixel_bytes);
    if (s_pixels == NULL) {
        fprintf(stderr, "swatch window: out of memory (%zu bytes)\n", pixel_bytes);
        DestroyWindow(hwnd);
        UnregisterClassW(L"SwatchNoise", hinst);
        return 1;
    }

    memset(&s_bmi, 0, sizeof(s_bmi));
    s_bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    s_bmi.bmiHeader.biWidth       = width;
    s_bmi.bmiHeader.biHeight      = -height; /* top-down */
    s_bmi.bmiHeader.biPlanes      = 1;
    s_bmi.bmiHeader.biBitCount    = 32;
    s_bmi.bmiHeader.biCompression = BI_RGB;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SetConsoleCtrlHandler(on_ctrl, TRUE);

    uint64_t rng_state = seed_state();

    long total_frames = (duration > 0) ? ((long)duration * (long)fps) : -1;
    long frames_drawn = 0;

    LARGE_INTEGER qpc_freq, qpc_deadline, qpc_now;
    QueryPerformanceFrequency(&qpc_freq);
    QueryPerformanceCounter(&qpc_deadline);
    LONGLONG interval_ticks = qpc_freq.QuadPart / fps;

    while (!InterlockedCompareExchange(&s_stop, 0, 0)) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                InterlockedExchange(&s_stop, 1);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (InterlockedCompareExchange(&s_stop, 0, 0)) break;

        swatch_window_fill_frame(s_pixels, s_width, s_height,
                                 opts.bw, &rng_state);
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);

        /* Absolute-deadline pacing via QPC. Sleep() granularity is OS-tick
         * (typically ~15ms unless timeBeginPeriod was called); accept the
         * jitter rather than busy-wait. */
        qpc_deadline.QuadPart += interval_ticks;
        QueryPerformanceCounter(&qpc_now);
        if (qpc_now.QuadPart < qpc_deadline.QuadPart) {
            LONGLONG diff_us = ((qpc_deadline.QuadPart - qpc_now.QuadPart)
                                * 1000000LL) / qpc_freq.QuadPart;
            if (diff_us > 1000) {
                Sleep((DWORD)(diff_us / 1000));
            }
        }

        frames_drawn++;
        if (total_frames >= 0 && frames_drawn >= total_frames) break;
    }

    if (IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }
    UnregisterClassW(L"SwatchNoise", hinst);
    SetConsoleCtrlHandler(on_ctrl, FALSE);
    free(s_pixels);
    s_pixels = NULL;
    s_width  = 0;
    s_height = 0;
    return 0;
}

#endif
