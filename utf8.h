#ifndef ZAHASH_UTF8_H
#define ZAHASH_UTF8_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * invalid case => "hello\xC0\xC0" => { .valid = false, .valid_upto = 5 }
 * valid case   => "hello world"   => { .valid = true,  .valud_upto = 11 }
 */
typedef struct {
    bool valid;
    size_t valid_upto;
} utf8_validity;

typedef struct {
    const char *str;
    size_t byte_len; // excluding terminating '\0'
} utf8_string;

typedef struct {
    const char *str;
    size_t byte_len;
} utf8_string_slice;

typedef struct {
    const char *str;
} utf8_char_iter;

typedef struct {
    const char *str;
    uint8_t byte_len; // utf8 char can be upto 4 bytes
} utf8_char;

/**
 * checks if given char ptr is utf8 compliant. terminating null byte is preserved.
 * O(n) time
 */
utf8_validity validate_utf8(const char *str);

/**
 * wraps the char ptr `str` in `utf8_string`, after verifying (see validate_utf8()) the string is utf8
 * if not utf8, it will return { .str = NULL, .byte_len = 0 }.
 *
 * Example:
 *
 * char *s = "definitely utf8 string こんにちは नमस्ते Здравствуйте";
 * utf8_string us = make_utf8_string(s);
 * assert(us.str != NULL);
 *
 * char *s = "non-utf8 sequence -> \xC0\xC0";
 * utf8_string us = make_utf8_string(s);
 * assert(us.str == NULL);
 */
utf8_string make_utf8_string(const char *str);

/**
 * returns utf8_string_slice if the byte slice between the range [offset, offset + byte_len) is also utf8
 * retruns { .str = NULL, .byte_len = 0 } for any failure reason (bounds checks fail)
 *
 * NOTE:
 * if `byte_index` + `byte_len` >= strlen(ustr.str) then only chars till terminating '\0' are considered.
 */
utf8_string_slice make_utf8_string_slice(utf8_string ustr, size_t byte_index, size_t byte_len);

/**
 * returns a utf8 character iterator
 * call the next_utf8_char() with iterator as input to get the next utf8 char
 */
utf8_char_iter make_utf8_char_iter(utf8_string ustr);

/**
 * returns the next utf8 character.
 * if iterator reaches its end, it keeps returning { .str = '\0', .byte_len = 0 }
 */
utf8_char next_utf8_char(utf8_char_iter *iter);

bool is_utf8_char_boundary(const char *str);

#endif

