#include "mcv_platform.h"
#include "module_main_menu.h"
#include "windows/app.h"
#include "input/devices/mouse.h"

bool CModuleMainMenu::start()
{
	CEngine::get().getRender().setBackgroundColor(1.f, 0.f, 0.f, 1.f);
  
	window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoScrollbar;
	window_width = 200;
	window_height = 150;
	
	// Initialization settings
	Input::CMouse* mouse = static_cast<Input::CMouse*>(EngineInput.getDevice("mouse"));
	mouse->setLockMouse();
	ShowCursor(true);

	return true;
}

void CModuleMainMenu::update(float delta)
{
	time += delta * 0.25f;
	float background_value = abs(sin(time));
	background_value = background_value * 0.3f + 0.3f;
	CEngine::get().getRender().setBackgroundColor(background_value, background_value, background_value, 1.f);

	VEC2 menu_position = VEC2(CApp::get().xres * .5f - (window_width * .5f), 
							  CApp::get().yres * .5f - (window_height * .5f));

	ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
	ImGui::Begin("MENU", false, window_flags);
	ImGui::CaptureMouseFromApp(false);
	ImGui::SetWindowPos("MENU", ImVec2(menu_position.x, menu_position.y));
	ImGui::Selectable("New Game", level_load == 0);
	if (ImGui::IsItemClicked())
	{
		dbg("changed level state\n");
		CEngine::get().getModules().changeGameState("test_axis");
	}

	ImGui::Selectable("Select level", level_load == 1);
	if (ImGui::IsItemClicked())
	{
		dbg("changed select state\n");
	}

	ImGui::Selectable("Controls", level_load == 2);
	if (ImGui::IsItemClicked())
	{
		dbg("changed controls state\n");
	}

	ImGui::Selectable("Options", level_load == 3);
	if (ImGui::IsItemClicked())
	{
		dbg("changed options state\n");
	}

	ImGui::Selectable("Credits", level_load == 4);
	if (ImGui::IsItemClicked())
	{
		dbg("changed credits state\n");
	}

	ImGui::Selectable("Exit game", level_load == 5);
	if (ImGui::IsItemClicked())
	{
		dbg("changed exit state\n");
		exit(0);
	}

	ImGui::End();
	//if (isPressed(VK_SPACE))
	//{
	//	CEngine::get().getModules().changeGameState("test_axis");
	//}
}