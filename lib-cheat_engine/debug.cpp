/*
 * debug.cpp
 *
 *  Created on: 6 янв. 2024 г.
 *      Author: eFnat
 */
#include "process.h"

#include <unistd.h>
#include <future>

Process programm;
Module programm_module;

#define PTROFFSET_PLAYERPOSITION {0x004F0330, 0x198, 0x9b0, 0xc4, 0x878, 0x4c - 0x4}
#define PTROFFSET_PERCCOUTER {0x004F0330, 0x198, 0xc0, 0x34, 0xc, 0x1c, 0x148}

template<class T>
struct alignas(sizeof(T)) Point
{
	T x;
	T y;
};

class Player : public DisplacedPointer
{
	struct {
		unsigned long Position;
	} _Address;
public:
	using DisplacedPointer::DisplacedPointer;
	using DisplacedPointer::GetAddress;

	inline Point<float> GetPosition()
	{
		return _Process->Read<Point<float>>(_Address.Position);
	}
	inline void  SetPosition(Point<float> position)
	{
		_Process->Write<Point<float>>(_Address.Position, position);
	}
	inline float GetX()
	{
		return _Process->Read<float>(_Address.Position);
	}
	inline float GetY()
	{
		return _Process->Read<float>(_Address.Position + 0x4);
	}
	inline void SetX(float x)
	{
		_Process->Write<float>(_Address.Position, x);
	}
	inline void SetY(float y)
	{
		_Process->Write<float>(_Address.Position + 0x4, y);
	}


	void Update() override
	{
		_Address.Position = GetAddress(PTROFFSET_PLAYERPOSITION);
	}

	void PrintState()
	{
		auto position = GetPosition();
		std:: cout << '[' << position.x << ", " << position.y << "] " << std::hex << _Address.Position << std::dec << std::endl;;
	}
};

Player player;

int main()
{
	programm = Process("GeometryDash.exe", PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE);
	programm_module = programm.GetModule("GeometryDash.exe");

	player = Player(&programm, &programm_module);
	player.Update();
	player.PrintState();

	auto output = std::async(std::launch::async, []{
		while (true)
		{
			player.Update();
			player.SetY(player.GetY() + 0.003);
			player.SetX(player.GetX() + 0.001);
			sleep(0.9);

			//programm.Write<float>(0x20EB6048 + 0x8, 45.0);
			//sleep(0.5);
		}
	});

	while (true)
	{

	}
	return 0;
}



