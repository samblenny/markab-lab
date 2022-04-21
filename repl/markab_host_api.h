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

#endif

