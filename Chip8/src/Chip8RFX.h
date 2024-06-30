#pragma once

#include <stack>
#include <array>
#include <vector>
#include <map>
#include <string>
#include <SDL.h>

const uint32_t FPS = 60;
const uint32_t FRAME_TARGET_TIME = 1000 / FPS;
const uint32_t SCALE = 20;

class Chip8RFX
{
public:
	Chip8RFX();
	~Chip8RFX();

	void Run();

private:
	enum class InstructionOp : uint16_t;

	enum InstructionOp FetchAndDecode();
	void Execute(const InstructionOp& instruction);

private:
	void Initialize();
	
	void StartSDL();
	void ClearSDL();
	void HandleSDLEvents();

private:

	// Invalid Instruction
	void Invalid();

	// Instructions functions
	void ClearScreen();			// 00E0
	void ReturnFromSubroutine();	// 00EE - Subroutine
	void JumpInSubroutine();		// 2NNN - Subroutine
	void Jump();				// 1NNN
	void SkipConditionally();	// 3XNN, 4XNN, 5XY0 and 9XY0
	void SetRegisterVX();		// 6XNN
	void AddRegisterVX();		// 7XNN
	void SetVxByVyValue();		// 8XY0
	void SetVxOrVy();			// 8XY1
	void SetVxAndVy();			// 8XY2
	void SetVxXorVy();			// 8XY3
	void SetVxPlusVy();			// 8XY4
	void Subtract();			// 8XY5 and 8XY7
	void Shift();				// 8XY6 and 8XYE
	void SetIndexRegister();	// ANNN
	void JumpWithOffset();		// BNNN
	void Random();				// CXNN
	void DrawDisplay();			// DXYN
	void SkipIfKey();			// EX9E and EXA1
	void Timers();				// FX07, FX15 and FX18
	void AddToIndex();			// FX1E
	void GetKey();				// F0XA
	void FontCharacter();		// FX29
	void IndexDecimal();		// FX33
	void StoreAndLoad();		// FX55 and FX65

private:
	// 4KB memory of RAM
	std::array<uint8_t, 4096> mRam;
	std::stack<uint16_t> mStack;

	uint16_t mPc;	// Program Counter of 12-bits
	uint16_t mIndexRegister; // Index register, used to point at locations in the memory and it's used 12-bits

	uint16_t mTempOpcode;

	// General purpose variable register of 8-bits
	std::array<uint8_t, 16> mVariableRegisters;

	// Timer Registers
	uint8_t mSoundTimerRegister;
	uint8_t mDelayTimerRegister;

	// Display
	std::array<uint32_t, 64 * 32> mDisplay;

	typedef struct Chip8Keys
	{
		uint8_t value;
		bool isPressed;
	};

	// Keyboard mapping
	std::map<SDL_Keycode, Chip8Keys> mKeyboard;


private:
	// Control flag
	bool isFinished;
	bool mStopPC;

private:
	// SDL stuff
	SDL_Renderer* mRenderer;
	SDL_Window* mWindow;
	SDL_Texture* mTexture;
	Uint32 mTicksLastFrame;

private:
	struct Instruction
	{
		std::string name;
		void(Chip8RFX::* operate)(void) = nullptr;	// Function associated
	};

	enum class InstructionOp : uint16_t
	{
		NONE,
		CLEAR_SCREEN,
		JUMP,
		SET_REG_VX,
		ADD_REG_VX,
		SET_INDEX_REG,
		DRAW_DISPLAY,
		SUBROUTINE_IN,
		SUBROUTINE_OUT,
		SKIP_CONDITIONALLY,
		SET_REG,
		REG_OR,
		REG_AND,
		REG_XOR,
		ADD_WITH_CARRY,
		SUBTRACT,
		SHIFT,
		JUMP_OFFSET,
		RANDOM,
		SKIP_IF_KEY,
		TIMERS,
		ADD_TO_INDEX,
		GET_KEY,
		FONT_CHARACTER,
		INDEX_BIN_DEC,
		STORE_AND_LOAD
	};

	std::map<InstructionOp,Instruction> lookup;
};

