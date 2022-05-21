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
}