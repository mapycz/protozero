#ifndef PROTOZERO_ITERATORS_HPP
#define PROTOZERO_ITERATORS_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

#include <cstring>
#include <iterator>
#include <utility>

#include <protozero/config.hpp>
#include <protozero/varint.hpp>

#if PROTOZERO_BYTE_ORDER != PROTOZERO_LITTLE_ENDIAN
# include <protozero/byteswap.hpp>
#endif

namespace protozero {

namespace detail {

    // Copy N bytes from src to dest on little endian machines, on big
    // endian swap the bytes in the process.
    template <int N>
    inline void copy_or_byteswap(const char* src, void* dest) noexcept {
#if PROTOZERO_BYTE_ORDER == PROTOZERO_LITTLE_ENDIAN
        std::memcpy(dest, src, N);
#else
        byteswap<N>(src, reinterpret_cast<char*>(dest));
#endif
    }

} // end namespace detail

/**
 * A range of iterators based on std::pair. Created from beginning and
 * end iterators. Used as a return type from some pbf_reader methods
 * that is easy to use with range-based for loops.
 */
template <typename T, typename P = std::pair<T, T>>
class iterator_range :
#ifdef PROTOZERO_STRICT_API
    protected
#else
    public
#endif
        P {

public:

    /// The type of the iterators in this range.
    using iterator = T;

    /**
     * Default constructor. Create empty iterator_range.
     */
    constexpr iterator_range() :
        P(iterator{}, iterator{}) {
    }

    /**
     * Create iterator range from two iterators.
     *
     * @param first Iterator to beginning or range.
     * @param last Iterator to end or range.
     */
    constexpr iterator_range(iterator&& first, iterator&& last) :
        P(std::forward<iterator>(first),
          std::forward<iterator>(last)) {
    }

    /// Return iterator to beginning of range.
    constexpr iterator begin() const noexcept {
        return this->first;
    }

    /// Return iterator to end of range.
    constexpr iterator end() const noexcept {
        return this->second;
    }

    /// Return iterator to beginning of range.
    constexpr iterator cbegin() const noexcept {
        return this->first;
    }

    /// Return iterator to end of range.
    constexpr iterator cend() const noexcept {
        return this->second;
    }

    /// Return true if this range is empty.
    constexpr std::size_t empty() const noexcept {
        return begin() == end();
    }

}; // struct iterator_range

#ifdef PROTOZERO_USE_BARE_POINTER_FOR_PACKED_FIXED

template <typename T>
using const_fixed_iterator = const T*;

template <typename T>
inline iterator_range<const_fixed_iterator<T>> create_fixed_iterator_range(const char* first, const char* last) {
    return iterator_range<const_fixed_iterator<T>>{reinterpret_cast<const T*>(first),
                                                   reinterpret_cast<const T*>(last)};
}

#else

template <typename T>
class const_fixed_iterator {

    const char* m_data;
    const char* m_end;

public:

    using iterator_category = std::forward_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;

    const_fixed_iterator() noexcept :
        m_data(nullptr),
        m_end(nullptr) {
    }

    const_fixed_iterator(const char* data, const char* end) noexcept :
        m_data(data),
        m_end(end) {
    }

    const_fixed_iterator(const const_fixed_iterator&) noexcept = default;
    const_fixed_iterator(const_fixed_iterator&&) noexcept = default;

    const_fixed_iterator& operator=(const const_fixed_iterator&) noexcept = default;
    const_fixed_iterator& operator=(const_fixed_iterator&&) noexcept = default;

    ~const_fixed_iterator() noexcept = default;

    value_type operator*() {
        value_type result;
        detail::copy_or_byteswap<sizeof(value_type)>(m_data , &result);
        return result;
    }

    const_fixed_iterator& operator++() {
        m_data += sizeof(value_type);
        return *this;
    }

    const_fixed_iterator operator++(int) {
        const const_fixed_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(const const_fixed_iterator& rhs) const noexcept {
        return m_data == rhs.m_data && m_end == rhs.m_end;
    }

    bool operator!=(const const_fixed_iterator& rhs) const noexcept {
        return !(*this == rhs);
    }

}; // class const_fixed_iterator

template <typename T>
inline iterator_range<const_fixed_iterator<T>> create_fixed_iterator_range(const char* first, const char* last) {
    return iterator_range<const_fixed_iterator<T>>{const_fixed_iterator<T>(first, last),
                                                   const_fixed_iterator<T>(last, last)};
}

#endif

template <typename T>
class const_varint_iterator {

protected:

    const char* m_data;
    const char* m_end;

public:

    using iterator_category = std::forward_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;

    const_varint_iterator() noexcept :
        m_data(nullptr),
        m_end(nullptr) {
    }

    const_varint_iterator(const char* data, const char* end) noexcept :
        m_data(data),
        m_end(end) {
    }

    const_varint_iterator(const const_varint_iterator&) noexcept = default;
    const_varint_iterator(const_varint_iterator&&) noexcept = default;

    const_varint_iterator& operator=(const const_varint_iterator&) noexcept = default;
    const_varint_iterator& operator=(const_varint_iterator&&) noexcept = default;

    ~const_varint_iterator() noexcept = default;

    value_type operator*() {
        const char* d = m_data; // will be thrown away
        return static_cast<value_type>(decode_varint(&d, m_end));
    }

    const_varint_iterator& operator++() {
        skip_varint(&m_data, m_end);
        return *this;
    }

    const_varint_iterator operator++(int) {
        const const_varint_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(const const_varint_iterator& rhs) const noexcept {
        return m_data == rhs.m_data && m_end == rhs.m_end;
    }

    bool operator!=(const const_varint_iterator& rhs) const noexcept {
        return !(*this == rhs);
    }

}; // class const_varint_iterator

template <typename T>
class const_svarint_iterator : public const_varint_iterator<T> {

public:

    using iterator_category = std::forward_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;

    const_svarint_iterator() noexcept :
        const_varint_iterator<T>() {
    }

    const_svarint_iterator(const char* data, const char* end) noexcept :
        const_varint_iterator<T>(data, end) {
    }

    const_svarint_iterator(const const_svarint_iterator&) = default;
    const_svarint_iterator(const_svarint_iterator&&) = default;

    const_svarint_iterator& operator=(const const_svarint_iterator&) = default;
    const_svarint_iterator& operator=(const_svarint_iterator&&) = default;

    ~const_svarint_iterator() = default;

    value_type operator*() {
        const char* d = this->m_data; // will be thrown away
        return static_cast<value_type>(decode_zigzag64(decode_varint(&d, this->m_end)));
    }

    const_svarint_iterator& operator++() {
        skip_varint(&this->m_data, this->m_end);
        return *this;
    }

    const_svarint_iterator operator++(int) {
        const const_svarint_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

}; // class const_svarint_iterator

} // end namespace protozero

#endif // PROTOZERO_ITERATORS_HPP
