
#include "lib.h"
#include "testlib.h"

#define ASSERT_MATCHES(REGEX, INPUT) \
  ASSERT(re_matches(re_compile(REGEX), INPUT), "r<%s> did not match '%s'", (REGEX), (INPUT))

#define ASSERT_NOT_MATCHES(REGEX, INPUT) \
  ASSERT(! re_matches(re_compile(REGEX), INPUT), "r<%s> (incorrectly) matched '%s'", (REGEX), (INPUT))

UNIT_TEST(test_re_empty)
void test_re_empty(void) {
  ASSERT_MATCHES("", "");
}

UNIT_TEST(test_re_char)
void test_re_char(void) {
  const char* r = "x";

  ASSERT_MATCHES(r, "x");

  ASSERT_NOT_MATCHES(r, "y");
  ASSERT_NOT_MATCHES(r, "xx");
}

UNIT_TEST(test_re_word)
void test_re_word(void) {
  const char* r = "abc";

  ASSERT_MATCHES(r, "abc");

  ASSERT_NOT_MATCHES(r, "xabc");
  ASSERT_NOT_MATCHES(r, "abcx");
  ASSERT_NOT_MATCHES(r, "abcabc");
}

UNIT_TEST(test_re_wildcard)
void test_re_wildcard(void) {
  const char* r = "a*c";

  ASSERT_MATCHES(r, "ac");
  ASSERT_MATCHES(r, "abc");
  ASSERT_MATCHES(r, "abbc");

  ASSERT_NOT_MATCHES(r, "a");
  ASSERT_NOT_MATCHES(r, "ab");
  ASSERT_NOT_MATCHES(r, "abb");
  ASSERT_NOT_MATCHES(r, "abcb");
}

UNIT_TEST(test_re_multi_wildcard)
void test_re_multi_wildcard(void) {
  const char* r = "a*b*c";

  ASSERT_MATCHES(r, "abc");
  ASSERT_MATCHES(r, "axxbc");
  ASSERT_MATCHES(r, "abxxc");
  ASSERT_MATCHES(r, "axxbxxc");
  ASSERT_MATCHES(r, "axxbxxcbc");
  ASSERT_MATCHES(r, "axxbxxcbxxc");
  ASSERT_MATCHES(r, "axxbxxcbxxcxxbc");
  ASSERT_MATCHES(r, "axxbxxcbxxcxxbxxcbc");
  ASSERT_MATCHES(r, "axxbxxcbxxcxxbxxcbxxc");
  ASSERT_MATCHES(r, "axxbxxcbxxcxxbxxcbxxcxxc");

  ASSERT_NOT_MATCHES(r, "a");
  ASSERT_NOT_MATCHES(r, "ab");
  ASSERT_NOT_MATCHES(r, "abb");
  ASSERT_NOT_MATCHES(r, "b");
  ASSERT_NOT_MATCHES(r, "c");
  ASSERT_NOT_MATCHES(r, "bc");
  ASSERT_NOT_MATCHES(r, "bxxc");
}