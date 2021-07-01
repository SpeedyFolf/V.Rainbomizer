#pragma once

#include <deque>
#include <cstdint>
#include <string.h>
#include <string_view>
#include <type_traits>

namespace rage {

constexpr char
NormaliseChar (const char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');

    else if (c == '\\')
        return '/';

    return c;
}

constexpr std::uint32_t
atPartialStringHash (std::string_view key, std::uint32_t initialHash = 0)
{
    uint32_t hash = initialHash;
    for (auto c : key)
        {
            hash += NormaliseChar (c);
            hash += hash << 10;
            hash ^= hash >> 6;
        }
    return hash;
}

constexpr std::uint32_t
atLiteralStringHash (std::string_view key, std::uint32_t initialHash = 0)
{
    uint32_t hash = initialHash;
    for (auto c : key)
        {
            hash += c;
            hash += hash << 10;
            hash ^= hash >> 6;
        }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

constexpr std::uint32_t
atStringHash (std::string_view key, std::uint32_t initialHash = 0)
{
    std::uint32_t hash = atPartialStringHash (key, initialHash);
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash;
}

class float16
{
public:
    uint16_t value;

    float16 () : value (0){};
    float16 (uint16_t val) : value (val){};
    float16 (const float16 &other) : value (other.value){};
    float16 (float val) : value (from_float (val).value){};

    float
    to_float ()
    {
        uint32_t f = ((value & 0x8000) << 16)
                     | (((value & 0x7c00) + 0x1C000) << 13)
                     | ((value & 0x03FF) << 13);
        return *reinterpret_cast<float *> (&f);
    }

    inline static float16
    from_float (float f)
    {
        uint32_t x = *reinterpret_cast<uint32_t *> (&f);
        uint16_t h = static_cast<uint16_t> (
            ((x >> 16) & 0x8000)
            | ((((x & 0x7f800000) - 0x38000000) >> 13) & 0x7c00)
            | ((x >> 13) & 0x03ff));
        return h;
    }
};

static_assert (sizeof (float16) == 2, "size of half not 2 bytes");

template <int size> class bitset
{
    uint32_t m_nBits[size / 32];

public:
    bool
    operator[] (size_t pos) const
    {
        return m_nBits[pos / 32] & (1 << (pos % 32));
    }

    inline void
    Set (size_t pos, bool value)
    {
        uint32_t &bits = m_nBits[pos / 32];
        if (value)
            bits = bits | (1 << (pos % 32));
        else
            bits = bits & ~(1 << (pos % 32));
    }
};

class Vec2f
{
public:
    float x;
    float y;
};

class Vec2V
{
public:
    float x;
    float y;

private:
    float __pad[2];
};

class Vec3V
{
public:
    float x;
    float y;
    float z;

private:
    float __pad;
};

class Vec4V
{
public:
    float x;
    float y;
    float z;
    float w;
};

class Mat33V
{
public:
    Vec3V right;
    Vec3V up;
    Vec3V at;
};

class Mat34V
{
public:
    Vec3V right;
    Vec3V up;
    Vec3V at;
    Vec3V pos;
};

class Mat44V
{
public:
    Vec4V right;
    Vec4V up;
    Vec4V at;
    Vec4V pos;
};

} // namespace rage

/* Not a part of rage */
template <typename T, int capacity> class CyclicContainer
{
    std::deque<T> m_Internal{};
    bool          m_Full = false;

public:
    void
    Push (const T &value)
    {
        if (m_Full)
            {
                m_Internal.pop_front ();
            }
        m_Internal.push_back (value);
        m_Full = m_Internal.size () >= capacity;
    }

    inline const std::deque<T>
    Get () const
    {
        return m_Internal;
    }
};

constexpr std::uint32_t operator"" _joaat (char const *s, size_t len)
{
    return rage::atLiteralStringHash (std::string_view (s, len), 0);
}

#pragma pack(push, 1)

template <typename T> struct atArrayBase
{
    T Data;

    using ElemType = std::decay_t<decltype (Data[0])>;

    ElemType &
    operator[] (size_t ix)
    {
        return Data[ix];
    }

    const ElemType &
    operator[] (size_t ix) const
    {
        return Data[ix];
    }

    constexpr ElemType *
    begin ()
    {
        return &Data[0];
    }

    constexpr const ElemType *
    begin () const
    {
        return &(*this)[0];
    }
};

template <typename T> struct atArrayGetSizeWrapperObject
{
    T *Data;

    T &
    operator[] (size_t ix)
    {
        return *reinterpret_cast<T *> (reinterpret_cast<char *> (Data)
                                       + T::GetSize () * ix);
    }

    const T &
    operator[] (size_t ix) const
    {
        return *reinterpret_cast<T *> (reinterpret_cast<char *> (Data)
                                       + T::GetSize () * ix);
    }
};

template <typename T = void *>
struct atArrayGetSizeWrapper
    : public atArrayBase<atArrayGetSizeWrapperObject<T>>
{
    uint16_t Size;
    uint16_t Capacity;

    constexpr T *
    end ()
    {
        return &(*this)[Size];
    }

    constexpr const T *
    end () const
    {
        return &(*this)[Size];
    }
};

template <typename T = void *> struct atArray : public atArrayBase<T *>
{
    uint16_t Size;
    uint16_t Capacity;

    constexpr T *
    end ()
    {
        return &(*this)[Size];
    }

    constexpr const T *
    end () const
    {
        return &(*this)[Size];
    }
};

template <typename T, uint32_t Size>
struct atFixedArray : public atArrayBase<T[Size]>
{
    constexpr T *
    end ()
    {
        return &(*this)[Size];
    }

    constexpr const T *
    end () const
    {
        return &(*this)[Size];
    }
};

struct atString
{
    char *m_szString;
    short m_nLength;
    short m_nCapacity;

    atString () = default;

    atString (const char *str)
    {
        m_szString  = const_cast<char *> (str);
        m_nLength   = strlen (str);
        m_nCapacity = m_nLength;
    }
};
#pragma pack(pop)
