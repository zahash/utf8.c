#include "utf8.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    bool valid;
    size_t next_offset;
} utf8_char_validity;

utf8_char_validity validate_utf8_char(const char* str, size_t offset) {
    // Single-byte UTF-8 characters have the form 0xxxxxxx
    if (((uint8_t)str[offset] & 0b10000000) == 0b00000000)
        return (utf8_char_validity) { .valid = true, .next_offset = offset + 1 };

    // Two-byte UTF-8 characters have the form 110xxxxx 10xxxxxx
    if (((uint8_t)str[offset + 0] & 0b11100000) == 0b11000000 &&
        ((uint8_t)str[offset + 1] & 0b11000000) == 0b10000000) {

        // Check for overlong encoding
        // 0(xxxxxxx)
        // 0(1111111)
        // 110(xxxxx) 10(xxxxxx)
        // 110(00001) 10(111111)
        // 110(00010) 10(000000)
        if (((uint8_t)str[offset] & 0b00011111) < 0b00000010)
            return (utf8_char_validity) { .valid = false, .next_offset = offset };

        return (utf8_char_validity) { .valid = true, .next_offset = offset + 2 };
    }

    // Three-byte UTF-8 characters have the form 1110xxxx 10xxxxxx 10xxxxxx
    if (((uint8_t)str[offset + 0] & 0b11110000) == 0b11100000 &&
        ((uint8_t)str[offset + 1] & 0b11000000) == 0b10000000 &&
        ((uint8_t)str[offset + 2] & 0b11000000) == 0b10000000) {

        // Check for overlong encoding
        // 110(xxxxx) 10(xxxxxx)
        // 110(11111) 10(111111)
        // 1110(xxxx) 10(xxxxxx) 10(xxxxxx)
        // 1110(0000) 10(011111) 10(111111)
        // 1110(0000) 10(100000) 10(000000)
        if (((uint8_t)str[offset + 0] & 0b00001111) == 0b00000000 &&
            ((uint8_t)str[offset + 1] & 0b00111111) < 0b00100000)
            return (utf8_char_validity) { .valid = false, .next_offset = offset };

        // Reject UTF-16 surrogates
        // U+D800 to U+DFFF
        // 1110(1101) 10(100000) 10(000000) ED A0 80 to 1110(1101) 10(111111) 10(111111) ED BF BF
        if ((uint8_t)str[offset + 0] == 0b11101101 &&
            (uint8_t)str[offset + 1] >= 0b10100000 &&
            (uint8_t)str[offset + 1] <= 0b10111111)
            return (utf8_char_validity) { .valid = false, .next_offset = offset };

        return (utf8_char_validity) { .valid = true, .next_offset = offset + 3 };
    }

    // Four-byte UTF-8 characters have the form 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    if (((uint8_t)str[offset + 0] & 0b11111000) == 0b11110000 &&
        ((uint8_t)str[offset + 1] & 0b11000000) == 0b10000000 &&
        ((uint8_t)str[offset + 2] & 0b11000000) == 0b10000000 &&
        ((uint8_t)str[offset + 3] & 0b11000000) == 0b10000000) {

        // Check for overlong encoding
        // 1110(xxxx) 10(xxxxxx) 10(xxxxxx)
        // 1110(1111) 10(111111) 10(111111)
        // 11110(xxx) 10(xxxxxx) 10(xxxxxx) 10(xxxxxx)
        // 11110(000) 10(001111) 10(111111) 10(111111)
        // 11110(000) 10(010000) 10(000000) 10(000000)
        if (((uint8_t)str[offset + 0] & 0b00000111) == 0b00000000 &&
            ((uint8_t)str[offset + 1] & 0b00111111) < 0b00010000)
            return (utf8_char_validity) { .valid = false, .next_offset = offset };

        return (utf8_char_validity) { .valid = true, .next_offset = offset + 4 };
    }

    return (utf8_char_validity) { .valid = false, .next_offset = offset };
}

utf8_validity validate_utf8(const char* str) {
    if (str == NULL) return (utf8_validity) { .valid = false, .valid_upto = 0 };

    size_t offset = 0;
    utf8_char_validity char_validity;

    while (str[offset] != '\0') {
        char_validity = validate_utf8_char(str, offset);
        if (char_validity.valid) offset = char_validity.next_offset;
        else return (utf8_validity) { .valid = false, .valid_upto = offset };
    }

    return (utf8_validity) { .valid = true, .valid_upto = offset };
}

utf8_string make_utf8_string(const char* str) {
    utf8_validity validity = validate_utf8(str);
    if (validity.valid) return (utf8_string) { .str = str, .byte_len = validity.valid_upto };
    return (utf8_string) { .str = NULL, .byte_len = 0 };
}

owned_utf8_string make_utf8_string_lossy(const char* str) {
    if (str == NULL) return (owned_utf8_string) { .str = NULL, .byte_len = 0 };

    size_t len = strlen(str);

    // Worst case scenario: every byte is invalid and is replaced with 3 bytes for U+FFFD
    size_t worst_case_size = len * 3 + 1;

    // Allocate buffer for the lossy UTF-8 string
    char* buffer = (char*)malloc(worst_case_size);
    if (!buffer) return (owned_utf8_string) { .str = NULL, .byte_len = 0 }; // failed allocation

    size_t buffer_offset = 0;
    size_t offset = 0;
    utf8_char_validity char_validity;

    while (offset < len) {
        char_validity = validate_utf8_char(str, offset);

        if (char_validity.valid) {
            // Copy valid UTF-8 character sequence to the buffer
            size_t char_len = char_validity.next_offset - offset;
            memcpy(buffer + buffer_offset, str + offset, char_len);
            buffer_offset += char_len;
            offset = char_validity.next_offset;
        } else {
            // Insert the UTF-8 bytes for U+FFFD (�)
            // FFFD = 1111111111111101
            //      = (1111) (111111) (111101)
            //      = 1110(1111) 10(111111) 10(111101)
            //      = EF BF BD
            buffer[buffer_offset++] = 0xEF;
            buffer[buffer_offset++] = 0xBF;
            buffer[buffer_offset++] = 0xBD;
            offset++;
        }
    }

    buffer[buffer_offset] = '\0';

    return (owned_utf8_string) { .str = buffer, .byte_len = buffer_offset };
}

utf8_string as_utf8_string(const owned_utf8_string* owned_str) {
    return (utf8_string) { .str = owned_str->str, .byte_len = owned_str->byte_len };
}

void free_owned_utf8_string(owned_utf8_string* owned_str) {
    if (owned_str->str) {
        free(owned_str->str);
        owned_str->str = NULL;
        owned_str->byte_len = 0;
    }
}

utf8_char_iter make_utf8_char_iter(utf8_string ustr) {
    return (utf8_char_iter) { .str = ustr.str, .terminator = &ustr.str[ustr.byte_len] };
}

bool is_utf8_char_boundary(const char* str) {
    return (uint8_t)*str <= 0b01111111 || (uint8_t)*str >= 0b11000000;
}

utf8_string slice_utf8_string(utf8_string ustr, size_t start_byte_index, size_t byte_len) {
    if (start_byte_index > ustr.byte_len) start_byte_index = ustr.byte_len;

    size_t excl_end_byte_index = start_byte_index + byte_len;
    if (excl_end_byte_index > ustr.byte_len) excl_end_byte_index = ustr.byte_len;

    if (is_utf8_char_boundary(ustr.str + start_byte_index) && is_utf8_char_boundary(ustr.str + excl_end_byte_index))
        return (utf8_string) { .str = ustr.str + start_byte_index, .byte_len = excl_end_byte_index - start_byte_index };

    return (utf8_string) { .str = NULL, .byte_len = 0 };
}

utf8_char next_utf8_char(utf8_char_iter* iter) {
    if (iter->str == iter->terminator) return (utf8_char) { .str = iter->str, .byte_len = 0 };

    // iter->str is at the current char's starting byte (char boundary).
    const char* curr_boundary = iter->str;

    iter->str++;
    uint8_t byte_len = 1;

    // find the next char's starting byte (next char boundary) and set the iter->str to that.
    while (!is_utf8_char_boundary(iter->str)) {
        iter->str++;
        byte_len++;
    }

    return (utf8_char) { .str = curr_boundary, .byte_len = byte_len };
}

utf8_char nth_utf8_char(utf8_string ustr, size_t char_index) {
    utf8_char_iter iter = make_utf8_char_iter(ustr);

    utf8_char ch;
    while ((ch = next_utf8_char(&iter)).byte_len != 0 && char_index-- != 0) {}

    if (ch.byte_len == 0) return (utf8_char) { .str = NULL, .byte_len = 0 };
    return ch;
}

size_t utf8_char_count(utf8_string ustr) {
    utf8_char_iter iter = make_utf8_char_iter(ustr);

    size_t count = 0;
    while (next_utf8_char(&iter).byte_len > 0) count++;
    return count;
}

uint32_t unicode_code_point(utf8_char uchar) {
    switch (uchar.byte_len) {
    case 1: return uchar.str[0] & 0b01111111;
    case 2: return
        (uchar.str[0] & 0b00011111) << 6 |
        (uchar.str[1] & 0b00111111);
    case 3: return
        (uchar.str[0] & 0b00001111) << 12 |
        (uchar.str[1] & 0b00111111) << 6 |
        (uchar.str[2] & 0b00111111);
    case 4: return
        (uchar.str[0] & 0b00000111) << 18 |
        (uchar.str[1] & 0b00111111) << 12 |
        (uchar.str[2] & 0b00111111) << 6 |
        (uchar.str[3] & 0b00111111);
    }

    return 0; // unreachable
}
