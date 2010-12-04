#include <string>
#include <algorithm>
#include <alloca.h>

#include "tut.h"
#include "StreamBoyerMooreHorspool.cpp"

using namespace std;

namespace tut {
	struct StreamTest {
		static int find(const string &needle, const string &haystack) {
			StreamBMH *ctx = (StreamBMH *) alloca(SBMH_SIZE(needle.size()));
			
			sbmh_init(ctx, (const unsigned char *) needle.c_str(), needle.size());
			sbmh_feed(ctx,
				(const unsigned char *) needle.c_str(), needle.size(),
				(const unsigned char *) haystack.c_str(), haystack.size());
			if (ctx->found) {
				return ctx->analyzed - needle.size();
			} else {
				return -1;
			}
		}
		
		static int feed_in_chunks_and_find(const string &needle, const string &haystack, int chunkSize = 1) {
			StreamBMH *ctx = (StreamBMH *) alloca(SBMH_SIZE(needle.size()));
			
			sbmh_init(ctx, (const unsigned char *) needle.c_str(), needle.size());
			for (string::size_type i = 0; i < haystack.size(); i += chunkSize) {
				sbmh_feed(ctx,
					(const unsigned char *) needle.c_str(), needle.size(),
					(const unsigned char *) haystack.c_str() + i,
					std::min((int) chunkSize, (int) (haystack.size() - i)));
			}
			
			if (ctx->found) {
				return ctx->analyzed - needle.size();
			} else {
				return -1;
			}
		}
	};
	
	DEFINE_TEST_GROUP_WITH_LIMIT(StreamTest, 100);
	
	/****** Test feeding all data in one pass ******/
	
	TEST_METHOD(1) {
		set_test_name("It returns the haystack length if the needle "
			"(1 character) can't be found.");
		
		ensure_equals(find("0", "123456789"), -1);
		ensure_equals(find("x", "hello world"), -1);
	}
	
	TEST_METHOD(2) {
		set_test_name("It returns the haystack length if the needle "
			"(2 different characters) can't be found.");
		
		ensure_equals(find("ab", "123456789"), -1);
		ensure_equals(find("ab", "a23456789"), -1);
		ensure_equals(find("ab", "1a3456789"), -1);
		ensure_equals(find("ab", "1b3456789"), -1);
		ensure_equals(find("ab", "123b56789"), -1);
		ensure_equals(find("ab", "12a456789"), -1);
		ensure_equals(find("ab", "12a45678a"), -1);
		ensure_equals(find("ab", "12a45678aa"), -1);
	}
	
	TEST_METHOD(3) {
		set_test_name("It returns the haystack length if the needle "
			"(2 identical characters) can't be found.");
		
		ensure_equals(find("aa", "123456789"), -1);
		ensure_equals(find("aa", "a23456789"), -1);
		ensure_equals(find("aa", "1a3456789"), -1);
		ensure_equals(find("aa", "12a4a6789"), -1);
		ensure_equals(find("aa", "12a4a678a"), -1);
		ensure_equals(find("aa", "12a4a678ba"), -1);
	}
	
	TEST_METHOD(4) {
		set_test_name("Searching in an empty string always fails.");
		
		ensure_equals(find("1", ""), -1);
		ensure_equals(find("abc", ""), -1);
		ensure_equals(find("hello world", ""), -1);
	}
	
	TEST_METHOD(5) {
		set_test_name("Searching for a needle that's larger than the haystack always fails");
		
		ensure_equals(find("ab", "a"), -1);
		ensure_equals(find("hello", "hm"), -1);
		ensure_equals(find("hello my world!", "this is small"), -1);
	}
	
	
	TEST_METHOD(10) {
		set_test_name("It returns the position in which the needle is first found (1 character needle)");
		
		ensure_equals(find("1", "1234567891"), 0);
		ensure_equals(find("2", "1234567892"), 1);
		ensure_equals(find("8", "1234567898"), 7);
		ensure_equals(find("9", "1234567899"), 8);
	}
	
	TEST_METHOD(11) {
		set_test_name("It returns the position in which the needle is first found (2 different character needle)");
		
		ensure_equals(find("ab", "ab3456789ab"), 0);
		ensure_equals(find("ab", "1ab456789ab"), 1);
		ensure_equals(find("ab", "12ab56789ab"), 2);
		ensure_equals(find("ab", "123ab6789ab"), 3);
		
		ensure_equals(find("ab", "bbab3456789ab"), 2);
		ensure_equals(find("ab", "bb1ab456789ab"), 3);
		ensure_equals(find("ab", "bb12ab56789ab"), 4);
		ensure_equals(find("ab", "bb123ab6789ab"), 5);
		
		ensure_equals(find("ab", "baab3456789ab"), 2);
		ensure_equals(find("ab", "ba1ab456789ab"), 3);
		ensure_equals(find("ab", "ba12ab56789ab"), 4);
		ensure_equals(find("ab", "ba123ab6789ab"), 5);
		
		ensure_equals(find("ab", "003456789ab"), 9);
		ensure_equals(find("ab", "100456789aabab"), 10);
		ensure_equals(find("ab", "120056789abbab"), 9);
	}
	
	TEST_METHOD(12) {
		set_test_name("It returns the position in which the needle is found (2 identifical character needle)");
		
		ensure_equals(find("\n\n", "\n\nhello world\n\n"), 0);
		ensure_equals(find("\n\n", "h\n\nello world"), 1);
		ensure_equals(find("\n\n", "he\n\nllo world"), 2);
		ensure_equals(find("\n\n", "hel\n\nllo world"), 3);
		ensure_equals(find("\n\n", "hello\n\nworld\n\n"), 5);
		ensure_equals(find("\n\n", "\nhello\n\nworld\n\n"), 6);
		ensure_equals(find("\n\n", "h\nello\n\nworld\n\n"), 6);
	}
	
	
	TEST_METHOD(13) {
		set_test_name("Misc tests");
		
		ensure_equals(find("hello", "hello world"), 0);
		ensure_equals(find("hello", "helo world"), -1);
		ensure_equals(find("hello world!", "oh my, hello world"), -1);
		ensure_equals(find("hello world!", "oh my, hello world!! again, hello world!!"), 7);
		
		ensure_equals(find("\r\n--boundary\r\n",
			"some binary data\r\n"
			"--boundary\rnot really\r\n"
			"more binary data\r\n"
			"--boundary\r\n"),
			57);
	}
	
	
	/****** Test feeding data byte-by-byte ******/
	
	TEST_METHOD(21) {
		set_test_name("It returns the haystack length if the needle "
			"(1 character) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("0", "123456789"), -1);
		ensure_equals(feed_in_chunks_and_find("x", "hello world"), -1);
	}
	
	TEST_METHOD(22) {
		set_test_name("It returns the haystack length if the needle "
			"(2 different characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("ab", "123456789"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "a23456789"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "1a3456789"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "1b3456789"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "123b56789"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a456789"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678a"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678aa"), -1);
	}
	
	TEST_METHOD(23) {
		set_test_name("It returns the haystack length if the needle "
			"(2 identical characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("aa", "123456789"), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "a23456789"), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "1a3456789"), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a6789"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678a"), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678aa"), -1);
	}
	
	TEST_METHOD(24) {
		set_test_name("Searching in an empty string always fails.");
		
		ensure_equals(feed_in_chunks_and_find("1", ""), -1);
		ensure_equals(feed_in_chunks_and_find("abc", ""), -1);
		ensure_equals(feed_in_chunks_and_find("hello world", ""), -1);
	}
	
	TEST_METHOD(25) {
		set_test_name("Searching for a needle that's larger than the haystack always fails");
		
		ensure_equals(feed_in_chunks_and_find("ab", "a"), -1);
		ensure_equals(feed_in_chunks_and_find("hello", "hm"), -1);
		ensure_equals(feed_in_chunks_and_find("hello my world!", "this is small"), -1);
	}
	
	
	TEST_METHOD(30) {
		set_test_name("It returns the position in which the needle is first found (1 character needle)");
		
		ensure_equals(feed_in_chunks_and_find("1", "1234567891"), 0);
		ensure_equals(feed_in_chunks_and_find("2", "1234567892"), 1);
		ensure_equals(feed_in_chunks_and_find("8", "1234567898"), 7);
		ensure_equals(feed_in_chunks_and_find("9", "1234567899"), 8);
	}
	
	TEST_METHOD(31) {
		set_test_name("It returns the position in which the needle is first found (2 different character needle)");
		
		ensure_equals(feed_in_chunks_and_find("ab", "ab3456789ab"), 0);
		ensure_equals(feed_in_chunks_and_find("ab", "1ab456789ab"), 1);
		ensure_equals(feed_in_chunks_and_find("ab", "12ab56789ab"), 2);
		ensure_equals(feed_in_chunks_and_find("ab", "123ab6789ab"), 3);
		
		ensure_equals(feed_in_chunks_and_find("ab", "bbab3456789ab"), 2);
		ensure_equals(feed_in_chunks_and_find("ab", "bb1ab456789ab"), 3);
		ensure_equals(feed_in_chunks_and_find("ab", "bb12ab56789ab"), 4);
		ensure_equals(feed_in_chunks_and_find("ab", "bb123ab6789ab"), 5);
		
		ensure_equals(feed_in_chunks_and_find("ab", "baab3456789ab"), 2);
		ensure_equals(feed_in_chunks_and_find("ab", "ba1ab456789ab"), 3);
		ensure_equals(feed_in_chunks_and_find("ab", "ba12ab56789ab"), 4);
		ensure_equals(feed_in_chunks_and_find("ab", "ba123ab6789ab"), 5);
		
		ensure_equals(feed_in_chunks_and_find("ab", "003456789ab"), 9);
		ensure_equals(feed_in_chunks_and_find("ab", "100456789aabab"), 10);
		ensure_equals(feed_in_chunks_and_find("ab", "120056789abbab"), 9);
	}
	
	TEST_METHOD(32) {
		set_test_name("It returns the position in which the needle is found (2 identifical character needle)");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "\n\nhello world\n\n"), 0);
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\n\nello world"), 1);
		ensure_equals(feed_in_chunks_and_find("\n\n", "he\n\nllo world"), 2);
		ensure_equals(feed_in_chunks_and_find("\n\n", "hel\n\nllo world"), 3);
		ensure_equals(feed_in_chunks_and_find("\n\n", "hello\n\nworld\n\n"), 5);
		ensure_equals(feed_in_chunks_and_find("\n\n", "\nhello\n\nworld\n\n"), 6);
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\nello\n\nworld\n\n"), 6);
	}
	
	
	TEST_METHOD(33) {
		set_test_name("Misc tests");
		
		ensure_equals(feed_in_chunks_and_find("hello", "hello world"), 0);
		ensure_equals(feed_in_chunks_and_find("hello", "helo world"), -1);
		ensure_equals(feed_in_chunks_and_find("hello world!", "oh my, hello world"), -1);
		ensure_equals(feed_in_chunks_and_find("hello world!", "oh my, hello world!! again, hello world!!"), 7);
		
		ensure_equals(feed_in_chunks_and_find(
			"\r\n--boundary\r\n",
			"some binary data\r\n"
			"--boundary\rnot really\r\n"
			"more binary data\r\n"
			"--boundary\r\n"),
			57);
	}
	
	
	/****** Test feeding data in chunks of 3 bytes ******/
	
	TEST_METHOD(41) {
		set_test_name("It returns the haystack length if the needle "
			"(1 character) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("0", "123456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("x", "hello world", 3), -1);
	}
	
	TEST_METHOD(42) {
		set_test_name("It returns the haystack length if the needle "
			"(2 different characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("ab", "123456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "a23456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "1a3456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "1b3456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "123b56789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678a", 3), -1);
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678aa", 3), -1);
	}
	
	TEST_METHOD(43) {
		set_test_name("It returns the haystack length if the needle "
			"(2 identical characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("aa", "123456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "a23456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "1a3456789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a6789", 3), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a678a", 3), -1);
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a678ba", 3), -1);
	}
	
	TEST_METHOD(44) {
		set_test_name("Searching in an empty string always fails.");
		
		ensure_equals(feed_in_chunks_and_find("1", "", 3), -1);
		ensure_equals(feed_in_chunks_and_find("abc", "", 3), -1);
		ensure_equals(feed_in_chunks_and_find("hello world", "", 3), -1);
	}
	
	TEST_METHOD(45) {
		set_test_name("Searching for a needle that's larger than the haystack always fails");
		
		ensure_equals(feed_in_chunks_and_find("ab", "a", 3), -1);
		ensure_equals(feed_in_chunks_and_find("hello", "hm", 3), -1);
		ensure_equals(feed_in_chunks_and_find("hello my world!", "this is small", 3), -1);
	}
	
	
	TEST_METHOD(50) {
		set_test_name("It returns the position in which the needle is first found (1 character needle)");
		
		ensure_equals(feed_in_chunks_and_find("1", "1234567891", 3), 0);
		ensure_equals(feed_in_chunks_and_find("2", "1234567892", 3), 1);
		ensure_equals(feed_in_chunks_and_find("8", "1234567898", 3), 7);
		ensure_equals(feed_in_chunks_and_find("9", "1234567899", 3), 8);
	}
	
	TEST_METHOD(51) {
		set_test_name("It returns the position in which the needle is first found (2 different character needle)");
		
		ensure_equals(feed_in_chunks_and_find("ab", "ab3456789ab", 3), 0);
		ensure_equals(feed_in_chunks_and_find("ab", "1ab456789ab", 3), 1);
		ensure_equals(feed_in_chunks_and_find("ab", "12ab56789ab", 3), 2);
		ensure_equals(feed_in_chunks_and_find("ab", "123ab6789ab", 3), 3);
		
		ensure_equals(feed_in_chunks_and_find("ab", "bbab3456789ab", 3), 2);
		ensure_equals(feed_in_chunks_and_find("ab", "bb1ab456789ab", 3), 3);
		ensure_equals(feed_in_chunks_and_find("ab", "bb12ab56789ab", 3), 4);
		ensure_equals(feed_in_chunks_and_find("ab", "bb123ab6789ab", 3), 5);
		
		ensure_equals(feed_in_chunks_and_find("ab", "baab3456789ab", 3), 2);
		ensure_equals(feed_in_chunks_and_find("ab", "ba1ab456789ab", 3), 3);
		ensure_equals(feed_in_chunks_and_find("ab", "ba12ab56789ab", 3), 4);
		ensure_equals(feed_in_chunks_and_find("ab", "ba123ab6789ab", 3), 5);
		
		ensure_equals(feed_in_chunks_and_find("ab", "003456789ab", 3), 9);
		ensure_equals(feed_in_chunks_and_find("ab", "100456789aabab", 3), 10);
		ensure_equals(feed_in_chunks_and_find("ab", "120056789abbab", 3), 9);
	}
	
	TEST_METHOD(52) {
		set_test_name("It returns the position in which the needle is found (2 identifical character needle)");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "\n\nhello world\n\n", 3), 0);
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\n\nello world", 3), 1);
		ensure_equals(feed_in_chunks_and_find("\n\n", "he\n\nllo world", 3), 2);
		ensure_equals(feed_in_chunks_and_find("\n\n", "hel\n\nllo world", 3), 3);
		ensure_equals(feed_in_chunks_and_find("\n\n", "hell\n\nlo world", 3), 4);
		ensure_equals(feed_in_chunks_and_find("\n\n", "hello\n\nworld\n\n", 3), 5);
		ensure_equals(feed_in_chunks_and_find("\n\n", "\nhello\n\nworld\n\n", 3), 6);
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\nello\n\nworld\n\n", 3), 6);
	}
	
	
	TEST_METHOD(53) {
		set_test_name("Misc tests");
		
		ensure_equals(feed_in_chunks_and_find("hello", "hello world", 3), 0);
		ensure_equals(feed_in_chunks_and_find("hello", "helo world", 3), -1);
		ensure_equals(feed_in_chunks_and_find("hello world!", "oh my, hello world", 3), -1);
		ensure_equals(feed_in_chunks_and_find("hello world!", "oh my, hello world!! again, hello world!!", 3), 7);
		
		ensure_equals(feed_in_chunks_and_find(
			"\r\n--boundary\r\n",
			"some binary data\r\n"
			"--boundary\rnot really\r\n"
			"more binary data\r\n"
			"--boundary\r\n", 3),
			57);
	}
}
