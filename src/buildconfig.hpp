/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

/// Enables log messages and assertions.
static constexpr bool gEnableLogging = false;

/// Enables the fast memory optimization.
/// Can somewhat increases framerates.
static constexpr bool gEnableFastMemory = false;

/// Sychronize ARM7 and ARM9 less frequently.
/// Dramatically increases framerates.
static constexpr bool gLooselySynchronizeCPUs = true;

/// Enables the experimental JIT recompiler
static constexpr bool gEnableJITRecompiler = true;
