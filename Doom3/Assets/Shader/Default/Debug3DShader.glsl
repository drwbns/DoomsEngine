//@begin_vert

#version 460 core

layout (location = 0) in vec3 aPos; 


// global uniform buffer for shared common set of uniforms among programs
// see: https://learnopengl.com/#!Advanced-OpenGL/Advanced-GLSL for table of std140 byte offsets

layout (std140, binding = 0) uniform Global
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


void main()
{
	gl_Position =  projection * view * vec4(aPos, 1.0);
}

//@end

//@begin_frag

#version 460 core
 
layout (location = 0) out vec4 oColor; // 

layout(set=0, binding = 0) uniform ColorData
{
	vec4 Color;
};

void main() 
{
	oColor = Color; 

}
//@end