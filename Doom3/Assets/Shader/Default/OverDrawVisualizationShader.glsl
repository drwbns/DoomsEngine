//@begin_vert
#version 460 core


// global uniform buffer for shared common set of uniforms among programs
// see: https://learnopengl.com/#!Advanced-OpenGL/Advanced-GLSL for table of std140 byte offsets

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



layout(location = 0) in vec3 aPos;

layout(binding = 1, std140) uniform ModelData
{
	mat4 model;
};

void main()
{
	gl_Position =  viewProjection * model * vec4(aPos, 1.0);
}

//@end

//@begin_frag
#version 460 core

layout (location = 0) out vec4 oOverdrawIntensity;

// Added to the target once per fragment, with additive blending on, so the red
// channel ends up holding the layer count scaled by this.
//
// It sets the ceiling: the target saturates at 1.0, so this is 1/50th and
// counts up to 50 overlapping layers. It was 0.1, which ran out at 10 and made
// any moderately deep pixel indistinguishable from a pathological one.
//
// OverDrawVisualizationPresentShader.glsl divides by this to recover the count,
// so the two values must be changed together.
const float OVERDRAW_INTENSITY_PER_FRAGMENT = 0.02;

void main()
{
	oOverdrawIntensity = vec4(OVERDRAW_INTENSITY_PER_FRAGMENT, 0.0, 0.0, 1.0);
}
//@end