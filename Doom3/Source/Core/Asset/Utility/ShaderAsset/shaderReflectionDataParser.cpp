#include "shaderReflectionDataParser.h"

#include <unordered_map>
#include <nlohmann/json.hpp>

namespace dooms::asset::shaderReflectionDataParser
{
	static std::unordered_map<std::string, dooms::asset::shaderReflectionDataParser::eShaderVariableType> ShaderVraibleTypeHashtable
	{
		{"float1", dooms::asset::shaderReflectionDataParser::eShaderVariableType::FLOAT1},
		{"float2", dooms::asset::shaderReflectionDataParser::eShaderVariableType::FLOAT2},
		{"float3", dooms::asset::shaderReflectionDataParser::eShaderVariableType::FLOAT3},
		{"float4", dooms::asset::shaderReflectionDataParser::eShaderVariableType::FLOAT4},
		{"mat2", dooms::asset::shaderReflectionDataParser::eShaderVariableType::MAT2X2},
		{"mat2x2", dooms::asset::shaderReflectionDataParser::eShaderVariableType::MAT2X2},
		{"mat3", dooms::asset::shaderReflectionDataParser::eShaderVariableType::MAT3X3},
		{"mat3x3", dooms::asset::shaderReflectionDataParser::eShaderVariableType::MAT3X3},
		{"mat4", dooms::asset::shaderReflectionDataParser::eShaderVariableType::MAT4X4},
		{"mat4x4", dooms::asset::shaderReflectionDataParser::eShaderVariableType::MAT4X4},
		{"int1", dooms::asset::shaderReflectionDataParser::eShaderVariableType::INT1},
		{"int2", dooms::asset::shaderReflectionDataParser::eShaderVariableType::INT2},
		{"int3", dooms::asset::shaderReflectionDataParser::eShaderVariableType::INT3},
		{"int4", dooms::asset::shaderReflectionDataParser::eShaderVariableType::INT4},
		{"uint1", dooms::asset::shaderReflectionDataParser::eShaderVariableType::UINT1},
		{"uint2", dooms::asset::shaderReflectionDataParser::eShaderVariableType::UINT2},
		{"uint3", dooms::asset::shaderReflectionDataParser::eShaderVariableType::UINT3},
		{"uint4", dooms::asset::shaderReflectionDataParser::eShaderVariableType::UINT4},
		{"unknown", dooms::asset::shaderReflectionDataParser::eShaderVariableType::UNKNOWN}
	};
}

dooms::asset::shaderReflectionDataParser::eShaderVariableType dooms::asset::shaderReflectionDataParser::ConvertStringToeShaderVariableType(const std::string& typeStr)
{
	const dooms::asset::shaderReflectionDataParser::eShaderVariableType result = ShaderVraibleTypeHashtable[typeStr];
	D_ASSERT(result != dooms::asset::shaderReflectionDataParser::eShaderVariableType::UNKNOWN);
	return result;
}

void dooms::asset::shaderReflectionDataParser::ShaderReflectionData::Clear()
{
	mIsGenerated = false;
	mTargetGraphicsAPIType = dooms::graphics::GraphicsAPI::eGraphicsAPIType::GraphicsAPIType_NONE;
	mProfileVersion.clear();
	ShaderReflectionDataFileName.clear();
	mShaderType = dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage::DUMMY;
	mInputVariables.clear();
	mOutputVariables.clear();
	mUniformBuffers.clear();
}

// Parse reflection data contained in json file generated by https://github.com/septag/glslcc
dooms::asset::shaderReflectionDataParser::ShaderReflectionData dooms::asset::shaderReflectionDataParser::ParseShaderReflectionStringText
(
	const std::string& reflectionDataJsonText
)
{
	bool isSuccess = true;

	ShaderReflectionData shaderReflectionData;

	D_ASSERT(nlohmann::json::accept(reflectionDataJsonText) == true);
	const nlohmann::json j = nlohmann::json::parse(reflectionDataJsonText);
	D_ASSERT(j.empty() == false);

	{
		D_ASSERT(j.find("language") != j.end());
		const std::string shaderLang = static_cast<std::string>(j["language"]);
		if(shaderLang == "glsl")
		{
			shaderReflectionData.mTargetGraphicsAPIType = dooms::graphics::GraphicsAPI::eGraphicsAPIType::OpenGL;
		}
		else if (shaderLang == "hlsl")
		{
			shaderReflectionData.mTargetGraphicsAPIType = dooms::graphics::GraphicsAPI::eGraphicsAPIType::DX11_10;
		}
		else
		{
			D_ASSERT(false);
			isSuccess = false;
		}
	}

	{
		D_ASSERT(j.find("profile_version") != j.end());
		shaderReflectionData.mProfileVersion = std::to_string(static_cast<UINT32>(j["profile_version"]));
	}

	{
		D_ASSERT(j.find("vs") != j.end() || j.find("fs") != j.end());
		D_ASSERT(j.find("vs") != j.end() ^ j.find("fs") != j.end());

		nlohmann::json::const_iterator p;

		if (j.find("vs") != j.end())
		{
			p = j.find("vs");
			shaderReflectionData.mShaderType = dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage::VERTEX_SHADER;
		}
		else if (j.find("fs") != j.end())
		{
			p = j.find("fs");
			shaderReflectionData.mShaderType = dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER;
		}
		else if (j.find("cs") != j.end())
		{
			p = j.find("cs");
			shaderReflectionData.mShaderType = dooms::graphics::GraphicsAPI::eGraphicsPipeLineStage::COMPUTE_SHADER;
		}
		else
		{
			D_ASSERT(false);
			isSuccess = false;
		}

		if(p->empty() == false)
		{
			if(p->find("file") != p->end())
			{
				
			}
			else
			{
				D_ASSERT(false);
				isSuccess = false;
			}

			if (p->find("inputs") != p->end())
			{
				for(auto& inputElement : p.value()["inputs"])
				{
					ShaderInputType input;
					input.mID = inputElement["id"];
					input.mName = inputElement["name"];
					input.mLocation = inputElement["location"];
					if(inputElement.find("semantic") != inputElement.end())
					{
						input.mSemanticType = inputElement["semantic"];
					}
					else
					{
						input.mSemanticType = "unknown";
					}

					if (inputElement.find("semantic_index") != inputElement.end())
					{
						input.mSemanticIndex = inputElement["semantic_index"];
					}
					else
					{
						input.mSemanticIndex = 0;
					}

					if (inputElement.find("type") != inputElement.end())
					{
						input.mType = ConvertStringToeShaderVariableType(inputElement["type"]);
					}
					else
					{
						input.mType = eShaderVariableType::UNKNOWN;
					}
					
					shaderReflectionData.mInputVariables.emplace_back(std::move(input));
				}
			}

			if (p->find("outputs") != p->end())
			{
				for (auto& outputElement : p.value()["outputs"])
				{
					ShaderOutputType output;
					output.mID = outputElement["id"];
					output.mName = outputElement["name"];
					output.mLocation = outputElement["location"];

					shaderReflectionData.mOutputVariables.emplace_back(std::move(output));
				}
			}

			if (p->find("uniform_buffers") != p->end())
			{
				for(auto& uniformBufferElement : p.value()["uniform_buffers"])
				{
					UniformBuffer uniformBuffer{};
					uniformBuffer.mID = uniformBufferElement["id"];
					uniformBuffer.mName = uniformBufferElement["name"];
					uniformBuffer.mSet = uniformBufferElement["set"];
					uniformBuffer.mBindingPoint = uniformBufferElement["binding"];
					uniformBuffer.mBlockSize = uniformBufferElement["block_size"];

					if (uniformBufferElement.find("members") != uniformBufferElement.end())
					{
						for (auto& uniformBufferMemberElement : uniformBufferElement["members"])
						{
							UniformBufferMember uniformBufferMember{};

							uniformBufferMember.mName = uniformBufferMemberElement["name"];
							uniformBufferMember.mType = ConvertStringToeShaderVariableType(uniformBufferMemberElement["type"]);
							uniformBufferMember.mOffset = uniformBufferMemberElement["offset"];
							uniformBufferMember.mSize = uniformBufferMemberElement["size"];

							if(uniformBufferMemberElement.find("array") != uniformBufferMemberElement.end())
							{
								uniformBufferMember.mArrayLength = uniformBufferMemberElement["array"];
							}
							else
							{
								uniformBufferMember.mArrayLength = 1;
							}

							uniformBuffer.mMembers.emplace_back(std::move(uniformBufferMember));
						}
					}

					shaderReflectionData.mUniformBuffers.emplace_back(std::move(uniformBuffer));
				}
			}

			if (p->find("textures") != p->end())
			{
				for (auto& textureElement : p.value()["textures"])
				{
					TextureData textureData{};
					textureData.mID = textureElement["id"];
					textureData.mName = textureElement["name"];
					textureData.mSet = textureElement["set"];
					textureData.mBindingPoint = textureElement["binding"];

					if (textureElement["dimension"] == "1d")
					{
						textureData.mDimension = eTextureDimensionType::DIMENSION_1D;
					}
					else if(textureElement["dimension"] == "2d")
					{
						textureData.mDimension = eTextureDimensionType::DIMENSION_2D;
					}
					else if (textureElement["dimension"] == "3d")
					{
						textureData.mDimension = eTextureDimensionType::DIMENSION_3D;
					}
					else if (textureElement["dimension"] == "cube")
					{
						textureData.mDimension = eTextureDimensionType::DIMENSION_CUBE;
					}
					else
					{
						D_ASSERT(false);
					}

					textureData.mFormat = textureElement["format"];

					shaderReflectionData.mTextureDatas.emplace_back(std::move(textureData));
				}
			}
		}
		else
		{
			D_ASSERT(false);
			isSuccess = false;
		}
	}


	if(isSuccess == true)
	{
		shaderReflectionData.mIsGenerated = true;
	}
	else
	{
		shaderReflectionData.mIsGenerated = false;
	}

	return shaderReflectionData;
}
