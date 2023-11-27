/* FXT - A library for creating Fuschia Tracing System (FXT) files
 *
 * FXT is the legal property of Adrian Astley
 * Copyright Adrian Astley 2023
 */

/**
 * The macros below are borrowed from the trace library
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

/**
 * Count the number of pairs of arguments passed to it without evaluating them.
 * When the number of arguments is uneven, rounds down.
 * Works with 0 to 15 pairs.
 */
#define FXT_INTERNAL_COUNT_PAIRS(...) \
	FXT_INTERNAL_COUNT_PAIRS_(__VA_ARGS__, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0)
#define FXT_INTERNAL_COUNT_PAIRS_(_15, _15X, _14, _14X, _13, _13X, _12, _12X, _11, _11X, _10, _10X, _9, _9X, _8, _8X, _7, _7X, _6, _6X, _5, _5X, _4, _4X, _3, _3X, _2, _2X, _1, _1X, N, ...) \
	N

// Applies a function or macro to each pair of arguments to produce a
// comma-separated result.  Works with 0 to 15 pairs.
//
// Example:
//     #define MY_FN(a, b)
//     FXT_INTERNAL_APPLY_PAIRWISE_CSV(MY_FN, "x", 1, "y", 2)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV(fn, ...)                            \
	FXT_INTERNAL_APPLY_PAIRWISE_CSV_(FXT_INTERNAL_COUNT_PAIRS(__VA_ARGS__)) \
	(fn, __VA_ARGS__)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV_(n) FXT_INTERNAL_APPLY_PAIRWISE_CSV__(n)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV__(n) FXT_INTERNAL_APPLY_PAIRWISE_CSV##n

// clang-format off
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV0(...)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV1(fn, k1, v1) \
	fn(k1, v1)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV2(fn, k1, v1, k2, v2) \
	fn(k1, v1), fn(k2, v2)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV3(fn, k1, v1, k2, v2, k3, v3) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV4(fn, k1, v1, k2, v2, k3, v3, k4, v4) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV5(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                  \
	fn(k5, v5)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV6(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                          \
	fn(k5, v5), fn(k6, v6)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV7(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                  \
	fn(k5, v5), fn(k6, v6), fn(k7, v7)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV8(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                          \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV9(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8, k9, v9) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                                  \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8),                                                                  \
	fn(k9, v9)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV10(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8, k9, v9, k10, v10) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                                             \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8),                                                                             \
	fn(k9, v9), fn(k10, v10)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV11(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8, k9, v9, k10, v10, k11, v11) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                                                       \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8),                                                                                       \
	fn(k9, v9), fn(k10, v10), fn(k11, v11)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV12(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8, k9, v9, k10, v10, k11, v11, k12, v12) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                                                                 \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8),                                                                                                 \
	fn(k9, v9), fn(k10, v10), fn(k11, v11), fn(k12, v12)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV13(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8, k9, v9, k10, v10, k11, v11, k12, v12, k13, v13) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                                                                           \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8),                                                                                                           \
	fn(k9, v9), fn(k10, v10), fn(k11, v11), fn(k12, v12),                                                                                                     \
	fn(k13, v13)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV14(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8, k9, v9, k10, v10, k11, v11, k12, v12, k13, v13, k14, v14) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                                                                                     \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8),                                                                                                                     \
	fn(k9, v9), fn(k10, v10), fn(k11, v11), fn(k12, v12),                                                                                                               \
	fn(k13, v13), fn(k14, v14)
#define FXT_INTERNAL_APPLY_PAIRWISE_CSV15(fn, k1, v1, k2, v2, k3, v3, k4, v4, k5, v5, k6, v6, k7, v7, k8, v8, k9, v9, k10, v10, k11, v11, k12, v12, k13, v13, k14, v14, k15, v15) \
	fn(k1, v1), fn(k2, v2), fn(k3, v3), fn(k4, v4),                                                                                                                               \
	fn(k5, v5), fn(k6, v6), fn(k7, v7), fn(k8, v8),                                                                                                                               \
	fn(k9, v9), fn(k10, v10), fn(k11, v11), fn(k12, v12),                                                                                                                         \
	fn(k13, v13), fn(k14, v14), fn(k15, v15)
// clang-format on
