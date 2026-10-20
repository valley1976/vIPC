#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "string_view.h"

namespace valley {
namespace base {

//! String utilities
/*!
    String utilities contains methods for upper/lower case conversions, join/split strings
    and other useful string manipulation methods.

    Thread-safe.
*/
class String_utils
{
public:
    String_utils() = delete;
    String_utils(const String_utils&) = delete;
    String_utils(String_utils&&) = delete;
    ~String_utils() = delete;

    String_utils& operator=(const String_utils&) = delete;
    String_utils& operator=(String_utils&&) = delete;

    //! Is the given character blank (empty or contains only space characters)?
    /*!
        \param ch - Character to check
        \return 'true' if given character is blank, 'false' if given character is not blank
    */
    static bool is_blank(char ch);
    //! Is the given C-string blank (empty or contains only space characters)?
    /*!
        \param str - C-string to check
        \return 'true' if given C-string is blank, 'false' if given C-string is not blank
    */
    static bool is_blank(const char* str);
    //! Is the given string blank (empty or contains only space characters)?
    /*!
        \param str - String to check
        \return 'true' if given string is blank, 'false' if given string is not blank
    */
    static bool is_blank(String_view str);

    //! Is the given string match to the given patterns?
    /*!
        Patterns string contains one or more regular expressions separated by ';'.
        If the regular expression has '!' prefix it treats as 'not matching'.
        Examples:
            "Demo.*;Live.*" + "DemoAccount" -> true
            "Demo.*;Live.*" + "LiveAccount" -> true
            "Demo.*;Live.*" + "UnknownAccount" -> false
            "!Demo.*;!Live.*" + "DemoAccount" -> false
            "!Demo.*;!Live.*" + "LiveAccount" -> false
            "!Demo.*;!Live.*" + "UnknownAccount" -> true

        \param patterns - Patterns to match with
        \param str - String to match
        \return 'true' if given string matches, 'false' if given string does not match
    */
    static bool is_pattern_match(const std::string& patterns, const std::string& str);

    //! Convert the given character to lower case
    /*!
        \param ch - Character to convert
    */
    static char to_lower(char ch);
    //! Convert the given character to upper case
    /*!
        \param ch - Character to convert
    */
    static char to_upper(char ch);

    //! Convert the given constant string converted to lower case
    /*!
        \param str - String to convert
        \return String converted to lower case
    */
    static std::string to_lower(String_view str);
    //! Convert the the given constant string converted to upper case
    /*!
        \param str - String to convert
        \return String converted to upper case
    */
    static std::string to_upper(String_view str);

    //! Convert the given string to lower case
    /*!
        \param str - String to convert
        \return The same converted string
    */
    static std::string& lower(std::string& str);
    //! Convert the given string to upper case
    /*!
        \param str - String to convert
        \return The same converted string
    */
    static std::string& upper(std::string& str);

    //! Trims space characters from the start of the given constant string
    /*!
        \param str - String to trim
        \return Trimmed string
    */
    static std::string to_ltrim(String_view str);
    //! Trims space characters from the end of the given constant string
    /*!
        \param str - String to trim
        \return Trimmed string
    */
    static std::string to_rtrim(String_view str);
    //! Trims space characters from the both sides of the given constant string
    /*!
        \param str - String to trim
        \return Trimmed string
    */
    static std::string to_trim(String_view str);

    //! Trims space characters from the start of the given string
    /*!
        \param str - String to trim
        \return The same trimmed string
    */
    static std::string& ltrim(std::string& str);
    //! Trims space characters from the end of the given string
    /*!
        \param str - String to trim
        \return The same trimmed string
    */
    static std::string& rtrim(std::string& str);
    //! Trims space characters from the both sides of the given string
    /*!
        \param str - String to trim
        \return The same trimmed string
    */
    static std::string& trim(std::string& str);

    //! Remove blank characters from the given string
    /*!
        \param str - String to remove blank characters
        \return String with removed blank characters
    */
    static std::string remove_blank(String_view str);
    //! Remove blank characters from the given string
    /*!
        \param str - String to remove blank characters
        \return The same string with removed blank characters
    */
    static std::string& remove_blank(std::string& str);

    //! compare two strings case sensitive version
    /*!
        \param str1 - First string to compare
        \param str2 - Second string to compare
        \return 'true' if two strings are equal, 'false' if two strings are different
    */
    static bool compare(String_view str1, String_view str2);
    //! compare two strings case insensitive version
    /*!
        \param str1 - First string to compare
        \param str2 - Second string to compare
        \return 'true' if two strings are equal, 'false' if two strings are different
    */
    static bool compare_no_case(String_view str1, String_view str2);

    //! Is the given string contains the given character?
    /*!
        \param str - String to search in
        \param ch - Character to find
        \return 'true' if the substring was found, 'false' if the substring was not found
    */
    static bool contains(String_view str, const char ch);
    //! Is the given string contains the given C-string?
    /*!
        \param str - String to search in
        \param substr - Substring to find
        \return 'true' if the substring was found, 'false' if the substring was not found
    */
    static bool contains(String_view str, const char* substr);
    //! Is the given string contains the given substring?
    /*!
        \param str - String to search in
        \param substr - Substring to find
        \return 'true' if the substring was found, 'false' if the substring was not found
    */
    static bool contains(String_view str, String_view substr);

    //! Count all occurrences of substring
    /*!
        \param str - Modifying string
        \param substr - Substring to find
        \return Count of all substring occurrences
    */
    static size_t count_all(String_view str, String_view substr);

    //! Replace the first occurrence of substring with another substring
    /*!
        \param str - Modifying string
        \param substr - Substring to find
        \param with - Substring to replace
        \return 'true' if first occurrence of substring was found and relapsed, 'false' if the substring was not found
    */
    static bool replace_first(std::string& str, String_view substr, String_view with);
    //! Replace the last occurrence of substring with another substring
    /*!
        \param str - Modifying string
        \param substr - Substring to find
        \param with - Substring to replace
        \return 'true' if last occurrence of substring was found and relapsed, 'false' if the substring was not found
    */
    static bool replace_last(std::string& str, String_view substr, String_view with);
    //! Replace all occurrences of substring with another substring
    /*!
        \param str - Modifying string
        \param substr - Substring to find
        \param with - Substring to replace
        \return 'true' if all occurrences of substring were found and relapsed, 'false' if the substring was not found
    */
    static bool replace_all(std::string& str, String_view substr, String_view with);

    //! Checks the given string for specific prefix
    /*!
        \param str - String to check
        \param prefix - Prefix to check
        \return 'true' if the given string starts with the specific prefix, 'false' if the given string does not start with the specific prefix
    */
    static bool starts_with(String_view str, String_view prefix);
    //! Checks the given string for specific suffix
    /*!
        \param str - String to check
        \param suffix - Suffix to check
        \return 'true' if the given string ends with the specific suffix, 'false' if the given string does not end with the specific suffix
    */
    static bool ends_with(String_view str, String_view suffix);

    //! split the string into tokens by the given delimiter character
    /*!
        \param str - String to split
        \param delimiter - Delimiter character
        \param skip_empty - Skip empty substrings flag (default is false)
        \return Vector of tokens
    */
    static std::vector<std::string> split(String_view str, char delimiter, bool skip_empty = false);
    //! split the string into tokens by the given delimiter string
    /*!
        \param str - String to split
        \param delimiter - Delimiter string
        \param skip_empty - Skip empty substrings flag (default is false)
        \return Vector of tokens
    */
    static std::vector<std::string> split(String_view str, String_view delimiter, bool skip_empty = false);
    //! split the string into tokens by the any character in the given delimiter string
    /*!
        \param str - String to split
        \param delimiters - Delimiters string
        \param skip_empty - Skip empty substrings flag (default is false)
        \return Vector of string tokens
    */
    static std::vector<std::string> split_by_any(String_view str, String_view delimiters, bool skip_empty = false);

    //! join tokens into the string
    /*!
        \param tokens - Vector of string tokens
        \param skip_empty - Skip empty tokens flag (default is false)
        \param skip_blank - Skip blank tokens flag (default is false)
        \return joined string
    */
    static std::string join(const std::vector<std::string>& tokens, bool skip_empty = false, bool skip_blank = false);
    //! join tokens into the string with delimiter character
    /*!
        \param tokens - Vector of string tokens
        \param delimiter - Delimiter character
        \param skip_empty - Skip empty tokens flag (default is false)
        \param skip_blank - Skip blank tokens flag (default is false)
        \return joined string
    */
    static std::string join(const std::vector<std::string>& tokens, char delimiter, bool skip_empty = false, bool skip_blank = false);
    //! join tokens into the string with delimiter C-string
    /*!
        \param tokens - Vector of string tokens
        \param delimiter - Delimiter C-string
        \param skip_empty - Skip empty tokens flag (default is false)
        \param skip_blank - Skip blank tokens flag (default is false)
        \return joined string
    */
    static std::string join(const std::vector<std::string>& tokens, const char* delimiter, bool skip_empty = false, bool skip_blank = false);
    //! join tokens into the string with delimiter string
    /*!
        \param tokens - Vector of string tokens
        \param delimiter - Delimiter string
        \param skip_empty - Skip empty tokens flag (default is false)
        \param skip_blank - Skip blank tokens flag (default is false)
        \return joined string
    */
    static std::string join(const std::vector<std::string>& tokens, String_view delimiter, bool skip_empty = false, bool skip_blank = false);

    //! Converts arbitrary datatypes into string using std::ostringstream
    /*!
        \param value - Value to convert
        \return Result converted string
    */
    template <typename T>
    static std::string to_string(const T& value);
    //! Converts strings to arbitrary datatypes using std::istringstream
    /*!
        \param str - String converted into the value
        \return Result converted value
    */
    template <typename T>
    static T from_string(String_view str);

private:
    static bool is_blank_internal(char ch);
    static char to_lower_internal(char ch);
    static char to_upper_internal(char ch);
};

/*! \example string_utils.cpp String utilities example */

}
}

#include "string_utils.inl"
