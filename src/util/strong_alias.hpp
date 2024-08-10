#pragma once

#include <format>
#include <limits>

namespace util
{
	template <typename T, typename tag>
	    requires std::is_arithmetic<T>::value
	class strong_alias
	{
	public:
		using type = T;

		constexpr strong_alias() : _value{} {}

		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias(const other& set_value) : _value{T(set_value)}
		{
		}

		constexpr T& value() { return _value; }
		constexpr const T& value() const { return _value; }

		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr operator other() const
		{
			return other(value());
		}

		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr bool operator==(const other& rhs) const
		{
			return value() == rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr bool operator<(const other& rhs) const
		{
			return value() < rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr bool operator>(const other& rhs) const
		{
			return value() > rhs;
		}

		constexpr bool operator<=(const auto& rhs) const { return !(*this > rhs); }
		constexpr bool operator>=(const auto& rhs) const { return !(*this < rhs); }

		constexpr auto& operator++()
		{
			++value();
			return *this;
		}
		constexpr auto operator++(int)
		{
			auto t = *this;
			operator++();
			return t;
		}
		constexpr auto& operator--()
		{
			--value();
			return *this;
		}
		constexpr auto operator--(int)
		{
			auto t = *this;
			operator--();
			return t;
		}

		constexpr strong_alias operator-() const { return -value(); }
		constexpr strong_alias operator~() const { return ~value(); }

		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator+(const other& rhs) const
		{
			return value() + rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator-(const other& rhs) const
		{
			return value() - rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator*(const other& rhs) const
		{
			return value() * rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator/(const other& rhs) const
		{
			return value() / rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator%(const other& rhs) const
		{
			return value() % rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator&(const other& rhs) const
		{
			return value() & rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator|(const other& rhs) const
		{
			return value() | rhs;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias operator^(const other& rhs) const
		{
			return value() ^ rhs;
		}

		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator+=(const other& rhs)
		{
			value() += rhs;
			return *this;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator-=(const other& rhs)
		{
			value() -= rhs;
			return *this;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator*=(const other& rhs)
		{
			value() *= rhs;
			return *this;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator/=(const other& rhs)
		{
			value() /= rhs;
			return *this;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator%=(const other& rhs)
		{
			value() %= rhs;
			return *this;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator&=(const other& rhs)
		{
			value() &= rhs;
			return *this;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator|=(const other& rhs)
		{
			value() |= rhs;
			return *this;
		}
		template <typename other>
		    requires std::is_arithmetic<other>::value
		constexpr strong_alias& operator^=(const other& rhs)
		{
			value() ^= rhs;
			return *this;
		}

	private:
		T _value;
	};

	template <typename T, typename tag>
	constexpr static bool operator==(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs == rhs.value();
	}
	template <typename T, typename tag>
	constexpr static bool operator<(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs < rhs.value();
	}
	template <typename T, typename tag>
	constexpr static bool operator>(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs > rhs.value();
	}

	template <typename T, typename tag>
	constexpr static auto operator+(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs + rhs.value();
	}
	template <typename T, typename tag>
	constexpr static auto operator-(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs - rhs.value();
	}
	template <typename T, typename tag>
	constexpr static auto operator*(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs * rhs.value();
	}
	template <typename T, typename tag>
	constexpr static auto operator/(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs / rhs.value();
	}
	template <typename T, typename tag>
	constexpr static auto operator%(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs % rhs.value();
	}
	template <typename T, typename tag>
	constexpr static auto operator&(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs & rhs.value();
	}
	template <typename T, typename tag>
	constexpr static auto operator^(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs ^ rhs.value();
	}
	template <typename T, typename tag>
	constexpr static auto operator|(const auto& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs | rhs.value();
	}

	template <typename T, typename tag>
	constexpr static auto& operator+=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs += rhs.value();
		return lhs;
	}
	template <typename T, typename tag>
	constexpr static auto& operator-=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs -= rhs.value();
		return lhs;
	}
	template <typename T, typename tag>
	constexpr static auto& operator*=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs *= rhs.value();
		return lhs;
	}
	template <typename T, typename tag>
	constexpr static auto& operator/=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs /= rhs.value();
		return lhs;
	}
	template <typename T, typename tag>
	constexpr static auto& operator%=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs %= rhs.value();
		return lhs;
	}
	template <typename T, typename tag>
	constexpr static auto& operator&=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs &= rhs.value();
		return lhs;
	}
	template <typename T, typename tag>
	constexpr static auto& operator|=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs |= rhs.value();
		return lhs;
	}
	template <typename T, typename tag>
	constexpr static auto& operator^=(auto& lhs, const strong_alias<T, tag>& rhs)
	{
		lhs ^= rhs.value();
		return lhs;
	}

	template <typename T, typename tag, typename other>
	    requires std::is_arithmetic<other>::value
	constexpr T operator<<(const strong_alias<T, tag>& lhs, const other& rhs)
	{
		return lhs.value() << rhs;
	}
	template <typename other, typename T, typename tag>
	constexpr other operator<<(const other& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs << rhs.value();
	}
	template <typename other, typename T, typename tag>
	constexpr other& operator<<(other& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs << rhs.value();
	}

	template <typename T, typename tag, typename other>
	    requires std::is_arithmetic<other>::value
	constexpr T operator>>(const strong_alias<T, tag>& lhs, const other& rhs)
	{
		return lhs.value() >> rhs;
	}
	template <typename other, typename T, typename tag>
	constexpr other operator>>(const other& lhs, const strong_alias<T, tag>& rhs)
	{
		return lhs >> rhs.value();
	}
	template <typename other, typename T, typename tag>
	constexpr other& operator>>(other& lhs, strong_alias<T, tag>& rhs)
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
	template <typename T, typename tag>
	class numeric_limits<util::strong_alias<T, tag>> : public numeric_limits<T>
	{
	};
	template <typename T, typename tag>
	class formatter<util::strong_alias<T, tag>> : public formatter<T>
	{
	};
}
