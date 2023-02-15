/* Copyright (c) 2022 Sam Blenny */
/* SPDX-License-Identifier: MIT  */
"use strict";


/*********************************/
/* Constants & Global State Vars */
/*********************************/

const SCREEN = document.querySelector('#screen');
const CTX = SCREEN.getContext('2d', {alpha: false});
const wasmModule = "markab-engine.wasm";

var WASM_EXPORT;   /* Wrapper object for symbols exported by wasm module */
var FRAME_BUFFER;  /* Wrapper object for shared framebuffer memory region */

var PREV_TIMESTAMP;  /* Timestamp of previous animation frame */

/* Dimensions and zoom factor for screen represented by framebuffer data */
var WIDE = 240;
var HIGH = 160;
var ZOOM = 2;


/*****************/
/* Function Defs */
/*****************/

/* Initialize canvas element with zoom factor to provide pixel effect */
function initializeCanvas(wide, high, zoom) {
    /* Prevent blur on displays with non-integer dpi-ratio */
    let pixelRatio = window.devicePixelRatio || 1;
    SCREEN.width = wide;
    SCREEN.height = high;
    SCREEN.style.width = (wide * zoom / pixelRatio) + 'px';
    SCREEN.style.height = (high * zoom / pixelRatio) + 'px';
    CTX.scale(zoom, zoom);
}

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
            .catch(function (e) {console.error(e);}
        );
    } else {
        console.error("Browser does not support instantiateStreaming()");
    }
}

/* Initialize shared memory IPC bindings once WASM module is ready */
function initSharedMemBindings(result) {
    WASM_EXPORT = result.instance.exports;
    /* Make a Uint8 array slice for the shared framebuffer. Slice size comes */
    /* from dereferencing FB_SIZE pointer exported from wasm module.         */
    let wasmBufU8 = new Uint8Array(WASM_EXPORT.memory.buffer);
    let wasmDV = new DataView(WASM_EXPORT.memory.buffer);
    const buf = WASM_EXPORT.FB_BYTES.value;
    /* Dereference the FB_SIZE pointer to get sizeof(FB_BYTES) */
    const sizePtr = WASM_EXPORT.FB_SIZE.value;
    const size = wasmDV.getUint32(sizePtr, true);  /* true = little-endian */
    /* Wrap buffer in an ImageData so it can be passed to putImageData() */
    let clamped = new Uint8ClampedArray(WASM_EXPORT.memory.buffer, buf, size);
    FRAME_BUFFER = new ImageData(clamped, WIDE, HIGH);
}


/**************/
/* Event Loop */
/**************/

/* Initialize the mechanism to track elapsed time between frames */
function frameZero(timestamp_ms) {
    // Schedule the next frame
    window.requestAnimationFrame(regularFrame);
    PREV_TIMESTAMP = timestamp_ms;
}

/* Skip this frame (target is 30 fps) and schedule a regular frame */
function skipFrame(timestamp_ms) {
    window.requestAnimationFrame(regularFrame);
}

/* Call the wasm module to prepare the next frame */
function regularFrame(timestamp_ms) {
    /* Schedule the next frame */
    window.requestAnimationFrame(skipFrame);
    /* Compute elapsed time since previous frame in ms, convert to uint32_t */
    let elapsed = timestamp_ms - PREV_TIMESTAMP;
    PREV_TIMESTAMP = timestamp_ms;
    let ms = elapsed & 0xffffffff;
    WASM_EXPORT.next(ms);
}


/*************************/
/* JS Module Entry Point */
/*************************/

/* Prepare the canvas element */
initializeCanvas(WIDE, HIGH, ZOOM);

/* Load the wasm module */
wasmloadModule(() => {
    WASM_EXPORT.init();
    window.requestAnimationFrame(frameZero);  /* Start the event loop */
});
