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


/**************************************/
/* Exported Symbols: Global Variables */
/**************************************/

/* Frame buffer byte array (RGBA, 4 bytes per pixel)*/
__attribute__((visibility("default")))
u8 FB_BYTES[FB_WIDE * FB_HIGH * 4];

/* Size of frame buffer in bytes */
__attribute__((visibility("default")))
u32 FB_SIZE = sizeof(FB_BYTES);


/******************************************************/
/* Imported Symobols (to be linked by js wasm loader) */
/******************************************************/

/* This relies on -Wl,--allow-undefined and linking by wasm loader */
extern void js_trace(u32 code);


/*******************************/
/* Exported Symbols: Functions */
/*******************************/

/* Initialization function (gets called by js) */
__attribute__((visibility("default")))
i32 init(void) {
    /* Put some noise in the frame buffer */
    u32 i;
    for(i = 0; i < sizeof(FB_BYTES); i += 4) {
        FB_BYTES[i] = (u8) (i >> 8);
        FB_BYTES[i+1] = (u8) (i >> 1);
        FB_BYTES[i+2] = (u8) (i >> 1);
        FB_BYTES[i+3] = 255;
    }
    js_trace(sizeof(FB_BYTES));
    return 0;
}
