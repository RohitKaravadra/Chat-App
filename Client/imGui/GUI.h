#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "GamesEngineeringBase.h"

using GamesEngineeringBase::Window;

struct color
{
	union {
		struct { float r, g, b, a; };
		float rgba[4];
	};

	color() {
		r = g = b = 0;
		a = 1.0f;
	}

	color(float _r, float _g, float _b, float _a = 1.0f) {
		r = min(_r, 1.0f);
		g = min(_g, 1.0f);
		b = min(_b, 1.0f);
		a = min(_a, 1.0f);
	}

	color operator*(const color& _c) {
		return color(a * _c.a, g * _c.g, b * _c.b);
	}
};

const color BLACK = color();
const color WHITE = color(1, 1, 1);

class GUIWindow
{
	Window canvas;
	ID3D11Device* device;
	ID3D11DeviceContext* devcontext;
	IDXGISwapChain* sc;
	ID3D11RenderTargetView* rtv;
public:
	ImGuiIO io;
	GUIWindow(unsigned int width, unsigned int height, std::string name)
	{
		canvas.create(width, height, name);
		device = canvas.getDXDevice();
		devcontext = canvas.getDXContext();
		sc = canvas.getSwapchain();
		rtv = canvas.getRenderTargetView();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		ImGui::GetIO().FontGlobalScale = 1.5f;					// Increase font size

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(canvas.getHwnd());
		ImGui_ImplDX11_Init(device, devcontext);
	}

	bool checkInputs()
	{
		canvas.checkInput();
		return canvas.keyPressed(VK_ESCAPE) || canvas.isQuit();
	}

	bool quit() {
		return canvas.isQuit();
	}

	void startFrame()
	{
		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void endFrame(color _background = BLACK)
	{
		// Rendering
		ImGui::Render();
		canvas.getDXContext()->ClearRenderTargetView(canvas.getRenderTargetView(), _background.rgba);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present
		HRESULT hr = canvas.getSwapchain()->Present(1, 0);   // Present with vsync
	}

	~GUIWindow()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
};