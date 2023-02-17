/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Note: __attribute__((visibility("default"))) tells LLVM to export a symbol.
 */

#include <stdint.h>
#include "libmkb/libmkb.h"  /* u8, u32, i32, ... */
/* Including C source here lets LLVM optimize the whole module as a single */
/* translation unit. This should give better results than relying on LTO.  */
#include "libmkb/libmkb.c"

/* Frame Buffer Resolution */
#define FB_WIDE (240)
#define FB_HIGH (160)

/* Gamepad Vector Indexes */
#define GP_A         (0)
#define GP_B         (1)
#define GP_SELECT    (2)
#define GP_START     (3)
#define GP_U         (4)
#define GP_D         (5)
#define GP_L         (6)
#define GP_R         (7)
#define GP_CONNECTED (8)


/**************************************/
/* Exported Symbols: Global Variables */
/**************************************/

/* Frame buffer byte array (RGBA, 4 bytes per pixel) */
__attribute__((visibility("default")))
u8 FB_BYTES[FB_WIDE * FB_HIGH * 4];

/* Size of frame buffer in bytes */
__attribute__((visibility("default")))
u32 FB_SIZE = sizeof(FB_BYTES);

/* Gamepad button state vector */
__attribute__((visibility("default")))
u8 GAMEPAD[GP_CONNECTED + 1];

/* Size of gamepad button state vector in bytes */
__attribute__((visibility("default")))
u32 GP_SIZE = sizeof(GAMEPAD);


/******************************************************/
/* Imported Symobols (to be linked by js wasm loader) */
/* These rely on -Wl,--allow-undefined to compile     */
/******************************************************/

extern void js_trace(u32 code);

extern void repaint();


/**************************/
/* Non-exported functions */
/**************************/

/* Generate a test pattern in the framebuffer */
void test_pattern(u32 offset) {
    /* Put some noise in the frame buffer */
    u32 i;
    for(i = 0; i < sizeof(FB_BYTES); i += 4) {
        u32 x = i + offset;
        FB_BYTES[i] = (u8) (x >> 8);
        FB_BYTES[i+1] = (u8) (x >> 1);
        FB_BYTES[i+2] = (u8) (x >> 1);
        FB_BYTES[i+3] = 255;
    }
}


/*******************************/
/* Exported Symbols: Functions */
/*******************************/

/* Initialization function (gets called by js) */
__attribute__((visibility("default")))
i32 init(void) {
    js_trace(sizeof(FB_BYTES));
    return 0;
}

/* Prepare the next frame */
/* elapsed_ms: number of milliseconds since previous frame */
__attribute__((visibility("default")))
void next(u32 elapsed_ms) {
    static u32 timer_ms;
    static u32 offset;
    const u32 delay_ms = 50;
    timer_ms += elapsed_ms;
    /* Return early if the frame would be the same (save clock cycles) */
    if(timer_ms < delay_ms) {
        return;
    }
    /* Otherwise, animate the offset. CAUTION! Using `%= delay_ms` here     */
    /* instead of `=0` or `-= delay_ms` allows for smoothing jitter at the  */
    /* normal 60 fps update rate. But, it also helps with catching up after */
    /* a long delay between frames. For example, long delays can happen     */
    /* when a background tab regains focus.                                 */
    timer_ms %= delay_ms;
    u32 turbo = GAMEPAD[GP_B] ? 3 : 1;
    offset = offset + 1
        + (GAMEPAD[GP_U] * turbo * (FB_WIDE * 2 - 16))
        + (GAMEPAD[GP_D] * turbo * (FB_WIDE * -2 + 16))
        + (GAMEPAD[GP_L] *  (turbo *  2)    )
        + (GAMEPAD[GP_R] * ((turbo * -2) -1));
    /* Recalculate the frame and paint it */
    test_pattern(offset);
    repaint();
}
