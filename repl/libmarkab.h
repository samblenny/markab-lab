// Copyright (c) 2022 Sam Blenny
// SPDX-License-Identifier: MIT
//
// Function exported by libmarkab
//

#ifndef LIBMARKAB_H
#define LIBMARKAB_H

// Cold boot Markab (this contains the outer interpreter's event loop)
extern void markab_cold();

// Interpret buf[0]..buf[count-1] as a line of Markab source code
extern void markab_outer(uint8_t *buf, uint32_t count);

#endif
