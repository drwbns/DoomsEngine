#pragma once
#include <string>

#include "Asset.h"
#include "../API/GLM.h"
namespace Doom
{
	struct ThreeDModelMesh;
	struct ThreeDModelNode;
	struct ThreeDModelAsset;

	struct ThreeDModelMesh
	{

		std::string Name;

		glm::vec3** Vertices;
		unsigned int NumOfVertices;
		
		glm::vec3** TexCoords;
		unsigned int NumOfTexCoords;

		glm::vec3* Tangents;
		glm::vec3* BiTangents;

		glm::vec3* Normals;
		
	};

	struct ThreeDModelNode
	{
		ThreeDModelAsset* ThreeDModelAsset;

		std::string Name;

		ThreeDModelNode* ThreeDModelNodeParent;

		ThreeDModelNode** ThreeDModelNodeChildrens;
		unsigned int NumOfThreeDModelNodeChildrens;

		/// <summary>
		/// each component contain index of ThreeDModelAsset::ThreeDModelMesh 
		/// so use like this->ThreeDModelAsset->ThreeDModelMesh[ThreeDModelMeshes[0]]
		/// </summary>
		ThreeDModelMesh** ThreeDModelMeshes;
		unsigned int NumOfThreeDModelMeshes; 
	};

	struct ThreeDModelAsset : public Asset
	{
		ThreeDModelNode rootNode;

		ThreeDModelMesh** ModelMeshes;
		unsigned int NumOfModelMeshed;



	};
}

