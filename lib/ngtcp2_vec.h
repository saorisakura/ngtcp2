/*
 * ngtcp2
 *
 * Copyright (c) 2018 ngtcp2 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NGTCP2_VEC_H
#define NGTCP2_VEC_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <ngtcp2/ngtcp2.h>

/*
 * ngtcp2_vec_len returns the sum of length in |vec| of |n| elements.
 */
size_t ngtcp2_vec_len(const ngtcp2_vec *vec, size_t n);

/*
 * ngtcp2_vec_split splits |src| to |dst| so that the sum of the
 * length in |src| does not exceed |left| bytes.  |dst| must be 0
 * length vector and the caller should set |*pdstcnt| to 0.  The
 * caller must set |*psrccnt| to the number of elements of |src|.  The
 * split does not necessarily occur at the boundary of ngtcp2_vec
 * object.  After split has done, this function updates |*psrccnt| and
 * |*pdstcnt|.
 */
void ngtcp2_vec_split(ngtcp2_vec *src, size_t *psrccnt, ngtcp2_vec *dst,
                      size_t *pdstcnt, size_t left);

/*
 * ngtcp2_vec_merge merges |src| into |dst| by moving at most |left|
 * bytes from |src|.  The |maxcnt| is the maximum number of elements
 * which |dst| array can contain.  The caller must set |*pdstcnt| to
 * the number of elements of |dst|.  Similarly, the caller must set
 * |*psrccnt| to the number of elements of |src|.  After merge has
 * done, this function updates |*psrccnt| and |*pdstcnt|.  This
 * function returns the number of bytes moved from |src| to |dst|.
 */
size_t ngtcp2_vec_merge(ngtcp2_vec *dst, size_t *pdstcnt, ngtcp2_vec *src,
                        size_t *psrccnt, size_t left, size_t maxcnt);

#endif /* NGTCP2_VEC_H */
