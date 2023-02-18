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

/* Gamepad Button Bitfield Masks */
#define GP_A        (1)
#define GP_B        (2)
#define GP_SELECT   (4)
#define GP_START    (8)
#define GP_U       (16)
#define GP_D       (32)
#define GP_L       (64)
#define GP_R      (128)


/**************************************/
/* Exported Symbols: Global Variables */
/**************************************/

/* Frame buffer byte array (RGBA, 4 bytes per pixel) */
__attribute__((visibility("default")))
u8 FB_BYTES[FB_WIDE * FB_HIGH * 4];

/* Size of frame buffer in bytes */
__attribute__((visibility("default")))
u32 FB_SIZE = sizeof(FB_BYTES);

/* Gamepad button state bitfield */
__attribute__((visibility("default")))
u32 GAMEPAD;


/******************************************************/
/* Imported Symobols (to be linked by js wasm loader) */
/* These rely on -Wl,--allow-undefined to compile     */
/******************************************************/

extern void js_trace(u32 code);

extern void repaint();

extern void drawTiles();

extern void drawLines();


/*********************************/
/* Non-exported Global Variables */
/*********************************/

/* Previous gamepad button state packed into a bitfield */
static u8 PREV_GAMEPAD = 0;


/**************************/
/* Non-exported Functions */
/**************************/


/*******************************/
/* Exported Symbols: Functions */
/*******************************/

/* Initialization function (gets called by js) */
__attribute__((visibility("default")))
i32 init(void) {
    js_trace(sizeof(FB_BYTES));
    drawTiles();
    return 0;
}

/* Prepare the next frame */
/* elapsed_ms: number of milliseconds since previous frame */
__attribute__((visibility("default")))
void next(u32 elapsed_ms) {
    static u32 timer_ms;
    const u32 delay_ms = 50;
    timer_ms += elapsed_ms;
    /* Return early if the frame would be the same (save clock cycles) */
    if(timer_ms < delay_ms) {
/*        return;
*/
    }
    /* Check if any gamepad buttons changed */
    if(GAMEPAD == PREV_GAMEPAD) {
        return;
    }
    u32 diff = PREV_GAMEPAD ^ GAMEPAD;
    PREV_GAMEPAD = GAMEPAD;
    if(diff) {
        /* Button press draws LINE_STRIP, none draws TRIANGLE_STRIP */
        if(GAMEPAD) {
            drawLines();
        } else {
            drawTiles();
        }
    }
}
