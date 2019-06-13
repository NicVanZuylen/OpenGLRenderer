#include "glfw3.h"

#include "Application.h"
#include "Renderer.h"
#include "Input.h"

#include <iostream>
#include <chrono>

#include "Shader.h"
#include "Texture.h"
#include "CubeMap.h"
#include "Material.h"
#include "Mesh.h"
#include "Batch.h"
#include "StaticMeshRenderer.h"
#include "MeshRenderer.h"
#include "FrameBuffer.h"
#include "Matrix4.h"
#include "NewMatrix4.h"
#include "glm.hpp"
#include "glm\include\gtc\quaternion.hpp"
#include "glm/include/ext.hpp"

#include "RenderSingle.h"

#define MOUSE_SENSITIVITY 0.1f
#define CAMERA_MOVE_SPEED 5.0f
#define BLOOM_PASS_COUNT 2

using namespace NVZMathLib;

Input* Application::m_input = nullptr;
Renderer* Application::m_renderer = nullptr;

Application::Application()
{

}

Application::~Application()
{
	if (m_bGLFWInit)
		glfwTerminate();
	else
		return;

	// Destroy renderer.
	delete m_renderer;

	// Destroy window.
	glfwDestroyWindow(m_window);

	// Destroy input.
	Input::Destroy();
}

int Application::Init() 
{
	m_bGLFWInit = false;

	if (!glfwInit())
		return -1;

	m_bGLFWInit = true;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window.
	m_window = glfwCreateWindow(1280, 720, "OpenGL Renderer", 0, 0);

	// Set key callback...
	glfwSetKeyCallback(m_window, &KeyCallBack);

	// Set mouse callbacks...
	glfwSetMouseButtonCallback(m_window, &MouseButtonCallBack);
	glfwSetCursorPosCallback(m_window, &CursorPosCallBack);
	glfwSetScrollCallback(m_window, &MouseScrollCallBack);

	// Create renderer.
	m_renderer = new Renderer(m_window);

	// Initialize input.
	Input::Create();
	m_input = Input::GetInstance();

	// Initialize camera.
	m_camera = Camera({ 0.0f, 2.5f, 5.0f }, { 0.0f, 0.0f, 0.0f }, 0.05f, 10.0f);

	return 0;
}

void Application::Run() 
{
	// Fullscreen quad shaders.
	Shader* quadShader = new Shader("Shaders/quad/standard_fsquad.vs", "Shaders/quad/standard_fsquad.fs");
	Shader* brightShader = new Shader("Shaders/quad/standard_fsquad.vs", "Shaders/quad/standard_bright.fs");
	Shader* bloomHDRShader = new Shader("Shaders/quad/standard_fsquad.vs", "Shaders/quad/standard_bloom_hdr.fs");
	Shader* directionalLightSdr = new Shader("Shaders/quad/standard_fsquad.vs", "Shaders/light/deferred_directional_light_pbr.fs");
	
	Shader* gaussianHrzntl = new Shader("Shaders/quad/standard_fsquad.vs", "Shaders/quad/standard_gaussian_h.fs");
	Shader* gaussianVert = new Shader("Shaders/quad/standard_fsquad.vs", "Shaders/quad/standard_gaussian_v.fs");

	// Skybox shader.
	Shader* skyboxShader = new Shader("Shaders/skybox/skybox.vs", "Shaders/skybox/skybox.fs");

	// Bloom uniform variables
	gaussianHrzntl->Use();
	gaussianHrzntl->SetUniformFloat("power", 0.003f);

	gaussianVert->Use();
	gaussianVert->SetUniformFloat("power", 0.003f);

	bloomHDRShader->Use();
	bloomHDRShader->SetUniformFloat("exposure", 1.0f);

	Shader::ResetBinding();

	// Scene object shaders
	Shader* plainShader = new Shader("Shaders/plain.vs", "Shaders/plain.fs");

	Shader* lightShader = new Shader("Shaders/light/deferred_point_light_pbr.vs", "Shaders/light/deferred_point_light_pbr.fs");

	Mesh* planeMesh = new Mesh("Assets/Primitives/plane.obj");
	Mesh* sphereMesh = new Mesh("Assets/Primitives/sphere.obj");

	DynamicArray<const char*> mapFaces = 
	{
		"Assets/Skybox/stormydays_ft.tga",
		"Assets/Skybox/stormydays_bk.tga",
		"Assets/Skybox/stormydays_up.tga",
		"Assets/Skybox/stormydays_dn.tga",
		"Assets/Skybox/stormydays_rt.tga",
		"Assets/Skybox/stormydays_lf.tga"
	};

	CubeMap* skyTex = new CubeMap(mapFaces);

	// Floor plane material
	Material* floorMat = new Material(plainShader);

	// Model matrix for the plane
	glm::mat4 floorModelMatrix;

	Framebuffer* gBuffer = new Framebuffer(m_renderer->WindowWidth(), m_renderer->WindowHeight());
	gBuffer->AddBufferColorAttachment(BUFFER_FLOAT_RGBA16); // Diffuse buffer.
	gBuffer->AddBufferColorAttachment(BUFFER_FLOAT_RGBA16); // Position buffer.
	gBuffer->AddBufferColorAttachment(BUFFER_FLOAT_RGBA16); // Normal buffer.
	gBuffer->AddBufferColorAttachment(BUFFER_FLOAT_RGBA16); // Specular buffer.
	gBuffer->AddBufferColorAttachment(BUFFER_RGB); // Roughness, spec strength, reflection coefficent
	gBuffer->AddBufferColorAttachment(BUFFER_RGB); // Emission
	gBuffer->AddDepthAttachment();

	// Color buffer for HDR.
	Framebuffer* brightColorBuffer = new Framebuffer(m_renderer->WindowWidth(), m_renderer->WindowHeight());
	brightColorBuffer->AddBufferColorAttachment(BUFFER_RGB); // Bright color.

	Framebuffer* bloomBuffer = new Framebuffer(m_renderer->WindowWidth(), m_renderer->WindowHeight());
	bloomBuffer->AddBufferColorAttachment(BUFFER_FLOAT_RGB16); // Albedo color.

	Framebuffer* blurBuffers[2] = { new Framebuffer(m_renderer->WindowWidth(), m_renderer->WindowHeight()), new Framebuffer(m_renderer->WindowWidth(), m_renderer->WindowHeight()) };
	blurBuffers[0]->AddBufferColorAttachment(BUFFER_RGB); // Bright colors.
	blurBuffers[1]->AddBufferColorAttachment(BUFFER_RGB); // Bright colors.

	// Set shaders used for deferred shading pass.
	m_renderer->SetDLightShader(directionalLightSdr);
	m_renderer->SetPLightShader(lightShader);

	// Contains a static mesh that can be rendered in a single draw call.
	StaticMeshRenderer staticMeshes(floorMat);

	// Add two spears to the mesh.
	staticMeshes.PushMesh(planeMesh, glm::value_ptr(glm::mat4()));
	staticMeshes.PushMesh(sphereMesh, glm::value_ptr(glm::translate(glm::vec3(-1.0f, 2.0f, -2.0f))));

	staticMeshes.FinalizeBuffers();

	// Add scene light.
	m_renderer->AddPointLight(NVZMathLib::Vector4(1.0f, 1.0f, 1.0f, 1.0f), NVZMathLib::Vector3(0.0f, 3.5f, 0.0f), 5.0f);
	m_renderer->AddPointLight(NVZMathLib::Vector4(1.0f, 0.0f, 0.0f, 1.0f), NVZMathLib::Vector3(-3.0f, 3.0f, -2.0f), 5.0f);

	float fDeltaTime = 0.0f;	

	while(!glfwWindowShouldClose(m_window)) 
	{
		// Time
		auto startTime = std::chrono::high_resolution_clock::now();

		// Quit if escape is pressed.
		if (m_input->GetKey(GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(m_window, 1);

		// ------------------------------------------------------------------------------------
		// Camera & View matrix

		m_camera.Update(fDeltaTime, m_input, m_window);

		m_renderer->SetViewMatrix(m_camera.GetViewMatrix(), m_camera.GetPosition());

		// ------------------------------------------------------------------------------------

		// Poll events.
		glfwPollEvents();

		// Rendering...

		// Bind G Buffer...

		m_renderer->Start();
		gBuffer->Bind();
		m_renderer->ClearFramebuffer();
		
		// Draw calls here...

		floorMat->DrawStaticMeshes();
		m_renderer->DrawFinal();

		bloomBuffer->Bind();

		// Draw fullscreen quad...
		m_renderer->DrawFSQuad(quadShader, gBuffer->GetTextureArray(), gBuffer->GetAttachmentCount());

		// Uncomment this to enable the directional light.
		//m_renderer->RunDeferredDirectionalLight(gBuffer->GetTextureArray(), gBuffer->GetAttachmentCount(), Vector3(-1.0f, -0.5f, -1.0f).Normalised(), Vector3(1.0f));

		m_renderer->ReportErrors();
		m_renderer->RunDeferredPointLighting(gBuffer->GetTextureArray(), gBuffer->GetAttachmentCount());

		// Bind fullscreen quad for minimal state changes during post effect rendering.
		m_renderer->BindFSQuad();

		// Bind first blur buffer.
		brightColorBuffer->Bind();

		// Render bright color into first hdr
		brightShader->Use();
		m_renderer->BindTextures(bloomBuffer->GetTextureArray(), 1);
		m_renderer->DrawFSQuadNoState();

		Framebuffer* currentBuffer = blurBuffers[1];
		Framebuffer* lastBuffer = blurBuffers[0];

		// Bind first blur buffer.
		lastBuffer->Bind();

		// First horizontal blur pass.
		gaussianHrzntl->Use();
		m_renderer->BindTextures(brightColorBuffer->GetTextureArray(), 1);
		m_renderer->DrawFSQuadNoState();

		// Remaining horizontal passes.
		for(int i = 0; i < BLOOM_PASS_COUNT - 1; ++i)
		{
			currentBuffer->Bind();

			m_renderer->BindTextures(lastBuffer->GetTextureArray(), 1);
			m_renderer->DrawFSQuadNoState();

			Framebuffer* lastBufferCpy = lastBuffer;

			lastBuffer = currentBuffer;
			currentBuffer = lastBufferCpy;
		}

		gaussianVert->Use();

		// Vertical pass.
		for (int i = 0; i < BLOOM_PASS_COUNT; ++i)
		{
			currentBuffer->Bind();

			m_renderer->BindTextures(lastBuffer->GetTextureArray(), 1);
			m_renderer->DrawFSQuadNoState();

			Framebuffer* lastBufferCpy = lastBuffer;

			lastBuffer = currentBuffer;
			currentBuffer = lastBufferCpy;
		}

		// Reset FBO binding.
		m_renderer->ResetFramebufferBinding();

		// Get final textures...
		Texture* m_finalBloomTex[2] = 
		{
			bloomBuffer->GetTextureArray()[0],
			lastBuffer->GetTextureArray()[0]
		};

		// Draw final result...
		bloomHDRShader->Use();
		m_renderer->BindTextures(&m_finalBloomTex[0], 2);
		m_renderer->DrawFSQuadNoState();

		// Unbind fullscreen quad.
		m_renderer->UnbindVAO();

		// Copy G Buffer's depth to the default FBO's depth texture.
		m_renderer->UseDepthTextureOf(gBuffer);
		m_renderer->EnableDepthTesting();

		// Stuff can be rendered in forward here.

		/*
		// Uncomment this to draw the skybox.

		m_renderer->BindSkybox();
		skyboxShader->Use();
		skyTex->BindCubeMap();
		m_renderer->DrawSkybox();
		*/

		m_renderer->End();

		// End time...
		auto endTime = std::chrono::high_resolution_clock::now();

		auto timeDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

		fDeltaTime = static_cast<float>(timeDuration) / 1000000.0f;
	}

	// Free memory.

	delete gBuffer;
	delete brightColorBuffer;

	delete bloomBuffer;

	delete blurBuffers[1];
	delete blurBuffers[0];

	delete quadShader;
	delete brightShader;
	delete bloomHDRShader;
	
	delete gaussianVert;
	delete gaussianHrzntl;
	
	delete skyboxShader;

	delete directionalLightSdr;
	delete lightShader;
	delete plainShader;

	delete floorMat;

	delete skyTex;

	delete planeMesh;
	delete sphereMesh;
}

void Application::ErrorCallBack(int error, const char* desc)
{
	std::cout << "GLFW Error: " << desc << "\n";
}

void Application::KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	m_input->GetCurrentState()[key] = action;
}

void Application::MouseButtonCallBack(GLFWwindow* window, int button, int action, int mods)
{
	m_input->GetCurrentMouseState()->m_buttons[button] = action - 1;
}

void Application::CursorPosCallBack(GLFWwindow* window, double dXPos, double dYPos)
{
	MouseState* currentState = m_input->GetCurrentMouseState();

	currentState->m_fMouseAxes[0] = dXPos;
	currentState->m_fMouseAxes[1] = dYPos;
}

void Application::MouseScrollCallBack(GLFWwindow* window, double dXOffset, double dYOffset)
{
	MouseState* currentState = Input::GetInstance()->GetCurrentMouseState();

	currentState->m_fMouseAxes[2] = dXOffset;
	currentState->m_fMouseAxes[3] = dYOffset;
}
