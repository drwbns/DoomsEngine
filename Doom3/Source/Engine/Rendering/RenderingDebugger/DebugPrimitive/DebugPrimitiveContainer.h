#pragma once

#include <Core.h>

#include <vector>
#include <array>

#include <Vector3.h>
#include <Vector4.h>
#include <Graphics/Color.h>
#include <Graphics/GraphicsAPI/GraphicsAPI.h>

namespace dooms
{
	namespace graphics
	{
		class DebugPrimitiveContainer
		{

		public:

			inline static const UINT32 COLOR_COUNT = ENUM_COLOR_COUNT;

			/// <summary>
			/// One draw call's worth of special-coloured primitives: every
			/// primitive that resolved to the same colour, made contiguous so
			/// they can be drawn together.
			/// </summary>
			struct SpecialColorBatch
			{
				math::Vector4 mColor;
				UINT32 mPrimitiveCount;
			};

		protected:

			std::array<std::vector<float>, ENUM_COLOR_COUNT> mColoredVertexData;

			std::vector<float> mSpecialColoredVertexData;
			std::vector<math::Vector4> mSpecialColorData;

			// mSpecialColoredVertexData reordered so that primitives sharing a
			// colour sit next to each other, with mSpecialColorBatches
			// describing the runs.
			std::vector<float> mBatchedSpecialColoredVertexData;
			std::vector<SpecialColorBatch> mSpecialColorBatches;
			bool bmIsSpecialColorBatchesBuilt = false;



			std::vector<float>& GetColoredVertexVector(const eColor color);

		public:

			/// <summary>
			///
			/// ex )
			/// 2D line -> 2
			/// 3D Triangle -> 3
			/// 
			/// </summary>
			/// <returns></returns>
			virtual UINT32 GetVertexCountPerPrimitive() const = 0;

			/// <summary>
			///
			/// ex )
			/// 2D line -> 6 ( 3 * 2 float )
			/// 3D Triangle -> 9 ( 3 * 3 float )
			/// 
			/// </summary>
			/// <returns></returns>
			virtual UINT32 GetComponentCountPerPrimitive() const = 0;
			size_t GetComponentSize() const;

			virtual bool Is3DPrimitive() const = 0;
			virtual dooms::graphics::GraphicsAPI::ePrimitiveType GetPrimitiveType() const = 0;

			const float* GetColoredVertexData(const eColor color) const;
			//size_t GetColoredVertexCount(const eColor color) const;
			size_t GetColoredPrimitiveCount(const eColor color) const;

			const float* GetSpecialColoredVertexData() const;
			size_t GetSpecialColoredVertexDataCount() const;
			const math::Vector4* GetSpecialColorData() const;
			size_t GetSpecialColorDataCount() const;
			size_t GetSpecialColoredPrimitiveCount() const;

			/// <summary>
			/// Groups the special-coloured primitives by colour.
			///
			/// Callers pass a colour per primitive, so drawing them in insertion
			/// order costs one draw call and one uniform update each. The
			/// per-tile occlusion debuggers emit tens of thousands of primitives
			/// per frame, which made them far too expensive to leave on in a
			/// harness whose whole purpose is measuring frame time. Grouping
			/// first collapses that to one draw call per distinct colour.
			///
			/// Colours are matched at 8 bits per channel, so a colour ramp only
			/// costs as many draw calls as it has distinguishable steps.
			///
			/// Idempotent within a frame; ClearDatas starts the next one.
			/// </summary>
			void BuildSpecialColorBatches();

			const float* GetBatchedSpecialColoredVertexData() const;
			const std::vector<SpecialColorBatch>& GetSpecialColorBatches() const;

			void ReserveVector(const size_t primitiveCount);
			void ClearDatas();

			bool IsColoredVertexDataEmpty(const eColor color) const;
			bool IsSpecialColoredVertexDataEmpty() const;
		};
	}
}

