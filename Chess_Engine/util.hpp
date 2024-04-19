#pragma once

#include <chrono>

namespace chess::util
{
	using timepoint = std::chrono::milliseconds::rep;
	timepoint time_in_ms();

	template<typename T, typename tag>
	class strong_alias
	{
	public:
		constexpr strong_alias() : _value{} {}

		constexpr strong_alias(T set_value) : _value{ set_value } {}

		template<typename other_type> // suppress narrowing conversion warning
			requires std::is_integral<other_type>::value
		constexpr strong_alias(other_type set_value) : _value(T(set_value)) {}

		T value() const { return _value; }

		constexpr bool operator< (const auto& rhs) const { return _value < rhs._value; }
		template<typename T2>
			requires (std::is_integral<T2>::value)
		constexpr bool operator< (const T2& rhs) const { return _value < rhs; }

		constexpr bool operator> (const auto& rhs) const { return rhs < *this; }
		constexpr bool operator<=(const auto& rhs) const { return !(*this > rhs); }
		constexpr bool operator>=(const auto& rhs) const { return !(*this < rhs); }

		auto& operator++() { ++_value; return *this; }
		auto operator++(int) { auto t = *this; operator++(); return t; }
		auto& operator--() { --_value; return *this; }
		auto operator--(int) { auto t = *this; operator--(); return t; }

	private:
		T _value;

		constexpr friend bool operator==(const strong_alias& lhs, const strong_alias& rhs)
		{
			return lhs._value == rhs._value;
		}

		constexpr friend strong_alias operator+(const strong_alias& lhs, const strong_alias& rhs)
		{
			return lhs._value + rhs._value;
		}
		constexpr friend strong_alias operator-(const strong_alias& lhs, const strong_alias& rhs)
		{
			return lhs._value - rhs._value;
		}
		constexpr friend strong_alias operator*(const strong_alias& lhs, const strong_alias& rhs)
		{
			return lhs._value * rhs._value;
		}
		constexpr friend strong_alias operator/(const strong_alias& lhs, const strong_alias& rhs)
		{
			return lhs._value / rhs._value;
		}
	};

}
