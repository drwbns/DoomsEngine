#include "AutoRotateAround.h"

void dooms::AutoRotateAround::InitComponent()
{
}

void dooms::AutoRotateAround::UpdateComponent()
{
	GetTransform()->RotateAround(mCenterPos, mRotateAxis, MainTimer::GetDeltaTime() * mRotateAngle);

}

void dooms::AutoRotateAround::OnEndOfFrame_Component()
{
}
