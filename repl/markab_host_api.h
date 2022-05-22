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
// consume zero or one bytes from stdin for each step, depending on what's
// going on with escape sequences and available input characters. Input bytes
// get accumulated in the Text Input Buffer (TIB).
// Return codes:
//  -1 --> EOF or ^C
//   0 --> normal
//   1 --> line of input is ready (got a CR)
int mkb_host_step_stdin();

#define BUF_SIZE 1023

// Text Input Buffer holding the line of text currently being edited
unsigned char mkb_host_TIB[BUF_SIZE];
size_t mkb_host_TIB_LEN = 0;

#endif

