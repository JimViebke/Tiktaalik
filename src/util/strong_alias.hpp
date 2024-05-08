#pragma once

#include <numeric>

namespace util
{
	template<typename T, typename tag>
		requires std::is_arithmetic<T>::value
	class strong_alias
	{
	public:
		using type = T;

		constexpr strong_alias() : _value{} {}

		template<typename other>
			requires std::is_arithmetic<other>::value
		constexpr strong_alias(const other& set_value) : _value{ T(set_value) } {}

		constexpr T& value() { return _value; }
		constexpr const T& value() const { return _value; }

		template <typename other>
			requires std::is_arithmetic<other>::value
		operator explicit other() const { return other(_value); }

		constexpr bool operator<(const auto& rhs) const { return _value < rhs._value; }
		template<typename other>
			requires (std::is_arithmetic<other>::value)
		constexpr bool operator<(const other& rhs) const { return _value < rhs; }
		constexpr bool operator>(const auto& rhs) const { return rhs < value(); }
		constexpr bool operator<=(const auto& rhs) const { return !(*this > rhs); }
		constexpr bool operator>=(const auto& rhs) const { return !(*this < rhs); }

		constexpr auto& operator++() { ++_value; return *this; }
		constexpr auto operator++(int) { auto t = *this; operator++(); return t; }
		constexpr auto& operator--() { --_value; return *this; }
		constexpr auto operator--(int) { auto t = *this; operator--(); return t; }

		constexpr auto operator-() const { return -_value; }

		constexpr auto& operator+=(const auto& rhs) { _value += rhs.value(); return *this; }
		constexpr auto& operator-=(const auto& rhs) { _value -= rhs.value(); return *this; }
		constexpr auto& operator*=(const auto& rhs) { _value *= rhs.value(); return *this; }
		constexpr auto& operator/=(const auto& rhs) { _value /= rhs.value(); return *this; }

		constexpr bool operator==(const strong_alias& rhs) const { return value() == rhs.value(); }

		constexpr strong_alias operator+(const strong_alias& rhs) const { return value() + rhs.value(); }
		constexpr strong_alias operator-(const strong_alias& rhs) const { return value() - rhs.value(); }
		constexpr strong_alias operator*(const strong_alias& rhs) const { return value() * rhs.value(); }
		constexpr strong_alias operator/(const strong_alias& rhs) const { return value() / rhs.value(); }

	private:
		T _value;
	};

	template <typename ostream_t, typename T, typename tag>
	constexpr ostream_t& operator>>(ostream_t& os, strong_alias<T, tag>& rhs)
	{
		os >> rhs.value();
		return os;
	}

	template <typename ostream_t, typename T, typename tag>
	constexpr ostream_t& operator<<(ostream_t& os, const strong_alias<T, tag>& rhs)
	{
		os << rhs.value();
		return os;
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
