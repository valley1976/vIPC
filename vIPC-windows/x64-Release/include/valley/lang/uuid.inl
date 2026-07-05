/*!
    \file uuid.inl
    \brief Universally unique identifier (UUID) inline implementation
    \author Ivan Shynkarenka
    \date 18.08.2016
    \copyright MIT License
*/

namespace valley {
namespace lang {

//! @cond INTERNALS
namespace internals {

inline /*constexpr*/ uint8_t unhex(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return ch - '0';
    else if ((ch >= 'a') && (ch <= 'f'))
        return 10 + ch - 'a';
    else if ((ch >= 'A') && (ch <= 'F'))
        return 10 + ch - 'A';
    else
        throw std::runtime_error("Invalid UUID character");
}

} // namespace Internals
//! @endcond

inline /*constexpr*/ UUID::UUID(const char* uuid, size_t size) : _data{}
{
    char v1 = 0;
    char v2 = 0;
    bool pack = false;
    size_t index = 0;

    // Parse UUID string
    for (size_t i = 0; i < size; ++i)
    {
        char ch = uuid[i];
        if ((ch == '-') || (ch == '{') || (ch == '}'))
            continue;

        if (pack)
        {
            v2 = ch;
            pack = false;
            uint8_t ui1 = internals::unhex(v1);
            uint8_t ui2 = internals::unhex(v2);
            if ((ui1 > 15) || (ui2 > 15))
                throw std::invalid_argument("Invalid UUID literal");
            _data[index++] = ui1 * 16 + ui2;
            if (index >= 16)
                break;
        }
        else
        {
            v1 = ch;
            pack = true;
        }
    }

    // Fill remaining data with zeros
    for (; index < 16; ++index)
        _data[index++] = 0;
}

inline void UUID::swap(UUID& uuid) noexcept
{
    using std::swap;
    swap(_data, uuid._data);
}

inline void swap(UUID& uuid1, UUID& uuid2) noexcept
{
    uuid1.swap(uuid2);
}

} // namespace lang
} // namespace valley


//! \cond DOXYGEN_SKIP
template <>
struct std::hash<valley::lang::UUID>
{
    typedef valley::lang::UUID argument_type;
    typedef size_t result_type;

    result_type operator() (const argument_type& value) const
    {
        result_type result = 17;
        std::hash<uint8_t> hasher;
        for (size_t i = 0; i < value.data().size(); ++i)
            result = result * 31 + hasher(value.data()[i]);
        return result;
    }
};
//! \endcond
