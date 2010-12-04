/*
 * Copyright (c) 2010 Phusion v.o.f.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Boyer-Moore-Horspool string search algorithm implementation with streaming support.
 * Most string search algorithm implementations require the entire haystack data to
 * be in memory. In contrast, this implementation allows one to feed the haystack data
 * piece-of-piece in a "streaming" manner.
 *
 * This implementation is optimized for both speed and memory usage.
 * Other than the memory needed for the context structure, it does not need any
 * additional memory allocations (except for minimal usage of the stack). The context
 * structure, which contains the Boyer-Moore-Horspool occurance table and various
 * state information, is is organized in such a way that it can be allocated with a
 * single memory allocation action, regardless of the length of the needle.
 * Its inner loop also deviates a little bit from the original algorithm: the original
 * algorithm matches data right-to-left, but this implementation first matches the
 * rightmost character, then matches the data left-to-right, thereby incorporating
 * some ideas from "Tuning the Boyer-Moore-Horspool String Searching Algorithm" by
 * Timo Raita, 1992. It uses memcmp() for this left-to-right match which is typically
 * heavily optimized.
 *
 * A few more notes:
 * - This code can be used for searching an arbitrary binary needle in an arbitrary binary
 *   haystack. It is not limited to text.
 * - Boyer-Moore-Horspool works best for long needles. Generally speaking, the longer the
 *   needle the faster the algorithm becomes. Thus, this implementation makes no effort
 *   at being fast at searching single-character needles or even short needles (say,
 *   less than 5 characters). You should just use memchr() and memmem() for that; those
 *   functions are usually heavily optimized (e.g. by using tricks like searching 4 bytes
 *   at the same time by treating data as an array of integers) and will probably be
 *   *much* faster than this code at searching short needles.
 * - You can further tweak this code to favor either memory usage or performance.
 *   See the typedef for sbmh_size_t for more information.
 *
 * Usage:
 *
 * 1. Allocate a StreamBMH structure either on the stack (alloca) or on the heap.
 *    It must be at least SBMH_SIZE(needle_len) bytes big.
 *    Do not write to any of StreamBMH's field: they are considered read-only except
 *    for the implementation code.
 *    The maximum supported needle size depends on the definition of sbmh_size_t. See
 *    its typedef for more information.
 *
 * 2. Initialize the structure with sbmh_init(). This structure is now usable for
 *    searching the given needle, and only the given needle.
 *    You must ensure that the StreamBMH structure has at least SBMH_SIZE(needle_len)
 *    bytes of space, otherwise sbmh_init() will overwrite too much memory.
 *    sbmh_init() does NOT make a copy of the needle data.
 *
 * 3. Feed haystack data using sbmh_feed(). You must pass it the same needle that you
 *    passed to sbmh_init(). We do not store a pointer to the needle passed to
 *    sbmh_init() for memory efficiency reasons: the caller already has a pointer
 *    to the needle data so there's no need for us to store it.
 *
 *    sbmh_feed() returns the number of bytes that has been analyzed:
 *
 *    - If the needle has now been found then the position of the last needle character
 *      in the currently fed data will be returned: all data until the end of the needle
 *      has been analyzed, but no more. Additionally, the 'found' field in the context
 *      structure will be set to true.
 *    - If the needle hasn't been found yet, then the size of the currently fed data
 *      will be returned: all fed data has been analyzed.
 *    - If the needle was already found, then any additional call to sbmh_feed()
 *      will cause it to return 0: nothing in the fed data is analyzed.
 *
 * The 'analyzed' field is used to keep track of the total number of haystack bytes that
 * have been analyzed so far, including those haystack data fed in previous sbmh_feed()
 * calls.
 *
 * There's no need deinitialize the StreamBMH structure. Just free its memory.
 *
 * You can also reuse the StreamBMH structure to find the same needle in a different
 * haystack. Call sbmh_reset() to reset all of its internal state except for the
 * Boyer-Moore-Horspool occurance table which contains needle-specific preparation data.
 * You can then call sbmh_feed() to analyze haystack data.
 *
 * Finally, you can reuse an existing StreamBMH structure for finding a different
 * needle. Call sbmh_init() to re-initialize it for use with a different needle.
 * However you must make sure that the StreamBMH structure is at least
 * SBMH_SIZE(new_needle_len) bytes big.
 */

/* This implementation is based on sample code originally written by Joel
 * Yliluoma <joel.yliluoma@iki.fi>, licensed under MIT.
 */

// We assume that other compilers support the 'restrict' keyword.
#ifdef __GNUC__
	#ifndef G_GNUC_RESTRICT
		#if defined (__GNUC__) && (__GNUC__ >= 4)
			#define G_GNUC_RESTRICT __restrict__
		#else
			#define G_GNUC_RESTRICT
		#endif
	#endif
	#ifndef restrict
		#define restrict G_GNUC_RESTRICT
	#endif
#endif

#ifdef __GNUC__
	#define likely(expr) __builtin_expect((expr), 1)
	#define unlikely(expr) __builtin_expect((expr), 0)
#else
	#define likely(expr) expr
	#define unlikely(expr) expr
#endif


#include <cstddef>
#include <cstring>
#include <cassert>


/*
 * sbmh_size_t is a type for representing the needle length. It should be unsigned;
 * it makes no sense for it not to be.
 * By default it's typedef'ed to 'unsigned short', which is a 16-bit integer on most
 * platforms, allowing us to support needles up to about 64 KB. This ough to be enough
 * for most people. In the odd situation that you're dealing with extremely large
 * needles, you can typedef this to 'unsigned int' or even 'unsigned long long'.
 *
 * Its typedef slightly affects performance. Benchmarks on OS X Snow Leopard (x86_64)
 * have shown that typedeffing this to size_t (64-bit integer) makes the benchmark
 * 4-8% faster at the cost of 4 times more memory usage per StreamBMH structure.
 * Consider changing the typedef depending on your needs.
 */
typedef unsigned short sbmh_size_t;

struct StreamBMH {
	size_t        analyzed;
	bool          found;
	sbmh_size_t   lookbehind_size;
	sbmh_size_t   occ[256];
	// Algorithm uses at most needle_len - 1 bytes of space in lookbehind buffer.
	unsigned char lookbehind[];
};

#define SBMH_SIZE(needle_len) (sizeof(struct StreamBMH) + (needle_len) - 1)


void
sbmh_reset(struct StreamBMH *restrict ctx) {
	ctx->found = false;
	ctx->analyzed = 0;
	ctx->lookbehind_size = 0;
}

void
sbmh_init(struct StreamBMH *restrict ctx, const unsigned char *restrict needle,
	sbmh_size_t needle_len)
{
	sbmh_size_t i;
	
	assert(needle_len > 0);
	sbmh_reset(ctx);
	
	/* Initialize occurrance table. */
	for (i = 0; i < 256; i++) {
		ctx->occ[i] = needle_len;
	}
	
	/* Populate occurance table with analysis of the needle,
	 * ignoring last letter.
	 */
	if (needle_len >= 1) {
		for (i = 0; i < needle_len - 1; i++) {
			ctx->occ[needle[i]] = needle_len - 1 - i;
		}
	}
}

static char
sbmh_lookup_char(const struct StreamBMH *restrict ctx,
	const unsigned char *restrict data, ssize_t pos)
{
	if (pos < 0) {
		return ctx->lookbehind[ctx->lookbehind_size + pos];
	} else {
		return data[pos];
	}
}

static bool
sbmh_memcmp(const struct StreamBMH *restrict ctx,
	const unsigned char *restrict needle, const unsigned char *restrict data,
	ssize_t pos, sbmh_size_t len)
{
	ssize_t i = 0;
	
	while (i < ssize_t(len)) {
		unsigned char data_ch = sbmh_lookup_char(ctx, data, pos + i);
		unsigned char needle_ch = needle[i];
		
		if (data_ch == needle_ch) {
			i++;
		} else {
			return false;
		}
	}
	return true;
}

size_t
sbmh_feed(struct StreamBMH *restrict ctx,
	const unsigned char *restrict needle, sbmh_size_t needle_len,
	const unsigned char *restrict data, size_t len)
{
	if (ctx->found) {
		return 0;
	}
	
	/* Positive: points to a position in 'data'
	 *           pos == 3 points to data[3]
	 * Negative: points to a position in the lookbehind buffer
	 *           pos == -2 points to lookbehind[lookbehind_size - 2]
	 */
	ssize_t pos = -ctx->lookbehind_size;
	unsigned char last_needle_char = needle[needle_len - 1];
	const sbmh_size_t *occ = ctx->occ;
	
	if (pos < 0) {
		/* Lookbehind buffer is not empty. Perform Boyer-Moore-Horspool
		 * search with character lookup code that considers both the
		 * lookbehind buffer and the current round's haystack data.
		 *
		 * Loop until
		 *   there is a match.
		 * or until
		 *   we've moved past the position that requires the
		 *   lookbehind buffer. In this case we switch to the
		 *   optimized loop.
		 * or until
		 *   the character to look at lies outside the haystack.
		 *   In this case we update the lookbehind buffer in
		 *   preparation for the next round.
		 */
		while (pos < 0 && pos <= ssize_t(len) - ssize_t(needle_len)) {
			 unsigned char ch = sbmh_lookup_char(ctx, data,
				pos + needle_len - 1);
			
			if (ch == last_needle_char
			 && sbmh_memcmp(ctx, needle, data, pos, needle_len - 1)) {
				ctx->found = true;
				ctx->lookbehind_size = 0;
				ctx->analyzed += pos + needle_len;
				return pos + needle_len;
			} else {
				pos += ctx->occ[ch];
			}
		}
		
		// No match.
		
		if (pos >= 0) {
			ctx->lookbehind_size = 0;
		} else {
			/* Cut off part of the lookbehind buffer that has
			 * been processed and append the entire haystack
			 * into it.
			 */
			sbmh_size_t bytesToCutOff = sbmh_size_t(ssize_t(ctx->lookbehind_size) + pos);
			
			memmove(ctx->lookbehind,
				ctx->lookbehind + bytesToCutOff,
				ctx->lookbehind_size - bytesToCutOff);
			ctx->lookbehind_size -= bytesToCutOff;
			
			assert(ssize_t(ctx->lookbehind_size + len) < ssize_t(needle_len));
			memcpy(ctx->lookbehind + ctx->lookbehind_size,
				data, len);
			ctx->lookbehind_size += len;
			ctx->analyzed += len;
			return len;
		}
	}
	
	assert(pos >= 0);
	assert(ctx->lookbehind_size == 0);
	
	/* Lookbehind buffer is now empty. Perform Boyer-Moore-Horspool
	 * search with optimized character lookup code that only considers
	 * the current round's haystack data.
	 */
	while (likely( pos <= ssize_t(len) - ssize_t(needle_len) )) {
		unsigned char ch = data[pos + needle_len - 1];
		
		if (unlikely(
		        unlikely( ch == last_needle_char )
		     && unlikely( *(data + pos) == needle[0] )
		     && unlikely( memcmp(needle, data + pos, needle_len - 1) == 0 )
		)) {
			ctx->found = true;
			ctx->analyzed += pos + needle_len;
			return pos + needle_len;
		} else {
			pos += occ[ch];
		}
	}
	
	/* There was no match. If there's trailing haystack data that we cannot
	 * match yet using the Boyer-Moore-Horspool algorithm (because the trailing
	 * data is less than the needle size) then match using a modified
	 * algorithm that starts matching from the beginning instead of the end.
	 * Whatever trailing data is left after running this algorithm is added to
	 * the lookbehind buffer.
	 */
	if (size_t(pos) < len) {
		while (size_t(pos) < len
		    && (
		          data[pos] != needle[0]
		       || memcmp(data + pos, needle, len - pos) != 0
		)) {
			pos++;
		}
		if (size_t(pos) < len) {
			memcpy(ctx->lookbehind, data + pos, len - pos);
			ctx->lookbehind_size = len - pos;
		}
	}
	
	ctx->analyzed += len;
	return len;
}
