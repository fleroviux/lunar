/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// TODO: replace this file with a proper configuration file.

#pragma once

/// Enables log messages and assertions.
static constexpr bool gEnableLogging = true;

/// Enables the fast memory optimization.
/// Can somewhat increases framerates.
static constexpr bool gEnableFastMemory = false;

/// Sychronize ARM7 and ARM9 less frequently.
/// Dramatically increases framerates.
static constexpr bool gLooselySynchronizeCPUs = false;

/// Enables the experimental JIT recompiler
static constexpr bool gEnableJITRecompiler = false;
