#include "mcv_platform.h"
#include "comp_camera_flyover.h"
#include "components/comp_transform.h"

DECL_OBJ_MANAGER("camera_flyover", TCompCameraFlyover);

void TCompCameraFlyover::debugInMenu() {
	ImGui::DragFloat("Speed", &_speed, 10.f, 1.f, 100.f);
	ImGui::DragFloat("Sensitivity", &_sensitivity, 0.01f, 0.001f, 0.1f);
}

void TCompCameraFlyover::load(const json& j, TEntityParseContext& ctx) {
	_speed = j.value("speed", _speed);
	_sensitivity = j.value("sensitivity", _sensitivity);
}

void TCompCameraFlyover::update(float dt)
{
	TCompTransform* c_transform = get<TCompTransform>();
	VEC3 pos = c_transform->getPosition();
	VEC3 front = c_transform->getFront();
	VEC3 left = c_transform->getLeft();
	VEC3 up = VEC3::UnitY;

	if (btDebugPause.getsPressed()) {
		paused = !paused;
		if (paused) {
			Engine.getCameras().blendInCamera(CHandle(this).getOwner(), 1.f, CModuleCameras::EPriority::TEMPORARY);
		}
		else {
			Engine.getCameras().blendOutCamera(CHandle(this).getOwner(), 1.f);
		}
	}

	if (paused) {

		// movement
		float deltaSpeed = _speed * dt;
		if (EngineInput["turbo"].isPressed())
			deltaSpeed *= 2.f;
		VEC3 off;
		off += front * EngineInput["btUp"].value * deltaSpeed;
		off += -front * EngineInput["btDown"].value * deltaSpeed;
		off += left * EngineInput["btLeft"].value * deltaSpeed;
		off += -left * EngineInput["btRight"].value * deltaSpeed;
		off += up * EngineInput["up"].value * deltaSpeed;
		off += -up * EngineInput["down"].value * deltaSpeed;

		// rotation
		float yaw, pitch;
		getYawPitchFromVector(front, &yaw, &pitch);

		auto& mouse = EngineInput.mouse();
		if (mouse.button(Input::MOUSE_RIGHT).isPressed())
		{
			VEC2 mOff = mouse._position_delta;
			yaw += -mOff.x * _sensitivity;
			pitch += -mOff.y * _sensitivity;
			pitch = Clamp(pitch, -_maxPitch, _maxPitch);
		}

		// final values
		VEC3 newPos = pos + off;
		VEC3 newFront = getVectorFromYawPitch(yaw, pitch);

		c_transform->lookAt(newPos, newPos + newFront);

		if (btDebugGetPos.getsPressed()) {
			dbg("Camera pos - (%f,%f,%f)", newPos.x, newPos.y, newPos.z);
		}
	}
}
