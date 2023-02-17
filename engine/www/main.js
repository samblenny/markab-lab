/* Copyright (c) 2022 Sam Blenny */
/* SPDX-License-Identifier: MIT  */
"use strict";


/*********************************/
/* Constants & Global State Vars */
/*********************************/

const SCREEN = document.querySelector('#screen');
const CTX = SCREEN.getContext('2d', {alpha: false});
const wasmModule = "markab-engine.wasm";

var WASM_EXPORT;     /* Wrapper object for symbols exported by wasm module */
var FRAME_BUFFER;    /* Wrapper object for shared framebuffer memory region */
var PREV_TIMESTAMP;  /* Timestamp of previous animation frame */
var GAMEPAD;         /* Wrapper object for shared gampad memory region */

/* Dimensions and zoom factor for screen represented by framebuffer data */
var WIDE = 240;
var HIGH = 160;


/*****************/
/* Function Defs */
/*****************/

/* Paint frame buffer (wasm shared memory) to screen (canvas element) */
function repaint() {
    CTX.putImageData(FRAME_BUFFER, 0, 0);
}


/*****************************/
/* WASM Module Load and Init */
/*****************************/

// Load WASM module, bind shared memory, then invoke callback.
function wasmloadModule(callback) {
    var importObject = {
        env: {
            js_trace: (code) => {console.log("wasm trace:", code);},
            repaint: repaint,
        },
    };
    if ("instantiateStreaming" in WebAssembly) {
        WebAssembly.instantiateStreaming(fetch(wasmModule), importObject)
            .then(initSharedMemBindings)
            .then(callback)
            .catch(function (e) {console.error(e);});
    } else {
        // Fallback for older versions of Safari
        fetch(wasmModule)
            .then(response => response.arrayBuffer())
            .then(bytes => WebAssembly.instantiate(bytes, importObject))
            .then(initSharedMemBindings)
            .then(callback)
            .catch(function (e) {console.error(e);});
    }
}

/* Clear the gamepad button vector, meaning set each button as not-pressed */
function clearGamepadButtons() {
    if(GAMEPAD !== undefined) {
        for(let i = 0; i < GAMEPAD.length; i++) {
            GAMEPAD[i] = 0x00;
        }
    }
}

/* Initialize shared memory IPC bindings once WASM module is ready */
function initSharedMemBindings(result) {
    WASM_EXPORT = result.instance.exports;
    /* Make a Uint8 array slice for the shared framebuffer. Slice size comes */
    /* from dereferencing FB_SIZE pointer exported from wasm module.         */
    let wasmBufU8 = new Uint8Array(WASM_EXPORT.memory.buffer);
    let wasmDV = new DataView(WASM_EXPORT.memory.buffer);
    /* NOTE! the `... | WASM_EXPORT.FB_BYTES` helps on old mobile safari */
    const buf = WASM_EXPORT.FB_BYTES.value | WASM_EXPORT.FB_BYTES;
    /* Dereference the FB_SIZE pointer to get sizeof(FB_BYTES) */
    /* NOTE! the `... | WASM_EXPORT.FB_SIZE` helps on old mobile safari */
    const sizePtr = WASM_EXPORT.FB_SIZE.value | WASM_EXPORT.FB_SIZE;
    const size = wasmDV.getUint32(sizePtr, true);  /* true = little-endian */
    /* Wrap buffer in an ImageData so it can be passed to putImageData() */
    let clamped = new Uint8ClampedArray(WASM_EXPORT.memory.buffer, buf, size);
    FRAME_BUFFER = new ImageData(clamped, WIDE, HIGH);

    /* Set up the gampad button vector */
    const gp = WASM_EXPORT.GAMEPAD.value | WASM_EXPORT.GAMEPAD;
    const gpSizePtr = WASM_EXPORT.GP_SIZE.value | WASM_EXPORT.GP_SIZE;
    const gpSize = wasmDV.getUint32(gpSizePtr, true);
    GAMEPAD = new Uint8Array(WASM_EXPORT.memory.buffer, gp, gpSize);
    clearGamepadButtons();
}


/**************/
/* Event Loop */
/**************/

/* Initialize the mechanism to track elapsed time between frames */
function frameZero(timestamp_ms) {
    // Schedule the next frame
    window.requestAnimationFrame(oddFrame);
    PREV_TIMESTAMP = timestamp_ms;
}

/* Poll gamepad 0 buttons and update the gamepad vector shared memory.       */
/* This was tested for 8BitDo Sn30 Pro on macOS Chrome and Debian Firefox.   */
/* Not sure what will happen with other OS + browser + gamepad combinations. */
var GamepadMapWarnArmed = true;
function pollGamepad() {
    /* Check if gamepad is present */
    const gp0 = navigator.getGamepads()[0];
    if(gp0 === undefined || gp0 == null) {
        /* No gamepad */
        GAMEPAD[8] = 0 | 0;  /* clear gamepad-connected bit */
        return;              /* bail out to avoid triggering exceptions */
    }
    /* Otherwise, attempt to decode button mapping */
    const b = gp0.buttons;
    const a = gp0.axes;
    if(gp0.mapping == "standard") {
        /* This is for Gamepad API "Standard Gamepad" layout */
        /* See https://w3c.github.io/gamepad/#remapping */
        /* In this case, dpad gets mapped as buttons (not axes) */
        GAMEPAD[0] = b[ 1].value | 0;  /* A      (right cluster: right)  */
        GAMEPAD[1] = b[ 0].value | 0;  /* B      (right cluster: bottom) */
        GAMEPAD[2] = b[ 8].value | 0;  /* Select (center cluster: left)  */
        GAMEPAD[3] = b[ 9].value | 0;  /* Start  (center cluster: right) */
        GAMEPAD[4] = b[12].value | 0;  /* Up     (dpad) */
        GAMEPAD[5] = b[13].value | 0;  /* Down   (dpad) */
        GAMEPAD[6] = b[14].value | 0;  /* Left   (dpad) */
        GAMEPAD[7] = b[15].value | 0;  /* Right  (dpad) */
        GAMEPAD[8] = 1 | 0;            /* Set gamepad-connected bit */
    } else if(b.length == 11 && a.length == 8) {
        /* Assuming this is an id="045e-028e-Microsoft X-Box 360 pad". */
        /* In Firefox on Debian 11, my Sn30 Pro has .mapping = "" and  */
        /* the dpad buttons shows up in .axes instead of .buttons.     */
        GAMEPAD[0] = b[1].value | 0;   /* A (right cluster: right)  */
        GAMEPAD[1] = b[0].value | 0;   /* B (right cluster: bottom) */
        GAMEPAD[2] = b[6].value | 0;          /* Select             */
        GAMEPAD[3] = b[7].value | 0;          /* Start              */
        GAMEPAD[4] = a[7] == -1 ? 1|0 : 0|0;  /* Up    = -1  (dpad) */
        GAMEPAD[5] = a[7] ==  1 ? 1|0 : 0|0;  /* Down  =  1  (dpad) */
        GAMEPAD[6] = a[6] == -1 ? 1|0 : 0|0;  /* Left  = -1  (dpad) */
        GAMEPAD[7] = a[6] ==  1 ? 1|0 : 0|0;  /* Right =  1  (dpad) */
        GAMEPAD[8] = 1 | 0;            /* Set gamepad-connected bit */
    } else {
        /* Ignore unknown gamepad mapping */
        GAMEPAD[8] = 0 | 0;         /* clear gamepad-connected bit */
        if(GamepadMapWarnArmed) {
            console.warn("Ignoring gamepad because it uses unknown mapping");
            GamepadMapWarnArmed = false;
        }
    }

}

/* Poll for gamepad input on odd frames */
function oddFrame(timestamp_ms) {
    /* Schedule the next frame */
    const requestID = window.requestAnimationFrame(evenFrame);
    try {
        /* Update the gamepad state */
        pollGamepad();
    } catch(e) {
        /* If something goes wrong, stop the animation loop */
        window.cancelAnimationFrame(requestID);
        throw e;
    }
}

/* Call the wasm module on even frames */
function evenFrame(timestamp_ms) {
    /* Schedule the next frame */
    const requestID = window.requestAnimationFrame(oddFrame);
    /* Compute elapsed time since previous frame in ms, convert to uint32_t */
    const elapsed = timestamp_ms - PREV_TIMESTAMP;
    PREV_TIMESTAMP = timestamp_ms;
    const ms = elapsed & 0xffffffff;
    try {
        /* Transfer control to wasm module to generate next frame */
        WASM_EXPORT.next(ms);
    } catch(e) {
        /* If something goes wrong, stop the animation loop */
        window.cancelAnimationFrame(requestID);
        throw e;
    }
}

/* Handle gamepad connect event */
function gamepadConn(e) {
    /* Log gamepad info to help with troubleshooting of button mappings */
    const ix = e.gamepad.index;
    const id = e.gamepad.id;
    const m = e.gamepad.mapping;
    const a = e.gamepad.axes.length;
    const b = e.gamepad.buttons.length;
    const msg = `index:${ix} id:"${id}" mapping:"${m}" buttons:${b} axes:${a}`;
    if(m == "standard") {
        console.log("gamepadConn:", msg);
    } else {
        console.warn("gamepadConn (nonstandard mapping):", msg);
    }
}

/* Handle gamepad disconnect event */
function gamepadDisconn(e) {
    if(e.gamepad.index == 0) {
        /* If buttons were still pressed at disconnect, unpress them */
        clearGamepadButtons();
    }
    console.log("gamepadDisonn", e);
}


/*************************/
/* JS Module Entry Point */
/*************************/

/* Register event handlers to use the Gamepad API */
/* Works on Chrome and Firefox. Safari mysteriously ignores my gamepad. */
window.addEventListener("gamepadconnected", gamepadConn);
window.addEventListener("gamepaddisconnected", gamepadDisconn);

/* Load the wasm module and start the event loop */
wasmloadModule(() => {
    WASM_EXPORT.init();
    window.requestAnimationFrame(frameZero);  /* Start the event loop */
});
