#include "mcv_platform.h"
#include "module_logic.h"
#include "components/comp_tags.h"
#include "components\lighting\comp_light_spot.h"
#include "components\ia\comp_bt_patrol.h"
#include "components\comp_group.h"
#include <experimental/filesystem>
#include "modules/game/module_game_manager.h"
#include <iostream>

#include "components/lighting/comp_light_dir.h"
#include "components/lighting/comp_light_spot.h"
#include "components/lighting/comp_light_point.h"
bool CModuleLogic::start() {
	BootLuaSLB();
	execEvent(Events::GAME_START);
	return true;
}

bool CModuleLogic::stop() {
	delete(s);
	delete(m);
	return true;
}

void CModuleLogic::update(float delta) {
	for (unsigned int i = 0; i < delayedScripts.size(); i++) {
		delayedScripts[i].remainingTime -= delta;
		if (delayedScripts[i].remainingTime <= 0) {
			execScript(delayedScripts[i].script);
			delayedScripts.erase(delayedScripts.begin() + i);
		}
	}
}

/* Where we publish all functions that we want and load all the scripts in the scripts folder */
void CModuleLogic::BootLuaSLB() {

    //Publish all the functions
    publishClasses();
    //Load all the scripts
    loadScriptsInFolder("data/scripts");
}

/* Load all scripts.lua in given path and its subfolders */
void CModuleLogic::loadScriptsInFolder(char * path)
{
	try {
		if (std::experimental::filesystem::exists(path) && std::experimental::filesystem::is_directory(path)) {

			std::experimental::filesystem::recursive_directory_iterator iter(path);
			std::experimental::filesystem::recursive_directory_iterator end;

			while (iter != end) {
				std::string fileName = iter->path().string();
				if (fileName.substr(fileName.find_last_of(".") + 1) == "lua" &&
					!std::experimental::filesystem::is_directory(iter->path())) {
					dbg("File : %s loaded\n", fileName.c_str());
					s->doFile(fileName);
				}
				std::error_code ec;
				iter.increment(ec);
				if (ec) {
					fatal("Error while accessing %s: %s\n", iter->path().string().c_str(), ec.message().c_str());
				}
			}
		}
	}
	catch (std::system_error & e) {
		fatal("Exception %s while loading scripts\n", e.what());
	}
}

/* Publish all the classes in LUA */
void CModuleLogic::publishClasses() {

    /* Classes */
    SLB::Class< CModuleGameConsole >("GameConsole", m)
        .comment("This is our wrapper of the console class")
        .set("addCommand", &CModuleGameConsole::addCommandToList);

    SLB::Class< CModuleLogic >("Logic", m)
        .comment("This is our wrapper of the logic class")
        .set("printLog", &CModuleLogic::printLog);


	/* Global functions */
	m->set("getConsole", SLB::FuncCall::create(&getConsole));
	m->set("getLogic", SLB::FuncCall::create(&getLogic));
	m->set("execDelayedScript", SLB::FuncCall::create(&execDelayedScript));
	m->set("pauseGame", SLB::FuncCall::create(&pauseGame));
	m->set("blendInCamera", SLB::FuncCall::create(&blendInCamera));
	m->set("blendOutCamera", SLB::FuncCall::create(&blendOutCamera));
	m->set("fpsToggle", SLB::FuncCall::create(&fpsToggle));
	m->set("debugToggle", SLB::FuncCall::create(&debugToggle));
	m->set("systemToggle", SLB::FuncCall::create(&systemToggle));
	m->set("movePlayer", SLB::FuncCall::create(&movePlayer));
	m->set("staminaInfinite", SLB::FuncCall::create(&staminaInfinite));
	m->set("immortal", SLB::FuncCall::create(&immortal));
	m->set("inShadows", SLB::FuncCall::create(&inShadows));
	m->set("speedBoost", SLB::FuncCall::create(&speedBoost));
	m->set("playerInvisible", SLB::FuncCall::create(&playerInvisible));
	m->set("spotlightsToggle", SLB::FuncCall::create(&spotlightsToggle));
	m->set("lanternToggle", SLB::FuncCall::create(&lanternToggle));
	m->set("spawn", SLB::FuncCall::create(&spawn));
	m->set("bind", SLB::FuncCall::create(&bind));
	m->set("loadscene", SLB::FuncCall::create(&loadscene));
	m->set("cg_drawlights", SLB::FuncCall::create(&cg_drawlights));


}

void CModuleLogic::execCvar(std::string& script) {

	// Only backslash commands fetched.
	if (script.find("/") != 0)
		return;

	// Little bit of dirty tricks to achieve same results with different string types.
	script.erase(0, 1);
	int index = script.find_first_of(' ');
	script = index != -1 ? script.replace(script.find_first_of(' '), 1, "(") : script;
	index = script.find_first_of(' ');
	script = index != -1 ? script.replace(script.find_first_of(' '), 1, ",") : script;
	script.append(")");
}

CModuleLogic::ConsoleResult CModuleLogic::execScript(const std::string& script) {
	std::string scriptLogged = script;
	std::string errMsg = "";
	bool success = false;

	try {
		s->doString(script);
		scriptLogged = scriptLogged + " - Success";
		success = true;
	}
	catch (std::exception e) {
		scriptLogged = scriptLogged + " - Failed";
		scriptLogged = scriptLogged + "\n" + e.what();
		errMsg = e.what();
	}

	dbg("Script %s\n", scriptLogged.c_str());
	log.push_back(scriptLogged);
	return ConsoleResult{ success, errMsg };
}

bool CModuleLogic::execScriptDelayed(const std::string & script, float delay)
{
	delayedScripts.push_back(DelayedScript{ script, delay });
	return true;
}

bool CModuleLogic::execEvent(Events event, const std::string & params, float delay)
{
    /* TODO: meter eventos */
    switch (event) {
    case Events::GAME_START:
        if (delay > 0) {
            return execScriptDelayed("onGameStart()", delay);
        }
        else {
            return execScript("onGameStart()").success;
        }
        break;
    case Events::GAME_END:

        break;
    default:

        break;
    }
    return false;
}

void CModuleLogic::printLog()
{
    dbg("Printing log\n");
    for (int i = 0; i < log.size(); i++) {
        dbg("%s\n", log[i].c_str());
    }
    dbg("End printing log\n");
}

/* Auxiliar functions */
CModuleLogic * getLogic() { return EngineLogic.getPointer(); }
CModuleGameConsole * getConsole() { return EngineConsole.getPointer(); }

void execDelayedScript(const std::string& script, float delay)
{
    EngineLogic.execScriptDelayed(script, delay);
}

void pauseGame(bool pause)
{
	TMsgScenePaused msg;
	msg.isPaused = pause;
	EngineEntities.broadcastMsg(msg);
}

void fpsToggle() {
	Engine.getGameManager().config.drawfps = !Engine.getGameManager().config.drawfps;
}

void debugToggle() {
	EngineRender.debugmode = !EngineRender.debugmode;
}

void blendInCamera(const std::string & cameraName, float blendInTime)
{
	CHandle camera = getEntityByName(cameraName);
	if (camera.isValid()) {
		EngineCameras.blendInCamera(camera, blendInTime, CModuleCameras::EPriority::TEMPORARY);
	}
	//TODO: implement
}

void blendOutCamera(const std::string & cameraName, float blendOutTime) {
	CHandle camera = getEntityByName(cameraName);
	if (camera.isValid()) {
		EngineCameras.blendOutCamera(camera, blendOutTime);
	}
}

void staminaInfinite() {
	CHandle h = getEntityByName("The Player");
	TMsgInfiniteStamina msg;
	h.sendMsg(msg);
}

void immortal() {
	CHandle h = getEntityByName("The Player");
	TMsgPlayerImmortal msg;
	h.sendMsg(msg);
}

void inShadows() {
	CHandle h = getEntityByName("The Player");
	TMsgPlayerInShadows msg;
	h.sendMsg(msg);
}

void speedBoost(const float speed) {
	CHandle h = getEntityByName("The Player");
	TMsgSpeedBoost msg;
	msg.speedBoost = speed;
	h.sendMsg(msg);
}

void playerInvisible() {
	CHandle h = getEntityByName("The Player");
	TMsgPlayerInvisible msg;
	h.sendMsg(msg);
	std::vector<CHandle> enemies = CTagsManager::get().getAllEntitiesByTag(getID("enemy"));
	for (int i = 0; i < enemies.size(); i++) {
		enemies[i].sendMsg(msg);
	}
}

void spotlightsToggle() {
	//For all spotlights, we change their status. This does not desactivate lanterns even though they are spotlights.
	getObjectManager<TCompLightSpot>()->forEach([](TCompLightSpot* c) {
		c->isEnabled = !c->isEnabled;
	});
	//Now we reactivate lanterns in case it is needed.
	std::vector<CHandle> enemies = CTagsManager::get().getAllEntitiesByTag(getID("patrol"));
		for (int i = 0; i < enemies.size(); i++) {
			CEntity* e = enemies[i];
			TCompGroup* group = e->get<TCompGroup>();
			CHandle lantern = group->getHandleByName("FlashLight");
			if (lantern.isValid()) {
				CEntity* e = lantern;
				TCompLightSpot* patrol_lantern = e->get<TCompLightSpot>();
				if (patrol_lantern->isEnabled) break; //If the spotlight is already active, nothing to do, we break the loop
				patrol_lantern->isEnabled = true;    //else, we activate the lanterns.
			}
	}

}

void lanternToggle() {
	std::vector<CHandle> enemies = CTagsManager::get().getAllEntitiesByTag(getID("patrol"));
	for (int i = 0; i < enemies.size(); i++) {
		CEntity* e = enemies[i];
		TCompGroup* group = e->get<TCompGroup>();
		CHandle lantern = group->getHandleByName("FlashLight");
		if (lantern.isValid()) {
			CEntity* e = lantern;
			TCompLightSpot* patrol_lantern = e->get<TCompLightSpot>();
			patrol_lantern->isEnabled = !patrol_lantern->isEnabled;
		}
	}
}

void systemToggle(const std::string& system) {
	if (system == "enemies") {
		std::vector<CHandle> enemies = CTagsManager::get().getAllEntitiesByTag(getID("enemy"));
		TMsgScenePaused msg;
		for (int i = 0; i < enemies.size(); i++) {
			enemies[i].sendMsg(msg);
		}
	}
}

void movePlayer(const float x, const float y, const float z) {
	CHandle h = getEntityByName("The Player");
	TMsgPlayerMove msg;
	msg.pos = VEC3(x, y, z);
	h.sendMsg(msg);
}
void bind(const std::string& key, const std::string& script) {

	int id = EngineInput.getButtonDefinition(key)->id;
	//Input::TButton button = EngineInput.keyboard().key(id);
	EngineLogic._bindings[id] = script;
}

void unbind(const std::string& key, const std::string& script) {

	int id = EngineInput.getButtonDefinition(key)->id;
	//Input::TButton button = EngineInput.keyboard().key(id);

	std::map<int, std::string>::iterator it;
	it = EngineLogic._bindings.find(id);
	EngineLogic._bindings.erase(it, EngineLogic._bindings.end());
}
void loadscene(const std::string &level) {

	EngineScene.loadScene(level);
}

void cg_drawlights(int type) {

	bool dir = false, spot = false, point = false;

	switch (type) {
	case 1: dir = spot = point = true; break;
	case 2: dir = true; break;
	case 3: spot = true; break;
	case 4: point = true; break;
	default: break;
	}

	getObjectManager<TCompLightDir>()->forEach([&](TCompLightDir* c) {
		c->isEnabled = dir;
	});

	getObjectManager<TCompLightSpot>()->forEach([&](TCompLightSpot* c) {
		c->isEnabled = spot;
	});

	getObjectManager<TCompLightPoint>()->forEach([&](TCompLightPoint* c) {
		c->isEnabled = point;
	});
}
void spawn(const std::string & name, const VEC3 & pos) {
	//To-Do
}
