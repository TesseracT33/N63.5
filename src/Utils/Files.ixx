export module Util.Files;

import NumericalTypes;

import <array>;
import <fstream>;
import <iostream>;
import <optional>;
import <string>;
import <vector>;

export namespace FileUtils
{
	template<size_t size>
	std::optional<std::array<u8, size>> LoadBinaryFileArray(const std::string& path) {
		std::ifstream ifs{ path, std::ifstream::in | std::ifstream::binary };
		if (!ifs) {
			return {};
		}
		/* Test the file size */
		ifs.seekg(0, ifs.end);
		if (ifs.tellg() != size) {
			return {};
		}
		/* Read the file */
		std::array<u8, size> arr;
		ifs.seekg(0, ifs.beg);
		ifs.read((char*)arr.data(), size);

		return arr;
	}

	
	std::optional<std::vector<u8>> LoadBinaryFileVec(const std::string& path)
	{
		std::ifstream ifs{ path, std::ifstream::in | std::ifstream::binary };
		if (!ifs) {
			return {};
		}

		std::vector<u8> vec;
		ifs.seekg(0, ifs.end);
		size_t size = ifs.tellg();
		vec.resize(size);

		ifs.seekg(0, ifs.beg);
		ifs.read((char*)vec.data(), size);

		return vec;
	}
}