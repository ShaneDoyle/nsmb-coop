#pragma once

#define THUMB_BARRIER_FUNCTION_JOIN(x, y) THUMB_BARRIER_FUNCTION_JOIN_AGAIN(x, y)
#define THUMB_BARRIER_FUNCTION_JOIN_AGAIN(x, y) x ## y

// GCC bug workaround
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121611
#define THUMB_BARRIER_FUNCTION \
    inline void THUMB_BARRIER_FUNCTION_JOIN(__thumb_barrier_, __LINE__)() {}
