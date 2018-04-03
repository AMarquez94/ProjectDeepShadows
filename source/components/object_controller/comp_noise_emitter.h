#pragma once

#include "components/comp_base.h"

class TCompNoiseEmitter : public TCompBase
{
  DECL_SIBLING_ACCESS();

public:
  void debugInMenu();
  void load(const json& j, TEntityParseContext& ctx);
  void update(float dt);

  static void registerMsgs();

private:

  CHandle _hSource;
  float _timeToMakeNoise;
  float _timer = 0.f;
  bool _once = false;
  bool _onceNoiseMade = false;
  std::vector<CHandle> hEntitiesInNoiseRadius;

  void onMsgTriggerEnter(const TMsgTriggerEnter& msg);
  void onMsgTriggerExit(const TMsgTriggerExit& msg);
  void onMsgMakeNoise(const TMsgMakeNoise& msg);

  void resizeEmitter(float radius);
};

