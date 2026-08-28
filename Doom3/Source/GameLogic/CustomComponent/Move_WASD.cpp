#include "Move_WASD.h"
#include "../IO/UserInput_Server.h"
#include <Transform.h>
#include <Vector3.h>
#include "Vector2.h"


void dooms::Move_WASD::InitComponent()
{
	//dooms::userinput::UserInput_Server::SetIsCursorLockedInScreen(true);
}

void dooms::Move_WASD::UpdateComponent()
{
	math::Vector3 translation{0.0f, 0.0f, 0.0f};
	bool isMove{ false };
	if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_W))
	{
		translation.z -= 1;
		isMove = true;
	}
	else if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_S))
	{
		translation.z += 1;
		isMove = true;
	}

	if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_A))
	{
		translation.x -= 1;
		isMove = true;
	}
	else if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_D))
	{
		translation.x += 1;
		isMove = true;
	}

	if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_Q))
	{
		translation.y += 1;
		isMove = true;
	}
	else if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_E))
	{
		translation.y -= 1;
		isMove = true;
	}

	auto delta = MainTimer::GetSingleton()->GetDeltaTime();
	if (isMove == true)
	{
		translation.Normalize();
		translation *= 100.0f * mMoveSpeed;
		
		GetTransform()->Translate(translation * delta, dooms::eSpace::Self);
	}
	

	/////////

	math::Vector3 rotation{ 0.0f, 0.0f, 0.0f };
	bool isRotated = false;

	// Mouse look, active whenever the interface is hidden. F1 switches between
	// the two: panels up gives a free cursor for reading them, panels hidden
	// captures it for looking around.
	if (UserInput_Server::GetIsMouseLookEnabled())
	{
		const FLOAT32 deltaX = UserInput_Server::GetDeltaMouseScreenPositionX();
		const FLOAT32 deltaY = UserInput_Server::GetDeltaMouseScreenPositionY();

		if ((deltaX != 0.0f) || (deltaY != 0.0f))
		{
			// Applied directly rather than through the normalise-and-scale path
			// below: mouse movement already carries its own magnitude, and
			// normalising would throw that away and make every flick identical.
			const math::Vector3 mouseRotation
			{
				deltaY * mMouseLookSensitivity,
				-deltaX * mMouseLookSensitivity,
				0.0f
			};

			GetTransform()->Rotate(mouseRotation, eSpace::Self);
		}
	}

	if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_UP))
	{
		rotation.x += 1;
		isRotated = true;
	}
	if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_DOWN))
	{
		rotation.x -= 1;
		isRotated = true;
	}

	if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_LEFT))
	{
		rotation.y += 1;
		isRotated = true;
	}
	if (UserInput_Server::GetKey(dooms::input::GraphicsAPIInput::eKEY_CODE::KEY_RIGHT))
	{
		rotation.y -= 1;
		isRotated = true;
	}

	if(isRotated)
	{
		GetTransform()->Rotate(rotation.normalized() * delta * mRotationSpeed, eSpace::Self);
	}



}

void dooms::Move_WASD::OnEndOfFrame_Component()
{

}

void dooms::Move_WASD::OnDestroy()
{

}

void dooms::Move_WASD::OnActivated()
{

}

void dooms::Move_WASD::OnDeActivated()
{

}
