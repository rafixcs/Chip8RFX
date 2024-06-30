#include "ROMReader.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

std::string logError()
{
	int errorNumber = 0;
	char errorMessage[256];
	std::stringstream resultMessage;

	if (strerror_s(errorMessage, sizeof(errorMessage), errorNumber) == 0)
	{
		resultMessage << "[" << errorNumber << "]: " << errorMessage;
	}
	else
	{
		resultMessage << "Failed to retrieve error message\n";
	}

	return resultMessage.str();
}

std::streamsize ROMReader::Read(const std::string& path, std::array<uint8_t, 4096>& ram)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		std::cerr << "Failed to open ROM file " << logError() << ".\n";
		return 0;
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> romData(size);
	if (!file.read(romData.data(), size))
	{
		std::cerr << "Failed to read ROM file: " << logError() << ".\n";
		return 0;
	}

	file.close();

	for (int i=0; i<size; i++)
	{
		ram[i + 0x200] = romData[i];
	}

	return size;
}
