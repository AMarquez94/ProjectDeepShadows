#include "mcv_platform.h"
#include "comp_animator.h"
#include "components/comp_transform.h"
#include "components/comp_fsm.h"

DECL_OBJ_MANAGER("animator", TCompAnimator);

void TCompAnimator::debugInMenu() {
	
}

void TCompAnimator::load(const json& j, TEntityParseContext& ctx) {
	_animationName = "";

}

void TCompAnimator::initializeAnimations() {
	
	
}

bool TCompAnimator::initializeAnimation(EAnimation animation, EAnimationType animationType, EAnimationSize animationSize, std::string animationName, std::string secondAnimationName, float weight, float speed) {
	
	CEntity* e = ownHandle;
	TCompSkeleton * compSkeleton = e->get<TCompSkeleton>();
	AnimationSet auxAnimSet;
	auxAnimSet.animation = animation;
	auxAnimSet.animationType = animationType;
	auxAnimSet.animationSize = animationSize;
	auxAnimSet.animationName = animationName;
	auxAnimSet.animationId = compSkeleton->getAnimationIdByName(auxAnimSet.animationName);
	if (auxAnimSet.animationId == -1) {
		fatal("The first animation non exists");
		return false;
	}
	auxAnimSet.secondAnimationName = secondAnimationName;
	auxAnimSet.secondAnimationId = compSkeleton->getAnimationIdByName(auxAnimSet.secondAnimationName);
	if (auxAnimSet.animationSize == EAnimationSize::DOUBLE && auxAnimSet.secondAnimationId == -1) {
		fatal("The second animation non exists");
		return false;
	}
	auxAnimSet.weight = weight;
	auxAnimSet.speed = speed;
	animationsMap[animation] = auxAnimSet;
	return false;
}

void TCompAnimator::update(float dt)
{

}

void TCompAnimator::registerMsgs() {
//	DECL_MSG(TCompAnimator, TMsgEntityCreated, onCreated);
}

bool TCompAnimator::playAnimation(EAnimation animation) {

	if (animationsMap.find(animation) == animationsMap.end()) {
		return false;
	}
	CEntity* e = ownHandle;
	TCompSkeleton * compSkeleton = e->get<TCompSkeleton>();
	AnimationSet animSet = animationsMap[animation];

	int anim1id = animSet.animationId;
	int anim2id = animSet.secondAnimationId;
	float weight = animSet.weight;
	float speed = animSet.speed;

	switch (animSet.animationType)
	{
	case EAnimationType::CYCLIC:
		compSkeleton->changeCyclicAnimation(anim1id, speed, anim2id, weight);
		break;

	case EAnimationType::ACTION:
		compSkeleton->executeActionAnimation(anim1id,speed);
		break;
	}

}

bool  TCompAnimator::isCyclic(EAnimation animation) {

	if (animationsMap.find(animation) == animationsMap.end()) {
		fatal("Animation doesn't exists");
	}
	AnimationSet animSet = animationsMap[animation];
	return animSet.animationType == EAnimationType::CYCLIC;
}

bool  TCompAnimator::isComposed(EAnimation animation) {

	if (animationsMap.find(animation) == animationsMap.end()) {
		fatal("Animation doesn't exists");
	}
	AnimationSet animSet = animationsMap[animation];
	return animSet.animationSize == EAnimationSize::DOUBLE;
}

TCompAnimator::EAnimation TCompAnimator::actualAnimation() {

	return actualCyclicAnimation.animation;
}

void  TCompAnimator::setFeetIds(std::vector<int> feetBonesId) {

	CEntity* e = ownHandle;
	TCompSkeleton * compSkeleton = e->get<TCompSkeleton>();

	compSkeleton->setFeetId(feetBonesId);
}

void TCompAnimator::setFeetNumAndCalculate(int numFeet) {

	CEntity* e = ownHandle;
	TCompSkeleton * compSkeleton = e->get<TCompSkeleton>();

	compSkeleton->guessFeetBonesId(numFeet);
}

std::vector<VEC3> TCompAnimator::getFeetPositions() {

	CEntity* e = ownHandle;
	TCompSkeleton * compSkeleton = e->get<TCompSkeleton>();

	return compSkeleton->getFeetPositions();
}
