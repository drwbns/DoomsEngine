#pragma once
#include "Core/PlainComponent.h"
#include "../Core/Math/LightMath_Cpp/Quaternion.h"
#include "../Core/Math/LightMath_Cpp/Vector3.h"
#include "../Core/Math/LightMath_Cpp/Matrix4x4.h"
#include "../Core/Math/LightMath_Cpp/Matrix_utility.h"
#include "EasyDirtyChecker/DirtyReceiver.h"

namespace doom
{
	enum class eSpace
	{
		World,
		Self
	};

	struct TransformCoreData
	{
		math::Vector3 mPosition;
		DirtyReceiver bmIsDirtyModelMatrix{ true };
	
		
	};

	static_assert(sizeof(TransformCoreData) < 64);

	class DOOM_API Transform : public PlainComponent
	{
		DOBJECT_CLASS_BODY(Transform)
		DOBJECT_CLASS_BASE_CHAIN(PlainComponent)

	private:

		alignas(64) TransformCoreData mTransformCoreData;

		/// <summary>
		/// why don't use FrameDirtyChecker::IsDirty -> FrameDirtyChecker is changed when pass frame
		/// but this modelMatrix is calculated when this is needed ( not every frame )
		/// </summary>
		alignas(64) mutable math::Matrix4x4 mModelMatrixCache;
		alignas(64) math::Matrix4x4 mLocalToWorldMatrix{ 1.0f };
		alignas(64) math::Matrix4x4 mWorldToLocalMatrix{ 1.0f };
		alignas(64) math::Matrix4x4 mTranslationMatrix{ 1.0f };
		alignas(64) math::Matrix4x4 mRotationMatrix{ 1.0f };
		alignas(64) math::Matrix4x4 mScaleMatrix{ 1.0f };
	
		math::Vector3 mScale{ 1.0f };
		math::Quaternion mRotation;
		math::Vector3 mLastFramePosition;

		//Matrix4X4 and Vector4 is aligned to 32, 16 byte
		//So To save memory, it is declared next to next

	



		virtual void InitComponent() final;
		virtual void UpdateComponent() final;
		virtual void OnEndOfFrame_Component() final;
		virtual void OnDestroy() final;
		
		

		bool IsEntityMobilityStatic() const;

	public:

		Transform();
		virtual ~Transform() = default;

		Transform(const Transform&) = default;
		Transform(Transform&&) noexcept = delete;
		Transform& operator=(const Transform&) = default;
		Transform& operator=(Transform&&) noexcept = delete;


		std::string ToString();

		FORCE_INLINE void SetPosition(const math::Vector3& position) noexcept
		{
			D_ASSERT(IsEntityMobilityStatic());

			if (IsEntityMobilityStatic())
			{
				mTranslationMatrix = math::translate(position);
				mTransformCoreData.mPosition = position;

				SetDirtyTrueAtThisFrame();
				mTransformCoreData.bmIsDirtyModelMatrix = true;
			}			
		}
		FORCE_INLINE void SetPosition(FLOAT32 x, FLOAT32 y, FLOAT32 z) noexcept
		{
			SetPosition({ x, y, z });
		}

		FORCE_INLINE void SetRotation(const math::Quaternion& rotation) noexcept
		{
			D_ASSERT(IsEntityMobilityStatic());

			if (IsEntityMobilityStatic())
			{
				mRotationMatrix = static_cast<math::Matrix4x4>(rotation);
				mRotation = rotation;
				SetDirtyTrueAtThisFrame();

				mTransformCoreData.bmIsDirtyModelMatrix = true;
			}
		} 
		FORCE_INLINE void SetRotation(const math::Vector3& eulerAngle) noexcept
		{
			SetRotation(math::Quaternion(eulerAngle));
		}

		FORCE_INLINE void SetRotation(const FLOAT32 eulerAngleX, const FLOAT32 eulerAngleY, const FLOAT32 eulerAngleZ) noexcept
		{
			SetRotation({ eulerAngleX, eulerAngleY, eulerAngleZ });
		}

		FORCE_INLINE void SetScale(const math::Vector3& scale) noexcept
		{
			D_ASSERT(IsEntityMobilityStatic());

			if (IsEntityMobilityStatic())
			{
				mScaleMatrix = math::scale(scale);
				mScale = scale;
				SetDirtyTrueAtThisFrame();
				mTransformCoreData.bmIsDirtyModelMatrix = true;
			}
		}
		FORCE_INLINE void SetScale(const FLOAT32 x, const FLOAT32 y, const FLOAT32 z) noexcept
		{
			SetScale({ x,y,z });
		}

		FORCE_INLINE const math::Vector3& GetPosition() const noexcept
		{
			return mTransformCoreData.mPosition;
		}

		FORCE_INLINE const math::Quaternion& GetRotation() const noexcept
		{
			return mRotation;
		}

		FORCE_INLINE const math::Vector3& GetScale() const noexcept
		{
			return mScale;
		}

		FORCE_INLINE const math::Matrix4x4& GetModelMatrix() const noexcept
		{
			if (mTransformCoreData.bmIsDirtyModelMatrix.GetIsDirty(true))
			{
				mModelMatrixCache = mTranslationMatrix * mRotationMatrix * mScaleMatrix;
			}
			return mModelMatrixCache;
		}
		
		FORCE_INLINE const math::Matrix4x4& GetRotationMatrix() const
		{
			return mRotationMatrix;
		}
		
		FORCE_INLINE math::Vector3 forward() const noexcept
		{
			return mRotation * math::Vector3::forward;
		}
		FORCE_INLINE math::Vector3 right() const noexcept
		{
			return mRotation * math::Vector3::right;
		}
		FORCE_INLINE math::Vector3 up() const noexcept
		{
			return mRotation * math::Vector3::up;
		}

		FORCE_INLINE void LookAt(const Transform& target, const math::Vector3& up) noexcept
		{
			SetRotation(static_cast<math::Quaternion>(math::lookAt(mTransformCoreData.mPosition, target.mTransformCoreData.mPosition, up)));
		}

		FORCE_INLINE void Rotate(const math::Quaternion& quat, const eSpace& relativeTo) noexcept
		{
			if (relativeTo == eSpace::Self)
			{
				SetRotation(mRotation * quat);
			}
			else if (relativeTo == eSpace::World)
			{
				SetRotation(quat * mRotation);
			}
			
		}
		FORCE_INLINE void Rotate(const math::Vector3& eulerAngles, const eSpace& relativeTo) noexcept
		{
			if (relativeTo == eSpace::Self)
			{
				SetRotation(mRotation * math::Quaternion(eulerAngles));
			}
			else if (relativeTo == eSpace::World)
			{
				SetRotation(math::Quaternion(eulerAngles) * mRotation);
			}
		}
		FORCE_INLINE void RotateAround(const math::Vector3& centerPoint, const math::Vector3& axis, const FLOAT32 angle) noexcept
		{
			math::Vector3 worldPos = GetPosition();
			const math::Quaternion q = math::Quaternion::angleAxis(angle, axis);
			math::Vector3 dif = worldPos - centerPoint;
			dif = q * dif;
			worldPos = centerPoint + dif;
			SetPosition(worldPos);

			
			SetRotation(math::Quaternion(angle * math::DEGREE_TO_RADIAN, axis));
		}

		FORCE_INLINE math::Vector3 TransformDirection(math::Vector3& direction) const noexcept
		{
			return mRotation * direction.normalized();
		}
		FORCE_INLINE math::Vector3 TransformPoint(const math::Vector3& point) const noexcept
		{
			return mRotation * point;
		}
		FORCE_INLINE math::Vector3 TransformVector(const math::Vector3& vector) const noexcept
		{
			return mRotation * vector;
		}
		FORCE_INLINE void Translate(const math::Vector3& translation, const eSpace& relativeTo = eSpace::World) noexcept
		{
			if (relativeTo == eSpace::World)
			{
				SetPosition(mTransformCoreData.mPosition + translation);
			}
			else if (relativeTo == eSpace::Self)
			{
				SetPosition(mTransformCoreData.mPosition + TransformVector(translation));
			}
		}


	};

}
