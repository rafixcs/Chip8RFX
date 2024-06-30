#include "Chip8RFX.h"
#include "ROMReader.h"

#include <iostream>
#include <random>


//#define DEBUG_DISPLAY

/**
* TODO:
* - Implement the rest of the instructions
* - Implement the draw function
* - Add the 60Hz refresh rate
*/


Chip8RFX::Chip8RFX()
{
	this->Initialize();
}

Chip8RFX::~Chip8RFX()
{
	this->ClearSDL();
}

void Chip8RFX::Initialize()
{
	// Clear members
	for (int i = 0; i < this->mRam.size(); i++)
	{
		this->mRam[i] = 0x00;
	}

	this->mPc = 0x0000;
	this->mIndexRegister = 0x0000;
	this->mTempOpcode = 0x0000;

	// Store the font in the first 512 bytes
	std::array<uint8_t, 16 * 5> font = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	memcpy(&this->mRam, &font, 16 * 5 * sizeof(uint8_t));

	// Initialize the lookup instruction vector
	this->lookup[InstructionOp::NONE] = Instruction{ "Invalid", &Chip8RFX::Invalid };
	this->lookup[InstructionOp::CLEAR_SCREEN] = Instruction{ "ClearScreen", &Chip8RFX::ClearScreen };
	this->lookup[InstructionOp::SUBROUTINE_IN] = Instruction{ "EnterSubroutine", &Chip8RFX::JumpInSubroutine };
	this->lookup[InstructionOp::SUBROUTINE_OUT] = Instruction{ "ReturnSubroutine", &Chip8RFX::ReturnFromSubroutine };
	this->lookup[InstructionOp::JUMP] = Instruction{ "Jump", &Chip8RFX::Jump };
	this->lookup[InstructionOp::SKIP_CONDITIONALLY] = Instruction{ "SkipConditionally", &Chip8RFX::SkipConditionally };
	this->lookup[InstructionOp::SET_REG_VX] = Instruction{ "SetRegisterVx", &Chip8RFX::SetRegisterVX };
	this->lookup[InstructionOp::ADD_REG_VX] = Instruction{ "AddToRegisterVx", &Chip8RFX::AddRegisterVX };
	this->lookup[InstructionOp::SET_REG] = Instruction{ "AddVxByVy", &Chip8RFX::SetVxByVyValue };
	this->lookup[InstructionOp::REG_OR] = Instruction{ "SetVxOrVy", &Chip8RFX::SetVxOrVy };
	this->lookup[InstructionOp::REG_AND] = Instruction{ "SetVxAndVy", &Chip8RFX::SetVxAndVy };
	this->lookup[InstructionOp::REG_XOR] = Instruction{ "SetVxXorVy", &Chip8RFX::SetVxXorVy };
	this->lookup[InstructionOp::ADD_WITH_CARRY] = Instruction{ "SetVxPlusVy", &Chip8RFX::SetVxPlusVy };
	this->lookup[InstructionOp::SUBTRACT] = Instruction{ "Subtract", &Chip8RFX::Subtract };
	this->lookup[InstructionOp::SHIFT] = Instruction{ "Shift", &Chip8RFX::Shift };
	this->lookup[InstructionOp::SET_INDEX_REG] = Instruction{ "SetIndexRegister", &Chip8RFX::SetIndexRegister };
	this->lookup[InstructionOp::JUMP_OFFSET] = Instruction{ "JumpWithOffset", &Chip8RFX::JumpWithOffset };
	this->lookup[InstructionOp::RANDOM] = Instruction{ "Random", &Chip8RFX::Random };
	this->lookup[InstructionOp::DRAW_DISPLAY] = Instruction{ "DrawToDisplay", &Chip8RFX::DrawDisplay };
	this->lookup[InstructionOp::SKIP_IF_KEY] = Instruction{ "SkipIfKey", &Chip8RFX::SkipIfKey };
	this->lookup[InstructionOp::TIMERS] = Instruction{ "Timers", &Chip8RFX::Timers };
	this->lookup[InstructionOp::ADD_TO_INDEX] = Instruction{ "AddToIndex", &Chip8RFX::AddToIndex };
	this->lookup[InstructionOp::GET_KEY] = Instruction{ "GetKey", &Chip8RFX::GetKey };
	this->lookup[InstructionOp::FONT_CHARACTER] = Instruction{ "FontCharacter", &Chip8RFX::FontCharacter };
	this->lookup[InstructionOp::INDEX_BIN_DEC] = Instruction{ "IndexDecimal", &Chip8RFX::IndexDecimal };
	this->lookup[InstructionOp::STORE_AND_LOAD] = Instruction{ "StoreAndLoad", &Chip8RFX::StoreAndLoad };


	/**
	* Keyboard loayout
	* 1 2 3 C	->	1 2 3 4
	* 4 5 6 D	->	q w e r
	* 7 8 9 E	->	a s d f
	* A 0 B F	->	z x c v
	*/
	this->mKeyboard[SDLK_1] = Chip8Keys{ 0x01, false };
	this->mKeyboard[SDLK_2] = Chip8Keys{ 0x02, false };
	this->mKeyboard[SDLK_3] = Chip8Keys{ 0x03, false };
	this->mKeyboard[SDLK_4] = Chip8Keys{ 0x0C, false };
	this->mKeyboard[SDLK_q] = Chip8Keys{ 0x04, false };
	this->mKeyboard[SDLK_w] = Chip8Keys{ 0x05, false };
	this->mKeyboard[SDLK_e] = Chip8Keys{ 0x06, false };
	this->mKeyboard[SDLK_r] = Chip8Keys{ 0x0D, false };
	this->mKeyboard[SDLK_a] = Chip8Keys{ 0x07, false };
	this->mKeyboard[SDLK_s] = Chip8Keys{ 0x08, false };
	this->mKeyboard[SDLK_d] = Chip8Keys{ 0x09, false };
	this->mKeyboard[SDLK_f] = Chip8Keys{ 0x0E, false };
	this->mKeyboard[SDLK_z] = Chip8Keys{ 0x0A, false };
	this->mKeyboard[SDLK_x] = Chip8Keys{ 0x00, false };
	this->mKeyboard[SDLK_c] = Chip8Keys{ 0x0B, false };
	this->mKeyboard[SDLK_v] = Chip8Keys{ 0x0F, false };

	// We neeed to load the program into the memory
	std::streamsize size = ROMReader::Read("./src/ROM/IBM_Logo.ch8", this->mRam);
	if (size == 0)
	{
		// Need to handle error
		std::cerr << "Failed to load program\n";
		this->isFinished = true;
		return;
	}
	else
	{
		this->mPc = 0x200;
		this->isFinished = false;
		this->mStopPC = false;
	}

	this->StartSDL();
}

void Chip8RFX::StartSDL()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << "\n";
		return;
	}

	// We have a 64x32 pixels display
	// but we need to scale it to have a bigger screen
	this->mWindow = SDL_CreateWindow(
		"CHIP-8 Emulator",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		64 * SCALE, 32 * SCALE,
		SDL_WINDOW_SHOWN
	);

	if (this->mWindow == nullptr)
	{
		std::cerr << "Failed to create window: " << SDL_GetError() << "\n";
		SDL_Quit();
		return;
	}

	this->mRenderer = SDL_CreateRenderer(this->mWindow, -1, 0);

	if (this->mRenderer == nullptr)
	{
		std::cerr << "Failed to create window: " << SDL_GetError() << "\n";
		SDL_DestroyWindow(this->mWindow);
		SDL_Quit();
		return;
	}

	this->mTexture = SDL_CreateTexture(this->mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
}

void Chip8RFX::ClearSDL()
{
	SDL_DestroyWindow(this->mWindow);
	SDL_DestroyRenderer(this->mRenderer);
	SDL_Quit();
}

void Chip8RFX::Run()
{
	// While theres need to run the program it will be executed
	while (!this->isFinished)
	{
		int timeToWait = FRAME_TARGET_TIME - (SDL_GetTicks() - this->mTicksLastFrame);
		if (timeToWait > 0 && timeToWait <= FRAME_TARGET_TIME)
			SDL_Delay(timeToWait);

		this->mTicksLastFrame = SDL_GetTicks();

		this->HandleSDLEvents();
		if (!this->mStopPC)
		{
			if (this->mDelayTimerRegister > 0)
			{
				this->mDelayTimerRegister--;
			}
			if (this->mSoundTimerRegister > 0)
			{
				this->mSoundTimerRegister--;
			}

			InstructionOp instruction = this->FetchAndDecode();
			Execute(instruction);
		}
	}
}

void Chip8RFX::HandleSDLEvents()
{
	SDL_Event event;
	SDL_PollEvent(&event);

	switch (event.type)
	{
		case SDL_QUIT:
			this->isFinished = true;
			break;
		case SDL_KEYDOWN:
			if (this->mKeyboard.count(event.key.keysym.sym))
			{
				this->mKeyboard[event.key.keysym.sym].isPressed = true;
			}
			break;
		case SDL_KEYUP:
			if (this->mKeyboard.count(event.key.keysym.sym))
			{
				this->mKeyboard[event.key.keysym.sym].isPressed = false;
			}
			break;
	}
}

// Decode which instruction to be executed
Chip8RFX::InstructionOp Chip8RFX::FetchAndDecode()
{
	if (this->mPc >= mRam.size() || this->mPc + 1 >= this->mRam.size())
	{
		this->mStopPC = true;
		std::cout << "End of the program\n";
		return InstructionOp::NONE;
	}

	uint8_t hi = this->mRam[this->mPc];
	uint8_t low = this->mRam[this->mPc + 1];
	this->mTempOpcode = (hi << 8) | (low);

	uint8_t opcode = (hi & 0xf0) >> 4;
	InstructionOp instruction = InstructionOp::NONE;

	switch (opcode)
	{
		case 0:
			if (low == 0xEE)
			{
				instruction = InstructionOp::SUBROUTINE_OUT;
			}
			else if(low == 0xE0)
			{
				instruction = InstructionOp::CLEAR_SCREEN;
			}
			break;
		case 1:
			instruction = InstructionOp::JUMP;
			break;
		case 2:
			instruction = InstructionOp::SUBROUTINE_IN;
			break;
		case 3:
		case 4:
		case 5:
		case 9:
			instruction = InstructionOp::SKIP_CONDITIONALLY;
			break;
		case 6:
			instruction = InstructionOp::SET_REG_VX;
			break;
		case 7:
			instruction = InstructionOp::ADD_REG_VX;
			break;
		case 8:
			switch (low & 0x0F)
			{
			case 0:
				instruction = InstructionOp::SET_REG;
				break;
			case 1:
				instruction = InstructionOp::REG_OR;
				break;
			case 2:
				instruction = InstructionOp::REG_AND;
				break;
			case 3:
				instruction = InstructionOp::REG_XOR;
				break;
			case 4:
				instruction = InstructionOp::ADD_WITH_CARRY;
				break;
			case 5:
			case 7:
				instruction = InstructionOp::SUBTRACT;
				break;
			case 6:
			case 0xE:
				instruction = InstructionOp::SHIFT;
				break;
			default:
				instruction = InstructionOp::NONE;
				break;
			}
			break;
		case 0xA:
			instruction = InstructionOp::SET_INDEX_REG;
			break;
		case 0XB:
			instruction = InstructionOp::JUMP_OFFSET;
			break;
		case 0XC:
			instruction = InstructionOp::RANDOM;
			break;
		case 0xD:
			instruction = InstructionOp::DRAW_DISPLAY;
			break;
		case 0xE:
			if (low == 0x9E || low == 0xA1)
			{
				instruction = InstructionOp::SKIP_IF_KEY;
			}
			break;
		case 0xF:
			if (low == 0x07 || low == 0x15 || low == 0x18)
			{
				instruction = InstructionOp::TIMERS;
			}
			else if (low == 0x1E)
			{
				instruction = InstructionOp::ADD_TO_INDEX;
			}
			else if (low == 0xA)
			{
				instruction = InstructionOp::GET_KEY;
			}
			else if (low == 0x29)
			{
				instruction = InstructionOp::FONT_CHARACTER;
			}
			else if (low == 0x33)
			{
				instruction = InstructionOp::INDEX_BIN_DEC;
			}
			else if (low == 0x55 || low == 0x65)
			{
				instruction = InstructionOp::STORE_AND_LOAD;
			}
			break;
		default:
			instruction = InstructionOp::NONE;
			break;
	}

	return instruction;
}

// Execute the function
void Chip8RFX::Execute(const InstructionOp& instruction)
{
	(this->*lookup[instruction].operate)();
	this->mPc += 2;
}

// -----------------------------------------------------------------------
// Instructions functions BEGIN
// -----------------------------------------------------------------------

void Chip8RFX::Invalid()
{
	std::cout << "Invalid opcode: " << std::hex << this->mTempOpcode << "\n";
}


// Clear the screen turning all the pixels off to zero
void Chip8RFX::ClearScreen()
{
	for (int i = 0; i < this->mDisplay.size(); i++)
	{
		this->mDisplay[i] = 0X31363F;
	}
}

/**
* 00EE
* 
*/
void Chip8RFX::ReturnFromSubroutine()
{
	if (this->mStack.size() > 0)
	{
		this->mPc = this->mStack.top();
		this->mStack.pop();
	}
}

/**
* 2NNN
*/
void Chip8RFX::JumpInSubroutine()
{
	uint16_t address = this->mTempOpcode & 0x0FFF;
	this->mStack.push(this->mPc);
	this->mPc = address;
}


/**
* 1NNN
* NNN is the location in the memory to jump to
*/
void Chip8RFX::Jump()
{
	uint16_t jaddress = this->mTempOpcode & 0x0FFF;
	this->mPc = jaddress;
}

/**
* 3XNN, 4XNN, 5XY0 and 9XY0
* 3XNN will skip one instruction if Vx is equal to NN
* 4XNN will skip one instruction if Vx is not equal to NN
* 5XY0 will skip one instruction if Vx is equal to Vy
* 9XY0 will skip one instruction if Vx is not equal to Vy
*/
void Chip8RFX::SkipConditionally()
{
	uint8_t code = (this->mTempOpcode & 0xF000) >> 12;
	bool should_skip = false;

	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	
	if (code == 3)
	{
		uint16_t value = this->mTempOpcode & 0x00FF;
		should_skip = this->mVariableRegisters[vx] == value;
	}
	else if (code == 4)
	{
		uint16_t value = this->mTempOpcode & 0x00FF;
		should_skip = this->mVariableRegisters[vx] != value;
	}
	else if (code == 5)
	{
		uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
		should_skip = this->mVariableRegisters[vx] == this->mVariableRegisters[vy];
	}
	else
	{
		uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
		should_skip = this->mVariableRegisters[vx] != this->mVariableRegisters[vy];
	}

	if (should_skip)
	{
		this->mPc += 2;
	}
}

/**
* 6XNN
* Simply set the register VX to the value NN 
*/
void Chip8RFX::SetRegisterVX()
{
	uint8_t value = this->mTempOpcode & 0x00FF;
	uint8_t registerIndex = (this->mTempOpcode & 0x0F00) >> 8;
	this->mVariableRegisters[registerIndex] = value;
}

/**
* 7XNN
* Just add the NN value to VX register
* if it overflows we don't need to seet the flag register
*/
void Chip8RFX::AddRegisterVX()
{
	uint8_t value = this->mTempOpcode & 0x00FF;
	uint8_t registerIndex = (this->mTempOpcode & 0x0F00) >> 8;
	this->mVariableRegisters[registerIndex] += value;
}


/**
* 8XY0
* VX is set to the value of VY
*/
void Chip8RFX::SetVxByVyValue()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
	this->mVariableRegisters[vx] = this->mVariableRegisters[vy];
}

/**
* 8XY1
* VX is set to the bitwise logical operation OR of VX and VY
*/
void Chip8RFX::SetVxOrVy()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
	this->mVariableRegisters[vx] |= this->mVariableRegisters[vy];
}

/**
* 8XY2
* VX is set to the bitwise logical operation AND of VX and VY
*/
void Chip8RFX::SetVxAndVy()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
	this->mVariableRegisters[vx] &= this->mVariableRegisters[vy];
}

/**
* 8XY3
* VX is set to the bitwise logical operation XOR of VX and VY
*/
void Chip8RFX::SetVxXorVy()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
	this->mVariableRegisters[vx] ^= this->mVariableRegisters[vy];
}

/**
* 8XY4
* VX is set to the plus value of VX and VY
* if the result is bigger then 255 will result 
* to set the carry flag
*/
void Chip8RFX::SetVxPlusVy()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
	this->mVariableRegisters[vx] = this->mVariableRegisters[vx] + this->mVariableRegisters[vy];

	if (this->mVariableRegisters[vx] > 0x00FF)
	{
		this->mVariableRegisters[0xF] = 1;
	}
	else
	{
		this->mVariableRegisters[0xF] = 0;
	}
}

/**
* 8XY5 and 8XY7
* VX is set to the minux value of VX and VY if 8XY5
* VX is set to the minux value of VY and VX if 8XY7
* the subtraction affect the carry flag
*/
void Chip8RFX::Subtract()
{
	uint8_t order = (this->mTempOpcode & 0x000F);
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
	uint16_t result = 0x0000;

	if (order == 5)
	{
		result = this->mVariableRegisters[vx] - this->mVariableRegisters[vy];
	}
	else
	{
		result = this->mVariableRegisters[vy] - this->mVariableRegisters[vx];
	}

	this->mVariableRegisters[vx] = result;

	if ((result & 0xF000) == 0x8)
	{
		this->mVariableRegisters[0xF] = 0;
	}
	else 
	{
		this->mVariableRegisters[0xF] = 1;
	}
}

/**
* 8XY6 and 8XYE
* VX is set to the minux value of VX and VY if 8XY5
* VX is set to the minux value of VY and VX if 8XY7
* the subtraction affect the carry flag
*/
void Chip8RFX::Shift()
{
	uint8_t order = (this->mTempOpcode & 0x000F);
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t vy = (this->mTempOpcode & 0x00F0) >> 4;
	uint16_t result = this->mVariableRegisters[vy];
	uint8_t shifted_bit = 0x00;

	if (order == 6)
	{
		shifted_bit = result & 0x01;
		result = result >> 1;
	}
	else
	{
		shifted_bit = result & 0x80;
		result = result << 1;
	}

	this->mVariableRegisters[0xF] = shifted_bit;
	this->mVariableRegisters[vx] = result;
}


/**
* ANNN
* Set the index register to the value of NNN
*/
void Chip8RFX::SetIndexRegister()
{
	uint16_t iAddress = this->mTempOpcode & 0x0FFF;
	this->mIndexRegister = iAddress;
}

/**
* BNNN
* Set the pc to the value of NNN plus the value of the register 0
*/
void Chip8RFX::JumpWithOffset()
{
	uint16_t address = this->mTempOpcode & 0x0FFF;
	this->mPc = address + this->mVariableRegisters[0x0];
}

/**
* CXNN
* Generate a random number and a binary and with the value NN
* and set into the register Vx
*/
void Chip8RFX::Random()
{
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, 255);

	uint16_t value = this->mTempOpcode & 0x00FF;
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	this->mVariableRegisters[vx] = dist(mt) & value;
}

/**
* DXYN
*/
void Chip8RFX::DrawDisplay()
{
	uint8_t x_reg = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t y_reg = (this->mTempOpcode & 0x00F0) >> 4;

	uint8_t x_pos = this->mVariableRegisters[x_reg] & 63;
	uint8_t y_pos = this->mVariableRegisters[y_reg] & 31;

	this->mVariableRegisters[0xF] = 0x00;

	uint8_t n_rows = (this->mTempOpcode & 0x000F);

	for (int i = 0; i < n_rows; i++)
	{
		// Index Register works as a sprite data
		// we want get the NTh byte, that is the sprite row
		uint8_t sprite_byte = this->mRam[this->mIndexRegister + i];

		for (int j = 0; j < 8; j++)
		{
			uint16_t display_pos = (i + y_pos) * 64 + (x_pos + j);
			uint8_t spritePixel = sprite_byte & (0x80u >> j);


			if (spritePixel && display_pos < this->mDisplay.size() && display_pos >= 0)
			{
				if (this->mDisplay[display_pos] == 0xFFFFFF)
				{
					this->mVariableRegisters[0xF] = 1;
				}

				this->mDisplay[display_pos] ^= 0xFFFFFF;
			}
		}
	}

#ifdef DEBUG_DISPLAY
	
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 32; y++)
		{
			if (this->mDisplay[x + (y * 64)] == 3225151)
				std::cout << "1 ";
			else
				std::cout << "0 ";
		}
		std::cout << "\n";
	}

	std::cout << "\n-----------------------------------------\n";

#endif

	// Set the texture to be draw
	SDL_RenderClear(this->mRenderer);
	SDL_SetRenderDrawColor(this->mRenderer, 0x31, 0x36, 0x3F, 145); // #31363F
	SDL_UpdateTexture(this->mTexture, nullptr, &this->mDisplay, 64 * sizeof(uint32_t));
	SDL_RenderCopy(this->mRenderer, this->mTexture, nullptr, nullptr);
	SDL_RenderPresent(this->mRenderer);
}

/**
* EX9E and EXA1
* EX9E skip one instruction if the key corresponding to the value in Vx is pressed
* EXA1 skip if key corresponding to the value in Vx is not pressed
*/
void Chip8RFX::SkipIfKey()
{
	uint8_t command = (this->mTempOpcode & 0x00FF);
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t key_index = this->mVariableRegisters[vx];
	bool isPressed = false;
	
	for (auto& key : this->mKeyboard)
	{
		if (key.second.value == key_index)
		{
			isPressed = key.second.isPressed;
		}
	}

	if (command == 0x9E)
	{
		this->mPc = (isPressed) ? this->mPc + 2 : this->mPc;
	}
	else
	{
		this->mPc = (!isPressed) ? this->mPc + 2 : this->mPc;
	}
}

/**
* FX07, FX15 and FX18
* FX07 set Vx to the current value of the delay timer
* FX15 sets the delay timer to the value in Vx
* FX18 sets the sound timer to the value in Vx
*/
void Chip8RFX::Timers()
{
	uint16_t operation = (this->mTempOpcode & 0x00FF);
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	
	if (operation == 0x07)
	{
		this->mVariableRegisters[vx] = this->mDelayTimerRegister;
	}
	else if (operation == 0x15)
	{
		this->mDelayTimerRegister = this->mVariableRegisters[vx];
	}
	else if (operation == 0x18)
	{
		this->mSoundTimerRegister = this->mVariableRegisters[vx];
	}
}

/**
* FX1E
* The index register will get the in Vx added to it
*/
void Chip8RFX::AddToIndex()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	this->mIndexRegister += this->mVariableRegisters[vx];
	if (mIndexRegister > 0x0FFF)
	{
		this->mVariableRegisters[0xF] = 1;
	}
}

/**
* FX0A
* 
*/
void Chip8RFX::GetKey()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	bool any_key_pressed = false;

	for (auto& key : this->mKeyboard)
	{
		if (key.second.isPressed)
		{
			this->mVariableRegisters[vx] = key.second.value;
			any_key_pressed = true;
		}
	}

	if (!any_key_pressed)
	{
		this->mPc -= 2;
	}
}

/**
* FX29
*
*/
void Chip8RFX::FontCharacter()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t value = this->mVariableRegisters[vx];
	this->mIndexRegister = 0x00 + (5 * value);
}

/**
* FX33
* We will set the memory in addresses I, I+1 and I+2
* with the value of each digit of Vx
*/
void Chip8RFX::IndexDecimal()
{
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;
	uint8_t value = this->mVariableRegisters[vx];

	this->mRam[this->mIndexRegister] = value / 100;
	this->mRam[this->mIndexRegister + 1] = value % 100 / 10;
	this->mRam[this->mIndexRegister + 2] = value % 10;
}

/**
* FX55 and FX65
* FX55 stores the values of V0 to Vx
* FX65 loads the values of V0 to Vx
*/
void Chip8RFX::StoreAndLoad()
{
	uint16_t operation = (this->mTempOpcode & 0x00FF);
	uint8_t vx = (this->mTempOpcode & 0x0F00) >> 8;

	if (operation == 0x55)
	{
		for (uint8_t i = 0; i <= vx; i++)
		{
			this->mRam[this->mIndexRegister + i] = this->mVariableRegisters[i];
		}
	}
	else
	{
		for (uint8_t i = 0; i <= vx; i++)
		{
			this->mVariableRegisters[i] = this->mRam[this->mIndexRegister + i];
		}
	}
}

// -----------------------------------------------------------------------
// Instructions functions END
// -----------------------------------------------------------------------
