#include "Application.h"

#include <crtdbg.h>
#include <iostream>

int main() 
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Application* application = new Application();

	// Initialize and quit if the code is not zero.
	int code = application->Init();
	if (code)
		return code;

	// Run application.
	application->Run();

	delete application;

	return 0;
}