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


typedef unsigned short sbmh_size_t;

struct StreamBMH {
	bool          done;
	sbmh_size_t   consumed;
	sbmh_size_t   lookbehind_size;
	sbmh_size_t   occ[256];
	// Algorithm uses at most needle_len - 1 bytes of space in lookbehind buffer.
	unsigned char lookbehind[];
};

#define SBMH_SIZE(needle_len) (sizeof(struct StreamBMH) + (needle_len) - 1)


void
sbmh_reset(struct StreamBMH *restrict ctx) {
	ctx->done = false;
	ctx->consumed = 0;
	ctx->lookbehind_size = 0;
}

void
sbmh_init(struct StreamBMH *restrict ctx, const unsigned char *restrict needle,
	sbmh_size_t needle_len)
{
	sbmh_size_t i;
	
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
	
	while (i < len) {
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
	if (ctx->done) {
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
				ctx->done = true;
				ctx->lookbehind_size = 0;
				ctx->consumed += pos + needle_len;
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
			
			assert(ctx->lookbehind_size + len < needle_len);
			memcpy(ctx->lookbehind + ctx->lookbehind_size,
				data, len);
			ctx->lookbehind_size += len;
			ctx->consumed += len;
			return len;
		}
	}
	
	/* Lookbehind buffer is now empty. Perform Boyer-Moore-Horspool
	 * search with optimized character lookup code that only considers
	 * the current round's haystack data.
	 */
	while (likely( pos <= ssize_t(len) - ssize_t(needle_len) )) {
		unsigned char ch = data[pos + needle_len - 1];
		
		if (unlikely(
		        unlikely( ch == last_needle_char )
		     && unlikely( memcmp(needle, data + pos, needle_len - 1) == 0 )
		)) {
			ctx->done = true;
			ctx->consumed  += pos + needle_len;
			return pos + needle_len;
		} else {
			pos += occ[ch];
		}
	}
	
	/* There was no match. If there's trailing haystack data that we cannot
	 * match yet, add that to the lookbehind buffer.
	 */
	if (size_t(pos) < len) {
		memcpy(ctx->lookbehind, data + pos, len - pos);
		ctx->lookbehind_size = len - pos;
	}
	
	ctx->consumed += len;
	return len;
}
