#pragma once
#include "Camera.h"

struct GLFWwindow;

class Input;
class Renderer;

class Application
{
public:

	Application();

	~Application();

	int Init();

	void Run();

private:

	// GLFW Callbacks
	static void ErrorCallBack(int error, const char* desc);
	static void KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseButtonCallBack(GLFWwindow* window, int button, int action, int mods);
	static void CursorPosCallBack(GLFWwindow* window, double dXPos, double dYPos);
	static void MouseScrollCallBack(GLFWwindow* window, double dXOffset, double dYOffset);

	GLFWwindow* m_window;
	static Renderer* m_renderer;
	static Input* m_input;
	bool m_bGLFWInit;

	Camera m_camera;
};

