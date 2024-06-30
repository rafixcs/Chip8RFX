#pragma once

#include <string>
#include <array>

class ROMReader
{
public:
	static std::streamsize Read(const std::string& path, std::array<uint8_t, 4096>& ram);
};

