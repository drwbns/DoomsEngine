#pragma once


#include "../Core.h"
#include <Vector3.h>
#include <Matrix4x4.h>
#include "Collider.h"
#include "AABB.h"
namespace dooms
{
	namespace physics
	{
		class AABB3D;

		class DOOM_API D_CLASS Sphere : public Collider
		{

		public:

			using component_type = typename math::Vector3;

			// Inherited via RenderPhysics
			virtual void DrawCollider(eColor color, bool drawInstantly = false) const;

			FORCE_INLINE Sphere()
				:mCenter{}, mRadius{}
			{

			}
			FORCE_INLINE Sphere(const math::Vector3& center, FLOAT32 radius)
				:mCenter{ center }, mRadius{ radius }
			{

			}
				
			FORCE_INLINE Sphere(const AABB3D& aabb3D)
				: mCenter{ (aabb3D.mUpperBound + aabb3D.mLowerBound) * 0.5f }, mRadius{ ((aabb3D.mUpperBound - aabb3D.mLowerBound) * 0.5f).magnitude() }
			{

			}
			
			
			math::Vector3 mCenter;
			/// <summary>
			/// for using SIMD -> Mat4X4 * Vec4 is much faster
			/// </summary>
			FLOAT32 padding{ 1.0f };
			FLOAT32 mRadius;

			FORCE_INLINE virtual void* data() final
			{
				return &(mCenter);
			}

			FORCE_INLINE virtual const void* data() const final
			{
				return &(mCenter);
			}

			FORCE_INLINE void Expand(const math::Vector3& movedVector)
			{
				mRadius += movedVector.magnitude();
			}

			FORCE_INLINE void SignedExpand(const math::Vector3& movedVector)
			{
				FLOAT32 distance = movedVector.magnitude();

				mCenter += movedVector * 0.5f;
				mRadius += distance * 0.5f;
			}

			ColliderType GetColliderType() const override;

			FORCE_INLINE static FLOAT32 GetArea(const Sphere& A)
			{
				return A.mRadius;
			}
			/// <summary>
			/// https://stackoverflow.com/questions/33532860/merge-two-spheres-to-get-a-new-one
			/// </summary>
			/// <param name="A"></param>
			/// <param name="B"></param>
			/// <returns></returns>

			inline static Sphere Union(const Sphere& A, const Sphere& B)
			{
				math::Vector3 v = A.mCenter - B.mCenter;
				FLOAT32 distanceWithCenters = v.magnitude();
				if (distanceWithCenters + A.mRadius < B.mRadius)
				{
					return B;
				}
				else if (distanceWithCenters + B.mRadius < A.mRadius)
				{
					return A;
				}

				FLOAT32 newRadius = (A.mRadius + B.mRadius + distanceWithCenters) * 0.5f;
				D_ASSERT(math::Equal(distanceWithCenters, 0.0f) == false);
				math::Vector3 newCenter = math::lerp(A.mCenter, B.mCenter, (newRadius - A.mRadius) / (math::Equal(distanceWithCenters, 0.0f) == false ? distanceWithCenters : 0.01f));
				return Sphere(newCenter, newRadius);
			}

			inline static FLOAT32 GetUnionArea(const Sphere& A, const Sphere& B)
			{
				math::Vector3 v = A.mCenter - B.mCenter;
				FLOAT32 distanceWithCenters = v.magnitude();
				if (distanceWithCenters + A.mRadius < B.mRadius)
				{
					return B.mRadius;
				}
				else if (distanceWithCenters + B.mRadius < A.mRadius)
				{
					return A.mRadius;
				}

				return (A.mRadius + B.mRadius + distanceWithCenters) * 0.5f;
			}
			inline static Sphere EnlargeAABB(const Sphere& sphere)
			{
				return sphere;
			}

			FORCE_INLINE static void ApplyModelMatrix(const Sphere& localSphere, const math::Matrix4x4& modelMatrix, Sphere& resultSphere)
			{
				FLOAT32 largestScale = math::sqrt(math::Max(math::Max(modelMatrix[0].sqrMagnitude(), modelMatrix[1].sqrMagnitude()), modelMatrix[2].sqrMagnitude()));
				resultSphere.mCenter = modelMatrix * localSphere.mCenter;
				resultSphere.mRadius = localSphere.mRadius * largestScale; // TODO : should i square largesScale??
			}

			/// <summary>
			/// This is different with IsOverlap
			/// This function check if innerAABB is completly enclosed by outerAABB
			/// </summary>
			/// <param name="innerAABB"></param>
			/// <param name="outerAABB"></param>
			/// <returns></returns>

			FORCE_INLINE static bool CheckIsCompletlyEnclosed(const Sphere& innerSphere, const Sphere& outerSphere)
			{
				return (outerSphere.mCenter - innerSphere.mCenter).sqrMagnitude() < math::pow(outerSphere.mRadius - innerSphere.mRadius, 2);
			}

		};

		FORCE_INLINE bool IsOverlapSphereAndSphere(const Sphere& sphere1, const Sphere& sphere2)
		{
			return (sphere1.mCenter - sphere2.mCenter).sqrMagnitude() < math::pow(sphere1.mRadius + sphere2.mRadius, 2);
		}
		FORCE_INLINE bool IsOverlapSphereAndSphere(const Collider* const sphere1, const Collider* const sphere2)
		{
			return IsOverlapSphereAndSphere(*static_cast<const Sphere*>(sphere1), *static_cast<const Sphere*>(sphere2));
		}

	}
}

