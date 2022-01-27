export module Cartridge;

import NumericalTypes;

import <fstream>;
import <string>;
import <vector>;

export
class Cartridge
{
public:
	Cartridge() = default;
	Cartridge(const std::string& rom_path);

	bool load_rom(const std::string& rom_path);
	u64 read(const u32 addr);

private:
	std::vector<u8> rom{};
};