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


/**************************************/
/* Exported Symbols: Global Variables */
/**************************************/

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


/**************************/
/* Non-exported Constants */
/**************************/

/* Tile Resolution */
#define TILES_WIDE (15)
#define TILES_HIGH (10)

/* Delays in ms for walking and running */
/* Diagonal paces are regular paces scaled by approximately sqrt(2) */
#define RUN_MS       (140)
#define WALK_MS      (RUN_MS * 2)
#define WALK_DIAG_MS ((WALK_MS * 90) >> 6)
#define RUN_DIAG_MS  ((RUN_MS * 90) >> 6)
#define DEBOUNCE_MS  (100)

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


/*********************************/
/* Non-exported Global Variables */
/*********************************/

/* Previous gamepad button state packed into a bitfield */
static u8 PREV_GAMEPAD = 0;

/* Player character's current location in tile coordinates */
static u8 PLAYER_X = 7;
static u8 PLAYER_Y = 5;

/* Hold time for pressed dpad buttons */
static u32 DPAD_MS = 0;

/* Debounce flag for dpad buttons */
static u32 DPAD_DEBOUNCED = 0;

/* Types of motion that can be triggered by dpad buttons */
typedef enum e_DpMove {
    DpWait = 0,
    DpFace,
    DpStep,
} DpMove;


/**************************/
/* Non-exported Functions */
/**************************/

/* Update timer to handle button debounce and repeat.  */
/* The repeat interval is determined by pace_ms        */
/* Returns: 0: don't do the thing yet, 1: do the thing */
static DpMove updateDpadTimer(u16 interval_ms, u32 pace_ms) {
    /* Debounce the button press. This allows for precise diagonal motion */
    /* and for quick presses to face a new direction without taking steps */
    if(!DPAD_DEBOUNCED) {
        /* For initial button-down, immediately face in that direction */
        if(DPAD_MS == 0) {
            DPAD_MS = interval_ms;
            return DpFace;
        }
        /* After debounce delay expires, take a step */
        DPAD_MS += interval_ms;
        if(DPAD_MS >= DEBOUNCE_MS) {
            DPAD_DEBOUNCED = 1;
            DPAD_MS %= DEBOUNCE_MS; /* make sure 1st repeat isn't too soon */
            return DpStep;
        }
    }
    /* Once initial debounce period is over, update timer and decide */
    /* when enough time has passed to trigger a repeating step.      */
    DPAD_MS += interval_ms;
    if(DPAD_MS >= pace_ms) {
        DPAD_MS %= pace_ms;
        return DpStep;
    }
    return DpWait;
}

/* Test if dpad input represents diagonal motion             */
/* Returns: 1: motion is diagonal, 0: motion is not diagonal */
static u32 dpadIsDiagonal(u32 buttons) {
    switch(buttons & GP_DPAD) {
        case GP_U|GP_R:
        case GP_U|GP_L:
        case GP_D|GP_R:
        case GP_D|GP_L:
            return 1;
    }
    return 0;
}

/* Move player up. Returns 1 when redraw needed, otherwise 0. */
static u32 dpadUp() {
    if(PLAYER_Y > 0) {
        PLAYER_Y -= 1;
        return 1;
    }
    return 0;
}

/* Move player down. Returns 1 when redraw needed, otherwise 0. */
static u32 dpadDown() {
    if(PLAYER_Y < TILES_HIGH - 1) {
        PLAYER_Y += 1;
        return 1;
    }
    return 0;
}

/* Move player left. Returns 1 when redraw needed, otherwise 0. */
static u32 dpadLeft() {
    if(PLAYER_X > 0) {
        PLAYER_X -= 1;
        return 1;
    }
    return 0;
}

/* Move player right. Returns 1 when redraw needed, otherwise 0. */
static u32 dpadRight() {
    if(PLAYER_X < TILES_WIDE - 1) {
        PLAYER_X += 1;
        return 1;
    }
    return 0;
}

/* Clear the dpad timers */
static void dpadNone() {
    DPAD_MS = 0;
    DPAD_DEBOUNCED = 0;
}

/* Tell the front-end to update the player's tile coordinates */
static void updatePlayerTile() {
    u8 tile = (PLAYER_Y * TILES_WIDE) + PLAYER_X;
    setPlayerTile(tile);
}


/*******************************/
/* Exported Symbols: Functions */
/*******************************/

/* Initialization function (gets called by js) */
__attribute__((visibility("default")))
i32 init(void) {
    js_trace(12345);
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
    DpMove action = 0;
    u32 moved = 0;
    u32 dpad_bits = buttons & GP_DPAD;
    u32 b_down = buttons & GP_B;
    u32 pace;
    if(dpadIsDiagonal(buttons)) {
        pace = b_down ? RUN_DIAG_MS : WALK_DIAG_MS;
    } else {
        pace = b_down ? RUN_MS : WALK_MS;
    }
    if(dpad_bits) {
        action = updateDpadTimer(elapsed_ms, pace);
    } else {
        dpadNone();
    }
    if(action == DpStep) {
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
