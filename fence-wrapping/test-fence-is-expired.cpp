/*
 * Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Build with:
 *
 *    $ g++ -o test-fence-is-expired test-fence-is-expired.cpp
 */

#include <stdint.h>
#include <stdio.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/* This is the function we want to test (pasted from drm/nouveau_fence.c). */
static bool
nouveau_fence_is_expired(uint32_t current_val, uint32_t future_val, uint32_t thresh)
{
#if 1
    /* Handles wrapping. */
    return future_val - thresh >= current_val - thresh;
#else
    /* The naive way. */
    return thresh <= current_val;
#endif
}

/* These are the interesting points of the uint32_t range where we want to test
 * that nouveau_fence_is_expired behaves correctly. */
static uint32_t s_deltas[] = {
    /* Around the wrapping point of u32 */
    0x00000000, 0xffffffff, 0xfffffffe, 0xfffffffd, 0xfffffffc, 0xfffffffb,
    /* Around the wrapping point of s32 */
    0x80000000, 0x7fffffff, 0x7ffffffe, 0x7ffffffd, 0x7ffffffc, 0x7ffffffb,
};

static int
test_nouveau_fence_is_expired(uint32_t current_val, uint32_t future_val, uint32_t thresh,
                              bool expected)
{
    int ret = 0;
    unsigned i;
    for (i = 0; i < ARRAY_SIZE(s_deltas); i++) {
        uint32_t c = current_val + s_deltas[i];
        uint32_t f = future_val  + s_deltas[i];
        uint32_t t = thresh      + s_deltas[i];
        bool rval = nouveau_fence_is_expired(c, f, t);
        if (rval != expected) {
            printf("test failed for c=0x%x f=0x%x t=0x%x\n", c, f, t);
            ret = -1;
        }
    }
    return ret;
}

int main()
{
    int ret = 0;

    printf("\n");
    printf("Testing the cases where current_val < thresh < future_val (active fence)...\n");
    ret |= test_nouveau_fence_is_expired(1, 3, 2, false);
    ret |= test_nouveau_fence_is_expired(2, 1, 3, false);
    ret |= test_nouveau_fence_is_expired(3, 2, 1, false);

    printf("Testing the cases where future_val < thresh < current_val (expired fence)...\n");
    ret |= test_nouveau_fence_is_expired(1, 2, 3, true);
    ret |= test_nouveau_fence_is_expired(3, 1, 2, true);
    ret |= test_nouveau_fence_is_expired(2, 3, 1, true);

    printf("Testing the cases where thresh equals current_val (expired fence)...\n");
    ret |= test_nouveau_fence_is_expired(1, 2, 1, true);
    ret |= test_nouveau_fence_is_expired(2, 1, 2, true);

    printf("Testing the cases where thresh equals future_val (active fence)...\n");
    ret |= test_nouveau_fence_is_expired(1, 2, 2, false);
    ret |= test_nouveau_fence_is_expired(2, 1, 1, false);

    printf("Testing the cases where current_val equals future_val (expired fence)...\n");
    ret |= test_nouveau_fence_is_expired(1, 1, 2, true);
    ret |= test_nouveau_fence_is_expired(2, 2, 1, true);

    printf("Testing the case where future_val, current_val and thresh are all the same (expired fence)...\n");
    ret |= test_nouveau_fence_is_expired(1, 1, 1, true);


    if (ret)
        printf("\nTest FAILED.\n\n");
    else
        printf("\nAll tests PASSED.\n\n");

    return ret;
}
