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
	
	return 0;
}
