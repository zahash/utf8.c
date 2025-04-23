/**
 * @file utf8.h
 * @brief simple library for working with UTF-8 encoded strings
 *
 * @code
 * #include "utf8.h"
 * #include <stdio.h>
 *
 * int main() {
 *     const char* str = "Hello, こんにちは, Здравствуйте";
 *     utf8_string ustr = make_utf8_string(str);
 *     utf8_string_slice slice = make_utf8_string_slice(ustr, 2, 11);
 *     utf8_char_iter iter = make_utf8_char_iter(ustr);
 *
 *     printf("string: %s\n", ustr.str);
 *     printf("slice: %.*s\n", (int)slice.byte_len, slice.str);
 *
 *     utf8_char ch;
 *     while ((ch = next_utf8_char(&iter)).byte_len > 0) {
 *         printf("character: %.*s\t", (int)ch.byte_len, ch.str);
 *         printf("unicode code point: U+%04X\n", unicode_code_point(ch));
 *     }
 *
 *     return 0;
 * }
 * @endcode
 */

#ifndef ZAHASH_UTF8_H
#define ZAHASH_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Represents the validity of a UTF-8 encoded string.
 *
 * @details The `utf8_validity` struct indicates whether a given UTF-8 encoded string is valid or not,
 * along with the position up to which it is valid.
 *
 * - Invalid case: "hello\xC0\xC0" => { .valid = false, .valid_upto = 5  }
 * - Valid case:   "hello world"   => { .valid = true,  .valid_upto = 11 }
 */
typedef struct {
    bool valid;          ///< Flag indicating the validity of the UTF-8 string.
    size_t valid_upto;   ///< The position up to which the string is valid.
} utf8_validity;

/**
 * @brief Represents a non-owning UTF-8 encoded string. (just a wrapper type).
 *
 * @details The `utf8_string` struct holds a pointer to a UTF-8 encoded string along with its byte length,
 */
typedef struct {
    const char* str;     ///< Pointer to the UTF-8 encoded string.
    size_t byte_len;     ///< Byte length of the UTF-8 string ('\0' not counted).
} utf8_string;

/**
 * @brief Represents a UTF-8 encoded string that fully owns its data.
 *
 * @details The `owned_utf8_string` struct holds a pointer to a UTF-8 encoded string that is dynamically allocated
 *          and therefore is owned by the struct, which means the caller is responsible for freeing the memory when
 *          it is no longer needed using the `free_owned_utf8_string` function.
 */
typedef struct {
    char* str;          ///< Pointer to the UTF-8 encoded string (owned). This memory is dynamically allocated.
    size_t byte_len;    ///< Byte length of the UTF-8 string ('\0' not counted).
} owned_utf8_string;

/**
 * @brief Represents an iterator for traversing UTF-8 characters in a string.
 *
 * @details The `utf8_char_iter` struct serves as an iterator for traversing UTF-8 characters
 * within a UTF-8 encoded string.
 */
typedef struct {
    const char* str;           ///< Pointer to the current position of the iterator.
    const char* terminator;    ///< Pointer to the last character in the string being iterated
} utf8_char_iter;

/**
 * @brief Represents a UTF-8 character.
 *
 * @details The `utf8_char` struct encapsulates a UTF-8 character, including its pointer and byte length.
 * The byte length represents the number of bytes occupied by the UTF-8 character.
 */
typedef struct {
    const char* str;     ///< Pointer to the UTF-8 character.
    uint8_t byte_len;    ///< Byte length of the UTF-8 character.
} utf8_char;

/**
 * @brief Validates whether a given string is UTF-8 compliant in O(n) time.
 *
 * @param str The input string to validate.
 * @return The validity of the UTF-8 string along with the position up to which it is valid.
 */
utf8_validity validate_utf8(const char* str);

/**
 * @brief Wraps a C-style string in a UTF-8 string structure after verifying its UTF-8 compliance.
 *
 * @param str The input C-style string to wrap.
 * @return A UTF-8 string structure containing the wrapped string if valid; otherwise, a structure with NULL string pointer.
 *
 * @code
 * // Example usage:
 * const char *str = "definitely utf8 string こんにちは नमस्ते Здравствуйте";
 * utf8_string ustr = make_utf8_string(str);
 * assert( ustr.str != NULL );
 *
 * const char *s = "non-utf8 sequence \xC0\xC0";
 * utf8_string ustr = make_utf8_string(str);
 * assert( ustr.str == NULL );
 * @endcode
 */
utf8_string make_utf8_string(const char* str);

/**
 * @brief Converts a C-style string to a UTF-8 string, replacing invalid sequences with U+FFFD REPLACEMENT CHARACTER (�).
 *
 * @details It takes a C-style string as input and converts it to a UTF-8 encoded string.
 *          Any invalid UTF-8 sequences in the input string are replaced with the U+FFFD REPLACEMENT CHARACTER (�) to ensure
 *          that the resulting string is valid UTF-8. The resulting string is dynamically allocated and the caller
 *          is responsible for freeing the memory when no longer needed using `free_owned_utf8_string`.
 *
 * @param str The input C-style string to convert. The string can contain invalid UTF-8 sequences.
 * @return An `owned_utf8_string` structure containing the resulting UTF-8 string. If memory allocation fails, the structure
 *         will contain a `NULL` pointer and a `byte_len` of 0.
 *
 * @code
 * // Example usage:
 * const char* str = "hello\xC0\xC0 world!";
 * owned_utf8_string owned_ustr = make_utf8_string_lossy(str);
 * @endcode
 */
owned_utf8_string make_utf8_string_lossy(const char* str);

/**
 * @brief Creates the non-owning UTF-8 encoded string `utf8_string` from an `owned_utf8_string`.
 *
 * @details The resulting `utf8_string` will point to the same underlying string without taking ownership.
 *          The caller must ensure the original `owned_utf8_string` remains valid as long as the reference is used.
 *
 * @param owned_str The owned UTF-8 string from which to create a non-owning reference.
 * @return utf8_string A non-owning UTF-8 string reference (`utf8_string`) pointing to the same data.
 *
 * @note This function does not free or transfer ownership of the `owned_utf8_string`.
 *       The caller is responsible for managing the lifetime of the owned string.
 */
utf8_string as_utf8_string(const owned_utf8_string* owned_str);

/**
 * @brief Frees the memory allocated for an `owned_utf8_string`.
 *
 * @details The `free_owned_utf8_string` function deallocates the memory used by an `owned_utf8_string`
 *          and sets the `str` pointer to `NULL` and `byte_len` to 0.
 *
 * @param owned_str A pointer to the `owned_utf8_string` structure to be freed.
 *
 * @code
 * // Example usage:
 * owned_utf8_string owned_ustr = make_utf8_string_lossy("hello\xC0\xC0 world!");
 * free_owned_utf8_string(&owned_ustr);
 * @endcode
 */
void free_owned_utf8_string(owned_utf8_string* owned_str);

/**
 * @brief Creates a UTF-8 string slice from a specified range of bytes in the original string.
 *
 * @param ustr The original UTF-8 string.
 * @param byte_index The starting byte index of the slice.
 * @param byte_len The byte length of the slice.
 * @return A UTF-8 string representing the specified byte range [offset, offset + byte_len) if valid (range between UTF-8 char boundaries);
 * otherwise { .str = NULL, .byte_len = 0 }
 *
 * @note if `byte_index` >= strlen(ustr.str) then returns terminating '\0' of ustr.str { .str = '\0', .byte_len = 0 }
 * @note if `byte_index` + `byte_len` >= strlen(ustr.str) then only chars till terminating '\0' are considered.
 */
utf8_string slice_utf8_string(utf8_string ustr, size_t byte_index, size_t byte_len);

/**
 * @brief Creates an iterator for traversing UTF-8 characters within a string. (see next_utf8_char( .. ) for traversal)
 *
 * @param ustr The UTF-8 string to iterate over.
 * @return An iterator structure initialized to the start of the string.
 */
utf8_char_iter make_utf8_char_iter(utf8_string ustr);

/**
 * @brief Retrieves the next UTF-8 character from the iterator.
 *
 * @param iter Pointer to the UTF-8 character iterator.
 * @return The next UTF-8 character from the iterator.
 * @note If the iterator reaches the end, it keeps returning terminating '\0' of iter.str { .str = '\0', .byte_len = 0 }
 */
utf8_char next_utf8_char(utf8_char_iter* iter);

/**
 * @brief Retrieves the UTF-8 character at the specified character index within a UTF-8 string in O(n) time.
 *
 * @details The `nth_utf8_char` function returns the UTF-8 character located at the specified character index
 * within the given UTF-8 string. The character index is zero-based, indicating the position of
 * the character in the string. If the index is out of bounds or invalid, the function returns
 * { .str = NULL, .byte_len = 0 }
 *
 * @param ustr The UTF-8 string from which to retrieve the character.
 * @param char_index The zero-based index of the character to retrieve.
 * @return The UTF-8 character at the specified index within the string.
 *
 * @code
 * // Example usage:
 * utf8_string str = make_utf8_string("Hello Здравствуйте こんにちは");
 * utf8_char char_at_index = nth_utf8_char(str, 7);    // д
 * @endcode
 */
utf8_char nth_utf8_char(utf8_string ustr, size_t char_index);

/**
 * @brief Counts the number of UTF-8 characters in the given utf8_string.
 *
 * @param ustr The UTF-8 string whose characters are to be counted.
 * @return The total number of characters in the UTF-8 string.
 */
size_t utf8_char_count(utf8_string ustr);

/**
 * @brief Checks if a given byte is the start of a UTF-8 character. ('\0' is also a valid character boundary)
 *
 * @param str Pointer to the byte to check.
 * @return `true` if the byte is the start of a UTF-8 character; otherwise, `false`.
 */
bool is_utf8_char_boundary(const char* str);

/**
 * @brief Converts a UTF-8 character to its corresponding Unicode code point (which is the same as a UTF-32 value).
 *
 * @param uchar The UTF-8 character to convert.
 * @return The Unicode code point.
 */
uint32_t unicode_code_point(utf8_char uchar);

#endif
