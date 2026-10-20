

namespace valley {
namespace base {

inline bool String_utils::is_blank_internal(char ch)
{
    return std::isspace(ch);
}

inline bool String_utils::is_blank(char ch)
{
    return is_blank_internal(ch);
}

inline char String_utils::to_lower_internal(char ch)
{
    return (char)std::tolower(ch);
}

inline char String_utils::to_lower(char ch)
{
    return to_lower_internal(ch);
}

inline char String_utils::to_upper_internal(char ch)
{
    return (char)std::toupper(ch);
}

inline char String_utils::to_upper(char ch)
{
    return to_upper_internal(ch);
}

inline std::string String_utils::to_lower(String_view str)
{
    std::string result(str.data(), str.size());
    lower(result);
    return result;
}

inline std::string String_utils::to_upper(String_view str)
{
    std::string result(str.data(), str.size());
    upper(result);
    return result;
}

inline std::string& String_utils::lower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), to_lower_internal);
    return str;
}

inline std::string& String_utils::upper(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), to_upper_internal);
    return str;
}

inline std::string& String_utils::trim(std::string& str)
{
    return ltrim(rtrim(str));
}

inline std::string String_utils::remove_blank(String_view str)
{
    std::string result(str.data(), str.size());
    remove_blank(result);
    return result;
}

inline std::string& String_utils::remove_blank(std::string& str)
{
    str.erase(std::remove_if(str.begin(), str.end(), is_blank_internal), str.end());
    return str;
}

inline bool String_utils::contains(String_view str, const char ch)
{
    return (str.find(ch) != std::string::npos);
}

inline bool String_utils::contains(String_view str, const char* substr)
{
    return (str.find(substr) != std::string::npos);
}

inline bool String_utils::contains(String_view str, String_view substr)
{
    return (str.find(substr) != std::string::npos);
}

inline bool String_utils::starts_with(String_view str, String_view prefix)
{
    return (str.size() >= prefix.size()) && (str.compare(0, prefix.size(), prefix) == 0);
}

inline bool String_utils::ends_with(String_view str, String_view suffix)
{
    return (str.size() >= suffix.size()) && (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
}

template <typename T>
inline std::string String_utils::to_string(const T& value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

template <>
inline std::string String_utils::to_string(const int8_t& value)
{
    std::ostringstream ss;
    ss << (int32_t)value;
    return ss.str();
}

template <>
inline std::string String_utils::to_string(const uint8_t& value)
{
    std::ostringstream ss;
    ss << (uint32_t)value;
    return ss.str();
}

template <typename T>
inline T String_utils::from_string(String_view str)
{
    T result;
    std::istringstream(std::string(str)) >> result;
    return result;
}

template <>
inline int8_t String_utils::from_string(String_view str)
{
    int32_t result;
    std::istringstream(std::string(str.data(), str.size())) >> result;
    return (int8_t)result;
}

template <>
inline uint8_t String_utils::from_string(String_view str)
{
    uint32_t result;
    std::istringstream(std::string(str.data(), str.size())) >> result;
    return (uint8_t)result;
}

template <>
inline std::string String_utils::from_string(String_view str)
{
    return std::string(str.data(), str.size());
}

template <>
inline String_view String_utils::from_string(String_view str)
{
    return str;
}

template <>
bool String_utils::from_string(String_view str);

} 
}
