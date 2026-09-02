//@begin_vert

#version 460 core

layout (location = 0) in vec3 aPos;  // All in, out variable should have layout (location = ?) option
layout (location = 1) in vec3 aUV0; 
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// The model matrix arrives per instance rather than in a uniform block, so
// every object sharing this mesh and material can be drawn in one call.
//
// Read this way whether or not instancing is switched on: a draw of one
// instance is still a draw of one instance. A shader that could take the
// matrix from either place would need the unused path kept alive, or the
// compiler strips these inputs and the reflection stops mentioning them --
// and the reflection is what the D3D11 input layout is built from. The
// toggle therefore changes how many objects a draw carries, not this file.
//
// These are columns. mat4(a, b, c, d) builds from columns, and the matrix
// uploaded per instance is column major in memory, so the two agree.
layout (location = 5) in vec4 aInstanceModelColumn0;
layout (location = 6) in vec4 aInstanceModelColumn1;
layout (location = 7) in vec4 aInstanceModelColumn2;
layout (location = 8) in vec4 aInstanceModelColumn3;

layout (location = 1) out vec3 UV0; // All in, out variable should have layout (location = ?) option
layout (location = 2) out vec3 FragPos;
layout (location = 3) out mat3 TBN;
layout (location = 6) out mat3 invertedTBN; // 
//If the declared input is an n × m matrix, 
//it will be assigned multiple locations starting with the location specified. 
//The number of locations assigned for each matrix will be the same as for an n-element array of m-component vectors.
layout (location = 9) out vec4 ClipSpacePos;
layout (location = 10) out vec4 PrevClipSpacePos;

layout(binding = 0, std140) uniform Global
{
    // trtansformations
    mat4 viewProjection;
    mat4 prevViewProjection;
    mat4 projection;
    mat4 view;
    mat4 invViewz;
    // scene
    vec3 camPos;
    // lighting
    vec3 DirectionalLightDirection[5];
	vec3 DirectionalLightRadiance[5];
    vec3 PointLightPos[16];
	vec3 PointLightRadiance[16];
    int dirLightCount;
    int pointLightCount;
    //
    float camNear;
    float camFar;
    float ambientLightIntensity;
};

// this code "layout(location = 0) uniform mat4 model" makes error!!
// Please put it in uniform block
layout(binding = 1, std140) uniform ModelData
{
    mat4 model; 
};

void main()
{
    const mat4 instanceModel = mat4
    (
        aInstanceModelColumn0,
        aInstanceModelColumn1,
        aInstanceModelColumn2,
        aInstanceModelColumn3
    );

	UV0 = aUV0;
	FragPos = vec3(instanceModel * vec4(aPos, 1.0));

    vec3 N = normalize(mat3(instanceModel) * aNormal);
    vec3 T = normalize(mat3(instanceModel) * aTangent);
    T = normalize(T - dot(N, T) * N);
    // vec3 B = cross(N, T);
    vec3 B = normalize(mat3(instanceModel) * aBitangent);

    // TBN must form a right handed coord system.
    // Some models have symetric UVs. Check and fix.
    if (dot(cross(N, T), B) < 0.0)
        T = T * -1.0;
    
    TBN = mat3(T, B, N);
    invertedTBN = transpose(TBN);

    //ClipSpacePos     = viewProjection * model * vec4(aPos, 1.0);
    //PrevClipSpacePos = prevViewProjection * prevModel * vec4(aPos, 1.0);
	
	gl_Position =  viewProjection * vec4(FragPos, 1.0);
}

//@end

//@begin_frag
#version 460 core

layout (location = 1) in vec3 UV0; // All in, out variable should have layout (location = ?) option
layout (location = 2) in vec3 FragPos;
layout (location = 3) in mat3 TBN;
layout (location = 6) in mat3 invertedTBN;
layout (location = 9) in vec4 ClipSpacePos;
layout (location = 10) in vec4 PrevClipSpacePos;


layout (location = 0) out vec4 oPosition; // All in, out variable should have layout (location = ?) option
layout (location = 1) out vec4 oNormal; // 
layout (location = 2) out vec4 oAlbedoSpec; // 

layout(binding=0) uniform sampler2D albedoTexture; // sampler2D should have layout(binding=?) option
layout(binding=1) uniform sampler2D normalTexture;
layout(binding=2) uniform sampler2D metalnessTexture;
layout(binding=3) uniform sampler2D roughnessTexture;
layout(binding=4) uniform samplerCube specularTexture;
layout(binding=5) uniform samplerCube irradianceTexture;
layout(binding=6) uniform sampler2D specularBRDF_LUT;


void main() 
{ 
	oPosition = vec4(FragPos, 1.0); 

    vec3 Normal = normalize(2.0 * texture(normalTexture, UV0.xy).rgb - 1.0);
	Normal = normalize(TBN * Normal);
	oNormal = vec4(Normal, 1.0); 

	oAlbedoSpec = vec4(vec3(texture(albedoTexture, UV0.xy)), 1.0); 
	oAlbedoSpec.a = texture(metalnessTexture, UV0.xy).r; 
}

//@end