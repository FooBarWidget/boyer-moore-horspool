/*
 * Originally written by Joel Yliluoma <joel.yliluoma@iki.fi>
 * http://en.wikipedia.org/wiki/Talk:Boyer%E2%80%93Moore_string_search_algorithm#Common_functions
 *
 * Copyright (c) 2008 Joel Yliluoma
 * Copyright (c) 2010 Hongli Lai
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

#include <vector>
 
typedef std::vector<size_t> occtable_type;

/* This function creates an occ table to be used by the search algorithms. */
/* It only needs to be created once per a needle to search. */
const occtable_type
    CreateOccTable(const unsigned char* needle, size_t needle_length)
{
    occtable_type occ(UCHAR_MAX+1, needle_length); // initialize a table of UCHAR_MAX+1 elements to value needle_length
 
    /* Populate it with the analysis of the needle */
    /* But ignoring the last letter */
    if(needle_length >= 1)
    {
        const size_t needle_length_minus_1 = needle_length-1;
        for(size_t a=0; a<needle_length_minus_1; ++a)
            occ[needle[a]] = needle_length_minus_1 - a;
    }
    return occ;
}

/* A Boyer-Moore-Horspool search algorithm. */
/* If it finds the needle, it returns an offset to haystack from which
 * the needle was found. Otherwise, it returns haystack_length.
 */
const size_t SearchInHorspool(const unsigned char* haystack, const size_t haystack_length,
    const occtable_type& occ,
    const unsigned char* needle,
    const size_t needle_length)
{
    if(needle_length > haystack_length) return haystack_length;
    if(needle_length == 1)
    {
        const unsigned char* result = (const unsigned char*)std::memchr(haystack, *needle, haystack_length);
        return result ? result-haystack : haystack_length;
    }
 
    const size_t needle_length_minus_1 = needle_length-1;
 
    const unsigned char last_needle_char = needle[needle_length_minus_1];
 
    size_t haystack_position=0;
    while(haystack_position <= haystack_length-needle_length)
    {
        const unsigned char occ_char = haystack[haystack_position + needle_length_minus_1];
 
        // The author modified this part. Original algorithm matches needle right-to-left.
        // This code calls memcmp() (usually matches left-to-right) after matching the last
        // character, thereby incorporating some ideas from
        // "Tuning the Boyer-Moore-Horspool String Searching Algorithm"
        // by Timo Raita, 1992.
        if(last_needle_char == occ_char
        && std::memcmp(needle, haystack+haystack_position, needle_length_minus_1) == 0)
        {
            return haystack_position;
        }
 
        haystack_position += occ[occ_char];
    }
    return haystack_length;
}
