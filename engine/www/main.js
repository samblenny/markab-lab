/* Copyright (c) 2022 Sam Blenny */
/* SPDX-License-Identifier: MIT  */
"use strict";


/*********************************/
/* Constants & Global State Vars */
/*********************************/

/* Initialize vars for accessing the "screen" (a canvas element) */
const SCREEN = document.querySelector('#screen');
const gl = SCREEN.getContext('webgl', {
    alpha: false,
    antialias: false,
    powerPreference: "low-power",
    preserveDrawingBuffer: false,
});
if(gl === null) {
    console.error("Unable to initialize webgl");
}
const GLD = {};  /* Dictionary to hold gl state objects during init chain */

/* WASM module stuff, including shared memory regions */
const wasmModule = "markab-engine.wasm";
var WASM_EXPORT;   /* Wrapper object for symbols exported by wasm module */
var GAMEPAD;       /* Wrapper object for shared gampad memory region */
var FRAME_BUFFER;  /* Wrapper object for shared framebuffer memory region */
var WIDE = 240;    /* Framebuffer px width */
var HIGH = 160;    /* Framebuffer px height */

/* Animation Control */
var PREV_TIMESTAMP;  /* Timestamp of previous animation frame */
var PAUSED = true;   /* Controls requestAnimationFrame() looping */


/******************************************************/
/* WebGL Shaders for 2D Orthographic Projection       */
/*                                                    */
/* Shader compile & link can be slow-ish, so the work */
/* is split across a chain of requestAnimationFrame   */
/* callback functions to avoid blocking main thread.  */
/******************************************************/

/* WebGL incremental init chain step 0: Clear canvas */
function glInit0Clear() {
    gl.clearColor(0.3, 0.3, 0.3, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    /* Schedule next init function */
    window.requestAnimationFrame(glInit1VertexShader);
}

/* WebGL incremental init chain step 1: Compile vertex shader */
function glInit1VertexShader() {
    const vertexSrc =
    `   precision mediump float;
        attribute vec4 a_position;  // <- assume gl provides .z=0, .w=1
        void main() {
            // Scale from tile coords to px coords
            vec2 pos = a_position.xy * vec2(16.0, 16.0);
            // Scale and translate from px coords to clip space
            pos /= vec2(120.0, 80.0);
            pos -= 1.0;
            pos *= vec2(1.0, -1.0);
            gl_Position = vec4(pos, 0.0, 1.0);
        }
    `;
    GLD.vShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(GLD.vShader, vertexSrc);
    gl.compileShader(GLD.vShader);
    if(!gl.getShaderParameter(GLD.vShader, gl.COMPILE_STATUS)) {
        throw "vShader compile log: " + gl.getShaderInfoLog(GLD.vShader);
    }
    /* Schedule next init function */
    window.requestAnimationFrame(glInit2FragmentShader);
}

/* WebGL incremental init chain step 2: Compile fragment shader */
function glInit2FragmentShader() {
    const fragmentSrc =
    `   precision mediump float;
        void main() {
            vec2 tile = mod(gl_FragCoord.xy, 16.0);
            float x = (tile.x < 1.0) || (tile.y < 1.0) ? 0.7 : 1.0;
            gl_FragColor = vec4(x, 0.0, x, 1.0); /* magenta */
        }
    `;
    GLD.fShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(GLD.fShader, fragmentSrc);
    gl.compileShader(GLD.fShader);
    if(!gl.getShaderParameter(GLD.fShader, gl.COMPILE_STATUS)) {
        throw "fShader compile log: " + gl.getShaderInfoLog(GLD.fShader);
    }
    /* Schedule next init function */
    window.requestAnimationFrame(glInit3Program);
}

/* WebGL incremental init chain step 3: Link and use program */
function glInit3Program() {
    /* Link gl Program */
    GLD.program = gl.createProgram();
    gl.attachShader(GLD.program, GLD.vShader);
    gl.attachShader(GLD.program, GLD.fShader);
    gl.linkProgram(GLD.program);
    if(!gl.getProgramParameter(GLD.program, gl.LINK_STATUS)) {
        throw "program link log: " + gl.getProgramInfoLog(program);
    }
    /* Start using the gl program */
    gl.useProgram(GLD.program);
    /* Release memory used to compile shaders */
    gl.deleteShader(GLD.vShader);
    gl.deleteShader(GLD.fShader);
    gl.detachShader(GLD.program, GLD.vShader);
    gl.detachShader(GLD.program, GLD.fShader);
    delete GLD.vShader;
    delete GLD.fShader;
    /* Schedule next init function */
    window.requestAnimationFrame(glInit4VerticesIndices);
}

/* WebGL incremental init chain step 4: Bind vertex and index data */
function glInit4VerticesIndices() {
    /* Add an indexed buffer of triangle strips to tile the screen          */
    /* - NDC coordinates are left-handed: +x=right +y=up +z=into_screen     */
    /* - Default front-face winding order is counter-clockwise              */
    /* - Triangle strip winding order is determined by first triangle       */
    /* - To append disjoint strips A and B into a single buffer, repeat the */
    /*   last vertex of A and first vertex of B to create a "degenerate"    */
    /*   triangle that marks the start of a new triangle strip.             */
    const columns = 240/16;
    const rows = 160/16;
    GLD.columns = columns;
    GLD.rows = rows;
    GLD.vertices = new Float32Array((columns + 1) * (rows + 1) * 2);
    for(let y = 0; y <= rows; y++) {
        for(let x = 0; x <= columns; x++) {
            let i = ((y * (columns + 1)) + x) * 2;
            GLD.vertices[i] = x;
            GLD.vertices[i+1] = y;
        }
    }
    GLD.vBuf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, GLD.vBuf);
    gl.bufferData(gl.ARRAY_BUFFER, GLD.vertices, gl.STATIC_DRAW);
    const a_position = gl.getAttribLocation(GLD.program, 'a_position');
    /* args: index, size, type, normalized, stride, pointer */
    /* CAUTION! This assumes gl will set .z=0 and .w=1 going for vec2->vec4 */
    gl.vertexAttribPointer(a_position, 2, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(a_position);

    /* Arrange the vertices into triangle strips */
    /* WebGL incremental init chain step 5: Bind index data         */
    /* Layout of indices for one row:                               */
    /*   ,+--------------------- 2 vertices to start first triangle */
    /*   |        ,+------------ plus 1 vertex per triangle         */
    /*   |        |          ,+- plus 2 vertices to separate strips */
    /*   2 + (columns * 2) + 2                                      */
    const i_per_row = 2 + (columns * 2) + 2;
    /* Note: The -2 is because the triangle strip for the final row does    */
    /* not need to end with the 2 extra vertices for a degenerate triangle. */
    GLD.indices = new Uint16Array(i_per_row * rows - 2);
    for(let y = 0; y < rows; y ++) {
        let baseY = y * i_per_row;
        let iy0 = (y    ) * (columns + 1);
        let iy1 = (y + 1) * (columns + 1);
        /* Start first triangle of triangle strip with 2 vertices, then */
        /* add 1 vertex per triangle for the remaining triangles. This  */
        /* adds 2 per iteration of the loop because there are two       */
        /* triangles per column. The triangle pattern tiles like this:  */
        /*   0--1--2--3--4   indices to begin first triangle:  0, 5     */
        /*   | /| /| /| /|   indices to finish first column:   1, 6     */
        /*   |/ |/ |/ |/ |   indices to define next column:    2, 7     */
        /*   5--6--7--8--9                                              */
        for(let x = 0; x <= columns; x++) {
            let i = baseY + (x * 2);
            GLD.indices[i] = iy0 + x;
            GLD.indices[i+1] = iy1 + x;
        }
        /* Make degenerate triangle to separate disjoint triangle strips */
        if(y + 1 < rows) {
            let i = baseY + i_per_row - 3;
            /* Duplicate this strip's last vertex  */
            GLD.indices[i+1] = GLD.indices[i];
            /* Duplicate next strip's first vertex */
            GLD.indices[i+2] = iy1;
        }
    }
    GLD.iBuf = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, GLD.iBuf);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, GLD.indices, gl.STATIC_DRAW);
    /* Schedule next init function */
    window.requestAnimationFrame(glInit5LoadWasm);
}

/* WebGL incremental init chain step 5: Load Wasm module */
function glInit5LoadWasm() {
    /* Draw some stuff */
    drawTiles();
    /* Load the wasm module with callback to start the event loop */
    wasmloadModule(() => {
        WASM_EXPORT.init();
        window.requestAnimationFrame(frameZero);  /* Start event loop */
    });
}

/* Draw indexed vertices as triangle strips */
function drawTiles() {
    gl.clear(gl.COLOR_BUFFER_BIT);
    const length = GLD.indices.length;
    gl.drawElements(gl.TRIANGLE_STRIP, length, gl.UNSIGNED_SHORT, 0);
}

/* Draw indexed vertices as a line strip */
function drawLines() {
    gl.clear(gl.COLOR_BUFFER_BIT);
    const length = GLD.indices.length;
    gl.drawElements(gl.LINE_STRIP, length, gl.UNSIGNED_SHORT, 0);
}


/*****************/
/* Function Defs */
/*****************/

/* Paint frame buffer (wasm shared memory) to screen (canvas element) */
function repaint() {
    // CTX.putImageData(FRAME_BUFFER, 0, 0);
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
            drawLines, drawLines,
            drawTiles, drawTiles,
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

/* Update the shared memory for gamepad button status */
function setGamepadButtons(bits) {
    GAMEPAD[0] = bits & 0xffffffff;
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
    const sizePtr = WASM_EXPORT.FB_SIZE.value | WASM_EXPORT.FB_SIZE;
    const size = wasmDV.getUint32(sizePtr, true);  /* true = little-endian */
    /* Wrap buffer in an ImageData so it can be passed to putImageData() */
    let clamped = new Uint8ClampedArray(WASM_EXPORT.memory.buffer, buf, size);
    FRAME_BUFFER = new ImageData(clamped, WIDE, HIGH);

    /* Set up the gampad button vector */
    const gp = WASM_EXPORT.GAMEPAD.value | WASM_EXPORT.GAMEPAD;
    GAMEPAD = new Uint32Array(WASM_EXPORT.memory.buffer, gp, 1);
    setGamepadButtons(0);
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
        return;  /* no gamepad... we're done */
    }
    /* Otherwise, attempt to decode button mapping */
    const b = gp0.buttons;
    const a = gp0.axes;
    var bits = 0x00;
    if(gp0.mapping == "standard") {
        /* This is for Gamepad API "Standard Gamepad" layout */
        /* See https://w3c.github.io/gamepad/#remapping */
        /* In this case, dpad gets mapped as buttons (not axes) */
        bits |= b[ 1].value ?   1 : 0;  /* A      (right cluster: right)  */
        bits |= b[ 0].value ?   2 : 0;  /* B      (right cluster: bottom) */
        bits |= b[ 8].value ?   4 : 0;  /* Select (center cluster: left)  */
        bits |= b[ 9].value ?   8 : 0;  /* Start  (center cluster: right) */
        bits |= b[12].value ?  16 : 0;  /* Up     (dpad) */
        bits |= b[13].value ?  32 : 0;  /* Down   (dpad) */
        bits |= b[14].value ?  64 : 0;  /* Left   (dpad) */
        bits |= b[15].value ? 128 : 0;  /* Right  (dpad) */
        setGamepadButtons(bits);
    } else if(b.length == 11 && a.length == 8) {
        /* Assuming this is an id="045e-028e-Microsoft X-Box 360 pad". */
        /* In Firefox on Debian 11, my Sn30 Pro has .mapping = "" and  */
        /* the dpad buttons shows up in .axes instead of .buttons.     */
        bits |= b[1].value ?   1 : 0;  /* A (right cluster: right)  */
        bits |= b[0].value ?   2 : 0;  /* B (right cluster: bottom) */
        bits |= b[6].value ?   4 : 0;  /* Select             */
        bits |= b[7].value ?   8 : 0;  /* Start              */
        bits |= a[7] == -1 ?  16 : 0;  /* Up    = -1  (dpad) */
        bits |= a[7] ==  1 ?  32 : 0;  /* Down  =  1  (dpad) */
        bits |= a[6] == -1 ?  64 : 0;  /* Left  = -1  (dpad) */
        bits |= a[6] ==  1 ? 128 : 0;  /* Right =  1  (dpad) */
        setGamepadButtons(bits);
    } else {
        /* Ignore unknown gamepad mapping */
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
    if(!PAUSED) {
        const requestID = window.requestAnimationFrame(oddFrame);
    }
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
    /* Start the animation loop */
    if(PAUSED) {
        PAUSED = false;
        window.requestAnimationFrame(oddFrame);
    }
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
        setGamepadButtons(0);
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

/* Start the WebGL canvas initialization chain (split across rAF callbacks) */
glInit0Clear();
