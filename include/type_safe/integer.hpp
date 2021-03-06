// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_INTEGER_HPP_INCLUDED
#define TYPE_SAFE_INTEGER_HPP_INCLUDED

#include <iosfwd>
#include <limits>
#include <type_traits>

#include <type_safe/detail/assert.hpp>
#include <type_safe/detail/force_inline.hpp>
#include <type_safe/arithmetic_policy.hpp>

namespace type_safe
{
    template <typename IntegerT, class Policy = arithmetic_policy_default>
    class integer;

    /// \exclude
    namespace detail
    {
        template <typename T>
        struct is_integer : std::integral_constant<bool, std::is_integral<T>::value
                                                             && !std::is_same<T, bool>::value
                                                             && !std::is_same<T, char>::value>
        {
        };

        template <typename From, typename To>
        struct is_safe_integer_conversion
            : std::integral_constant<bool, detail::is_integer<From>::value
                                               && detail::is_integer<To>::value
                                               && sizeof(From) <= sizeof(To)
                                               && std::is_signed<From>::value
                                                      == std::is_signed<To>::value>
        {
        };

        template <typename From, typename To>
        using enable_safe_integer_conversion =
            typename std::enable_if<is_safe_integer_conversion<From, To>::value>::type;

        template <typename From, typename To>
        using fallback_safe_integer_conversion =
            typename std::enable_if<!is_safe_integer_conversion<From, To>::value>::type;

        template <typename A, typename B>
        struct is_safe_integer_comparision
            : std::integral_constant<bool, is_safe_integer_conversion<A, B>::value
                                               || is_safe_integer_conversion<B, A>::value>
        {
        };

        template <typename A, typename B>
        using enable_safe_integer_comparision =
            typename std::enable_if<is_safe_integer_comparision<A, B>::value>::type;

        template <typename A, typename B>
        using fallback_safe_integer_comparision =
            typename std::enable_if<!is_safe_integer_comparision<A, B>::value>::type;

        template <typename A, typename B>
        struct is_safe_integer_operation
            : std::integral_constant<bool,
                                     detail::is_integer<A>::value && detail::is_integer<B>::value
                                         && std::is_signed<A>::value == std::is_signed<B>::value>
        {
        };

        template <typename A, typename B>
        struct integer_result_type
            : std::enable_if<is_safe_integer_operation<A, B>::value,
                             typename std::conditional<sizeof(A) < sizeof(B), B, A>::type>
        {
        };

        template <typename A, typename B>
        using integer_result_t = typename integer_result_type<A, B>::type;

        template <typename A, typename B>
        using fallback_integer_result =
            typename std::enable_if<!is_safe_integer_operation<A, B>::value>::type;
    } // namespace detail

    /// A type safe integer class.
    ///
    /// This is a tiny, no overhead wrapper over a standard integer type.
    /// It behaves exactly like the built-in types except that narrowing conversions are not allowed.
    /// It also checks against `unsigned` under/overflow in debug mode
    /// and marks it as undefined for the optimizer otherwise.
    ///
    /// A conversion is considered safe if both integer types have the same signedness
    /// and the size of the value being converted is less than or equal to the destination size.
    ///
    /// \requires `IntegerT` must be an integral type except `bool` and `char` (use `signed char`/`unsigned char`).
    /// \notes It intentionally does not provide the bitwise operations.
    template <typename IntegerT, class Policy /* = arithmetic_policy_default*/>
    class integer
    {
        static_assert(detail::is_integer<IntegerT>::value, "must be a real integer type");

    public:
        using integer_type = IntegerT;

        //=== constructors ===//
        integer() = delete;

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE constexpr integer(const T& val) : value_(val)
        {
        }

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE constexpr integer(const integer<T, Policy>& val)
        : value_(static_cast<T>(val))
        {
        }

        template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>
        constexpr integer(T) = delete;

        //=== assignment ===//
        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator=(const T& val)
        {
            value_ = val;
            return *this;
        }

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator=(const integer<T, Policy>& val)
        {
            value_ = static_cast<T>(val);
            return *this;
        }

        template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>
        integer& operator=(T) = delete;

        //=== conversion back ===//
        TYPE_SAFE_FORCE_INLINE explicit constexpr operator integer_type() const
        {
            return value_;
        }

        //=== unary operators ===//
        TYPE_SAFE_FORCE_INLINE constexpr integer operator+() const
        {
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE constexpr integer operator-() const
        {
            static_assert(std::is_signed<integer_type>::value,
                          "cannot call unary minus on unsigned integer");
            return -value_;
        }

        TYPE_SAFE_FORCE_INLINE integer& operator++()
        {
            value_ = Policy::template do_addition(value_, integer_type(1));
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE integer operator++(int)
        {
            auto res = *this;
            ++*this;
            return res;
        }

        TYPE_SAFE_FORCE_INLINE integer& operator--()
        {
            value_ = Policy::template do_subtraction(value_, integer_type(1));
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE integer operator--(int)
        {
            auto res = *this;
            --*this;
            return res;
        }

//=== compound assignment ====//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>      \
    TYPE_SAFE_FORCE_INLINE integer& operator Op(const T& other)                                    \
    {                                                                                              \
        return *this Op integer<T, Policy>(other);                                                 \
    }                                                                                              \
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>    \
    integer& operator Op(integer<T, Policy>) = delete;                                             \
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>    \
    integer& operator Op(T) = delete;

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator+=(const integer<T, Policy>& other)
        {
            value_ = Policy::template do_addition(value_, static_cast<T>(other));
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(+=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator-=(const integer<T, Policy>& other)
        {
            value_ = Policy::template do_subtraction(value_, static_cast<T>(other));
            return *this;
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(-=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator*=(const integer<T, Policy>& other)
        {
            value_ = Policy::template do_multiplication(value_, static_cast<T>(other));
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(*=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator/=(const integer<T, Policy>& other)
        {
            value_ = Policy::template do_division(value_, static_cast<T>(other));
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(/=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator%=(const integer<T, Policy>& other)
        {
            value_ = Policy::template do_modulo(value_, static_cast<T>(other));
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(%=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

    private:
        integer_type value_;
    };

    //=== operations ===//
    /// \exclude
    namespace detail
    {
        template <typename T>
        struct make_signed;

        template <typename T, class Policy>
        struct make_signed<integer<T, Policy>>
        {
            using type = integer<typename std::make_signed<T>::type, Policy>;
        };

        template <typename T>
        struct make_unsigned;

        template <typename T, class Policy>
        struct make_unsigned<integer<T, Policy>>
        {
            using type = integer<typename std::make_unsigned<T>::type, Policy>;
        };
    } // namespace detail

    /// [std::make_signed]() for [type_safe::integer]().
    template <class Integer>
    using make_signed_t = typename detail::make_signed<Integer>::type;

    /// \returns A new [type_safe::integer]() of the corresponding signed integer type.
    /// \requires The value of `i` must fit into signed type.
    template <typename Integer, class Policy>
    TYPE_SAFE_FORCE_INLINE constexpr make_signed_t<integer<Integer, Policy>> make_signed(
        const integer<Integer, Policy>& i)
    {
        using result_type = typename std::make_signed<Integer>::type;
        return i <= static_cast<Integer>(std::numeric_limits<result_type>::max()) ?
                   integer<result_type, Policy>(static_cast<result_type>(static_cast<Integer>(i))) :
                   (DEBUG_UNREACHABLE(detail::assert_handler{}, "conversion "
                                                                "would "
                                                                "overflow"),
                    result_type());
    }

    /// [std::make_unsigned]() for [type_safe::integer]().
    template <class Integer>
    using make_unsigned_t = typename detail::make_unsigned<Integer>::type;

    /// \returns A new [type_safe::integer]() of the corresponding unsigned integer type.
    /// \requires The value of `i` must not be negative.
    template <typename Integer, class Policy>
    TYPE_SAFE_FORCE_INLINE constexpr make_unsigned_t<integer<Integer, Policy>> make_unsigned(
        const integer<Integer, Policy>& i)
    {
        using result_type = typename std::make_unsigned<Integer>::type;
        return i >= Integer(0) ?
                   integer<result_type, Policy>(static_cast<result_type>(static_cast<Integer>(i))) :
                   (DEBUG_UNREACHABLE(detail::assert_handler{}, "conversion would underflow"),
                    result_type(0));
    }

    /// \returns The absolute value of an [type_safe::integer]().
    /// \unique_name type_safe::abs-signed
    template <typename SignedInteger, class Policy,
              typename = typename std::enable_if<std::is_signed<SignedInteger>::value>::type>
    TYPE_SAFE_FORCE_INLINE constexpr make_unsigned_t<integer<SignedInteger, Policy>> abs(
        const integer<SignedInteger, Policy>& i)
    {
        return make_unsigned(i > 0 ? i : -i);
    }

    /// \returns `i` unchanged.
    /// \notes This is an optimization of [type_safe::abs-signed]() for `unsigned` [type_safe::integer]().
    /// \unique_name type_safe::abs-unsigned
    template <typename UnsignedInteger, class Policy,
              typename = typename std::enable_if<std::is_unsigned<UnsignedInteger>::value>::type>
    TYPE_SAFE_FORCE_INLINE constexpr integer<UnsignedInteger, Policy> abs(
        const integer<UnsignedInteger, Policy>& i)
    {
        return i;
    }

//=== comparision ===//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::enable_safe_integer_conversion<A, B>>                             \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const A& a, const integer<B, Policy>& b)     \
    {                                                                                              \
        return integer<A, Policy>(a) Op b;                                                         \
    }                                                                                              \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::enable_safe_integer_conversion<A, B>>                             \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const integer<A, Policy>& a, const B& b)     \
    {                                                                                              \
        return a Op integer<B, Policy>(b);                                                         \
    }                                                                                              \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::fallback_safe_integer_comparision<A, B>>                          \
    constexpr bool operator Op(integer<A, Policy>, integer<B, Policy>) = delete;                   \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::fallback_safe_integer_comparision<A, B>>                          \
    constexpr bool operator Op(A, integer<B, Policy>) = delete;                                    \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::fallback_safe_integer_comparision<A, B>>                          \
    constexpr bool operator Op(integer<A, Policy>, B) = delete;

    template <typename A, typename B, class Policy,
              typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const integer<A, Policy>& a,
                                                     const integer<B, Policy>& b)
    {
        return static_cast<A>(a) == static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(==)

    template <typename A, typename B, class Policy,
              typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const integer<A, Policy>& a,
                                                     const integer<B, Policy>& b)
    {
        return static_cast<A>(a) != static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(!=)

    template <typename A, typename B, class Policy,
              typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator<(const integer<A, Policy>& a,
                                                    const integer<B, Policy>& b)
    {
        return static_cast<A>(a) < static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(<)

    template <typename A, typename B, class Policy,
              typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator<=(const integer<A, Policy>& a,
                                                     const integer<B, Policy>& b)
    {
        return static_cast<A>(a) <= static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(<=)

    template <typename A, typename B, class Policy,
              typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator>(const integer<A, Policy>& a,
                                                    const integer<B, Policy>& b)
    {
        return static_cast<A>(a) > static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(>)

    template <typename A, typename B, class Policy,
              typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator>=(const integer<A, Policy>& a,
                                                     const integer<B, Policy>& b)
    {
        return static_cast<A>(a) >= static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

//=== binary operations ===//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename A, typename B, class Policy>                                                \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const A& a, const integer<B, Policy>& b)     \
        ->integer<detail::integer_result_t<A, B>, Policy>                                          \
    {                                                                                              \
        return integer<A, Policy>(a) Op b;                                                         \
    }                                                                                              \
    template <typename A, class Policy, typename B>                                                \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const integer<A, Policy>& a, const B& b)     \
        ->integer<detail::integer_result_t<A, B>, Policy>                                          \
    {                                                                                              \
        return a Op integer<B, Policy>(b);                                                         \
    }                                                                                              \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::fallback_integer_result<A, B>>                                    \
    constexpr int operator Op(integer<A, Policy>, integer<B, Policy>) = delete;                    \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::fallback_integer_result<A, B>>                                    \
    constexpr int operator Op(A, integer<B, Policy>) = delete;                                     \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::fallback_integer_result<A, B>>                                    \
    constexpr int operator Op(integer<A, Policy>, B) = delete;

    template <typename A, typename B, class Policy>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator+(const integer<A, Policy>& a,
                                                    const integer<B, Policy>& b)
        -> integer<detail::integer_result_t<A, B>, Policy>
    {
        using type = detail::integer_result_t<A, B>;
        return Policy::template do_addition<type>(static_cast<A>(a), static_cast<B>(b));
    }
    TYPE_SAFE_DETAIL_MAKE_OP(+)

    template <typename A, typename B, class Policy>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator-(const integer<A, Policy>& a,
                                                    const integer<B, Policy>& b)
        -> integer<detail::integer_result_t<A, B>, Policy>
    {
        using type = detail::integer_result_t<A, B>;
        return Policy::template do_subtraction<type>(static_cast<A>(a), static_cast<B>(b));
    }
    TYPE_SAFE_DETAIL_MAKE_OP(-)

    template <typename A, typename B, class Policy>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator*(const integer<A, Policy>& a,
                                                    const integer<B, Policy>& b)
        -> integer<detail::integer_result_t<A, B>, Policy>
    {
        using type = detail::integer_result_t<A, B>;
        return Policy::template do_multiplication<type>(static_cast<A>(a), static_cast<B>(b));
    }
    TYPE_SAFE_DETAIL_MAKE_OP(*)

    template <typename A, typename B, class Policy>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator/(const integer<A, Policy>& a,
                                                    const integer<B, Policy>& b)
        -> integer<detail::integer_result_t<A, B>, Policy>
    {
        using type = detail::integer_result_t<A, B>;
        return Policy::template do_division<type>(static_cast<A>(a), static_cast<B>(b));
    }
    TYPE_SAFE_DETAIL_MAKE_OP(/)

    template <typename A, typename B, class Policy>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator%(const integer<A, Policy>& a,
                                                    const integer<B, Policy>& b)
        -> integer<detail::integer_result_t<A, B>, Policy>
    {
        using type = detail::integer_result_t<A, B>;
        return Policy::template do_modulo<type>(static_cast<A>(a), static_cast<B>(b));
    }
    TYPE_SAFE_DETAIL_MAKE_OP(%)

#undef TYPE_SAFE_DETAIL_MAKE_OP

    //=== input/output ===/
    template <typename Char, class CharTraits, typename IntegerT, class Policy>
    std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& in,
                                                     integer<IntegerT, Policy>& i)
    {
        IntegerT val;
        in >> val;
        i = val;
        return in;
    }

    template <typename Char, class CharTraits, typename IntegerT, class Policy>
    std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& out,
                                                     const integer<IntegerT, Policy>& i)
    {
        return out << static_cast<IntegerT>(i);
    }
} // namespace type_safe

#endif // TYPE_SAFE_INTEGER_HPP_INCLUDED
