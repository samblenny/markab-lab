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

/* Tile Resolution */
#define TILES_WIDE (15)
#define TILES_HIGH (10)

/* Delays in ms for walking and running */
#define WALK_MS (300)
#define RUN_MS  (150)

/* Gamepad Button Bitfield Masks */
#define GP_A        (1)
#define GP_B        (2)
#define GP_SELECT   (4)
#define GP_START    (8)
#define GP_U       (16)
#define GP_D       (32)
#define GP_L       (64)
#define GP_R      (128)
#define GP_DPAD   (GP_U|GP_D|GP_L|GP_R)

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

extern void setPlayerTile(u32 tile);

extern void drawLines();


/*********************************/
/* Non-exported Global Variables */
/*********************************/

/* Previous gamepad button state packed into a bitfield */
static u8 PREV_GAMEPAD = 0;

/* Player character's current location in tile coordinates */
static u8 PLAYER_X = 7;
static u8 PLAYER_Y = 5;

/* Hold times for pressed buttons */
static u32 HOLD_UDLR = 0;


/**************************/
/* Non-exported Functions */
/**************************/

/* Update timer to handle button debounce and repeat.  */
/* The repeat interval is determined by pace_ms        */
/* Modifies: *timer_ms gets updated from interval_ms   */
/* Returns: 0: don't do the thing yet, 1: do the thing */
u32 updateTimer(u32 interval_ms, u32 pace_ms, u32 *timer_ms) {
    if(timer_ms == (u32 *)0) {
        return 0;
    }
    if(*timer_ms == 0) {
        *timer_ms = 1;
        return 1;
    }
    *timer_ms += interval_ms;
    if(*timer_ms >= pace_ms) {
        *timer_ms %= pace_ms;
        return 1;
    }
    return 0;
}

/* Move player up. Returns 1 when redraw needed, otherwise 0. */
u32 dpadUp() {
    if(PLAYER_Y > 0) {
        PLAYER_Y -= 1;
        return 1;
    }
    return 0;
}

/* Move player down. Returns 1 when redraw needed, otherwise 0. */
u32 dpadDown() {
    if(PLAYER_Y < TILES_HIGH - 1) {
        PLAYER_Y += 1;
        return 1;
    }
    return 0;
}

/* Move player left. Returns 1 when redraw needed, otherwise 0. */
u32 dpadLeft() {
    if(PLAYER_X > 0) {
        PLAYER_X -= 1;
        return 1;
    }
    return 0;
}

/* Move player right. Returns 1 when redraw needed, otherwise 0. */
u32 dpadRight() {
    if(PLAYER_X < TILES_WIDE - 1) {
        PLAYER_X += 1;
        return 1;
    }
    return 0;
}

/* Clear the dpad timers */
void dpadNone() {
    HOLD_UDLR = 0;
}

/* Tell the front-end to update the player's tile coordinates */
void updatePlayerTile() {
    u8 tile = (PLAYER_Y * TILES_WIDE) + PLAYER_X;
    setPlayerTile(tile);
}


/*******************************/
/* Exported Symbols: Functions */
/*******************************/

/* Initialization function (gets called by js) */
__attribute__((visibility("default")))
i32 init(void) {
    js_trace(sizeof(FB_BYTES));
    updatePlayerTile();
    drawTiles();
    return 0;
}

/* Prepare the next frame */
/* elapsed_ms: number of milliseconds since previous frame */
__attribute__((visibility("default")))
void next(u32 elapsed_ms) {
    /* Check if any gamepad buttons changed */
    u32 diff = PREV_GAMEPAD ^ GAMEPAD;
    PREV_GAMEPAD = GAMEPAD;
    u32 buttons = GAMEPAD;
    /* Update player position based on dpad button state */
    u32 doDpadAction = 0;
    u32 moved = 0;
    if((buttons & GP_B) && (buttons & GP_DPAD)) {
        /* Run if B is pressed */
        doDpadAction = updateTimer(elapsed_ms, RUN_MS, &HOLD_UDLR);
    } else if(buttons & GP_DPAD) {
        /* Otherwise walk */
        doDpadAction = updateTimer(elapsed_ms, WALK_MS, &HOLD_UDLR);
    } else {
        dpadNone();
    }
    if(doDpadAction) {
        if(buttons & GP_U) { moved += dpadUp();    }
        if(buttons & GP_D) { moved += dpadDown();  }
        if(buttons & GP_L) { moved += dpadLeft();  }
        if(buttons & GP_R) { moved += dpadRight(); }
    }
    /* Redraw if needed */
    if(moved || diff) {
        updatePlayerTile();
        if(GAMEPAD & (GP_SELECT|GP_START|GP_A)) {
            drawLines();
        } else {
            drawTiles();
        }
    }
}
