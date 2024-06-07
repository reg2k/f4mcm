#pragma once
#include <locale>

namespace StringUtil
{
	struct StringEqualTo
	{
		bool operator()(const std::string& v1, const std::string& v2) const
		{
			return v1 == v2;
		}
	};
	struct StringEqualToNoCase
	{
		bool operator()(const std::string& v1, const std::string& v2) const
		{
			std::locale locale;
			return std::equal(v1.begin(), v1.end(), v2.begin(), v2.end(), [&locale](char c1, char c2)
			{
				return std::tolower(c1, locale) == std::tolower(c2, locale);
			});
		}
	};

	struct StringHash
	{
		size_t operator()(const std::string& value) const
		{
			return std::hash<std::string>()(value);
		}
	};
	struct StringHashNoCase
	{
		// From Boost
		template<class T> static void hash_combine(size_t& seed, const T& v)
		{
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
		}

		size_t operator()(const std::string& value) const
		{
			std::locale locale;
			size_t hashValue = 0;
			for (char c: value)
			{
				hash_combine(hashValue, std::tolower(c, locale));
			}
			return hashValue;
		}
	};
}

namespace StringUtil
{
	template<class TValue> using UnorderedMapA = std::unordered_map<std::string, TValue, StringHash, StringEqualTo>;
	template<class TValue> using UnorderedMapANoCase = std::unordered_map<std::string, TValue, StringHashNoCase, StringEqualToNoCase>;
}
