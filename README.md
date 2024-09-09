<div align="center">

<pre>
██╗   ██╗████████╗███████╗ █████╗     ██████╗
██║   ██║╚══██╔══╝██╔════╝██╔══██╗   ██╔════╝
██║   ██║   ██║   █████╗  ╚█████╔╝   ██║     
██║   ██║   ██║   ██╔══╝  ██╔══██╗   ██║     
╚██████╔╝   ██║   ██║     ╚█████╔╝██╗╚██████╗
 ╚═════╝    ╚═╝   ╚═╝      ╚════╝ ╚═╝ ╚═════╝
---------------------------------------------
simple C library for working with UTF-8 encoded strings
</pre>

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

</div>

## 🧑‍💻 Usage example

```c
#include "utf8.h"
#include <stdio.h>

int main() {
    const char* str = "Hello, こんにちは, Здравствуйте";
    utf8_string ustr = make_utf8_string(str);
    utf8_string_slice slice = make_utf8_string_slice(ustr, 2, 11);
    utf8_char_iter iter = make_utf8_char_iter(ustr);

    printf("string: %s\n", ustr.str);
    printf("slice: %.*s\n", (int)slice.byte_len, slice.str);

    utf8_char ch;
    while ((ch = next_utf8_char(&iter)).byte_len > 0) {
        printf("character: %.*s\t", (int)ch.byte_len, ch.str);
        printf("unicode code point: U+%04X\n", unicode_code_point(ch));
    }

    return 0;
}
```

## 🌟 Connect with Us

M. Zahash – zahash.z@gmail.com

Distributed under the MIT license. See `LICENSE` for more information.

[https://github.com/zahash/](https://github.com/zahash/)

## 🤝 Contribute to utf8.c!

1. Fork it (<https://github.com/zahash/utf8.c/fork>)
2. Create your feature branch (`git checkout -b feature/fooBar`)
3. Commit your changes (`git commit -am 'Add some fooBar'`)
4. Push to the branch (`git push origin feature/fooBar`)
5. Create a new Pull Request

## ❤️ Show Some Love!

If you find this helpful and enjoy using it, consider giving it a [⭐ on GitHub!](https://github.com/zahash/utf8.c/stargazers) Your star is a gesture of appreciation and encouragement for the continuous improvement of this library.
