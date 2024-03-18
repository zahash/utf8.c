#include "utf8.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

// english characters are 1 byte each
// russian  2 bytes each
// japanese 3 bytes each

void test_validate_utf8_ok() {
  utf8_validity validity = validate_utf8("Hello Здравствуйте こんにちは");
  assert(validity.valid == true);
  assert(validity.valid_upto == 5 + 1 + 12 * 2 + 1 + 5 * 3);
}

void test_validate_utf8_err() {
  utf8_validity validity = validate_utf8("Hello Здравствуйте\xC0\xC0 こんにちは");
  assert(validity.valid == false);
  assert(validity.valid_upto == 5 + 1 + 12 * 2);
}

void test_make_utf8_string_ok() {
  const char* str = "Hello Здравствуйте こんにちは";
  utf8_string ustr = make_utf8_string(str);
  assert(ustr.byte_len == 5 + 1 + 12 * 2 + 1 + 5 * 3);
  assert(strcmp(ustr.str, str) == 0);
}

void test_make_utf8_string_err() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте\xC0\xC0 こんにちは");
  assert(ustr.str == NULL);
  assert(ustr.byte_len == 0);
}

void test_make_utf8_string_slice_ok() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは");
  utf8_string_slice slice = make_utf8_string_slice(str, 6, 24);
  assert(slice.byte_len == 12 * 2);
  assert(strncmp(slice.str, "Здравствуйте", slice.byte_len) == 0);
}

void test_make_utf8_string_slice_ok_start_out_of_bounds() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは");
  utf8_string_slice slice = make_utf8_string_slice(str, 1000, 1);
  assert(slice.byte_len == 0);
  assert(strcmp(slice.str, "") == 0);
}

void test_make_utf8_string_slice_ok_end_out_of_bounds() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは");
  utf8_string_slice slice = make_utf8_string_slice(str, 6, 1000);
  assert(slice.byte_len == 12 * 2 + 1 + 5 * 3);
  assert(strncmp(slice.str, "Здравствуйте こんにちは", slice.byte_len) == 0);
}

void test_make_utf8_string_slice_err_start_non_boundary() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは");
  utf8_string_slice slice = make_utf8_string_slice(str, 7, 3);
  assert(slice.str == NULL);
  assert(slice.byte_len == 0);
}

void test_make_utf8_string_slice_err_end_non_boundary() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは");
  utf8_string_slice slice = make_utf8_string_slice(str, 6, 3);
  assert(slice.str == NULL);
  assert(slice.byte_len == 0);
}

void test_utf8_char_iter() {
  utf8_string str = make_utf8_string("Hдこ");
  utf8_char_iter iter = make_utf8_char_iter(str);

  utf8_char ch;

  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 1);
  assert(strncmp(ch.str, "H", ch.byte_len) == 0);

  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 2);
  assert(strncmp(ch.str, "д", ch.byte_len) == 0);

  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 3);
  assert(strncmp(ch.str, "こ", ch.byte_len) == 0);

  // iterator keeps returning { .str = '\0', .byte_len = 0 } when exhausted
  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 0);
  assert(strcmp(ch.str, "") == 0);

  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 0);
  assert(strcmp(ch.str, "") == 0);
}

void test_is_utf8_char_boundary() {
  const char* str = "Hдこ";
  assert(is_utf8_char_boundary(str)); //    H
  assert(is_utf8_char_boundary(++str)); //  д
  assert(!is_utf8_char_boundary(++str));
  assert(is_utf8_char_boundary(++str)); //  こ
  assert(!is_utf8_char_boundary(++str));
  assert(!is_utf8_char_boundary(++str));
  assert(strcmp(++str, "") == 0);
}

int ntests = 0;
#define TEST(test_fn) test_fn(); ntests++; printf("%s\n", #test_fn);

int main() {
  TEST(test_validate_utf8_ok);
  TEST(test_validate_utf8_err);
  TEST(test_make_utf8_string_ok);
  TEST(test_make_utf8_string_err);
  TEST(test_make_utf8_string_slice_ok);
  TEST(test_make_utf8_string_slice_ok_start_out_of_bounds);
  TEST(test_make_utf8_string_slice_ok_end_out_of_bounds);
  TEST(test_make_utf8_string_slice_err_start_non_boundary);
  TEST(test_make_utf8_string_slice_err_end_non_boundary);
  TEST(test_utf8_char_iter);
  TEST(test_is_utf8_char_boundary);

  printf("\n** %d tests passed **\n", ntests);
  return 0;
}
