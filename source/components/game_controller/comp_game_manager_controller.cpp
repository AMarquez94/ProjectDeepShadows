#include "mcv_platform.h"
#include "comp_game_manager_controller.h"
#include "entity/entity_parser.h"
#include "windows/app.h"

DECL_OBJ_MANAGER("game_manager_controller", TCompGameManagerController);

void TCompGameManagerController::debugInMenu() {

}

void TCompGameManagerController::renderDebug()
{
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoScrollbar;

	if (menuVisible) {
		ImGui::SetNextWindowSize(ImVec2(100.f, 75.f));
		ImGui::Begin("MENU", false, window_flags);
		ImGui::CaptureMouseFromApp(false);
		ImGui::SetWindowPos("MENU", ImVec2(CApp::get().xres/2, CApp::get().yres/2 - 100.f));
		ImGui::Selectable("Restart game", menuPosition == 0);
		ImGui::Selectable("Exit game", menuPosition == 1);
		ImGui::End();
	}
}

void TCompGameManagerController::load(const json& j, TEntityParseContext& ctx) {
	player = ctx.findEntityByName(j.value("player", "The Player"));
	menuVisible = false;
}

void TCompGameManagerController::update(float dt) {
	if (EngineInput["btPause"].getsPressed()) {
		if (menuVisible) {
			menuPosition = -1;
			menuVisible = false;
		}
		else {
			menuVisible = true;
		}
	}

	if (menuVisible) {
		if (EngineInput["btUpAux"].getsPressed()) {
			if (menuPosition == -1) {
				menuPosition = menuSize - 1;
			}
			else {
				menuPosition = (menuPosition + menuSize - 1) % menuSize;
			}
			dbg("Posicion %d\n", menuPosition);
		}
		else if(EngineInput["btDownAux"].getsPressed()){
			menuPosition = (menuPosition + 1) % menuSize;
			dbg("Posicion %d\n", menuPosition);
		}

		if (EngineInput["btMenuConfirm"].getsPressed()) {
			switch (menuPosition) {
				case 0:
					CEngine::get().getModules().changeGameState("test_axis");
					break;
				case 1:
					exit(0);
					break;
			}
		}
		else if (EngineInput["btMenuCancel"].getsPressed()) {
			menuPosition = -1;
			menuVisible = false;
		}
	}
}