//@begin_vert

#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 iUV0;

layout (location = 0) out vec2 oUV0;

void main()
{
	gl_Position =  vec4(vec2(aPos), 0.0, 1.0);
	oUV0 = iUV0;
}

//@end

//@begin_frag

#version 460 core

layout (location = 0) in vec2 oUV0;

layout (location = 0) out vec4 oColor;

layout(binding=0) uniform sampler2D ColorTexture;

// x is unused, yz is one texel of the level being read, in UV.
//
// The level is not passed because the texture bound here is a view of one
// level only, which renumbers it to zero. Passing the real level index would
// sample past the end of the view.
layout(set=0, binding = 0) uniform HiZData
{
	vec4 SourceLevelAndTexelSize;
};

// Produces one level of the pyramid from the one above it.
//
// The reduction is a maximum, which is what makes the pyramid usable for
// occlusion. Depth here is the standard convention, zero near and one far, so
// the largest value over a region is the farthest anything was drawn in it.
// An occludee whose nearest point is behind that is behind everything in the
// region and can be rejected without testing the region's individual texels.
//
// A minimum would be wrong in the dangerous direction: it would reject objects
// that are actually visible.
void main()
{
	vec2 sourceTexelSize = SourceLevelAndTexelSize.yz;

	// The four texels of the level above that this one covers.
	float d0 = textureLod(ColorTexture, oUV0 + vec2(-0.25, -0.25) * sourceTexelSize * 2.0, 0.0).r;
	float d1 = textureLod(ColorTexture, oUV0 + vec2( 0.25, -0.25) * sourceTexelSize * 2.0, 0.0).r;
	float d2 = textureLod(ColorTexture, oUV0 + vec2(-0.25,  0.25) * sourceTexelSize * 2.0, 0.0).r;
	float d3 = textureLod(ColorTexture, oUV0 + vec2( 0.25,  0.25) * sourceTexelSize * 2.0, 0.0).r;

	oColor = vec4(max(max(d0, d1), max(d2, d3)), 0.0, 0.0, 1.0);
}
//@end
