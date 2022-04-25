// Copyright (c) 2022 Sam Blenny
// SPDX-License-Identifier: MIT
//
// Host api functions that libmarkab expects to be provided so it can access
// hardware devices.
//

#ifndef MARKAB_HOST_API_H
#define MARKAB_HOST_API_H

// Provides a stdlib write() to stdout, or the equivalent
size_t mkb_host_write(const void *buf, size_t count);

// Step the state machine that does non-blocking reads from stdin. This may
// consume zero, some, or many, bytes from stdin for each step, depending on
// what's going on with escape sequences and available input characters. There
// is no return value here. Instead, the state machine decides if and when to
// call libmarkab functions to handle input events. The primary input event
// that triggers such function calls is receiving a CR from the Enter key,
// signaling that the contents of the Text Input Buffer (TIB) are ready to be
// interpreted.
// Return codes:
//  -1 --> EOF or ^C
//   0 --> normal
int mkb_host_step_stdin();

#endif

