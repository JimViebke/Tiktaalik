#pragma once

#include <limits>

namespace util
{
	template<typename T, typename tag>
		requires std::is_arithmetic<T>::value
	class strong_alias
	{
	public:
		using type = T;

		constexpr strong_alias() : _value{} {}

		template<typename other_t>
			requires std::is_arithmetic<other_t>::value
		constexpr strong_alias(const other_t& set_value) : _value{ T(set_value) } {}

		constexpr T& value() { return _value; }
		constexpr const T& value() const { return _value; }

		template <typename other_t>
			requires std::is_arithmetic<other_t>::value
		operator other_t() const { return other_t(value()); }

		constexpr bool operator==(const strong_alias& rhs) const { return value() == rhs.value(); }
		constexpr bool operator<(const auto& rhs) const { return value() < rhs.value(); }
		template<typename other_t>
			requires (std::is_arithmetic<other_t>::value)
		constexpr bool operator<(const other_t& rhs) const { return value() < T(rhs); }
		template<typename other_t>
			requires (std::is_arithmetic<other_t>::value)
		constexpr bool operator>(const other_t& rhs) const { return value() > T(rhs); }
		constexpr bool operator<=(const auto& rhs) const { return !(*this > rhs); }
		constexpr bool operator>=(const auto& rhs) const { return !(*this < rhs); }

		constexpr bool operator==(const auto& rhs) const { return value() == T(rhs); }

		constexpr auto& operator++() { ++value(); return *this; }
		constexpr auto operator++(int) { auto t = *this; operator++(); return t; }
		constexpr auto& operator--() { --value(); return *this; }
		constexpr auto operator--(int) { auto t = *this; operator--(); return t; }

		constexpr strong_alias operator-() const { return -value(); }
		constexpr strong_alias operator~() const { return ~value(); }

		template <typename other_t>
			requires std::is_arithmetic<other_t>::value
		constexpr strong_alias operator+(const other_t& rhs) const { return value() + rhs; }
		template <typename other_t>
			requires std::is_arithmetic<other_t>::value
		constexpr strong_alias operator-(const other_t& rhs) const { return value() - rhs; }
		template <typename other_t>
			requires std::is_arithmetic<other_t>::value
		constexpr strong_alias operator*(const other_t& rhs) const { return value() * rhs; }
		template <typename other_t>
			requires std::is_arithmetic<other_t>::value
		constexpr strong_alias operator/(const other_t& rhs) const { return value() / rhs; }
		template <typename other_t>
			requires std::is_arithmetic<other_t>::value
		constexpr strong_alias operator%(const other_t& rhs) const { return value() % rhs; }

		template <typename other_t> requires (std::is_arithmetic<other_t>::value)
			constexpr strong_alias& operator+=(const other_t& rhs) { value() += rhs; return *this; }
		template <typename other_t> requires (std::is_arithmetic<other_t>::value)
			constexpr strong_alias& operator-=(const other_t& rhs) { value() -= rhs; return *this; }
		template <typename other_t> requires (std::is_arithmetic<other_t>::value)
			constexpr strong_alias& operator*=(const other_t& rhs) { value() *= rhs; return *this; }
		template <typename other_t> requires (std::is_arithmetic<other_t>::value)
			constexpr strong_alias& operator/=(const other_t& rhs) { value() /= rhs; return *this; }
		template <typename other_t> requires (std::is_arithmetic<other_t>::value)
			constexpr strong_alias& operator%=(const other_t& rhs) { value() %= rhs; return *this; }

		template <typename other_t> requires (std::is_arithmetic<other_t>::value)
			constexpr strong_alias operator&(const other_t& rhs) const { return value() & rhs; }
		template <typename other_t> requires std::is_arithmetic<other_t>::value
			constexpr strong_alias operator|(const other_t& rhs) const { return value() | rhs; }
		template <typename other_t> requires std::is_arithmetic<other_t>::value
			constexpr strong_alias operator^(const other_t& rhs) const { return value() ^ rhs; }

		constexpr strong_alias& operator&=(const strong_alias& rhs) { value() &= rhs.value(); return *this; }
		constexpr strong_alias& operator|=(const strong_alias& rhs) { value() |= rhs.value(); return *this; }
		constexpr strong_alias& operator^=(const strong_alias& rhs) { value() ^= rhs.value(); return *this; }
		constexpr strong_alias& operator&=(const auto& rhs) { value() &= rhs; return *this; }
		constexpr strong_alias& operator|=(const auto& rhs) { value() |= rhs; return *this; }
		constexpr strong_alias& operator^=(const auto& rhs) { value() ^= rhs; return *this; }

	private:
		T _value;
	};

	template<typename other, typename T, typename tag>
	constexpr static bool operator>(const other& lhs, const strong_alias<T, tag>& rhs) { return lhs > rhs.value(); }

	template<typename other, typename T, typename tag>
	constexpr static other operator+(const other& lhs, const strong_alias<T, tag>& rhs) { return lhs + rhs.value(); }
	template<typename other, typename T, typename tag>
	constexpr static other operator-(const other& lhs, const strong_alias<T, tag>& rhs) { return lhs - rhs.value(); }
	template<typename other, typename T, typename tag>
	constexpr static other operator*(const other& lhs, const strong_alias<T, tag>& rhs) { return lhs * rhs.value(); }
	template<typename other, typename T, typename tag>
	constexpr static other operator/(const other& lhs, const strong_alias<T, tag>& rhs) { return lhs / rhs.value(); }
	template<typename other, typename T, typename tag>
	constexpr static other operator%(const other& lhs, const strong_alias<T, tag>& rhs) { return lhs % rhs.value(); }

	template<typename other, typename T, typename tag>
	constexpr static other& operator+=(other& lhs, const strong_alias<T, tag>& rhs) { lhs += rhs.value(); return lhs; }
	template<typename other, typename T, typename tag>
	constexpr static other& operator-=(other& lhs, const strong_alias<T, tag>& rhs) { lhs -= rhs.value(); return lhs; }
	template<typename other, typename T, typename tag>
	constexpr static other& operator*=(other& lhs, const strong_alias<T, tag>& rhs) { lhs *= rhs.value(); return lhs; }
	template<typename other, typename T, typename tag>
	constexpr static other& operator/=(other& lhs, const strong_alias<T, tag>& rhs) { lhs /= rhs.value(); return lhs; }
	template<typename other, typename T, typename tag>
	constexpr static other& operator%=(other& lhs, const strong_alias<T, tag>& rhs) { lhs %= rhs.value(); return lhs; }

	template<typename T, typename tag>
	constexpr static strong_alias<T, tag> operator&(const auto& lhs, const strong_alias<T, tag>& rhs) { return lhs & rhs.value(); }
	template<typename T, typename tag>
	constexpr static strong_alias<T, tag> operator^(const auto& lhs, const strong_alias<T, tag>& rhs) { return lhs ^ rhs.value(); }
	template<typename T, typename tag>
	constexpr static strong_alias<T, tag> operator|(const auto& lhs, const strong_alias<T, tag>& rhs) { return lhs | rhs.value(); }

	template <typename T, typename tag, typename other_t>
		requires std::is_arithmetic<other_t>::value
	constexpr T operator<<(const strong_alias<T, tag>& lhs, const other_t& rhs)
	{
		return lhs.value() << rhs;
	}
	template <typename other_t, typename T, typename tag>
	constexpr other_t operator<<(const other_t& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs << rhs.value();
	}
	template <typename other_t, typename T, typename tag>
	constexpr other_t& operator<<(other_t& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs << rhs.value();
	}

	template <typename T, typename tag, typename other_t>
		requires std::is_arithmetic<other_t>::value
	constexpr T operator>>(const strong_alias<T, tag>& lhs, const other_t& rhs)
	{
		return lhs.value() >> rhs;
	}
	template <typename other_t, typename T, typename tag>
	constexpr other_t operator>>(const other_t& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs >> rhs.value();
	}
	template <typename other_t, typename T, typename tag>
	constexpr other_t& operator>>(other_t& lhs, strong_alias<T, tag>& rhs)
	{
		lhs >> rhs.value();
		return lhs;
	}
}

/*
Per the C++20 standard:
"Unless explicitly prohibited, a program may add a template specialization for
any standard library class template to namespace std provided that
(a) the added declaration depends on at least one program-defined type and
(b) the specialization meets the standard library requirements for the original template."
*/
namespace std
{
	template<typename T, typename tag>
	class numeric_limits<util::strong_alias<T, tag>> : public numeric_limits<T> {};
}
