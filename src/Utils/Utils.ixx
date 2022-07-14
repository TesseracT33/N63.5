export module Util;

import <array>;
import <concepts>;
import <random>;

export template<size_t bound>
constexpr std::array<size_t, bound> range = [] {
	static_assert(bound > 0);
	std::array<std::size_t, bound> arr{};
	for (size_t i = 0; i < arr.size(); ++i) {
		arr[i] = i;
	}
	return arr;
}();


export template<typename... T>
constexpr bool AlwaysFalse = false;


export template<typename T, size_t size>
constexpr std::array<T, size> MakeArray(const T& value)
{
	std::array<T, size> arr{};
	arr.fill(value);
	return arr;
}


std::random_device random_device;
std::default_random_engine random_engine(random_device());


export template<std::integral Int>
Int Random(Int lower_bound, Int upper_bound)
{
	std::uniform_int_distribution<Int> uniform_dist(lower_bound, upper_bound);
	return uniform_dist(random_engine);
}