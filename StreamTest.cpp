#include <string>
#include <algorithm>
#include <alloca.h>

#include "tut.h"
#include "StreamBoyerMooreHorspool.h"

using namespace std;

namespace tut {
	struct StreamTest {
		string unmatched_data;
		string lookbehind;
		
		static void append_unmatched_data(const struct StreamBMH *ctx,
			const unsigned char *data, size_t len)
		{
			StreamTest *self = (StreamTest *) ctx->user_data;
			self->unmatched_data.append((const char *) data, len);
		}
		
		int find(const string &needle, const string &haystack) {
			StreamBMH *ctx = (StreamBMH *) alloca(SBMH_SIZE(needle.size()));
			
			unmatched_data.clear();
			lookbehind.clear();
			
			sbmh_init(ctx, (const unsigned char *) needle.c_str(), needle.size());
			ctx->callback = append_unmatched_data;
			ctx->user_data = this;
			
			size_t analyzed = sbmh_feed(ctx,
				(const unsigned char *) needle.c_str(), needle.size(),
				(const unsigned char *) haystack.c_str(), haystack.size());
			lookbehind.assign((const char *) ctx->lookbehind, ctx->lookbehind_size);
			if (ctx->found) {
				return analyzed - needle.size();
			} else {
				return -1;
			}
		}
		
		int feed_in_chunks_and_find(const string &needle, const string &haystack, int chunkSize = 1) {
			StreamBMH *ctx = (StreamBMH *) alloca(SBMH_SIZE(needle.size()));
			
			unmatched_data.clear();
			lookbehind.clear();
			
			sbmh_init(ctx, (const unsigned char *) needle.c_str(), needle.size());
			ctx->callback = append_unmatched_data;
			ctx->user_data = this;
			
			size_t analyzed = 0;
			for (string::size_type i = 0; i < haystack.size(); i += chunkSize) {
				analyzed += sbmh_feed(ctx,
					(const unsigned char *) needle.c_str(), needle.size(),
					(const unsigned char *) haystack.c_str() + i,
					std::min((int) chunkSize, (int) (haystack.size() - i)));
			}
			
			lookbehind.assign((const char *) ctx->lookbehind, ctx->lookbehind_size);
			if (ctx->found) {
				return analyzed - needle.size();
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
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("x", "hello world"), -1);
		ensure_equals(unmatched_data, "hello world");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(2) {
		set_test_name("It returns the haystack length if the needle "
			"(2 different characters) can't be found.");
		
		ensure_equals(find("ab", "123456789"), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "a23456789"), -1);
		ensure_equals(unmatched_data, "a23456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "1a3456789"), -1);
		ensure_equals(unmatched_data, "1a3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "1b3456789"), -1);
		ensure_equals(unmatched_data, "1b3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "123b56789"), -1);
		ensure_equals(unmatched_data, "123b56789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "12a456789"), -1);
		ensure_equals(unmatched_data, "12a456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "12a45678a"), -1);
		ensure_equals(unmatched_data, "12a45678");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(find("ab", "12a45678aa"), -1);
		ensure_equals(unmatched_data, "12a45678a");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(find("ab", "12a45678b"), -1);
		ensure_equals(unmatched_data, "12a45678b");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(3) {
		set_test_name("It returns the haystack length if the needle "
			"(2 identical characters) can't be found.");
		
		ensure_equals(find("aa", "123456789"), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("aa", "a23456789"), -1);
		ensure_equals(unmatched_data, "a23456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("aa", "1a3456789"), -1);
		ensure_equals(unmatched_data, "1a3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("aa", "12a4a6789"), -1);
		ensure_equals(unmatched_data, "12a4a6789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("aa", "12a4a678a"), -1);
		ensure_equals(unmatched_data, "12a4a678");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(find("aa", "12a4a678ba"), -1);
		ensure_equals(unmatched_data, "12a4a678b");
		ensure_equals(lookbehind, "a");
	}
	
	TEST_METHOD(4) {
		set_test_name("Searching in an empty string always fails.");
		
		ensure_equals(find("1", ""), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("abc", ""), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("hello world", ""), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(5) {
		set_test_name("Searching for a needle that's larger than the haystack always fails");
		
		ensure_equals(find("ab", "a"), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(find("hello", "hm"), -1);
		ensure_equals(unmatched_data, "hm");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("hello my world!", "this is small"), -1);
		ensure_equals(unmatched_data, "this is small");
		ensure_equals(lookbehind, "");
	}
	
	
	TEST_METHOD(10) {
		set_test_name("It returns the position in which the needle is first found (1 character needle)");
		
		ensure_equals(find("1", "1234567891"), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("2", "1234567892"), 1);
		ensure_equals(unmatched_data, "1");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("8", "1234567898"), 7);
		ensure_equals(unmatched_data, "1234567");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("9", "1234567899"), 8);
		ensure_equals(unmatched_data, "12345678");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(11) {
		set_test_name("It returns the position in which the needle is first found (2 different character needle)");
		
		ensure_equals(find("ab", "ab3456789ab"), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "1ab456789ab"), 1);
		ensure_equals(unmatched_data, "1");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "12ab56789ab"), 2);
		ensure_equals(unmatched_data, "12");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "123ab6789ab"), 3);
		ensure_equals(unmatched_data, "123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "bbab3456789ab"), 2);
		ensure_equals(unmatched_data, "bb");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "bb1ab456789ab"), 3);
		ensure_equals(unmatched_data, "bb1");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "bb12ab56789ab"), 4);
		ensure_equals(unmatched_data, "bb12");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "bb123ab6789ab"), 5);
		ensure_equals(unmatched_data, "bb123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "baab3456789ab"), 2);
		ensure_equals(unmatched_data, "ba");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "ba1ab456789ab"), 3);
		ensure_equals(unmatched_data, "ba1");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "ba12ab56789ab"), 4);
		ensure_equals(unmatched_data, "ba12");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "ba123ab6789ab"), 5);
		ensure_equals(unmatched_data, "ba123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("ab", "003456789ab"), 9);
		ensure_equals(unmatched_data, "003456789");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "100456789aabab"), 10);
		ensure_equals(unmatched_data, "100456789a");
		ensure_equals(lookbehind, "");
		ensure_equals(find("ab", "120056789abbab"), 9);
		ensure_equals(unmatched_data, "120056789");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(12) {
		set_test_name("It returns the position in which the needle is found (2 identifical character needle)");
		
		ensure_equals(find("\n\n", "\n\nhello world\n\n"), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("\n\n", "h\n\nello world"), 1);
		ensure_equals(unmatched_data, "h");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("\n\n", "he\n\nllo world"), 2);
		ensure_equals(unmatched_data, "he");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("\n\n", "hel\n\nllo world"), 3);
		ensure_equals(unmatched_data, "hel");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("\n\n", "hello\n\nworld\n\n"), 5);
		ensure_equals(unmatched_data, "hello");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("\n\n", "\nhello\n\nworld\n\n"), 6);
		ensure_equals(unmatched_data, "\nhello");
		ensure_equals(lookbehind, "");
		
		ensure_equals(find("\n\n", "h\nello\n\nworld\n\n"), 6);
		ensure_equals(unmatched_data, "h\nello");
		ensure_equals(lookbehind, "");
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
		
		ensure_equals(find(
			"I have control\n",
			"[sbmh] inconclusive\n"
			"HorspoolTest: .........\n"
			"I hive control\n"
			"I have control\n"
			"x"),
			59);
		ensure_equals(unmatched_data,
			"[sbmh] inconclusive\n"
			"HorspoolTest: .........\n"
			"I hive control\n");
		ensure_equals(lookbehind, "");
	}
	
	
	/****** Test feeding data byte-by-byte ******/
	
	TEST_METHOD(21) {
		set_test_name("It returns the haystack length if the needle "
			"(1 character) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("0", "123456789"), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("x", "hello world"), -1);
		ensure_equals(unmatched_data, "hello world");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(22) {
		set_test_name("It returns the haystack length if the needle "
			"(2 different characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("ab", "123456789"), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "a23456789"), -1);
		ensure_equals(unmatched_data, "a23456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "1a3456789"), -1);
		ensure_equals(unmatched_data, "1a3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "1b3456789"), -1);
		ensure_equals(unmatched_data, "1b3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "123b56789"), -1);
		ensure_equals(unmatched_data, "123b56789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a456789"), -1);
		ensure_equals(unmatched_data, "12a456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678a"), -1);
		ensure_equals(unmatched_data, "12a45678");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678aa"), -1);
		ensure_equals(unmatched_data, "12a45678a");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678x"), -1);
		ensure_equals(unmatched_data, "12a45678x");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678b"), -1);
		ensure_equals(unmatched_data, "12a45678b");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(23) {
		set_test_name("It returns the haystack length if the needle "
			"(2 identical characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("aa", "123456789"), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("aa", "a23456789"), -1);
		ensure_equals(unmatched_data, "a23456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("aa", "1a3456789"), -1);
		ensure_equals(unmatched_data, "1a3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a6789"), -1);
		ensure_equals(unmatched_data, "12a4a6789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678a"), -1);
		ensure_equals(unmatched_data, "12a45678");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678aa"), -1);
		ensure_equals(unmatched_data, "12a45678a");
		ensure_equals(lookbehind, "a");
	}
	
	TEST_METHOD(24) {
		set_test_name("Searching in an empty string always fails.");
		
		ensure_equals(feed_in_chunks_and_find("1", ""), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("abc", ""), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("hello world", ""), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(25) {
		set_test_name("Searching for a needle that's larger than the haystack always fails");
		
		ensure_equals(feed_in_chunks_and_find("ab", "a"), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("hello", "hm"), -1);
		ensure_equals(unmatched_data, "hm");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("hello my world!", "this is small"), -1);
		ensure_equals(unmatched_data, "this is small");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(30) {
		set_test_name("It returns the position in which the needle is first found (1 character needle)");
		
		ensure_equals(feed_in_chunks_and_find("1", "1234567891"), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("2", "1234567892"), 1);
		ensure_equals(unmatched_data, "1");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("8", "1234567898"), 7);
		ensure_equals(unmatched_data, "1234567");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("9", "1234567899"), 8);
		ensure_equals(unmatched_data, "12345678");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(31) {
		set_test_name("It returns the position in which the needle is first found (2 different character needle)");
		
		ensure_equals(feed_in_chunks_and_find("ab", "ab3456789ab"), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "1ab456789ab"), 1);
		ensure_equals(unmatched_data, "1");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "12ab56789ab"), 2);
		ensure_equals(unmatched_data, "12");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "123ab6789ab"), 3);
		ensure_equals(unmatched_data, "123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "bbab3456789ab"), 2);
		ensure_equals(unmatched_data, "bb");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "bb1ab456789ab"), 3);
		ensure_equals(unmatched_data, "bb1");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "bb12ab56789ab"), 4);
		ensure_equals(unmatched_data, "bb12");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "bb123ab6789ab"), 5);
		ensure_equals(unmatched_data, "bb123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "baab3456789ab"), 2);
		ensure_equals(unmatched_data, "ba");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "ba1ab456789ab"), 3);
		ensure_equals(unmatched_data, "ba1");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "ba12ab56789ab"), 4);
		ensure_equals(unmatched_data, "ba12");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "ba123ab6789ab"), 5);
		ensure_equals(unmatched_data, "ba123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "003456789ab"), 9);
		ensure_equals(unmatched_data, "003456789");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "100456789aabab"), 10);
		ensure_equals(unmatched_data, "100456789a");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "120056789abbab"), 9);
		ensure_equals(unmatched_data, "120056789");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(32) {
		set_test_name("It returns the position in which the needle is found (2 identifical character needle)");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "\n\nhello world\n\n"), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\n\nello world"), 1);
		ensure_equals(unmatched_data, "h");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "he\n\nllo world"), 2);
		ensure_equals(unmatched_data, "he");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "hel\n\nllo world"), 3);
		ensure_equals(unmatched_data, "hel");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "hello\n\nworld\n\n"), 5);
		ensure_equals(unmatched_data, "hello");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "\nhello\n\nworld\n\n"), 6);
		ensure_equals(unmatched_data, "\nhello");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\nello\n\nworld\n\n"), 6);
		ensure_equals(unmatched_data, "h\nello");
		ensure_equals(lookbehind, "");
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
		
		ensure_equals(feed_in_chunks_and_find(
			"I have control\n",
			"[sbmh] inconclusive\n"
			"HorspoolTest: .........\n"
			"I hive control\n"
			"I have control\n"
			"x"),
			59);
		ensure_equals(unmatched_data,
			"[sbmh] inconclusive\n"
			"HorspoolTest: .........\n"
			"I hive control\n");
		ensure_equals(lookbehind, "");
	}
	
	
	/****** Test feeding data in chunks of 3 bytes ******/
	
	TEST_METHOD(41) {
		set_test_name("It returns the haystack length if the needle "
			"(1 character) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("0", "123456789", 3), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("x", "hello world", 3), -1);
		ensure_equals(unmatched_data, "hello world");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(42) {
		set_test_name("It returns the haystack length if the needle "
			"(2 different characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("ab", "123456789", 3), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "a23456789", 3), -1);
		ensure_equals(unmatched_data, "a23456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "1a3456789", 3), -1);
		ensure_equals(unmatched_data, "1a3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "1b3456789", 3), -1);
		ensure_equals(unmatched_data, "1b3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "123b56789", 3), -1);
		ensure_equals(unmatched_data, "123b56789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a456789", 3), -1);
		ensure_equals(unmatched_data, "12a456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678a", 3), -1);
		ensure_equals(unmatched_data, "12a45678");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678aa", 3), -1);
		ensure_equals(unmatched_data, "12a45678a");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678x", 3), -1);
		ensure_equals(unmatched_data, "12a45678x");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "12a45678b", 3), -1);
		ensure_equals(unmatched_data, "12a45678b");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(43) {
		set_test_name("It returns the haystack length if the needle "
			"(2 identical characters) can't be found.");
		
		ensure_equals(feed_in_chunks_and_find("aa", "123456789", 3), -1);
		ensure_equals(unmatched_data, "123456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("aa", "a23456789", 3), -1);
		ensure_equals(unmatched_data, "a23456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("aa", "1a3456789", 3), -1);
		ensure_equals(unmatched_data, "1a3456789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a6789", 3), -1);
		ensure_equals(unmatched_data, "12a4a6789");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a678a", 3), -1);
		ensure_equals(unmatched_data, "12a4a678");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("aa", "12a4a678ba", 3), -1);
		ensure_equals(unmatched_data, "12a4a678b");
		ensure_equals(lookbehind, "a");
	}
	
	TEST_METHOD(44) {
		set_test_name("Searching in an empty string always fails.");
		
		ensure_equals(feed_in_chunks_and_find("1", "", 3), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("abc", "", 3), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("hello world", "", 3), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(45) {
		set_test_name("Searching for a needle that's larger than the haystack always fails");
		
		ensure_equals(feed_in_chunks_and_find("ab", "a", 3), -1);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "a");
		
		ensure_equals(feed_in_chunks_and_find("hello", "hm", 3), -1);
		ensure_equals(unmatched_data, "hm");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("hello my world!", "this is small", 3), -1);
		ensure_equals(unmatched_data, "this is small");
		ensure_equals(lookbehind, "");
	}
	
	
	TEST_METHOD(50) {
		set_test_name("It returns the position in which the needle is first found (1 character needle)");
		
		ensure_equals(feed_in_chunks_and_find("1", "1234567891", 3), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("2", "1234567892", 3), 1);
		ensure_equals(unmatched_data, "1");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("8", "1234567898", 3), 7);
		ensure_equals(unmatched_data, "1234567");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("9", "1234567899", 3), 8);
		ensure_equals(unmatched_data, "12345678");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(51) {
		set_test_name("It returns the position in which the needle is first found (2 different character needle)");
		
		ensure_equals(feed_in_chunks_and_find("ab", "ab3456789ab", 3), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "1ab456789ab", 3), 1);
		ensure_equals(unmatched_data, "1");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "12ab56789ab", 3), 2);
		ensure_equals(unmatched_data, "12");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "123ab6789ab", 3), 3);
		ensure_equals(unmatched_data, "123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "bbab3456789ab", 3), 2);
		ensure_equals(unmatched_data, "bb");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "bb1ab456789ab", 3), 3);
		ensure_equals(unmatched_data, "bb1");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "bb12ab56789ab", 3), 4);
		ensure_equals(unmatched_data, "bb12");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "bb123ab6789ab", 3), 5);
		ensure_equals(unmatched_data, "bb123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "baab3456789ab", 3), 2);
		ensure_equals(unmatched_data, "ba");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "ba1ab456789ab", 3), 3);
		ensure_equals(unmatched_data, "ba1");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "ba12ab56789ab", 3), 4);
		ensure_equals(unmatched_data, "ba12");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "ba123ab6789ab", 3), 5);
		ensure_equals(unmatched_data, "ba123");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("ab", "003456789ab", 3), 9);
		ensure_equals(unmatched_data, "003456789");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "100456789aabab", 3), 10);
		ensure_equals(unmatched_data, "100456789a");
		ensure_equals(lookbehind, "");
		ensure_equals(feed_in_chunks_and_find("ab", "120056789abbab", 3), 9);
		ensure_equals(unmatched_data, "120056789");
		ensure_equals(lookbehind, "");
	}
	
	TEST_METHOD(52) {
		set_test_name("It returns the position in which the needle is found (2 identifical character needle)");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "\n\nhello world\n\n", 3), 0);
		ensure_equals(unmatched_data, "");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\n\nello world", 3), 1);
		ensure_equals(unmatched_data, "h");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "he\n\nllo world", 3), 2);
		ensure_equals(unmatched_data, "he");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "hel\n\nllo world", 3), 3);
		ensure_equals(unmatched_data, "hel");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "hell\n\nlo world", 3), 4);
		ensure_equals(unmatched_data, "hell");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "hello\n\nworld\n\n", 3), 5);
		ensure_equals(unmatched_data, "hello");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "\nhello\n\nworld\n\n", 3), 6);
		ensure_equals(unmatched_data, "\nhello");
		ensure_equals(lookbehind, "");
		
		ensure_equals(feed_in_chunks_and_find("\n\n", "h\nello\n\nworld\n\n", 3), 6);
		ensure_equals(unmatched_data, "h\nello");
		ensure_equals(lookbehind, "");
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
		
		ensure_equals(feed_in_chunks_and_find(
			"I have control\n",
			"[sbmh] inconclusive\n"
			"HorspoolTest: .........\n"
			"I hive control\n"
			"I have control\n"
			"x", 3),
			59);
		ensure_equals(unmatched_data,
			"[sbmh] inconclusive\n"
			"HorspoolTest: .........\n"
			"I hive control\n");
		ensure_equals(lookbehind, "");
	}
}
