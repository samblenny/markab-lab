/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_H
#define LIBMKB_H

#ifdef PLAN_9
#  include "libmkb/op.h"
#  include "libmkb/vm.h"
#  include "libmkb/autogen.h"
#else
#  include "op.h"
#  include "vm.h"
#  include "autogen.h"
#endif

int mk_load_rom(const u8 * code, u32 code_len_bytes);

#endif /* LIBMKB_H */
