#pragma once

#include <DoomsGameLogicCore.h>

#include "Move_WASD.reflection.h"
namespace dooms
{
	class DOOM_API D_CLASS Move_WASD : public Component
	{
		GENERATE_BODY()

		D_PROPERTY()
		FLOAT32 mMoveSpeed = 2.0f;

		D_PROPERTY()
		FLOAT32 mRotationSpeed = 2.0f;

		// Radians of rotation per pixel of mouse movement while looking.
		//
		// Radians, not degrees: Transform::Rotate builds a quaternion through
		// EulerAngleToQuaternion, which feeds the values straight to cos and
		// sin without converting. The default is about 0.2 degrees per pixel.
		D_PROPERTY()
		FLOAT32 mMouseLookSensitivity = 0.0035f;

	protected:

		void InitComponent() override;
		void UpdateComponent() override;
		void OnEndOfFrame_Component() override;
		void OnDestroy() override;
		void OnActivated() override;
		void OnDeActivated() override;
		
	};
}


