#pragma once
#include "Renderer.h"

#include "../Core/Graphics/Material/Material.h"
#include "../Core/Graphics/Buffer/Mesh.h"
#include "Transform.h"

namespace doom
{
	class DOOM_API MeshRenderer : public Renderer
	{
		DOBJECT_CLASS_BODY(MeshRenderer)
		DOBJECT_CLASS_BASE_CHAIN(Renderer)

	private:
		const graphics::Mesh* mTargetMesh;



		virtual void InitComponent() final
		{
			Renderer::InitComponent();
		}
		FORCE_INLINE virtual void UpdateComponent() final
		{
			Renderer::UpdateComponent();
		}

		FORCE_INLINE virtual void OnEndOfFrame_Component() final
		{
			Renderer::OnEndOfFrame_Component();
		}

	protected:

		

	public:

		MeshRenderer();
		virtual ~MeshRenderer();

		MeshRenderer(const MeshRenderer&) = default;
		MeshRenderer(MeshRenderer&&) noexcept = delete;
		MeshRenderer& operator=(const MeshRenderer&) = delete;
		MeshRenderer& operator=(MeshRenderer&&) noexcept = delete;

		FORCE_INLINE void Draw() const override
		{
			BindMaterial();

			if (mTargetMaterial != nullptr)
			{
				graphics::Material::SetMatrix4x4(graphics::eUniformLocation::ModelMatrix, GetTransform()->GetModelMatrix());
				
			}
			if (mTargetMesh != nullptr)
			{
				mTargetMesh->Draw();
			}
		}

		void SetMesh(const graphics::Mesh* const mesh);
		
		virtual physics::AABB3D GetLocalAABBBound() const final;

	};
}
