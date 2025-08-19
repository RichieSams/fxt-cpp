/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

/**
 * The macros below are inspired by the trace library
 * https://fuchsia.googlesource.com/fuchsia/+/9cf01c788257/zircon/system/ulib/trace/include/lib/trace/internal/pairs_internal.h
 *
 * Fuchsia is is governed by a BSD-style license
 *
 * Copyright 2019 The Fuchsia Authors.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

// Multiple expansion layers to ensure proper macro processing
#define FXT_INTERNAL_EXPAND(...) __VA_ARGS__
#define FXT_INTERNAL_EXPAND2(...) FXT_INTERNAL_EXPAND(__VA_ARGS__)
#define FXT_INTERNAL_EXPAND3(...) FXT_INTERNAL_EXPAND2(__VA_ARGS__)

/**
 * @brief Count the number of pairs in the argument list
 *
 * When the number of arguments is uneven, rounds down.
 * Works with 0 to 15 pairs.
 */
#define FXT_INTERNAL_COUNT_PAIRS(...) \
	FXT_INTERNAL_EXPAND(FXT_INTERNAL_COUNT_PAIRS_(__VA_ARGS__, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0))

#define FXT_INTERNAL_COUNT_PAIRS_(_15, _15X, _14, _14X, _13, _13X, _12, _12X, _11, _11X, _10, _10X, _9, _9X, _8, _8X, _7, _7X, _6, _6X, _5, _5X, _4, _4X, _3, _3X, _2, _2X, _1, _1X, N, ...) N

// Implementation macros for different pair counts
#define FXT_INTERNAL_SELECT_0(fn, ...) /* empty */
#define FXT_INTERNAL_SELECT_1(fn, ...) /* empty */
#define FXT_INTERNAL_SELECT_2(fn, a, b, ...) fn(a, b)
#define FXT_INTERNAL_SELECT_3(fn, a, b, ...) fn(a, b)
#define FXT_INTERNAL_SELECT_4(fn, a, b, c, d, ...) fn(a, b), fn(c, d)
#define FXT_INTERNAL_SELECT_5(fn, a, b, c, d, ...) fn(a, b), fn(c, d)
#define FXT_INTERNAL_SELECT_6(fn, a, b, c, d, e, f, ...) fn(a, b), fn(c, d), fn(e, f)
#define FXT_INTERNAL_SELECT_7(fn, a, b, c, d, e, f, ...) fn(a, b), fn(c, d), fn(e, f)
#define FXT_INTERNAL_SELECT_8(fn, a, b, c, d, e, f, g, h, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h)
#define FXT_INTERNAL_SELECT_9(fn, a, b, c, d, e, f, g, h, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h)
#define FXT_INTERNAL_SELECT_10(fn, a, b, c, d, e, f, g, h, i, j, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j)
#define FXT_INTERNAL_SELECT_11(fn, a, b, c, d, e, f, g, h, i, j, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j)
#define FXT_INTERNAL_SELECT_12(fn, a, b, c, d, e, f, g, h, i, j, k, l, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l)
#define FXT_INTERNAL_SELECT_13(fn, a, b, c, d, e, f, g, h, i, j, k, l, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l)
#define FXT_INTERNAL_SELECT_14(fn, a, b, c, d, e, f, g, h, i, j, k, l, m, n, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l), fn(m, n)
#define FXT_INTERNAL_SELECT_15(fn, a, b, c, d, e, f, g, h, i, j, k, l, m, n, ...) fn(a, b), fn(c, d), fn(e, f), fn(g, h), fn(i, j), fn(k, l), fn(m, n)

// Helper macros for indirection and selection
#define FXT_INTERNAL_SELECT_HELPER(N) FXT_INTERNAL_SELECT_##N
#define FXT_INTERNAL_SELECT(N) FXT_INTERNAL_SELECT_HELPER(N)

/**
 * Applies the given fn to each pair of arguments
 */
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV(fn, ...) \
	FXT_INTERNAL_EXPAND3(FXT_INTERNAL_SELECT(FXT_INTERNAL_EXPAND(FXT_INTERNAL_COUNT_PAIRS(__VA_ARGS__)))(fn, __VA_ARGS__))
