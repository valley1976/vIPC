#pragma once

#include <cstdint>
#include <string>

#include "String_view.h"

namespace valley {
namespace base {

//! Encoding utilities
/*!
    Encoding utilities contains methods for UTF-8, UTF-16, UTF-32 encoding conversions.

    Thread-safe.
*/
class Encoding
{
public:
    Encoding() = delete;
    Encoding(const Encoding&) = delete;
    Encoding(Encoding&&) = delete;
    ~Encoding() = delete;

    Encoding& operator=(const Encoding&) = delete;
    Encoding& operator=(Encoding&&) = delete;

    //! Convert system wide-string to UTF-8 encoded string
    /*!
        System wide-string could be UTF-16 (Windows) or UTF-32 (Unix).

        \param wstr - System wide-string to convert
        \return UTF-8 encoded string
    */
    static std::string to_utf8(WString_view wstr);

    //! Convert UTF-8 encoded string to system wide-string
    /*!
        System wide-string could be UTF-16 (Windows) or UTF-32 (Unix).

        \param str - UTF-8 encoded string to convert
        \return System wide-string
    */
    static std::wstring from_utf8(String_view str);

    //! Convert UTF-8 encoded string to UTF-16 encoded string
    /*!
        \param str - UTF-8 encoded string to convert
        \return UTF-16 encoded string
    */
    static std::u16string utf8_to_utf16(String_view str);
    //! Convert UTF-8 encoded string to UTF-32 encoded string
    /*!
        \param str - UTF-8 encoded string to convert
        \return UTF-32 encoded string
    */
    static std::u32string utf8_to_utf32(String_view str);

    //! Convert UTF-16 encoded string to UTF-8 encoded string
    /*!
        \param str - UTF-16 encoded string to convert
        \return UTF-8 encoded string
    */
    static std::string utf16_to_utf8(U16String_view str);
    //! Convert UTF-16 encoded string to UTF-32 encoded string
    /*!
        \param str - UTF-16 encoded string to convert
        \return UTF-32 encoded string
    */
    static std::u32string utf16_to_utf32(U16String_view str);

    //! Convert UTF-32 encoded string to UTF-8 encoded string
    /*!
        \param str - UTF-32 encoded string to convert
        \return UTF-8 encoded string
    */
    static std::string utf32_to_utf8(U32String_view str);
    //! Convert UTF-32 encoded string to UTF-16 encoded string
    /*!
        \param str - UTF-32 encoded string to convert
        \return UTF-16 encoded string
    */
    static std::u16string utf32_to_utf16(U32String_view str);

    //! Base16 encode string
    /*!
        \param str - String to encode
        \return Base16 encoded string
    */
    static std::string base16_encode(String_view str);
    //! Base16 decode string
    /*!
        \param str - Base16 encoded string
        \return Decoded string
    */
    static std::string base16_decode(String_view str);

    //! Base32 encode string
    /*!
        \param str - String to encode
        \return Base32 encoded string
    */
    static std::string base32_encode(String_view str);
    //! Base32 decode string
    /*!
        \param str - Base32 encoded string
        \return Decoded string
    */
    static std::string base32_decode(String_view str);

    //! Base64 encode string
    /*!
        \param str - String to encode
        \return Base64 encoded string
    */
    static std::string base64_encode(String_view str);
    //! Base64 decode string
    /*!
        \param str - Base64 encoded string
        \return Decoded string
    */
    static std::string base64_decode(String_view str);

    //! URL encode string
    /*!
        \param str - String to encode
        \return URL encoded string
    */
    static std::string url_encode(String_view str);
    //! URL decode string
    /*!
        \param str - URL encoded string
        \return Decoded string
    */
    static std::string url_decode(String_view str);
};

} 
} 

