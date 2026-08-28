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

// Must match OVERDRAW_INTENSITY_PER_FRAGMENT in OverDrawVisualizationShader.glsl.
const float OVERDRAW_INTENSITY_PER_FRAGMENT = 0.02;

// Layer count the ramp saturates at. Beyond this everything reads as the hot
// end, so it is the number worth tuning when a scene is much heavier or much
// lighter than this one.
const float OVERDRAW_SATURATION_LAYERS = 10.0;

// Cold to hot, matching the occlusion heatmap in MaskedOcclusionCullingTester.
vec3 HeatmapColor(float normalisedValue)
{
	const vec3 stops[5] = vec3[5]
	(
		vec3(0.0, 0.1, 0.6),	// blue
		vec3(0.0, 0.7, 1.0),	// cyan
		vec3(0.1, 0.9, 0.1),	// green
		vec3(1.0, 0.9, 0.0),	// yellow
		vec3(1.0, 0.0, 0.0)		// red
	);

	float rampPosition = clamp(normalisedValue, 0.0, 1.0) * 4.0;
	int lowerStop = int(rampPosition);
	int upperStop = min(lowerStop + 1, 4);

	return mix(stops[lowerStop], stops[upperStop], rampPosition - float(lowerStop));
}

void main()
{
	// The geometry pass adds a fixed amount per fragment with additive
	// blending, so the red channel is the layer count scaled by that amount.
	float layerCount = texture(ColorTexture, oUV0).r / OVERDRAW_INTENSITY_PER_FRAGMENT;

	// Pixels nothing was drawn to stay black, so the ramp reads against the
	// empty background rather than starting from it.
	if (layerCount < 0.5)
	{
		oColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	oColor = vec4(HeatmapColor(layerCount / OVERDRAW_SATURATION_LAYERS), 1.0);
}
//@end
