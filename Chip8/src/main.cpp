#include <iostream>
#include "Chip8RFX.h"
#include "ROMReader.h"
#include "Emulator.h"
#include <SDL.h>

int testProgramLoad() 
{
	// Clear the memory
	std::array<uint8_t, 4096> ram;
	for (int i = 0; i < ram.size(); i++)
	{
		ram[i] = 0x00;
	}

	std::streamsize size = ROMReader::Read("./src/ROM/IBM_Logo.ch8", ram);

	if (size == 0)
	{
		std::cerr << "ROM was not loaded properly\n";
		return 1;
	}

	std::cout << "Printing program memory:\n";
	uint16_t offset = 0x200;
	for (int i = 0; i < size; i+=2)
	{
		//std::cout << "ram[" << std::hex << i + offset << "]: " << std::hex << ram[i + offset] << "\n";
		printf("ram[%x + 2]: 0x%x%x\n", i + offset, ram[i + offset], ram[i + offset + 1]);
	}

	std::cout << "\n------------------------------------------------------------------\n";
}

int main(int argc, char** argv)
{
	Chip8RFX app;
	app.Run();
	//testProgramLoad();
	return 0;
}