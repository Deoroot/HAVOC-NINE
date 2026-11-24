// mathlib_defs.h - small scalar helpers shared by mathlib.h and vector.h.
// H9: written new for reTS from the interface (the names the renderer calls and what each returns), replacing
// PrimeXT's file, which had no licence notice. Kept as macros: callers mix int, float and double arguments.
// The renderer's callers rely on these exact results, so they are kept even where they differ from the C
// library: Q_floor truncates toward zero, and Q_ceil adds one before truncating (an exact integer goes up by one).
#pragma once
#include <math.h>

#define Q_min(a, b)    (((a) < (b)) ? (a) : (b))               // the smaller of two values
#define Q_max(a, b)    (((a) > (b)) ? (a) : (b))               // the larger of two values
#define Q_recip(a)     ((float)(1.0f / (float)(a)))            // reciprocal, as a float
#define Q_floor(a)     ((float)(int)(a))                       // truncated toward zero, as a float
#define Q_ceil(a)      ((float)(int)((a) + 1))                 // one more, truncated, as a float
#define Q_abs(a)       (fabsf((a)))                            // absolute value, as a float
#define Q_round(x, y)  (floor((x) / (y) + 0.5) * (y))          // x rounded to the nearest multiple of y
#define Q_square(a)    ((a) * (a))                             // a squared
#define Q_sign(a)      (((a) >= 0) ? 1.0 : -1.0)               // 1.0 for zero or more, else -1.0
