#pragma once

#include <string>
#include <array>

#include "Asset.h"
#include <Graphics/GraphicsAPI/GraphicsAPI.h>
#include <Rendering/Buffer/BufferID.h>

#include "ShaderAsset.reflection.h"
#include "Utility/ShaderAsset/shaderReflectionDataParser.h"

namespace dooms
{
	namespace assetImporter
	{
		class AssetImporterWorker_Shader;
	}

	namespace graphics 
	{
		class Material;
		class UniformBufferObject;
	}

	namespace asset
	{
		struct DOOM_API D_STRUCT ShaderTextData : public DObject
		{
			GENERATE_BODY_ShaderTextData()

			D_PROPERTY()
			std::filesystem::path mShaderTextFilePath;

			D_PROPERTY()
			dooms::graphics::GraphicsAPI::eGraphicsAPIType mShaderTextGraphicsAPIType;

			D_PROPERTY()
			std::string mShaderStringText;

			D_PROPERTY()
			std::string mShaderReflectionDataStringText;

			D_PROPERTY()
			shaderReflectionDataParser::ShaderReflectionData mShaderReflectionData;

			ShaderTextData();

			ShaderTextData
			(
				const std::string& shaderStringText,
				const std::string& shaderReflectionDataStringText
			);

			ShaderTextData
			(
				const dooms::graphics::GraphicsAPI::eGraphicsAPIType graphicsAPIType, 
				const std::string& shaderStringText,
				const std::string& shaderReflectionDataStringText
			);

			void Clear();
			bool IsCompileliable() const;

			bool LoadShaderReflectionDataFromTextIfNotLoaded();
		};

		class DOOM_API D_CLASS ShaderAsset : public Asset
		{
			GENERATE_BODY()


		public:

			enum class D_ENUM eShaderCompileStatus : UINT32
			{
				READY,
				SHADER_OBJECT_CREATED,
				COMPILE_SUCCESS,
				COMPILE_FAIL
			};
				
		private:

			struct DOOM_API D_STRUCT ShaderObject
			{
				/// <summary>
				/// OPENGL : Shader Object
				///	DIRECTX : ID3DBlob
				/// </summary>
				D_PROPERTY()
				dooms::graphics::BufferID mShaderObjectID;

				D_PROPERTY()
				eShaderCompileStatus mShaderCompileStatus;

				ShaderObject();
				bool IsShaderObjectValid() const;
			};
			
			D_PROPERTY()
			std::array<ShaderTextData, GRAPHICS_PIPELINE_STAGE_COUNT> mShaderTextDatas;

			D_PROPERTY()
			graphics::BufferID mInputLayoutForD3D {};

			D_PROPERTY()
			std::array<ShaderObject, GRAPHICS_PIPELINE_STAGE_COUNT> mShaderObject;

			struct DOOM_API D_STRUCT UniformBufferObjectContainer
			{
				GENERATE_BODY_UniformBufferObjectContainer()

				D_PROPERTY()
				std::vector<dooms::graphics::UniformBufferObject*> UniformBufferObjectList;
			};

			D_PROPERTY()
			std::array<UniformBufferObjectContainer, GRAPHICS_PIPELINE_STAGE_COUNT> mContainedUniformBufferObjects;
			/*
			bool ConvertShaderTextStringToCurrentGraphicsAPIShaderFormat(ShaderTextData& outShaderText);
			*/
			
			/// <summary>
			/// Don't call this subthread, Should Call this at mainthread
			/// </summary>
			bool CompileShaders();
			bool CompileSpecificTypeShader(ShaderTextData& shaderText, const graphics::GraphicsAPI::eGraphicsPipeLineStage shaderType, ShaderObject& shaderObject);
			const std::vector<dooms::graphics::UniformBufferObject*>& GenerateUniformBufferObjectFromShaderReflectionData(const shaderReflectionDataParser::ShaderReflectionData& shaderReflectionData, const graphics::GraphicsAPI::eGraphicsPipeLineStage shaderType);

			void CreateInputLayoutForD3D(dooms::asset::ShaderAsset* const shaderAsset);

		public:

			ShaderAsset();
			ShaderAsset(const ShaderAsset& shader) = delete;
			ShaderAsset(ShaderAsset&& shader) noexcept;
			ShaderAsset operator=(const ShaderAsset& shader) = delete;
			ShaderAsset& operator=(ShaderAsset&& shader) noexcept;
			virtual ~ShaderAsset();

			virtual void OnSetPendingKill() override;

			const std::vector<dooms::graphics::UniformBufferObject*>& GetContainedUniformBufferObject(const graphics::GraphicsAPI::eGraphicsPipeLineStage InShaderType) const;

			bool SetShaderText
			(
				const std::array<ShaderTextData, GRAPHICS_PIPELINE_STAGE_COUNT>& shaderTextDatas,
				const bool compileShader
			);

			/*
			void SetShaderText
			(
				const std::string& shaderStringText, 
				const std::string& shaderReflectionDataStringText, 
				const dooms::graphics::GraphicsAPI::eGraphicsAPIType shaderTextraphicsAPIType,
				const bool compileShader
			);
			*/

			FORCE_INLINE const graphics::BufferID& GetInputLayoutForD3D() const
			{
				D_ASSERT(mInputLayoutForD3D.IsValid());
				return mInputLayoutForD3D;
			}
			const std::string& GetShaderStringText(const dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage targetGraphicsPipeLineStage) const;
			const std::string& GetShaderReflectionDataStringText(const dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage targetGraphicsPipeLineStage) const;
			const shaderReflectionDataParser::ShaderReflectionData& GetShaderReflectionData(const dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage targetGraphicsPipeLineStage) const;


			/// <summary>
			/// You can delete shaders after Linking to material program
			/// If Shader is linked to material program when the shader is deleted,
			/// Deleting is delayed until the shader is unlinked to the mateiral  
			/// </summary>
			void DestroyShaderObjects();
			void ClearShaderTextDatas();

		

			void OnEndImportInMainThread_Internal() final;
			
			const dooms::graphics::BufferID& GetShaderObject(const dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage targetGraphicsPipeLineStage) const;
			bool IsShaderObjectSuccessfullyCreated(const dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage targetGraphicsPipeLineStage) const;
			bool IsHasAnyValidShaderObject() const;
			bool IsHasAnyValidShaderTextString() const;

			graphics::Material* CreateMatrialWithThisShaderAsset();
			void CompileShaderIfNotCompiled();

			virtual dooms::asset::eAssetType GetEAssetType() const final;

			eShaderCompileStatus GetCurrentShaderCompileStatus(const graphics::GraphicsAPI::eGraphicsPipeLineStage shaderType) const;
			
		};
		
	}
}
