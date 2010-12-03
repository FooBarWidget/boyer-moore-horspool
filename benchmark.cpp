#include "Horspool.cpp"
#include "StreamBoyerMooreHorspool.cpp"
 

typedef std::vector<size_t> skiptable_type;
 
/* This function compares the two strings starting at ptr1 and ptr2,
 * which are assumed to be strlen bytes long, and returns a size_t
 * indicating how many bytes were identical, counting from the _end_
 * of both strings.
 * It is an utility function used by the actual Boyer-Moore algorithms.
 */
const size_t backwards_match_len(
    const unsigned char* ptr1,
    const unsigned char* ptr2,
    size_t strlen,
    size_t maxlen,
    size_t minlen)
{
    size_t result = minlen;
    while(result < maxlen && ptr1[strlen-1-result] == ptr2[strlen-1-result])
        ++result;
    return result;
}

/* This function creates a skip table to be used by the search algorithms. */
/* It only needs to be created once per a needle to search. */
const skiptable_type
    CreateSkipTable(const unsigned char* needle, size_t needle_length)
{
    skiptable_type skip(needle_length, needle_length); // initialize a table of needle_length elements to value needle_length
 
    if(needle_length <= 1) return skip;
 
    /* I have absolutely no idea how this works. I just copypasted
     * it from http://www-igm.univ-mlv.fr/~lecroq/string/node14.html
     * and have since edited it in trying to seek clarity and efficiency.
     * In particular, the skip[] table creation is now interleaved within
     * building of the suff[] table, and the assumption that skip[] is
     * preinitialized into needle_length is utilized.
     * -Bisqwit
     */
 
    const size_t needle_length_minus_1 = needle_length-1;
 
    std::vector<ssize_t> suff(needle_length);
 
    suff[needle_length_minus_1] = needle_length;
 
    ssize_t f = 0;
    ssize_t g = needle_length_minus_1;
    size_t j = 0; // index for writing into skip[]
    for(ssize_t i = needle_length-2; i >= 0; --i) // For each suffix length
    {
        if(i > g)
        {
            const ssize_t tmp = suff[i + needle_length_minus_1 - f];
            if (tmp < i - g)
            {
                suff[i] = tmp;
                goto i_done;
            }
        }
        else
            g = i;
 
        f = i;
 
        g -= backwards_match_len(
                needle,
                needle + needle_length_minus_1 - f,
                g+1, // length of string to test
                g+1, // maximum match length
                0 // assumed minimum match length
              );
 
        suff[i] = f - g;
    i_done:;
 
        if(suff[i] == i+1) // This "if" matches sometimes. Less so on random data.
        {
            // Probably, this only happens when the needle contains self-similarity.
            size_t jlimit = needle_length_minus_1 - i;
            while(j < jlimit)
                skip[j++] = jlimit;
        }
    }
    /* j may be smaller than needle_length here. It doesn't matter, because
     * if we ran the skip[] initialization loop until j < needle_length (when i=-1),
     * we would only set the value as needle_length, which it already is.
     */
 
    for (size_t i = 0; i < needle_length_minus_1; ++i)
        skip[needle_length_minus_1 - suff[i]] = needle_length_minus_1 - i;
 
    return skip;
}

/* A Boyer-Moore search algorithm. */
/* If it finds the needle, it returns an offset to haystack from which
 * the needle was found. Otherwise, it returns haystack_length.
 */
const size_t SearchIn(const unsigned char* haystack, const size_t haystack_length,
    const occtable_type& occ,
    const skiptable_type& skip,
    const unsigned char* needle,
    const size_t needle_length)
{
    if(needle_length > haystack_length) return haystack_length;
 
    if(needle_length == 1)
    {
        const unsigned char* result =
            (const unsigned char*)std::memchr(haystack, *needle, haystack_length);
        return result ? result-haystack : haystack_length;
    }
 
    const size_t needle_length_minus_1 = needle_length-1;
 
    size_t haystack_position=0;
    while(haystack_position <= haystack_length-needle_length)
    {
        const size_t match_len = backwards_match_len(
            needle,
            haystack+haystack_position,
            needle_length, // length of string to test
            needle_length, // maximum match length
            0 // assumed minimum match length
           );
        if(match_len == needle_length) return haystack_position;
 
        const size_t mismatch_position = needle_length_minus_1 - match_len;
 
        const unsigned char occ_char = haystack[haystack_position + mismatch_position];
 
        const ssize_t bcShift = occ[occ_char] - match_len;
        const ssize_t gcShift = skip[mismatch_position];
 
        size_t shift = std::max(gcShift, bcShift);
 
        haystack_position += shift;
    }
    return haystack_length;
}


/* A Turbo Boyer-Moore search algorithm. */
/* If it finds the needle, it returns an offset to haystack from which
 * the needle was found. Otherwise, it returns haystack_length.
 */
const size_t SearchInTurbo(const unsigned char* haystack, const size_t haystack_length,
    const occtable_type& occ,
    const skiptable_type& skip,
    const unsigned char* needle,
    const size_t needle_length)
{
    if(needle_length > haystack_length) return haystack_length;
 
    if(needle_length == 1)
    {
        const unsigned char* result =
            (const unsigned char*)std::memchr(haystack, *needle, haystack_length);
        return result ? result-haystack : haystack_length;
    }
 
    const size_t needle_length_minus_1 = needle_length-1;
 
    size_t haystack_position = 0;
    size_t ignore_num = 0, shift = needle_length;
    while(haystack_position <= haystack_length-needle_length)
    {
        size_t match_len;
        if(ignore_num == 0)
        {
            match_len = backwards_match_len(
                needle,
                haystack+haystack_position,
                needle_length, // length of string to test
                needle_length, // maximum match length
                0 // assumed minimum match length
               );
            if(match_len == needle_length) return haystack_position;
        }
        else
        {
            match_len =
                backwards_match_len(needle, haystack+haystack_position,
                    needle_length, // length of string to test
                    shift, // maximum match length
                    0 // assumed minimum match length
                   );
 
            if(match_len == shift)
            {
                match_len =
                    backwards_match_len(needle, haystack+haystack_position,
                        needle_length, // length of string to test
                        needle_length, // maximum match length
                        shift + ignore_num // assumed minimum match length
                      );
            }
            if(match_len >= needle_length) return haystack_position;
        }
 
        const size_t mismatch_position = needle_length_minus_1 - match_len;
 
        const unsigned char occ_char = haystack[haystack_position + mismatch_position];
 
        const ssize_t bcShift = occ[occ_char] - match_len;
        const size_t gcShift  = skip[mismatch_position];
        const ssize_t turboShift = ignore_num - match_len;
 
        shift = std::max(std::max((ssize_t)gcShift, bcShift), turboShift);
 
        if(shift == gcShift)
            ignore_num = std::min( needle_length - shift, match_len);
        else
        {
            if(turboShift < bcShift && ignore_num >= shift)
                shift = ignore_num + 1;
            ignore_num = 0;
        }
        haystack_position += shift;
    }
    return haystack_length;
}


#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/time.h>
#include <alloca.h>

using namespace std;

unsigned long long
getTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long) tv.tv_sec * 1000 + (unsigned long long) tv.tv_usec / 1000;
}

int
main(int argc, char *argv[]) {
	const char *filename;
	const unsigned char *needle;
	size_t needle_len;
	int iterations;
	
	if (argc >= 2) {
		filename = argv[1];
	} else {
		filename = "binary.dat";
	}
	if (argc >= 3) {
		needle = (const unsigned char *) argv[2];
	} else {
		needle = (const unsigned char *) "I have control\n";
	}
	if (argc >= 4) {
		iterations = atoi(argv[3]);
	} else {
		iterations = 10;
	}
	needle_len = strlen((const char *) needle);
	
	string data;
	FILE *f = fopen(filename, "rb");
	while (!feof(f)) {
		char buf[1024 * 8];
		size_t ret = fread(buf, 1, sizeof(buf), f);
		data.append(buf, ret);
	}
	fclose(f);
	data.append(":");
	data.append((const char *) needle);
	
	const occtable_type occ = CreateOccTable(needle, needle_len);
	const skiptable_type skip = CreateSkipTable(needle, needle_len);
	
	unsigned long long t1, t2;
	size_t found = 0;
	int i;
	
	t1 = getTime();
	for (i = 0; i < iterations; i++) {
		found = SearchIn((const unsigned char *) data.c_str(), data.size(), occ, skip, needle, needle_len);
	}
	t2 = getTime();
	printf("Boyer-Moore         : found at position %d in %d msec\n", int(found), int(t2 - t1));
	
	t1 = getTime();
	for (i = 0; i < iterations; i++) {
		found = SearchInHorspool((const unsigned char *) data.c_str(), data.size(), occ, needle, needle_len);
	}
	t2 = getTime();
	printf("Boyer-Moore-Horspool: found at position %d in %d msec\n", int(found), int(t2 - t1));
	
	t1 = getTime();
	StreamBMH *ctx = (StreamBMH *) alloca(SBMH_SIZE(needle_len));
	for (i = 0; i < iterations; i++) {
		sbmh_init(ctx, needle, needle_len);
		sbmh_feed(ctx, needle, needle_len, (const unsigned char *) data.c_str(), data.size());
		if (ctx->done) {
			found = ctx->consumed - needle_len;
		} else {
			found = ctx->consumed;
		}
	}
	t2 = getTime();
	printf("Stream Horspool     : found at position %d in %d msec\n", int(found), int(t2 - t1));
	
	t1 = getTime();
	for (i = 0; i < iterations; i++) {
		found = SearchInTurbo((const unsigned char *) data.c_str(), data.size(), occ, skip, needle, needle_len);
	}
	t2 = getTime();
	printf("Turbo Boyer-Moore   : found at position %d in %d msec\n", int(found), int(t2 - t1));
	
	return 0;
}
