#include "utf8.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// english characters are 1 byte each
// russian  2 bytes each
// japanese 3 bytes each
// 🚩 and 😁 is 4 bytes each

void test_validate_utf8_ok() {
  utf8_validity validity = validate_utf8("Hello Здравствуйте こんにちは 🚩😁");
  assert(validity.valid == true);
  assert(validity.valid_upto == 5 + 1 + 12 * 2 + 1 + 5 * 3 + 1 + 2 * 4);
}

void test_validate_utf8_boundary_ok() {
  utf8_validity validity;

  // last 1b -> 0(1111111)
  validity = validate_utf8("\x7F");
  assert(validity.valid);
  assert(validity.valid_upto == 1);

  // first 2b -> 110(00010) 10(000000)
  validity = validate_utf8("\xC2\x80");
  assert(validity.valid);
  assert(validity.valid_upto == 2);

  // last 2b -> 110(11111) 10(111111)
  validity = validate_utf8("\xDF\xBF");
  assert(validity.valid);
  assert(validity.valid_upto == 2);

  // first 3b -> 1110(0000) 10(100000) 10(000000)
  validity = validate_utf8("\xE0\xA0\x80");
  assert(validity.valid);
  assert(validity.valid_upto == 3);

  // last 3b -> 1110(1111) 10(111111) 10(111111)
  validity = validate_utf8("\xEF\xBF\xBF");
  assert(validity.valid);
  assert(validity.valid_upto == 3);

  // first 4b -> 11110(000) 10(010000) 10(000000) 10(000000)
  validity = validate_utf8("\xF0\x90\x80\x80");
  assert(validity.valid);
  assert(validity.valid_upto == 4);

  // last 4b -> 11110(111) 10(111111) 10(111111) 10(111111)
  validity = validate_utf8("\xF7\xBF\xBF\xBF");
  assert(validity.valid);
  assert(validity.valid_upto == 4);
}

void test_surrogate_rejection() {
  utf8_validity validity;

  validity = validate_utf8("\xED\xA0\x80");
  assert(validity.valid == false);
  assert(validity.valid_upto == 0);

  validity = validate_utf8("\xED\xAC\x80");
  assert(validity.valid == false);
  assert(validity.valid_upto == 0);

  validity = validate_utf8("\xED\xA0\x8C");
  assert(validity.valid == false);
  assert(validity.valid_upto == 0);

  validity = validate_utf8("\xED\xBF\xBF");
  assert(validity.valid == false);
  assert(validity.valid_upto == 0);
}

void test_validate_utf8_err() {
  utf8_validity validity = validate_utf8("Hello Здравствуйте\xC0\xC0 こんにちは 🚩😁");
  assert(validity.valid == false);
  assert(validity.valid_upto == 5 + 1 + 12 * 2);
}

void assert_overlong_encodings(utf8_char actual, utf8_char overlong) {
  assert(unicode_code_point(actual) == unicode_code_point(overlong));

  utf8_validity validity;

  validity = validate_utf8(actual.str);
  assert(validity.valid == true);
  assert(validity.valid_upto == actual.byte_len);

  validity = validate_utf8(overlong.str);
  assert(validity.valid == false);
  assert(validity.valid_upto == 0);
}

void test_validate_utf8_overlong_encoding_err() {
  assert_overlong_encodings((utf8_char) { .str = "H", .byte_len = 1 }, (utf8_char) { .str = "\xC1\x88", .byte_len = 2 });
  assert_overlong_encodings((utf8_char) { .str = "H", .byte_len = 1 }, (utf8_char) { .str = "\xE0\x81\x88", .byte_len = 3 });
  assert_overlong_encodings((utf8_char) { .str = "H", .byte_len = 1 }, (utf8_char) { .str = "\xF0\x80\x81\x88", .byte_len = 4 });

  assert_overlong_encodings((utf8_char) { .str = "д", .byte_len = 2 }, (utf8_char) { .str = "\xE0\x90\xB4", .byte_len = 3 });
  assert_overlong_encodings((utf8_char) { .str = "д", .byte_len = 2 }, (utf8_char) { .str = "\xF0\x80\x90\xB4", .byte_len = 4 });

  assert_overlong_encodings((utf8_char) { .str = "こ", .byte_len = 3 }, (utf8_char) { .str = "\xF0\x83\x81\x93", .byte_len = 4 });

  // boundary characters
  assert_overlong_encodings((utf8_char) { .str = "\x7F", .byte_len = 1 }, (utf8_char) { .str = "\xC1\xBF", .byte_len = 2 }); // last 1b, last 1b overlong
  assert_overlong_encodings((utf8_char) { .str = "\xDF\xBF", .byte_len = 2 }, (utf8_char) { .str = "\xE0\x9F\xBF", .byte_len = 3 }); // last 2b, last 2b overlong
  assert_overlong_encodings((utf8_char) { .str = "\xEF\xBF\xBF", .byte_len = 3 }, (utf8_char) { .str = "\xF0\x8F\xBF\xBF", .byte_len = 4 }); // last 3b, last 3b overlong
}

void test_make_utf8_string_ok() {
  const char* str = "Hello Здравствуйте こんにちは 🚩😁";
  utf8_string ustr = make_utf8_string(str);
  assert(ustr.byte_len == 5 + 1 + 12 * 2 + 1 + 5 * 3 + 1 + 2 * 4);
  assert(strcmp(ustr.str, str) == 0);
}

void test_make_utf8_string_err() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте\xC0\xC0 こんにちは 🚩😁");
  assert(ustr.str == NULL);
  assert(ustr.byte_len == 0);
}

void test_make_utf8_string_lossy_ok() {
    const char* str = "Hello Здравствуйте こんにちは 🚩😁";
    owned_utf8_string owned_ustr = make_utf8_string_lossy(str);
    assert(owned_ustr.byte_len == 5 + 1 + 12 * 2 + 1 + 5 * 3 + 1 + 2 * 4);
    assert(strcmp(owned_ustr.str, str) == 0);
    free_owned_utf8_string(&owned_ustr);
}

void test_make_utf8_string_lossy_invalid_sequence() {
    const char* str = "\xC0He\xC0llo Здр\xC0авствуйте\xC0\xC0 こんに\xC0\xC0\xC0\xC0ちは 🚩\xC0😁\xC0";
    const char* expected = "�He�llo Здр�авствуйте�� こんに����ちは 🚩�😁�";

    owned_utf8_string owned_ustr = make_utf8_string_lossy(str);

    assert(owned_ustr.byte_len == strlen(expected));
    assert(strcmp(owned_ustr.str, expected) == 0);
    free_owned_utf8_string(&owned_ustr);
}

void test_make_utf8_string_lossy_completely_invalid() {
    const char* str = "\xC0\xC0\xC0\xC0";
    const char* expected = "����";

    owned_utf8_string owned_ustr = make_utf8_string_lossy(str);

    assert(owned_ustr.byte_len == strlen(expected));
    assert(strcmp(owned_ustr.str, expected) == 0);
    free_owned_utf8_string(&owned_ustr);
}

void test_make_utf8_string_slice_ok() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_string slice = slice_utf8_string(str, 6, 24);
  assert(slice.byte_len == 12 * 2);
  assert(strncmp(slice.str, "Здравствуйте", slice.byte_len) == 0);
}

void test_make_utf8_string_slice_start_out_of_bounds_ok() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_string slice = slice_utf8_string(str, 1000, 1);
  assert(slice.byte_len == 0);
  assert(strcmp(slice.str, "") == 0);
}

void test_make_utf8_string_slice_end_out_of_bounds_ok() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_string slice = slice_utf8_string(str, 6, 1000);
  assert(slice.byte_len == 12 * 2 + 1 + 5 * 3 + 1 + 2 * 4);
  assert(strncmp(slice.str, "Здравствуйте こんにちは 🚩😁", slice.byte_len) == 0);
}

void test_make_utf8_string_slice_start_non_boundary_err() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_string slice = slice_utf8_string(str, 7, 3);
  assert(slice.str == NULL);
  assert(slice.byte_len == 0);
}

void test_make_utf8_string_slice_end_non_boundary_err() {
  utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_string slice = slice_utf8_string(str, 6, 3);
  assert(slice.str == NULL);
  assert(slice.byte_len == 0);
}

void test_utf8_char_iter() {
  utf8_string str = make_utf8_string("Hдこ😁");
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

  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 4);
  assert(strncmp(ch.str, "😁", ch.byte_len) == 0);

  // iterator keeps returning { .str = '\0', .byte_len = 0 } when exhausted
  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 0);
  assert(strcmp(ch.str, "") == 0);

  ch = next_utf8_char(&iter);
  assert(ch.byte_len == 0);
  assert(strcmp(ch.str, "") == 0);
}

void test_utf8_char_count_zero() {
  utf8_string ustr = make_utf8_string("");
  size_t count = utf8_char_count(ustr);
  assert(count == 0);
}

void test_utf8_char_count() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  size_t count = utf8_char_count(ustr);
  assert(count == 5 + 1 + 12 + 1 + 5 + 1 + 2);
}

void test_utf8_slice_char_count() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_string slice = slice_utf8_string(ustr, 0, 5 + 1 + 24);
  size_t count = utf8_char_count(slice);
  assert(count == 5 + 1 + 12);
}

void test_is_utf8_char_boundary() {
  const char* str = "Hдこ😁";
  assert(is_utf8_char_boundary(str)); //    H

  assert(is_utf8_char_boundary(++str)); //  д
  assert(!is_utf8_char_boundary(++str));

  assert(is_utf8_char_boundary(++str)); //  こ
  assert(!is_utf8_char_boundary(++str));
  assert(!is_utf8_char_boundary(++str));

  assert(is_utf8_char_boundary(++str)); //  😁
  assert(!is_utf8_char_boundary(++str));
  assert(!is_utf8_char_boundary(++str));
  assert(!is_utf8_char_boundary(++str));

  assert(strcmp(++str, "") == 0);
}

void test_nth_utf8_char_valid_index_ok() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_char ch = nth_utf8_char(ustr, 20);
  assert(ch.byte_len == 3);
  assert(strncmp(ch.str, "ん", ch.byte_len) == 0);
}

void test_nth_utf8_char_first_index_ok() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_char ch = nth_utf8_char(ustr, 0);
  assert(ch.byte_len == 1);
  assert(*ch.str == 'H');
}

void test_nth_utf8_char_last_index_ok() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_char ch = nth_utf8_char(ustr, 26);
  assert(ch.byte_len == 4);
  assert(strncmp(ch.str, "😁", ch.byte_len) == 0);
}

void test_nth_utf8_char_invalid_index_err() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_char ch = nth_utf8_char(ustr, 100);
  assert(ch.str == NULL);
  assert(ch.byte_len == 0);
}

void test_slice_nth_utf8_char_invalid_index_err() {
  utf8_string ustr = make_utf8_string("Hello Здравствуйте こんにちは 🚩😁");
  utf8_string slice = slice_utf8_string(ustr, 0, 5 + 1 + 24);
  utf8_char ch = nth_utf8_char(slice, 18);
  assert(ch.str == NULL);
  assert(ch.byte_len == 0);
}

void test_nth_utf8_char_empty_string_err() {
  utf8_string ustr = make_utf8_string("");
  utf8_char ch = nth_utf8_char(ustr, 0);
  assert(ch.str == NULL);
  assert(ch.byte_len == 0);
}

void test_unicode_code_point() {
  utf8_string ustr = make_utf8_string("Hдこ😁");
  utf8_char_iter iter = make_utf8_char_iter(ustr);

  assert(unicode_code_point(next_utf8_char(&iter)) == 72); // H
  assert(unicode_code_point(next_utf8_char(&iter)) == 1076); // д
  assert(unicode_code_point(next_utf8_char(&iter)) == 12371); // こ
  assert(unicode_code_point(next_utf8_char(&iter)) == 128513); // 😁
}

int ntests = 0;
#define TEST(test_fn) test_fn(); ntests++; printf("%s\n", #test_fn);

int main() {
  TEST(test_validate_utf8_ok);
  TEST(test_validate_utf8_boundary_ok);
  TEST(test_surrogate_rejection);
  TEST(test_validate_utf8_err);
  TEST(test_validate_utf8_overlong_encoding_err);
  TEST(test_make_utf8_string_ok);
  TEST(test_make_utf8_string_err);
  TEST(test_make_utf8_string_lossy_ok);
  TEST(test_make_utf8_string_lossy_invalid_sequence);
  TEST(test_make_utf8_string_lossy_completely_invalid);
  TEST(test_make_utf8_string_slice_ok);
  TEST(test_make_utf8_string_slice_start_out_of_bounds_ok);
  TEST(test_make_utf8_string_slice_end_out_of_bounds_ok);
  TEST(test_make_utf8_string_slice_start_non_boundary_err);
  TEST(test_make_utf8_string_slice_end_non_boundary_err);
  TEST(test_utf8_char_iter);
  TEST(test_utf8_char_count_zero);
  TEST(test_utf8_char_count);
  TEST(test_utf8_slice_char_count);
  TEST(test_is_utf8_char_boundary);
  TEST(test_nth_utf8_char_valid_index_ok);
  TEST(test_nth_utf8_char_first_index_ok);
  TEST(test_nth_utf8_char_last_index_ok);
  TEST(test_nth_utf8_char_invalid_index_err);
  TEST(test_slice_nth_utf8_char_invalid_index_err);
  TEST(test_nth_utf8_char_empty_string_err);
  TEST(test_unicode_code_point);

  printf("\n** %d tests passed **\n", ntests);
  return 0;
}
