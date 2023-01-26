/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_H
#define LIBMKB_H

#include "op.h"
#include "vm.h"
#include "autogen.h"

int mk_load_rom(const u8 * code, u32 code_len_bytes);

#endif /* LIBMKB_H */
