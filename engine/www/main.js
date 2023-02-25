/* Copyright (c) 2023 Sam Blenny */
/* SPDX-License-Identifier: CC-BY-NC-SA-4.0 */
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

/* Animation Control */
var PREV_TIMESTAMP;     /* Timestamp of previous animation frame */
var PAUSED = false;     /* Controls requestAnimationFrame() animation loop */
var ANIMATE_EN = true;  /* Track whether window is focused (allow animation) */

/* Gamepad and Keyboard-WASD-pad button state */
var WASD_BITS = 0;     /* Bitfied for state of WASD keys */
var GAMEPAD_BITS = 0;  /* Bitfied for state of gamepad buttons */


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
        attribute vec2 a_position;
        attribute float a_tile_number;
        varying float v_tile_number;
        void main() {
            // Scale and translate from tile coords to clip space coords
            vec2 pos = a_position / vec2(15.0, 10.0);
            pos -= 0.5;
            pos *= vec2(2.0, -2.0);
            gl_Position = vec4(pos, 0.0, 1.0);
            v_tile_number = a_tile_number;
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
        uniform float player_tile;
        varying float v_tile_number;
        void main() {
            vec2 tile = mod(gl_FragCoord.xy, 32.0);
            float lum = v_tile_number / (15.0 * 10.0);
            float grn = (player_tile == v_tile_number) ? 0.9 : 0.0;
            float x = (tile.x < 2.0) || (tile.y < 2.0) ? 0.7 : lum;
            gl_FragColor = vec4(x, grn, x, 1.0); /* magenta */
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
    /* Save the uniform offsets */
    GLD.playerTile = gl.getUniformLocation(GLD.program, "player_tile");

    /* Schedule next init function */
    window.requestAnimationFrame(glInit4Vertices);
}

/* WebGL incremental init chain step 4: Bind vertex data */
function glInit4Vertices() {
    /* Add a buffer of triangles to tile the screen                     */
    /* - NDC coordinates are left-handed: +x=right +y=up +z=into_screen */
    /* - Default front-face winding order is counter-clockwise          */
    /* - Vertices are aligned on 4-byte boundary: (x, y, tile, padding) */
    const columns = 240/16;
    const rows = 160/16;
    const vertsPerTile = 6;
    const bytesPerTile = 24;  /* 6 vert/tile * 4 bytes/vert -> 24 bytes/tile */
    GLD.columns = columns;
    GLD.rows = rows;
    GLD.vertexCount = columns * rows * vertsPerTile;
    GLD.vertices = new Uint8Array(columns * rows * bytesPerTile);
    for(let y = 0; y < rows; y++) {
        for(let x = 0; x < columns; x++) {
            let tile = (y * columns) + x;
            let i = tile * bytesPerTile;
            GLD.vertices[i   ] = x;      /*  X--/ */
            GLD.vertices[i+ 1] = y;      /*  | /  */
            GLD.vertices[i+ 2] = tile;   /*  |/   */
            GLD.vertices[i+ 3] = 0;

            GLD.vertices[i+ 4] = x;      /*  +--/ */
            GLD.vertices[i+ 5] = y+1;    /*  | /  */
            GLD.vertices[i+ 6] = tile;   /*  X/   */
            GLD.vertices[i+ 7] = 0;

            GLD.vertices[i+ 8] = x+1;    /*  +--X */
            GLD.vertices[i+ 9] = y;      /*  | /  */
            GLD.vertices[i+10] = tile;   /*  |/   */
            GLD.vertices[i+11] = 0;

            GLD.vertices[i+12] = x+1;    /*    /X */
            GLD.vertices[i+13] = y;      /*   / | */
            GLD.vertices[i+14] = tile;   /*  /--+ */
            GLD.vertices[i+15] = 0;

            GLD.vertices[i+16] = x;      /*    /| */
            GLD.vertices[i+17] = y+1;    /*   / | */
            GLD.vertices[i+18] = tile;   /*  X--+ */
            GLD.vertices[i+19] = 0;

            GLD.vertices[i+20] = x+1;    /*    /| */
            GLD.vertices[i+21] = y+1;    /*   / | */
            GLD.vertices[i+22] = tile;   /*  /--X */
            GLD.vertices[i+23] = 0;
        }
    }
    GLD.vBuf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, GLD.vBuf);
    gl.bufferData(gl.ARRAY_BUFFER, GLD.vertices, gl.STATIC_DRAW);
    /* Bind the interleaved x,y position and tile_number vertex attributes */
    /* vertexAttribPointer(index, size, type, normalized, stride, pointer) */
    const gltype = gl.UNSIGNED_BYTE;
    const normalize = false;
    const stride = 4;
    const a_position = gl.getAttribLocation(GLD.program, 'a_position');
    gl.vertexAttribPointer(a_position, 2, gltype, normalize, stride, 0);
    gl.enableVertexAttribArray(a_position);
    const a_tile_number = gl.getAttribLocation(GLD.program, 'a_tile_number');
    gl.vertexAttribPointer(a_tile_number, 2, gltype, normalize, stride, 2);
    gl.enableVertexAttribArray(a_tile_number);

    /* Schedule next init function */
    window.requestAnimationFrame(glInit5LoadWasm);
}

/* WebGL incremental init chain step 5: Load Wasm module */
function glInit5LoadWasm() {
    /* Load the wasm module with callback to start the event loop */
    wasmloadModule(() => {
        WASM_EXPORT.init();
        window.requestAnimationFrame(frameZero);  /* Start event loop */
    });
}

/* Draw the grid of tiles, 15 wide by 10 high */
function drawTiles() {
    const first = 0;
    gl.drawArrays(gl.TRIANGLES, first, GLD.vertexCount);
}

/* Tell shaders which tile the player character is on */
function setPlayerTile(n) {
    const tile = n < 0 ? 0 : (n > 150 ? 150 : n);
    gl.uniform1f(GLD.playerTile, tile);
}

/* Draw indexed vertices as a line strip */
function drawLines() {
    const first = 0;
    gl.drawArrays(gl.LINE_STRIP, first, GLD.vertexCount);
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
            drawLines: drawLines,
            setPlayerTile: setPlayerTile,
            drawTiles: drawTiles,
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

/* Initialize shared memory IPC bindings once WASM module is ready */
function initSharedMemBindings(result) {
    WASM_EXPORT = result.instance.exports;
    /* Make a Uint8 array slice for the shared framebuffer. Slice size comes */
    /* from dereferencing FB_SIZE pointer exported from wasm module.         */
    let wasmBufU8 = new Uint8Array(WASM_EXPORT.memory.buffer);
    let wasmDV = new DataView(WASM_EXPORT.memory.buffer);

    /* Set up the gampad button vector */
    /* The `| WASM_EXPORT.GAMEPAD` (i.e. not `.value`) is for Safari */
    const gp = WASM_EXPORT.GAMEPAD.value | WASM_EXPORT.GAMEPAD;
    GAMEPAD = new Uint32Array(WASM_EXPORT.memory.buffer, gp, 1);
    unpressAllButtons();
}


/********************************************************/
/* Gamepad and Keyboard equivalents (WASD, arrows, etc) */
/********************************************************/

/* Gamepad button bits */
const gpA   =   1;
const gpB   =   2;
const gpSel =   4;  /* Select */
const gpSt  =   8;  /* Start  */
const gpUp  =  16;  /* dpad up    */
const gpDn  =  32;  /* dpad down  */
const gpL   =  64;  /* dpad left  */
const gpR   = 128;  /* dpad right */

/* Clear gamepad and WASD-pad buttons and schedule a frame if needed */
function unpressAllButtons() {
    const oldBits = GAMEPAD_BITS | WASD_BITS;
    WASD_BITS = 0;
    GAMEPAD_BITS = 0;
    GAMEPAD[0] = 0;
    if(oldBits != 0) {
        window.requestAnimationFrame(evenFrame);
    }
}

/* Update keyboard WASD-pad buttons and schedule a frame if needed */
function setWASDButtons(bits) {
    /* Carefully combine (OR) the WASD-pad and gamepad button states */
    const newBits = (bits & 0xffffffff);
    const oldBits = GAMEPAD_BITS | WASD_BITS;
    WASD_BITS = newBits;
    var mergedBits = newBits | GAMEPAD_BITS;
    /* Deconflict simultaneous left+right or up+down inputs */
    if(newBits & gpL ) { mergedBits &= ~gpR;  }
    if(newBits & gpR ) { mergedBits &= ~gpL;  }
    if(newBits & gpUp) { mergedBits &= ~gpDn; }
    if(newBits & gpDn) { mergedBits &= ~gpUp; }
    /* Only proceed if merged button state differs from old button state */
    if(oldBits == mergedBits) {
        return;
    }
    if(PAUSED) {
        /* Trigger a frame since gampad polling loop is not active */
        window.requestAnimationFrame(evenFrame);
        /* Run the animation loop */
        PAUSED = 0;
    }
    /* Update shared memory for gamepad bitfield */
    GAMEPAD[0] = mergedBits;
}

/* Update shared memory for gamepad buttons and schedule a frame if needed */
function setGamepadButtons(bits) {
    /* Carefully combine (OR) the WASD-pad and gamepad button states */
    const newBits = (bits & 0xffffffff);
    const oldBits = GAMEPAD_BITS | WASD_BITS;
    GAMEPAD_BITS = newBits;
    var mergedBits = newBits | WASD_BITS;
    /* Deconflict simultaneous left+right or up+down inputs */
    if(newBits & gpL ) { mergedBits &= ~gpR;  }
    if(newBits & gpR ) { mergedBits &= ~gpL;  }
    if(newBits & gpUp) { mergedBits &= ~gpDn; }
    if(newBits & gpDn) { mergedBits &= ~gpUp; }
    /* Only proceed if merged button state differs from old button state */
    if(oldBits == mergedBits) {
        return;
    }
    if(PAUSED) {
        /* Trigger a frame since gampad polling loop is not active */
        window.requestAnimationFrame(evenFrame);
    }
    /* Update shared memory for gamepad bitfield */
    GAMEPAD[0] = mergedBits;
}

/* Get a copy of the WASD-pad button status bitfield */
function getWASDButtons() {
    return WASD_BITS;
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
        bits |= b[ 1].value ?   gpA : 0;  /* A      (right cluster: right)  */
        bits |= b[ 0].value ?   gpB : 0;  /* B      (right cluster: bottom) */
        bits |= b[ 8].value ? gpSel : 0;  /* Select (center cluster: left)  */
        bits |= b[ 9].value ?  gpSt : 0;  /* Start  (center cluster: right) */
        bits |= b[12].value ?  gpUp : 0;  /* Up     (dpad) */
        bits |= b[13].value ?  gpDn : 0;  /* Down   (dpad) */
        bits |= b[14].value ?   gpL : 0;  /* Left   (dpad) */
        bits |= b[15].value ?   gpR : 0;  /* Right  (dpad) */
        setGamepadButtons(bits);
    } else if(b.length == 11 && a.length == 8) {
        /* Assuming this is an id="045e-028e-Microsoft X-Box 360 pad". */
        /* In Firefox on Debian 11, my Sn30 Pro has .mapping = "" and  */
        /* the dpad buttons shows up in .axes instead of .buttons.     */
        bits |= b[1].value ?   gpA : 0;  /* A (right cluster: right)  */
        bits |= b[0].value ?   gpB : 0;  /* B (right cluster: bottom) */
        bits |= b[6].value ? gpSel : 0;  /* Select             */
        bits |= b[7].value ?  gpSt : 0;  /* Start              */
        bits |= a[7] == -1 ?  gpUp : 0;  /* Up    = -1  (dpad) */
        bits |= a[7] ==  1 ?  gpDn : 0;  /* Down  =  1  (dpad) */
        bits |= a[6] == -1 ?   gpL : 0;  /* Left  = -1  (dpad) */
        bits |= a[6] ==  1 ?   gpR : 0;  /* Right =  1  (dpad) */
        setGamepadButtons(bits);
    } else {
        /* Ignore unknown gamepad mapping */
        if(GamepadMapWarnArmed) {
            console.warn("Ignoring gamepad because it uses unknown mapping");
            GamepadMapWarnArmed = false;
        }
    }
}

/* Handle keydown events by setting equivalent gamepad button press bits */
function handleKeydown(e) {
    if(e.defaultPrevented || e.ctrlKey || e.metaKey || e.shiftKey) {
        return;
    }
    /* CAUTION! event.code uses QWERTY layout locations regardless */
    /*          of what the actual keyboard layout may be set to.  */
    var bits = getWASDButtons();
    switch(e.code) {
        case "Space":  /* A */
        case "Slash":
        case "KeyX":
            bits |= gpA;
            break;
        case "Period":  /* B */
        case "KeyZ":
            bits |= gpB;
            break;
        case "Backspace":  /* Select */
            bits |= gpSel;
            break;
        case "Enter":      /* Start */
            bits |= gpSt;
            break;
        case "ArrowUp":  /* Up (clear down) */
        case "KeyW":
            bits |= gpUp;
            bits &= ~gpDn;
            break;
        case "ArrowDown":  /* Down (clear up) */
        case "KeyS":
            bits |= gpDn;
            bits &= ~gpUp;
            break;
        case "ArrowLeft":  /* Left (clear right) */
        case "KeyA":
            bits |= gpL;
            bits &= ~gpR;
            break;
        case "ArrowRight":  /* Right (clear left) */
        case "KeyD":
            bits |= gpR;
            bits &= ~gpL;
            break;
        default:
            return;
    }
    e.preventDefault();
    setWASDButtons(bits);
}

/* Handle keydown events by clearing equivalent gamepad button press bits */
function handleKeyup(e) {
    if(e.defaultPrevented /* ignoring modifiers here is intentional */) {
        return;
    }
    var bits = getWASDButtons();
    switch(e.code) {
        case "Space":  /* A */
        case "Slash":
        case "KeyX":
            bits &= ~gpA;
            break;
        case "Period":  /* B */
        case "KeyZ":
            bits &= ~gpB;
            break;
        case "Backspace":  /* Select */
            bits &= ~gpSel;
            break;
        case "Enter":  /* Start */
            bits &= ~gpSt;
            break;
        case "ArrowUp":  /* Up */
        case "KeyW":
            bits &= ~gpUp;
            break;
        case "ArrowDown":  /* Down */
        case "KeyS":
            bits &= ~gpDn;
            break;
        case "ArrowLeft":  /* Left */
        case "KeyA":
            bits &= ~gpL;
            break;
        case "ArrowRight":  /* Right */
        case "KeyD":
            bits &= ~gpR;
            break;
        default:
            return;
    }
    e.preventDefault();
    setWASDButtons(bits);
}

/* When window loses focus, inhibit gamepad polling */
function handleBlur() {
    ANIMATE_EN = false;
    /* Unpress any buttons that were pressed during loss of focus */
    unpressAllButtons();
}

/* When window regains focus, resume gamepad polling */
function handleFocus() {
    ANIMATE_EN = true;
    if(!PAUSED) {
        /* Restart gamepad polling if a gamepad is connected */
        window.requestAnimationFrame(oddFrame);
    }
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

/* Poll for gamepad input on odd frames */
function oddFrame(timestamp_ms) {
    /* Schedule the next frame */
    const requestID = window.requestAnimationFrame(evenFrame);
    try {
        /* Update the gamepad state */
        if(!PAUSED && ANIMATE_EN) {
            pollGamepad();
        }
    } catch(e) {
        /* If something goes wrong, stop the animation loop */
        window.cancelAnimationFrame(requestID);
        throw e;
    }
}

/* Call wasm module on even frames and schedule another frame if needed */
function evenFrame(timestamp_ms) {
    /* Keep polling gamepad if window has focus and gamepad is connected */
    if(!PAUSED && ANIMATE_EN) {
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
    /* Start animation frame loop to poll gamepad buttons */
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
        unpressAllButtons();
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

/* Register event handlers for keyboard gamepad equivalents (WASD, etc) */
window.addEventListener("keydown", handleKeydown);
window.addEventListener("keyup", handleKeyup);

/* Register onblur and onfocus events to interrupt gamepad polling when     */
/* window has lost focus. This is an attempt to preempt edge-case weirdness */
/* due to unintended input, the browser throttling our framerate, etc.      */
window.addEventListener("blur", handleBlur);
window.addEventListener("focus", handleFocus);


/* Start the WebGL canvas initialization chain (split across rAF callbacks) */
glInit0Clear();
