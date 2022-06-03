export module Utils;

import <array>;

export
{
	template<std::size_t bound>
	constexpr std::array<std::size_t, bound> range = []
	{
		static_assert(bound > 0);
		std::array<std::size_t, bound> arr{};
		for (std::size_t i = 0; i < arr.size(); ++i)
			arr[i] = i;
		return arr;
	}();


	template <typename... T>
	constexpr bool always_false = false;


	template<typename T, std::size_t size>
	constexpr std::array<T, size> make_array(const T& value)
	{
		std::array<T, size> arr{};
		arr.fill(value);
		return arr;
	}
}