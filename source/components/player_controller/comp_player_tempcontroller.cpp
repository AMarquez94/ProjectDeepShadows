#include "mcv_platform.h"
#include "comp_player_tempcontroller.h"
#include "components/comp_fsm.h"
#include "components/comp_tags.h"
#include "components/comp_render.h"
#include "components/ia/comp_bt_patrol.h"
#include "components/ia/comp_bt_mimetic.h"
#include "components/comp_transform.h"
#include "components/physics/comp_rigidbody.h"
#include "components/physics/comp_collider.h"
#include "components/player_controller/comp_shadow_controller.h"
#include "components/player_controller/comp_player_attack_cast.h"
#include "components/lighting/comp_emission_controller.h"
#include "components/player_controller/comp_sonar_controller.h"
#include "components/object_controller/comp_noise_emitter.h"
#include "components/comp_particles.h"
#include "physics/physics_collider.h"
#include "render/mesh/mesh_loader.h"
#include "components/comp_name.h"
#include "windows/app.h"
#include "comp_player_input.h"
#include "components/comp_group.h"
#include "render/render_utils.h"

#include "render/render_objects.h"
#include "render/render_utils.h"

DECL_OBJ_MANAGER("player_tempcontroller", TCompTempPlayerController);

void TCompTempPlayerController::debugInMenu() {
}

void TCompTempPlayerController::renderDebug() {
    /*
    //UI Window's Size
    ImGui::SetNextWindowSize(ImVec2((float)CApp::get().xres, (float)CApp::get().yres), ImGuiCond_Always);
    //UI Window's Position
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    //Transparent background - ergo alpha = 0 (RGBA)
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    //Some style added
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255.0f, 255.0f, 0.0f, 1.0f));

    ImGui::Begin("UI", NULL,
        ImGuiWindowFlags_::ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetCursorPos(ImVec2(CApp::get().xres * 0.02f, CApp::get().yres * 0.06f));
        ImGui::Text("Stamina:");
        ImGui::SetCursorPos(ImVec2(CApp::get().xres * 0.05f + 25, CApp::get().yres * 0.05f));
        ImGui::ProgressBar(stamina / maxStamina, ImVec2(CApp::get().xres / 5.f, CApp::get().yres / 30.f));
        ImGui::Text("State: %s", dbCameraState.c_str());
    //ImGui::Text("VECTOR DIR: (%f - %f - %f)", debugDir.x, debugDir.y, debugDir.z);
    //ImGui::Text("VECTOR FRONT: (%f - %f - %f)", debugMyFront.x, debugMyFront.y, debugMyFront.z);

    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);*/
}

void TCompTempPlayerController::load(const json& j, TEntityParseContext& ctx) {

    state = (actionhandler)&TCompTempPlayerController::idleState;

    auto pj_idle = loadMesh("data/meshes/pj_idle.mesh");
    auto pj_attack = loadMesh("data/meshes/pj_attack.mesh");
    auto pj_fall = loadMesh("data/meshes/pj_fall.mesh");
    auto pj_walk = loadMesh("data/meshes/pj_walk.mesh");
    auto pj_run = loadMesh("data/meshes/pj_run.mesh");
    auto pj_crouch = loadMesh("data/meshes/pj_crouch.mesh");
    auto pj_shadowmerge = Resources.get("axis.mesh")->as<CRenderMesh>();

    playerColor.colorIdle = j.count("colorIdle") ? loadVEC4(j["colorIdle"]) : VEC4(1, 1, 1, 1);
    playerColor.colorDead = j.count("colorDead") ? loadVEC4(j["colorDead"]) : VEC4(0, 0, 0, 1);
    playerColor.colorInhib = j.count("colorInhib") ? loadVEC4(j["colorInhib"]) : VEC4(1, 0, 1, 1);
    playerColor.colorMerge = j.count("colorMerge") ? loadVEC4(j["colorMerge"]) : VEC4(0, 1, 1, 1);

    mergeAngle = j.value("mergeAngle", 0.45f);
    maxFallingTime = j.value("maxFallingTime", 0.8f);
    hardFallingTime = j.value("hardFallingTime", 0.5f);
    maxFallingDistance = j.value("maxFallingDistance", 1.5f);
    maxAttackDistance = j.value("maxAttackDistance", 1.f);
    minStamina = j.value("minStamina", 0.f);
    maxStamina = j.value("maxStamina", 100.f);
    incrStamina = j.value("incrStamina", 15.f);
    decrStaticStamina = j.value("decrStaticStamina", 0.75f),
    decrStaminaHorizontal = j.value("decrStaminaHorizontal", 12.5f);
    decrStaminaVertical = j.value("decrStaminaVertical", 17.5f);
    minStaminaChange = j.value("minStaminaChange", 15.f);
    auxCamera = j.value("auxCamera", "");
    timesRemoveInhibitorKeyPressed = j.value("timesRemoveInhibitorKeyPressed", -1);
    paused = true;
    canAttack = false;
    canRemoveInhibitor = false;
}

/* Player controller main update */
void TCompTempPlayerController::update(float dt) {

    if (!paused && !isConsoleOn && !isInNoClipMode) {

        (this->*state)(dt);

		// Methods that always must be running on background
		isGrounded = groundTest(dt);
		isMerged = onMergeTest(dt);
		updateStamina(dt);
        updateLife(dt);
		updateShader(dt); // Move this to player render component...
		timeInhib += dt;
		canAttack = canAttackTest(dt);
		Engine.getGUI().getVariables().setVariant("staminaBarFactor", stamina / maxStamina);	 
        Engine.getGUI().getVariables().setVariant("lifeBarFactor", life / maxLife);
    }

    // Update player global speed into the shader.
    float inputSpeed = Clamp(fabs(EngineInput["Horizontal"].value) + fabs(EngineInput["Vertical"].value), 0.f, 1.f);
    cb_globals.global_player_speed = (inputSpeed * currentSpeed) / 6.f; // Maximum speed, change this in the future. 
    //cb_globals.updateGPU();
}

void TCompTempPlayerController::registerMsgs() {

    DECL_MSG(TCompTempPlayerController, TMsgStateStart, onStateStart);
    DECL_MSG(TCompTempPlayerController, TMsgStateFinish, onStateFinish);
    DECL_MSG(TCompTempPlayerController, TMsgEntityCreated, onCreate);
    DECL_MSG(TCompTempPlayerController, TMsgPlayerHit, onPlayerHit);
    DECL_MSG(TCompTempPlayerController, TMsgPlayerDead, onPlayerKilled);
    DECL_MSG(TCompTempPlayerController, TMsgInhibitorShot, onPlayerInhibited);
    DECL_MSG(TCompTempPlayerController, TMsgPlayerIlluminated, onPlayerExposed);
    DECL_MSG(TCompTempPlayerController, TMsgScenePaused, onPlayerPaused);
    DECL_MSG(TCompTempPlayerController, TMsgConsoleOn, onConsoleChanged);
    DECL_MSG(TCompTempPlayerController, TMsgShadowChange, onShadowChange);
    DECL_MSG(TCompTempPlayerController, TMsgInfiniteStamina, onInfiniteStamina);
    DECL_MSG(TCompTempPlayerController, TMsgPlayerImmortal, onPlayerImmortal);
    DECL_MSG(TCompTempPlayerController, TMsgPlayerInShadows, onPlayerInShadows);
    DECL_MSG(TCompTempPlayerController, TMsgSpeedBoost, onSpeedBoost);
    DECL_MSG(TCompTempPlayerController, TMsgPlayerInvisible, onPlayerInvisible);
    DECL_MSG(TCompTempPlayerController, TMsgNoClipToggle, onMsgNoClipToggle);
    DECL_MSG(TCompTempPlayerController, TMsgBulletHit, onMsgBulletHit);
}

void TCompTempPlayerController::onShadowChange(const TMsgShadowChange& msg) {

    //VEC4 merged_color = msg.is_shadowed ? playerColor.colorMerge : playerColor.colorIdle;

    //TCompEmissionController * e_controller = get<TCompEmissionController>();
    //e_controller->blend(merged_color, .5);
}

void TCompTempPlayerController::onInfiniteStamina(const TMsgInfiniteStamina & msg)
{
    infiniteStamina = !infiniteStamina;
}

void TCompTempPlayerController::onPlayerImmortal(const TMsgPlayerImmortal & msg)
{
    isImmortal = !isImmortal;
}

void TCompTempPlayerController::onPlayerInShadows(const TMsgPlayerInShadows & msg)
{
    hackShadows = !hackShadows;
}

void TCompTempPlayerController::onSpeedBoost(const TMsgSpeedBoost& msg) {
    speedBoost = msg.speedBoost;
}

void TCompTempPlayerController::onPlayerInvisible(const TMsgPlayerInvisible & msg)
{
    isInvisible = !isInvisible;
}

void TCompTempPlayerController::onMsgNoClipToggle(const TMsgNoClipToggle & msg)
{
    isInNoClipMode = !isInNoClipMode;
}

void TCompTempPlayerController::onMsgBulletHit(const TMsgBulletHit & msg)
{
    getDamage(msg.damage);
}

void TCompTempPlayerController::onCreate(const TMsgEntityCreated& msg) {

    /* Variable initialization */
    TCompCollider * c_my_collider = get<TCompCollider>();

    pxShadowFilterData = new physx::PxFilterData();
    pxShadowFilterData->word0 = c_my_collider->config->group;
    pxShadowFilterData->word1 = FilterGroup::Wall;

    pxPlayerFilterData = new physx::PxFilterData();
    pxPlayerFilterData->word0 = c_my_collider->config->group;
    pxPlayerFilterData->word1 = FilterGroup::All;

    physx::PxFilterData pxFilterData;
    pxFilterData.word1 = FilterGroup::Scenario;
    //pxFilterData.word1 = FilterGroup::Scenario | FilterGroup::MovableObject/*  | FilterGroup::MovableObject | FilterGroup::DItem*/;
    PxPlayerDiscardQuery.data = pxFilterData;

    /* Initial reset messages */
    hitPoints = 0;
    stamina = 100.f;
    fallingTime = 0.f;
    currentSpeed = 4.f;
    initialTimesToPressInhibitorRemoveKey = 5;
    rotationSpeed = 10.f;
    fallingDistance = 0.f;
    isInhibited = isGrounded = isMerged = false;
    infiniteStamina = false;
    paused = false;

    temp_deg = 0;
    temp_invert = VEC3::One;

    TCompParticles * c_e_particle = get<TCompParticles>();
    assert(c_e_particle);
    c_e_particle->setSystemState(false);
}

/* Call this function once the state has been changed */
void TCompTempPlayerController::onStateStart(const TMsgStateStart& msg) {

    if (msg.action_start != NULL) {

        state = msg.action_start;
        currentSpeed = msg.speed * speedBoost;

        TCompRigidbody * rigidbody = get<TCompRigidbody>();
        TCompTransform * t_trans = get<TCompTransform>();
        rigidbody->Resize(msg.size);

        physx::PxCapsuleController* caps = (physx::PxCapsuleController*)rigidbody->controller;
        if (caps != nullptr) {
            caps->setRadius(msg.radius);
            caps->setFootPosition(physx::PxExtendedVec3(t_trans->getPosition().x, t_trans->getPosition().y, t_trans->getPosition().z));
        }

        // Get the target camera and set it as our new camera.
        if (msg.target_camera) {
            CHandle new_camera = getEntityByName(msg.target_camera->name);
            if (new_camera != target_camera) {
                Engine.getCameras().blendOutCamera(target_camera, msg.target_camera->blendOut);
            }
            target_camera = new_camera;
            Engine.getCameras().blendInCamera(target_camera, msg.target_camera->blendIn, CModuleCameras::EPriority::GAMEPLAY);
        }
        else {
            target_camera = getEntityByName("TPCamera"); //replace this
        }

        /* Noise emitter */
        TCompNoiseEmitter * noiseEmitter = get<TCompNoiseEmitter>();
        noiseEmitter->makeNoise(msg.noise->noiseRadius, msg.noise->timeToRepeat, msg.noise->isNoise, msg.noise->isOnlyOnce, msg.noise->isArtificial);
    }
}

/* Call this function once the state has finished */
void TCompTempPlayerController::onStateFinish(const TMsgStateFinish& msg) {

    (this->*msg.action_finish)();
}

void TCompTempPlayerController::onPlayerHit(const TMsgPlayerHit & msg)
{
    die();
}

void TCompTempPlayerController::onPlayerKilled(const TMsgPlayerDead & msg)
{
    die();
}

void TCompTempPlayerController::onPlayerInhibited(const TMsgInhibitorShot & msg)
{
    if (!isInhibited) {
        isInhibited = true;
    }
    timesRemoveInhibitorKeyPressed = initialTimesToPressInhibitorRemoveKey;
}

void TCompTempPlayerController::onPlayerExposed(const TMsgPlayerIlluminated & msg)
{
    if (isMerged && msg.isIlluminated) {
        CEntity* e = CHandle(this).getOwner();
        TMsgSetFSMVariable notMergeMsg;
        notMergeMsg.variant.setName("onmerge");
        notMergeMsg.variant.setBool(false); // & isGrounded
        isMerged = false;
        e->sendMsg(notMergeMsg);
    }
}

void TCompTempPlayerController::onPlayerPaused(const TMsgScenePaused& msg) {

    paused = msg.isPaused;
}

void TCompTempPlayerController::onConsoleChanged(const TMsgConsoleOn & msg)
{
    isConsoleOn = msg.isConsoleOn;
}

/* Idle state method, no logic yet */
void TCompTempPlayerController::idleState(float dt) {

}

/* Main thirdperson player motion movement handled here */
void TCompTempPlayerController::walkState(float dt) {

    // Player movement and rotation related method.
    float yaw, pitch, roll;
    CEntity *player_camera = target_camera;
    TCompTransform *c_my_transform = get<TCompTransform>();
    TCompTransform * trans_camera = player_camera->get<TCompTransform>();
    c_my_transform->getYawPitchRoll(&yaw, &pitch, &roll);

    //float inputSpeed = Clamp(fabs(EngineInput["Horizontal"].value) + fabs(EngineInput["Vertical"].value), 0.f, 1.f);
    float player_accel = currentSpeed * dt;

    VEC3 up = trans_camera->getFront();
    VEC3 normal_norm = c_my_transform->getUp();
    VEC3 proj = projectVector(up, normal_norm);
    VEC3 dir = getMotionDir(proj, normal_norm.Cross(-proj));

    //TODO: Destroy this shiat-----------------------------------------------------------------------

    float factor = 1.0f;
    if (EngineInput["btRun"].isPressed()) factor = 0.95f;

    if (currentSpeed <= 1.8f) {
        factor = 1.3f;
    }
    stepTimer += dt;
    if (stepTimer > 0.33f * factor) {

        Engine.getSound().exeStepSound();
        stepRight = !stepRight;
        stepTimer = 0.0f;
    }
    //Destroy this shiat-----------------------------------------------------------------------

    if (dir == VEC3::Zero) dir = proj;

    float dir_yaw = getYawFromVector(dir);
    Quaternion my_rotation = c_my_transform->getRotation();
    Quaternion new_rotation = Quaternion::CreateFromYawPitchRoll(dir_yaw, pitch, 0);
    Quaternion quat = Quaternion::Lerp(my_rotation, new_rotation, rotationSpeed * dt);
    c_my_transform->setRotation(quat);
    c_my_transform->setPosition(c_my_transform->getPosition() + dir * player_accel);
}

/* Player motion movement when is shadow merged, tests included */
void TCompTempPlayerController::mergeState(float dt) {

    // Determine player speed
    float inputSpeed = Clamp(fabs(EngineInput["Horizontal"].value) + fabs(EngineInput["Vertical"].value), 0.f, 1.f);
    float player_accel = inputSpeed * currentSpeed * dt;

    // Player movement and rotation related method.
    CEntity* player_camera = target_camera;
    TCompRigidbody* rigidbody = get<TCompRigidbody>();
    TCompTransform* p_transform = get<TCompTransform>();
    TCompTransform* c_transform = player_camera->get<TCompTransform>();

    float angle_test = fabs(EnginePhysics.gravity.Dot(p_transform->getUp()));
    VEC3 up = angle_test < mergeAngle ? -EnginePhysics.gravity : c_transform->getFront();

    VEC3 normal_norm = rigidbody->normal_gravity;
    normal_norm.Normalize();
    VEC3 proj = projectVector(up, normal_norm);
    VEC3 dir = getMotionDir(proj, normal_norm.Cross(proj));
    MAT44 temp_rot = MAT44::CreateFromAxisAngle(p_transform->getUp(), deg2rad(temp_deg));
    dir = VEC3::Transform(dir, temp_rot);

    VEC3 prevUp = p_transform->getUp();
    VEC3 new_pos = p_transform->getPosition() - dir;
    Matrix test = Matrix::CreateLookAt(p_transform->getPosition(), new_pos, p_transform->getUp()).Transpose();
    Quaternion quat = Quaternion::CreateFromRotationMatrix(test);
    p_transform->setRotation(quat);
    p_transform->setPosition(p_transform->getPosition() + dir * player_accel);

    if (/*player_accel != 0 && */(convexTest() || concaveTest())) {

        VEC3 postUp = p_transform->getUp();
        angle_test = fabs(EnginePhysics.gravity.Dot(prevUp));
        float angle_amount = fabsf(acosf(prevUp.Dot(postUp)));
        std::string target_name = angle_test > mergeAngle ? "SMCameraVer" : "SMCameraHor"; // WARN: Watch this if gives problems...  

        if (angle_amount > deg2rad(30.f) || target_name.compare(dbCameraState) != 0) {

            /* Only "change" cameras when the amount of degrees turned is more than 30� */
            CEntity* e_target_camera = target_camera;
            CEntity* e_camera = getEntityByName("SMCameraAux");
            TCompName* name = e_target_camera->get<TCompName>();
            VEC3 dirToLookAt = -(prevUp + postUp);
            dirToLookAt.Normalize();

            TMsgSetCameraActive msg;
            msg.previousCamera = name->getName();
            target_camera = getEntityByName(target_name);
            msg.actualCamera = target_name;
            msg.directionToLookAt = dirToLookAt;
            e_camera->sendMsg(msg);
        }
        dbCameraState = target_name;
    }
}

/* Resets the player to it's default state parameters */
void TCompTempPlayerController::resetState(float dt) {

    CEntity *player_camera = target_camera;
    TCompRigidbody *rigidbody = get<TCompRigidbody>();
    TCompTransform *c_my_transform = get<TCompTransform>();
    TCompTransform * trans_camera = player_camera->get<TCompTransform>();
    physx::PxCapsuleController* c_capsule = (physx::PxCapsuleController*)rigidbody->controller;

    VEC3 up = trans_camera->getFront();
    VEC3 proj = projectVector(up, -EnginePhysics.gravity);
    VEC3 dir = getMotionDir(proj, EnginePhysics.gravity.Cross(proj));

    // Set collider gravity settings
    rigidbody->SetUpVector(-EnginePhysics.gravity);
    rigidbody->normal_gravity = EnginePhysics.gravityMod * EnginePhysics.gravity;
    
    VEC3 new_pos = c_my_transform->getPosition() - dir;
    float mod_angle = (1 - abs(-EnginePhysics.gravity.Dot(c_my_transform->getUp())));
    VEC3 new_offset_pos = c_my_transform->getPosition() + 0.4f * mod_angle * c_my_transform->getUp();
    Matrix test = Matrix::CreateLookAt(c_my_transform->getPosition(), new_pos, -EnginePhysics.gravity).Transpose();
    Quaternion quat = Quaternion::CreateFromRotationMatrix(test);
    c_my_transform->setPosition(new_offset_pos);
    c_my_transform->setRotation(quat);
}

void TCompTempPlayerController::exitMergeState(float dt) {
    TMsgSetCameraCancelled msg;
    CEntity * eCamera = getEntityByName(auxCamera);
    eCamera->sendMsg(msg);
}

/* Player dead state */
void TCompTempPlayerController::deadState(float dt) {
    TMsgPlayerDead newMsg;
    newMsg.h_sender = CHandle(this).getOwner();
    auto& handles = CTagsManager::get().getAllEntitiesByTag(getID("enemy"));
    for (auto h : handles) {
        CEntity* enemy = h;
        enemy->sendMsg(newMsg);
    }

    state = (actionhandler)&TCompTempPlayerController::idleState;
}

void TCompTempPlayerController::removingInhibitorState(float dt) {

    CEntity* player = CHandle(this).getOwner();

    TMsgSetFSMVariable inhibitorTryToRemove;
    inhibitorTryToRemove.variant.setName("inhibitorTryToRemove");
    inhibitorTryToRemove.variant.setBool(false);
    player->sendMsg(inhibitorTryToRemove);

    TMsgSetFSMVariable finished;
    finished.variant.setName("inhibitor_removed");
    finished.variant.setBool(false);
    player->sendMsg(finished);

    if (timesRemoveInhibitorKeyPressed > 0) {

        timesRemoveInhibitorKeyPressed--;
        if (timesRemoveInhibitorKeyPressed <= 0) {

            timesRemoveInhibitorKeyPressed = initialTimesToPressInhibitorRemoveKey;
            isInhibited = false;

            TMsgSetFSMVariable finished;
            finished.variant.setName("inhibitor_removed");
            finished.variant.setBool(true);
            player->sendMsg(finished);
        }
    }

    state = (actionhandler)&TCompTempPlayerController::idleState;

}

void TCompTempPlayerController::movingObjectState(float dt)
{
  float player_accel = currentSpeed * dt;

  /* Set noise radius */
  TCompNoiseEmitter * noiseEmitter = get<TCompNoiseEmitter>();
  if (currentSpeed == 0) {
    noiseEmitter->changeNoiseRadius(0);
  }
  else {
    noiseEmitter->changeNoiseRadius(10);
  }

  TCompPlayerInput * player_input = get<TCompPlayerInput>();
  TCompTransform * player_trans = get<TCompTransform>();

  VEC3 front = player_trans->getFront();
  VEC3 dir = player_input->movementValue.y * front;

  markObjectAsMoving(true, dir, currentSpeed);

  player_trans->setPosition(player_trans->getPosition() + dir * currentSpeed * dt);
}

void TCompTempPlayerController::resetRemoveInhibitor()
{
    canRemoveInhibitor = true;
    timesRemoveInhibitorKeyPressed = initialTimesToPressInhibitorRemoveKey;
}

void TCompTempPlayerController::markObjectAsMoving(bool isBeingMoved, VEC3 newDirection, float newSpeed)
{
  TMsgObjectBeingMoved msg;
  msg.hMovingObject = CHandle(this).getOwner();
  if (isBeingMoved) {

    TCompPlayerAttackCast * tCast = get<TCompPlayerAttackCast>();
    CHandle newMovableObject = tCast->getClosestMovableObjectInRange();
    if (newMovableObject.isValid()) {
      TCompTransform* myPos = get<TCompTransform>();
      CEntity* eNewMovableObject = newMovableObject;
      TCompTransform *objectPos = eNewMovableObject->get<TCompTransform>();

      VEC3 ppos = myPos->getPosition();
      VEC3 opos = objectPos->getPosition();

      VEC3 diffPos = opos - ppos;

      if (fabsf(diffPos.x) > fabsf(diffPos.z)) {
        /* Player has to be moved in z axis */
        myPos->lookAt(myPos->getPosition() + VEC3(0, 0, diffPos.z), opos);
      }
      else {
        /* Player has to be moved in x axis */
        myPos->lookAt(myPos->getPosition() + VEC3(diffPos.x, 0, 0), opos);
      }

      movingObjectOffset = objectPos->getPosition() - myPos->getPosition();

      if (movableObject != newMovableObject || newDirection != directionMovableObject || newSpeed != movingObjectSpeed) {
        /* Object valid and some attributes changed */

        directionMovableObject = newDirection;
        movingObjectSpeed = newSpeed;
        msg.isBeingMoved = true;
        msg.direction = directionMovableObject;
        msg.speed = movingObjectSpeed;
        movableObject = newMovableObject;
        movableObject.sendMsg(msg);
      }
    }
  }
  else {
    msg.isBeingMoved = false;
    movableObject.sendMsg(msg);
    movableObject = CHandle();
    directionMovableObject = VEC3::Zero;
    movingObjectSpeed = 0;
  }
}


/* Method used to determine control invert */
void TCompTempPlayerController::invertAxis(VEC3 old_up, bool type) {

    // Refactor this with thew new player controller pad moves
    TCompTransform* p_transform = get<TCompTransform>();
    VEC3 temp_up = p_transform->getUp();
    bool pre_test = fabs(EnginePhysics.gravity.Dot(old_up)) < mergeAngle ? true : false;
    bool pos_test = fabs(EnginePhysics.gravity.Dot(temp_up)) < mergeAngle ? true : false;

    // Hardcoded a little bit, fix in the future if it fully works..
    if (type) {

        TCompPlayerInput *player_input = get<TCompPlayerInput>();
        if ((pos_test && !pre_test)) {

            VEC2 dir1 = VEC2(0, 1);
            VEC2 temp_dir = player_input->movementValue;
            //temp_dir.Normalize();
            float tdot = dir1.Dot(temp_dir);
            float dot_result = Clamp(dir1.Dot(temp_dir), -1.f, 1.f);
            float angle = acos(dot_result);
            if (temp_dir.x < 0) angle = -angle;

            temp_deg = rad2deg(angle);
        }
    }
    else {

        TCompPlayerInput *player_input = get<TCompPlayerInput>();
        if ((pos_test && !pre_test)) {

            VEC2 dir1 = VEC2(0, 1);
            VEC2 temp_dir = player_input->movementValue;
            //temp_dir.Normalize();
            float dot_result = Clamp(dir1.Dot(temp_dir), -1.f, 1.f);
            float angle = acos(dot_result);
            if (temp_dir.x < 0) angle = -angle;
            temp_deg = rad2deg(angle) - 180;
        }
    }
}

void TCompTempPlayerController::getDamage(float dmg)
{

    if (!isImmortal && !isDead()) {
        life = Clamp(life - dmg, 0.f, maxLife);
        timerSinceLastDamage = 0.f;

        if (life <= 0.f) {
            die();
        }
    }
}

void TCompTempPlayerController::die()
{
    if (!isImmortal && !isDead()) {
        CEntity* e = CHandle(this).getOwner();
        TMsgSetFSMVariable groundMsg;
        groundMsg.variant.setName("onDead");
        groundMsg.variant.setBool(true);
        e->sendMsg(groundMsg);
        life = 0;
    }
}

/* Concave test, this determines if there is a surface normal change on concave angles */
const bool TCompTempPlayerController::concaveTest(void) {

    physx::PxRaycastHit hit;
    TCompRigidbody *rigidbody = get<TCompRigidbody>();
    TCompTransform *c_my_transform = get<TCompTransform>();
    VEC3 old_up = c_my_transform->getUp();
    VEC3 upwards_offset = c_my_transform->getPosition() + c_my_transform->getUp() * .01f;

    if (EnginePhysics.Raycast(upwards_offset, c_my_transform->getFront(), 0.35f, hit, physx::PxQueryFlag::eSTATIC, PxPlayerDiscardQuery))
    {
        VEC3 hit_normal = VEC3(hit.normal.x, hit.normal.y, hit.normal.z);
        VEC3 hit_point = VEC3(hit.position.x, hit.position.y, hit.position.z);
        if (hit_normal == c_my_transform->getUp()) return false;

        if (EnginePhysics.gravity.Dot(hit_normal) < .01f)
        {
            VEC3 new_forward = hit_normal.Cross(c_my_transform->getLeft());
            VEC3 target = c_my_transform->getPosition() + new_forward;

            rigidbody->SetUpVector(hit_normal);
            rigidbody->normal_gravity = EnginePhysics.gravityMod * -hit_normal;

            Matrix test = Matrix::CreateLookAt(c_my_transform->getPosition(), target, hit_normal).Transpose();
            Quaternion new_rotation = Quaternion::CreateFromRotationMatrix(test);
            VEC3 new_pos = hit_point;
            c_my_transform->setRotation(new_rotation);
            c_my_transform->setPosition(new_pos);
            invertAxis(old_up, true);

            return true;
        }
    }

    return false;
}

/* Convex test, this determines if there is a surface normal change on convex angles */
const bool TCompTempPlayerController::convexTest(void) {

    physx::PxRaycastHit hit;
    TCompTransform *c_my_transform = get<TCompTransform>();
    TCompRigidbody *rigidbody = get<TCompRigidbody>();
    VEC3 old_up = c_my_transform->getUp();
    VEC3 upwards_offset = c_my_transform->getPosition() + old_up * .01f;
    VEC3 new_dir = old_up + c_my_transform->getFront();
    new_dir.Normalize();

    if (EnginePhysics.Raycast(upwards_offset, -new_dir, 0.95f, hit, physx::PxQueryFlag::eSTATIC, PxPlayerDiscardQuery))
    {
        VEC3 hit_normal = VEC3(hit.normal.x, hit.normal.y, hit.normal.z);
        VEC3 hit_point = VEC3(hit.position.x, hit.position.y, hit.position.z);
        if (hit_normal == c_my_transform->getUp()) return false;

        if (hit.distance > .015f && EnginePhysics.gravity.Dot(hit_normal) < .01f)
        {
            VEC3 new_forward = -hit_normal.Cross(c_my_transform->getLeft());
            VEC3 target = hit_point + new_forward;

            rigidbody->SetUpVector(hit_normal);
            rigidbody->normal_gravity = EnginePhysics.gravityMod * -hit_normal;

            QUAT new_rotation = createLookAt(hit_point, target, hit_normal);
            VEC3 new_pos = hit_point + 0.1f * new_forward; // Adding little offset
            c_my_transform->setRotation(new_rotation);
            c_my_transform->setPosition(new_pos);
            invertAxis(old_up, false);

            return true;
        }
    }

    return false;
}

/* Bitshifting test to determine if we are merged within the shadows */
const bool TCompTempPlayerController::onMergeTest(float dt) {

    CEntity* e = CHandle(this).getOwner();
    TCompRigidbody *c_my_rigidbody = get<TCompRigidbody>();
    TCompShadowController * shadow_oracle = get<TCompShadowController>();

    // Tests: inShadows + minStamina + grounded + button hold -> sent to fsm
    bool mergeTest = true;
    bool mergefall = fallingDistance > 0 && fallingDistance < maxFallingDistance;

    if (!hackShadows) {
        mergeTest &= shadow_oracle->is_shadow;
    }
    mergeTest &= stamina > minStamina;
    mergeTest &= !isInhibited;

    // If we are not merged.
    if (!isMerged) {
        mergeTest &= stamina > minStaminaChange;
        mergeTest &= EngineInput["btShadowMerging"].hasChanged();

        TMsgSetFSMVariable onFallMsg;
        onFallMsg.variant.setName("onFallMerge");
        mergefall &= mergeTest;
        mergefall &= EngineInput["btShadowMerging"].isPressed();
        onFallMsg.variant.setBool(mergefall);
        e->sendMsg(onFallMsg);
    }

    mergeTest &= EngineInput["btShadowMerging"].isPressed();
    mergeTest &= isGrounded;
    mergeTest &= !isDead();

    // If the mergetest changed since last frame, update the fsm
    if (mergeTest != isMerged) {

        TMsgSetFSMVariable groundMsg;
        groundMsg.variant.setName("onmerge");
        groundMsg.variant.setBool(mergeTest);
        e->sendMsg(groundMsg);
        c_my_rigidbody->filters.mFilterData = isMerged == true ? pxPlayerFilterData : pxShadowFilterData;
    }

    return mergeTest;
}

/* Players logic depending on ground state */
const bool TCompTempPlayerController::groundTest(float dt) {

    TCompRigidbody *c_my_collider = get<TCompRigidbody>();

    if (isGrounded != c_my_collider->is_grounded) {

        CEntity* e = CHandle(this).getOwner();

        TMsgSetFSMVariable groundMsg;
        groundMsg.variant.setName("onGround");
        groundMsg.variant.setBool(c_my_collider->is_grounded);
        e->sendMsg(groundMsg);

        TMsgSetFSMVariable falldead;
        falldead.variant.setName("onFallDead");
        falldead.variant.setBool(c_my_collider->is_grounded & (fallingTime > maxFallingTime) & !isMerged);
        e->sendMsg(falldead);

        TMsgSetFSMVariable hardLanded;
        hardLanded.variant.setName("onHardLanded");
        hardLanded.variant.setBool(c_my_collider->is_grounded & (fallingTime > hardFallingTime && fallingTime < maxFallingTime) & !isMerged);
        if (hardLanded.variant.getBool()) {
            getDamage(30.f);
        }
        e->sendMsg(hardLanded);

        TMsgSetFSMVariable crouch;
        crouch.variant.setName("crouch");
        crouch.variant.setBool(false);
        e->sendMsg(crouch);

        //dbg("falling time %f\n", fallingTime);
        fallingTime = 0.f;
    }

    // Get distance to ground
    // Compute falling time
    if (!isGrounded) {

        fallingTime += dt;

        physx::PxRaycastHit hit;
        TCompTransform *c_my_transform = get<TCompTransform>();
        if (EnginePhysics.Raycast(c_my_transform->getPosition(), -c_my_transform->getUp(), 1000, hit, physx::PxQueryFlag::eSTATIC, PxPlayerDiscardQuery)) {
            fallingDistance = hit.distance;
        }
    }

    return c_my_collider->is_grounded;
}

const bool TCompTempPlayerController::canAttackTest(float dt)
{
    bool canAttackNow = false;
    if (!isDead() && !isMerged && isGrounded) {
        TCompPlayerAttackCast* comp_attack_cast = get<TCompPlayerAttackCast>();
        canAttackNow = comp_attack_cast->canAttackEnemiesInRange();
    }

    return canAttackNow;
}

const bool TCompTempPlayerController::canSonarPunch()
{
    TCompSonarController * sonar = get < TCompSonarController>();
    return sonar->canDeploySonar();
}

/* Sets the player current stamina depending on player status */
void TCompTempPlayerController::updateStamina(float dt) {

    if (isMerged) {

        // Determine stamina decreasing ratio multiplier depending on movement
        TCompRigidbody *c_my_rigidbody = get<TCompRigidbody>();
        TCompTransform *c_my_transform = get<TCompTransform>();
        float staminaMultiplier = c_my_rigidbody->lastFramePosition == c_my_transform->getPosition() ? decrStaticStamina : 1;

        if (!infiniteStamina) {
            // Determine stamina decreasing ratio depending on players up vector.
            if (fabs(EnginePhysics.gravity.Dot(c_my_rigidbody->GetUpVector())) < mergeAngle) {
                stamina = Clamp(stamina - (decrStaminaVertical * staminaMultiplier * dt), minStamina, maxStamina);
            }
            else {
                stamina = Clamp(stamina - (decrStaminaHorizontal * staminaMultiplier * dt), minStamina, maxStamina);
            }
        }
    }
    else {
        stamina = Clamp(stamina + (incrStamina * dt), minStamina, maxStamina);
    }
}

/* Attack state, kills the closest enemy if true*/
void TCompTempPlayerController::attackState(float dt) {

    if (attackTimer > 0.7f) {   //TODO: Remove this. Only a fix for milestone 2
        TCompPlayerAttackCast * cAttackCast = get<TCompPlayerAttackCast>();
        CHandle closestEnemy;
        bool enemyFound = cAttackCast->canAttackEnemiesInRange(closestEnemy);

        if (enemyFound) {
            TMsgEnemyStunned msg;
            msg.h_sender = CHandle(this).getOwner();
            closestEnemy.sendMsg(msg);
        }

        attackTimer = 0.f;
        state = (actionhandler)&TCompTempPlayerController::idleState;
    }
    else {
        attackTimer += dt;
    }
}

/* Attack state, kills the closest enemy if true*/
void TCompTempPlayerController::mergeEnemy() {

    TCompPlayerAttackCast * tAttackCast = get<TCompPlayerAttackCast>();
    CHandle enemy = tAttackCast->closestEnemyToMerge();
    if (isMerged) {
        if (enemy.isValid()) {
            TMsgPatrolShadowMerged msg;
            msg.h_sender = CHandle(this).getOwner();
            msg.h_objective = enemy;
            enemy.sendMsg(msg);
        }
    }
}

/* Temporal function to determine our player shadow color, set this to a shader change..*/
void TCompTempPlayerController::updateShader(float dt) {

    TCompRender *c_my_render = get<TCompRender>();
    TCompEmissionController *e_controller = get<TCompEmissionController>();
    TCompShadowController * shadow_oracle = get<TCompShadowController>();

    if (isDead()) {
        e_controller->blend(playerColor.colorDead, 1.f);
    }
    else if (canAttack) {       //TEMP: TODO: Delete
        e_controller->blend(VEC4(1.f, 0.f, 0.f, 1.f), 0.1f);
    }
    else if (isInhibited) {
        e_controller->blend(playerColor.colorInhib, 0.1f);
    }
    else if (shadow_oracle->is_shadow) {
        e_controller->blend(playerColor.colorMerge, 0.5f);
    }
    else {
        e_controller->blend(playerColor.colorIdle, 0.5f);
    }
}

void TCompTempPlayerController::updateLife(float dt)
{
    TCompShadowController *shadow_oracle = get<TCompShadowController>();
    if (shadow_oracle->is_shadow) {
        timerSinceLastDamage = Clamp(timerSinceLastDamage + dt, 0.f, timeToStartRecoverFromDamage);
        if (timerSinceLastDamage >= timeToStartRecoverFromDamage && !isDead()) {
            life = Clamp(life + lifeIncr * dt, 0.f, maxLife);
        }
    }
}

VEC3 TCompTempPlayerController::getMotionDir(const VEC3 & front, const VEC3 & left) {

    VEC3 dir = VEC3::Zero;
    TCompPlayerInput *player_input = get<TCompPlayerInput>();

    dir += player_input->movementValue.y * front;
    dir += player_input->movementValue.x * left;
    dir.Normalize();

    if (dir == VEC3::Zero) return front;

    return dir;
}

/* Auxiliary functions */

void TCompTempPlayerController::upButtonReselased() {

    temp_deg = 0;
    temp_invert = VEC3::One;
}

bool TCompTempPlayerController::isDead()
{
    TCompFSM *fsm = get<TCompFSM>();
    return fsm->getStateName().compare("dead") == 0 || fsm->getStateName().compare("die") == 0;
}

// Needed to avoid the isGround problem by now
void TCompTempPlayerController::resetMerge() {

    isMerged = true;
    CEntity* e = CHandle(this).getOwner();

    TMsgSetFSMVariable groundMsg;
    groundMsg.variant.setName("onmerge");
    groundMsg.variant.setBool(true);
    e->sendMsg(groundMsg);

    TMsgSetFSMVariable fallMsg;
    fallMsg.variant.setName("onFallDead");
    fallMsg.variant.setBool(false);
    e->sendMsg(fallMsg);

    TMsgSetFSMVariable hardFallMsg;
    hardFallMsg.variant.setName("onHardLanded");
    hardFallMsg.variant.setBool(false);
    e->sendMsg(hardFallMsg);
}