export module FileUtils;

import NumericalTypes;

import <array>;
import <fstream>;
import <iostream>;
import <optional>;
import <string>;
import <vector>;

namespace FileUtils
{
	export
	template<std::size_t size>
	std::optional<std::array<u8, size>> LoadBinaryFileArray(const auto& path)
	{
		/* Attempt to open the file */
		std::ifstream ifs{ path, std::ifstream::in | std::ifstream::binary };
		if (!ifs)
		{
			return {};
		}

		/* Test the file size */
		ifs.seekg(0, ifs.end);
		const std::size_t file_size = ifs.tellg();
		if (file_size != size)
		{
			return {};
		}

		/* Read the file */
		std::array<u8, size> arr;
		ifs.seekg(0, ifs.beg);
		ifs.read((char*)arr.data(), size);

		return arr;
	}

	
	export
	std::optional<std::vector<u8>> LoadBinaryFileVec(const auto& path)
	{
		std::ifstream ifs{ path, std::ifstream::in | std::ifstream::binary };
		if (!ifs)
		{
			return {};
		}

		std::vector<u8> vec;
		ifs.seekg(0, ifs.end);
		const std::size_t file_size = ifs.tellg();
		vec.resize(file_size);

		ifs.seekg(0, ifs.beg);
		ifs.read((char*)vec.data(), file_size);

		return vec;
	}
}