/* Non-empty translation unit marker — ISO C forbids empty TUs under -Wpedantic.
 * On non-Apple platforms the entire backend below compiles to nothing. */
typedef int swatch_window_cocoa_unit_;

#if defined(__APPLE__)

/* Pure-C Cocoa backend via objc_msgSend.
 * objc_msgSend is variadic in <objc/message.h> for backwards compat,
 * but on arm64 (Apple Silicon) the variadic ABI differs from the regular
 * ABI. EVERY call must cast objc_msgSend to a typed function pointer
 * matching the actual signature of the receiver/selector pair.
 * Do not call objc_msgSend without casting first. */

#include "window.h"

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* AppKit / Foundation enum values inlined so we avoid pulling Objective-C
 * headers. These constants are stable Apple ABI. */
#define SWATCH_NS_APP_POLICY_REGULAR              0
#define SWATCH_NS_WINDOW_STYLE_MASK_TITLED        (1u << 0)
#define SWATCH_NS_WINDOW_STYLE_MASK_CLOSABLE      (1u << 1)
#define SWATCH_NS_BACKING_STORE_BUFFERED          2
#define SWATCH_NS_EVENT_MASK_ANY                  (~(unsigned long)0)
#define SWATCH_CG_BITMAP_BYTE_ORDER_32_LITTLE     (2u << 12)
#define SWATCH_CG_IMAGE_ALPHA_NONE_SKIP_FIRST     6u

static volatile sig_atomic_t s_stop = 0;

static void on_signal(int sig) {
    (void)sig;
    s_stop = 1;
}

static unsigned rand_unsigned(void) {
    return (unsigned)rand();
}

/* Convenience for sending zero-arg selectors that return id. */
static inline id msg_id(id recv, SEL sel) {
    return ((id (*)(id, SEL))objc_msgSend)(recv, sel);
}

static inline id cls_msg_id(Class cls, SEL sel) {
    return ((id (*)(Class, SEL))objc_msgSend)(cls, sel);
}

static id make_nsstring(const char *utf8) {
    Class NSString_cls = objc_getClass("NSString");
    return ((id (*)(Class, SEL, const char *))objc_msgSend)(
        NSString_cls,
        sel_registerName("stringWithUTF8String:"),
        utf8);
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
    if (fps > 60) fps = 60;

    int duration = opts.duration_seconds;
    if (duration < 0) duration = 0;
    if (duration > 3600) duration = 3600;

    /* NSApplication *app = [NSApplication sharedApplication]; */
    Class NSApplication_cls = objc_getClass("NSApplication");
    id app = cls_msg_id(NSApplication_cls, sel_registerName("sharedApplication"));
    if (app == nil) {
        fputs("swatch window: NSApplication unavailable\n", stderr);
        return 1;
    }

    /* [app setActivationPolicy:NSApplicationActivationPolicyRegular]; */
    ((void (*)(id, SEL, long))objc_msgSend)(
        app, sel_registerName("setActivationPolicy:"),
        SWATCH_NS_APP_POLICY_REGULAR);

    /* [app finishLaunching]; */
    ((void (*)(id, SEL))objc_msgSend)(app, sel_registerName("finishLaunching"));

    /* NSWindow *win = [[NSWindow alloc]
     *      initWithContentRect:rect
     *      styleMask:(titled|closable)
     *      backing:NSBackingStoreBuffered
     *      defer:NO]; */
    Class NSWindow_cls = objc_getClass("NSWindow");
    id win_alloc = cls_msg_id(NSWindow_cls, sel_registerName("alloc"));
    CGRect frame = CGRectMake(0, 0, (CGFloat)width, (CGFloat)height);
    id win = ((id (*)(id, SEL, CGRect, unsigned long, unsigned long, BOOL))objc_msgSend)(
        win_alloc,
        sel_registerName("initWithContentRect:styleMask:backing:defer:"),
        frame,
        SWATCH_NS_WINDOW_STYLE_MASK_TITLED | SWATCH_NS_WINDOW_STYLE_MASK_CLOSABLE,
        SWATCH_NS_BACKING_STORE_BUFFERED,
        NO);
    if (win == nil) {
        fputs("swatch window: cannot create NSWindow\n", stderr);
        return 1;
    }

    /* [win setTitle:@"swatch noise"]; */
    ((void (*)(id, SEL, id))objc_msgSend)(
        win, sel_registerName("setTitle:"),
        make_nsstring("swatch noise"));

    /* NSImageView *view = [[NSImageView alloc] initWithFrame:frame]; */
    Class NSImageView_cls = objc_getClass("NSImageView");
    id view_alloc = cls_msg_id(NSImageView_cls, sel_registerName("alloc"));
    id view = ((id (*)(id, SEL, CGRect))objc_msgSend)(
        view_alloc, sel_registerName("initWithFrame:"), frame);

    /* [win setContentView:view]; */
    ((void (*)(id, SEL, id))objc_msgSend)(
        win, sel_registerName("setContentView:"), view);

    /* [win makeKeyAndOrderFront:nil]; */
    ((void (*)(id, SEL, id))objc_msgSend)(
        win, sel_registerName("makeKeyAndOrderFront:"), nil);

    /* [app activateIgnoringOtherApps:YES]; */
    ((void (*)(id, SEL, BOOL))objc_msgSend)(
        app, sel_registerName("activateIgnoringOtherApps:"), YES);

    size_t pixel_bytes = (size_t)width * (size_t)height * 4u;
    uint8_t *pixels = (uint8_t *)malloc(pixel_bytes);
    if (pixels == NULL) {
        fprintf(stderr, "swatch window: out of memory (%zu bytes)\n", pixel_bytes);
        msg_id(view, sel_registerName("release"));
        msg_id(win,  sel_registerName("release"));
        return 1;
    }

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    /* For nextEventMatchingMask:untilDate:inMode:dequeue: */
    Class NSDate_cls = objc_getClass("NSDate");
    id distant_past = cls_msg_id(NSDate_cls, sel_registerName("distantPast"));
    id mode = make_nsstring("kCFRunLoopDefaultMode");
    SEL sel_next_event = sel_registerName(
        "nextEventMatchingMask:untilDate:inMode:dequeue:");
    SEL sel_send_event = sel_registerName("sendEvent:");
    SEL sel_is_visible = sel_registerName("isVisible");
    SEL sel_set_image  = sel_registerName("setImage:");
    SEL sel_release    = sel_registerName("release");
    SEL sel_alloc      = sel_registerName("alloc");
    SEL sel_init_cg    = sel_registerName("initWithCGImage:size:");
    Class NSImage_cls  = objc_getClass("NSImage");

    long total_frames = (duration > 0) ? ((long)duration * (long)fps) : -1;
    long frames_drawn = 0;
    struct timespec delay = {
        .tv_sec  = 0,
        .tv_nsec = 1000000000L / fps
    };

    while (!s_stop) {
        /* Drain pending events. */
        for (;;) {
            id ev = ((id (*)(id, SEL, unsigned long, id, id, BOOL))objc_msgSend)(
                app, sel_next_event,
                SWATCH_NS_EVENT_MASK_ANY,
                distant_past,
                mode,
                YES);
            if (ev == nil) break;
            ((void (*)(id, SEL, id))objc_msgSend)(app, sel_send_event, ev);
        }

        if (s_stop) break;

        BOOL visible = ((BOOL (*)(id, SEL))objc_msgSend)(win, sel_is_visible);
        if (!visible) break;

        swatch_window_fill_frame(pixels, width, height, opts.bw, rand_unsigned);

        CGDataProviderRef provider = CGDataProviderCreateWithData(
            NULL, pixels, pixel_bytes, NULL);
        CGImageRef cg_img = CGImageCreate(
            (size_t)width, (size_t)height,
            8, 32, (size_t)width * 4u,
            cs,
            SWATCH_CG_BITMAP_BYTE_ORDER_32_LITTLE
                | SWATCH_CG_IMAGE_ALPHA_NONE_SKIP_FIRST,
            provider, NULL, false,
            kCGRenderingIntentDefault);

        CGSize sz = CGSizeMake((CGFloat)width, (CGFloat)height);
        id img_alloc = cls_msg_id(NSImage_cls, sel_alloc);
        id ns_img = ((id (*)(id, SEL, CGImageRef, CGSize))objc_msgSend)(
            img_alloc, sel_init_cg, cg_img, sz);

        ((void (*)(id, SEL, id))objc_msgSend)(view, sel_set_image, ns_img);

        msg_id(ns_img, sel_release);
        CGImageRelease(cg_img);
        CGDataProviderRelease(provider);

        nanosleep(&delay, NULL);
        frames_drawn++;
        if (total_frames >= 0 && frames_drawn >= total_frames) break;
    }

    msg_id(win,  sel_registerName("close"));
    msg_id(view, sel_release);
    msg_id(win,  sel_release);
    CGColorSpaceRelease(cs);
    free(pixels);
    return 0;
}

#endif
