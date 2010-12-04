#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/time.h>
#include <alloca.h>

#include "Horspool.cpp"
#include "BoyerMooreAndTurbo.cpp"
#include "StreamBoyerMooreHorspool.cpp"

using namespace std;

unsigned long long
getTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long) tv.tv_sec * 1000 + (unsigned long long) tv.tv_usec / 1000;
}

const char *
memmem2(const char *haystack, size_t haystack_len, const char *needle, size_t needle_len) {
	if (needle_len == 0) {
		return haystack;
	}
	
	const char *last_possible = haystack + haystack_len - needle_len;
	do {
		const char *result = (const char *) memchr(haystack, needle[0], haystack_len);
		if (result != NULL) {
			if (result > last_possible) {
				return NULL;
			} else if (memcmp(result, needle, needle_len) == 0) {
				return result;
			} else {
				ssize_t new_len = ssize_t(haystack_len) - (result - haystack) - 1;
				if (new_len <= 0) {
					return NULL;
				} else {
					haystack = result + 1;
					haystack_len = new_len;
				}
			}
		} else {
			return NULL;
		}
	} while (true);
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
		filename = "benchmark_input/binary.dat";
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
	if (f == NULL) {
		printf("Cannot open %s\n", filename);
		return 1;
	}
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
	sbmh_init(ctx, needle, needle_len);
	for (i = 0; i < iterations; i++) {
		sbmh_reset(ctx);
		sbmh_feed(ctx, needle, needle_len, (const unsigned char *) data.c_str(), data.size());
		if (ctx->found) {
			found = ctx->analyzed - needle_len;
		} else {
			found = ctx->analyzed;
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
	
	if (data.find('\0') == string::npos) {
		t1 = getTime();
		for (i = 0; i < iterations; i++) {
			const char *result = strstr(data.c_str(), (const char *) needle);
			if (result == NULL) {
				found = data.size();
			} else {
				found = result - data.c_str();
			}
		}
		t2 = getTime();
		printf("strstr              : found at position %d in %d msec\n", int(found), int(t2 - t1));
	}
	
	t1 = getTime();
	for (i = 0; i < iterations; i++) {
		const char *result = memmem2(data.c_str(), data.size(), (const char *) needle, needle_len);
		if (result == NULL) {
			found = data.size();
		} else {
			found = result - data.c_str();
		}
	}
	t2 = getTime();
	printf("memmem2             : found at position %d in %d msec\n", int(found), int(t2 - t1));
	
	return 0;
}
