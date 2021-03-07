#pragma once

#include <Vector3.h>

namespace doom
{
	namespace physics
	{
		class Collider;
		class AABB2D;
		class AABB3D;
		class Line;
		class Plane;
		class Ray;
		class Sphere;

		bool IsOverlapSphereAndAABB3D(const Sphere& sphere, const AABB3D& aabb);
		bool IsOverlapSphereAndAABB3D(Collider* sphere, Collider* aabb);
		math::Vector3 GetClosestPointOnAABB(const Sphere& sphere, const AABB3D& aabb);

		bool IsOverlapSphereAndPlane(const Sphere& sphere, const Plane& plane);
		bool IsOverlapSphereAndPlane(Collider* sphere, Collider* plane);
		float DistanceFromSphereToPlane(const Sphere& sphere, const Plane& plane);
		math::Vector3 GetClosestPointOnPlane(const Sphere& sphere, const Plane& plane);

		float DistanceFromAABBToPlane(const AABB3D& aabb, const Plane& plane);
		bool IsOverlapAABB3DAndPlane(const AABB3D& aabb, const Plane& plane);
		bool IsOverlapAABB3DAndPlane(Collider* aabb, Collider* plane);



		float RaycastRayAndAABB3D(const Ray& ray, const AABB3D& aabb);
		float RaycastRayAndSphere(const Ray& ray, const Sphere& sphere);
		float RaycastRayAndPlane(const Ray& ray, const Plane& plane); 
		bool RaycastRayAndAABB3D(Collider* ray, Collider* aabb);
		bool RaycastRayAndSphere(Collider* ray, Collider* sphere);
		bool RaycastRayAndPlane(Collider* ray, Collider* plane);

		/// <summary>
		/// return lenght if line's length is larger than argumnet length
		/// if not, return -1
		/// </summary>
		/// <param name="line"></param>
		/// <param name="length"></param>
		/// <returns></returns>
		float CheckLenghIsShorterThanLine(const Line& line, float length);
		bool RaycastLineAndAABB3D(const Line& line, const AABB3D& aabb);
		bool RaycastLineAndSphere(const Line& line, const Sphere& sphere);
		bool RaycastLineAndPlane(const Line& line, const Plane& plane); 
		bool RaycastLineAndAABB3D(Collider* line, Collider* aabb);
		bool RaycastLineAndSphere(Collider* line, Collider* sphere);
		bool RaycastLineAndPlane(Collider* line, Collider* plane);

	}
}
