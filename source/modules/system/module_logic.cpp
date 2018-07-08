#include "mcv_platform.h"
#include "module_logic.h"
#include "components/comp_tags.h"
#include <experimental/filesystem>
#include "modules/game/module_game_manager.h"
#include "modules/system/module_particles.h"
#include <iostream>
#include "components/lighting/comp_light_dir.h"
#include "components/lighting/comp_light_spot.h"
#include "components/lighting/comp_light_point.h"
#include "components/postfx/comp_render_ao.h"
#include "components/player_controller/comp_player_tempcontroller.h"
#include "components/object_controller/comp_button.h"

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

    for (auto k : _bindings) {
        Input::TButton button = EngineInput.keyboard().key(k.first);
        if (button.getsPressed()) {
            std::string current_value = k.second;
            execCvar(current_value);
            execScript(current_value);
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
                if (iter->path().extension().string() == ".lua" &&
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

    SLB::Class< CModuleParticles >("Particles", m)
        .comment("This is our wrapper of the particles class")
        .set("killAll", &CModuleParticles::killAll);

    SLB::Class< VEC3 >("VEC3", m)
        .constructor<float, float, float>()
        .comment("This is our wrapper of the VEC3 class")
        .property("x", &VEC3::x)
        .property("y", &VEC3::y)
        .property("z", &VEC3::z);

    SLB::Class< TCompTempPlayerController >("PlayerController", m)
        .comment("This is our wrapper of the player controller component")
        .property("inhibited", &TCompTempPlayerController::isInhibited)
        .set("die", &TCompTempPlayerController::die);

    SLB::Class<TCompLightSpot>("SpotLight", m)
        .comment("This is our wrapper of the spotlight controller")
        .property("isEnabled", &TCompLightSpot::isEnabled);

    SLB::Class<TCompButton>("Button", m)
        .comment("This is our wrapper of the button controller")
        .property("canBePressed", &TCompButton::canBePressed);

    //SLB::Class < CHandle >("CHandle", m)
    //    .comment("CHandle wrapper")
    //    .set("getLight", &CHandle::sendMsg);

    /* Global functions */

    // Utilities
    m->set("getConsole", SLB::FuncCall::create(&getConsole));
    m->set("getLogic", SLB::FuncCall::create(&getLogic));
    m->set("getParticles", SLB::FuncCall::create(&getParticles));
    m->set("getPlayerController", SLB::FuncCall::create(&getPlayerController));
    m->set("execScriptDelayed", SLB::FuncCall::create(&execDelayedScript));
    m->set("pauseGame", SLB::FuncCall::create(&pauseGame));
    m->set("pauseEnemies", SLB::FuncCall::create(&pauseEnemies));
    m->set("deleteEnemies", SLB::FuncCall::create(&deleteEnemies));

    // Camera
    m->set("blendInCamera", SLB::FuncCall::create(&blendInCamera));
    m->set("blendOutCamera", SLB::FuncCall::create(&blendOutCamera));
    m->set("blendOutActiveCamera", SLB::FuncCall::create(&blendOutActiveCamera));

    // Player hacks
    m->set("pausePlayerToggle", SLB::FuncCall::create(&pausePlayerToggle));
    m->set("infiniteStamineToggle", SLB::FuncCall::create(&infiniteStamineToggle));
    m->set("immortal", SLB::FuncCall::create(&immortal));
    m->set("inShadows", SLB::FuncCall::create(&inShadows));
    m->set("speedBoost", SLB::FuncCall::create(&speedBoost));
    m->set("playerInvisible", SLB::FuncCall::create(&playerInvisible));
    m->set("noClipToggle", SLB::FuncCall::create(&noClipToggle));

    // postfx hacks
    m->set("postFXToggle", SLB::FuncCall::create(&postFXToggle));

    // Other
    m->set("lanternsDisable", SLB::FuncCall::create(&lanternsDisable));
    m->set("shadowsToggle", SLB::FuncCall::create(&shadowsToggle));
    m->set("debugToggle", SLB::FuncCall::create(&debugToggle));
    m->set("spawn", SLB::FuncCall::create(&spawn));
    m->set("bind", SLB::FuncCall::create(&bind));
    m->set("loadCheckpoint", SLB::FuncCall::create(&loadCheckpoint));
    m->set("loadScene", SLB::FuncCall::create(&loadScene));
    m->set("unloadScene", SLB::FuncCall::create(&unloadScene));
    m->set("cg_drawfps", SLB::FuncCall::create(&cg_drawfps));
    m->set("cg_drawlights", SLB::FuncCall::create(&cg_drawlights));
    m->set("renderNavmeshToggle", SLB::FuncCall::create(&renderNavmeshToggle));
    m->set("playSound2D", SLB::FuncCall::create(&playSound2D));
    m->set("exeShootImpactSound", SLB::FuncCall::create(&exeShootImpactSound));
    m->set("sleep", SLB::FuncCall::create(&sleep));
    m->set("cinematicModeToggle", SLB::FuncCall::create(&cinematicModeToggle));

    /* Only for debug */
    m->set("sendOrderToDrone", SLB::FuncCall::create(&sendOrderToDrone));
    m->set("toggle_spotlight", SLB::FuncCall::create(&toggle_spotlight));
    m->set("toggleButtonCanBePressed", SLB::FuncCall::create(&toggleButtonCanBePressed));
}

/* Check if it is a fast format command */
void CModuleLogic::execCvar(std::string& script) {

    // Only backslash commands fetched.
    if (script.find("/") != 0)
        return;

    // Little bit of dirty tricks to achieve same results with different string types.
    script.erase(0, 1);
    int index = (int)script.find_first_of(' ');
    script = index != -1 ? script.replace(script.find_first_of(' '), 1, "(") : script;
    index = (int)script.find_first_of(' ');
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
    case Events::SCENE_START:
        if (delay > 0) {
            return execScriptDelayed("onSceneStart()", delay) && execScriptDelayed("onSceneStart_" + params + "()", delay);
        }
        else {
            return execScript("onSceneStart()").success && execScript("onSceneStart_" + params + "()").success;
        }
        break;
    case Events::SCENE_END:
        if (delay > 0) {
            return execScriptDelayed("onSceneEnd()", delay) && execScriptDelayed("onSceneEnd_" + params + "()", delay);
        }
        else {
            return execScript("onSceneEnd()").success && execScript("onSceneEnd_" + params + "()").success;
        }
        break;

    case Events::TRIGGER_ENTER:
        if (delay > 0) {
            return execScriptDelayed("onTriggerEnter_" + params, delay);
        }
        else {
            return execScript("onTriggerEnter_" + params).success;
        }
        break;
    case Events::TRIGGER_EXIT:
        if (delay > 0) {
            return execScriptDelayed("onTriggerExit_" + params, delay);
        }
        else {
            return execScript("onTriggerExit_" + params).success;
        }
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

CModuleParticles * getParticles() { return EngineParticles.getPointer(); }

TCompTempPlayerController * getPlayerController()
{
    TCompTempPlayerController * playerController = nullptr;
    CEntity* e = getEntityByName("The Player");
    if (e) {
        playerController = e->get<TCompTempPlayerController>();
    }
    return playerController;
}
CModuleGameConsole * getConsole() { return EngineConsole.getPointer(); }

void execDelayedScript(const std::string& script, float delay)
{
    EngineLogic.execScriptDelayed(script, delay);
}

void pauseEnemies(bool pause) {

    std::vector<CHandle> enemies = CTagsManager::get().getAllEntitiesByTag(getID("enemy"));
    TMsgAIPaused msg;
    msg.isPaused = pause;
    for (int i = 0; i < enemies.size(); i++) {
        enemies[i].sendMsg(msg);
    }
}

void deleteEnemies()
{
    VHandles enemies = CTagsManager::get().getAllEntitiesByTag(getID("enemy"));
    for (auto h : enemies) {
        h.destroy();
    }
}

void pauseGame(bool pause) {

    TMsgScenePaused msg;
    msg.isPaused = pause;
    EngineEntities.broadcastMsg(msg);
}

void infiniteStamineToggle() {
    TMsgInfiniteStamina msg;
    CHandle h = getEntityByName("The Player");
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
}

void noClipToggle()
{
    CHandle h = getEntityByName("The Player");
    TMsgSystemNoClipToggle msg;
    h.sendMsg(msg);
}

void lanternsDisable(bool disable) {
    VHandles patrols = CTagsManager::get().getAllEntitiesByTag(getID("patrol"));
    TMsgLanternsDisable msg{ disable };
    for (auto h : patrols) {
        h.sendMsg(msg);
    }
}

void blendInCamera(const std::string & cameraName, float blendInTime) {

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

void blendOutActiveCamera(float blendOutTime) {
    EngineCameras.blendOutCamera(EngineCameras.getCurrentCamera(), blendOutTime);
}

/* Spawn item on given position */
void spawn(const std::string & name, const VEC3 & pos) {


}

void loadScene(const std::string &level) {
    EngineScene.loadScene(level);
}

void unloadScene() {
    EngineScene.unLoadActiveScene();
}

void sleep(float time) {
    Sleep(time);
}

void cinematicModeToggle() {
    TMsgPlayerAIEnabled msg;
    CHandle h = getEntityByName("The Player");
    h.sendMsg(msg);
}

void activateScene(const std::string& scene) {
    //EngineScene.setActiveScene()
}

void loadCheckpoint()
{
    CModuleGameManager gameManager = CEngine::get().getGameManager();
    gameManager.loadCheckpoint();
}

void shadowsToggle()
{
    EngineRender.setGenerateShadows(!EngineRender.getGenerateShadows());
}

void postFXToggle() {
    //Deactivating AO
    EngineRender.setGeneratePostFX(!EngineRender.getGeneratePostFX());

    //Deactivating rest of postFX
    getObjectManager<TCompRenderAO>()->forEach([&](TCompRenderAO* c) {
        c->setState(!c->getState());
    });
}

void pausePlayerToggle() {
    CEntity* p = getEntityByName("The Player");
    TCompTempPlayerController* player = p->get<TCompTempPlayerController>();

    TMsgScenePaused stopPlayer;
    stopPlayer.isPaused = !player->paused;
    EngineEntities.broadcastMsg(stopPlayer);
}

void debugToggle()
{
    EngineRender.setDebugMode(!EngineRender.getDebugMode());
}

void destroy() {


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

void renderNavmeshToggle() {
    EngineNavmeshes.renderNamvesh = !EngineNavmeshes.renderNamvesh;
}

// Toggle CVARS.
void cg_drawfps(bool value) {
    CEngine::get().getGameManager().config.drawfps = value;
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

void playSound2D(const std::string& soundName) {
    EngineSound.playSound2D(soundName);
}

void exeShootImpactSound() {
    EngineSound.exeShootImpactSound();
}

void sendOrderToDrone(const std::string & droneName, VEC3 position)
{
    CEntity* drone = getEntityByName(droneName);
    TMsgOrderReceived msg;
    msg.position = position;
    msg.hOrderSource = getEntityByName("The Player");
    drone->sendMsg(msg);
}

void toggle_spotlight(const std::string & lightName)
{
    CHandle hLight = getEntityByName(lightName);
    if (!hLight.isValid()) {
        return;
    }
    CEntity* light = hLight;
    TCompLightSpot* spotlight = light->get<TCompLightSpot>();
    spotlight->isEnabled = !spotlight->isEnabled;
}

void toggleButtonCanBePressed(const std::string & buttonName, bool canBePressed)
{
    CHandle hButton = getEntityByName(buttonName);
    if (!hButton.isValid()) {
        return;
    }
    CEntity* button = hButton;
    TCompButton* comp_button = button->get<TCompButton>();
    comp_button->canBePressed = canBePressed;
}
