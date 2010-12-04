#include <string>

#include "tut.h"
#include "Horspool.cpp"

using namespace std;

namespace tut {
	struct HorspoolTest {
		static int find(const string &needle, const string &haystack) {
			const occtable_type occ = CreateOccTable(
				(const unsigned char *) needle.c_str(),
				needle.size());
			size_t result = (int) SearchInHorspool(
				(const unsigned char *) haystack.c_str(), haystack.size(),
				occ,
				(const unsigned char *) needle.c_str(), needle.size());
			if (result == haystack.size()) {
				return -1;
			} else {
				return (int) result;
			}
		}
	};
	
	DEFINE_TEST_GROUP(HorspoolTest);
	
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
		set_test_name("It returns the position a which the needle is first found (1 character needle)");
		
		ensure_equals(find("1", "1234567891"), 0);
		ensure_equals(find("2", "1234567892"), 1);
		ensure_equals(find("8", "1234567898"), 7);
		ensure_equals(find("9", "1234567899"), 8);
	}
	
	TEST_METHOD(11) {
		set_test_name("It returns the position a which the needle is first found (2 different character needle)");
		
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
		set_test_name("It returns the position a which the needle is found (2 identifical character needle)");
		
		ensure_equals(find("\n\n", "\n\nhello world\n\n"), 0);
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
}
