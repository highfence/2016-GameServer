#include <thread>
#include <iostream>

#include "../MyLogicLib/MainLogic.h"


int main(void)
{
	MyLogicLib::MainLogic mainLogic;

	mainLogic.Init();

	std::thread logicThread([&]() {
		mainLogic.Run();
	});

	std::cout << "input any word to exit...";
	getchar();

	mainLogic.Stop();
	logicThread.join();

	return 0;
}