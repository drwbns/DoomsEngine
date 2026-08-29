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

// x is the pyramid level to show. Explicit, because sampling without a level
// lets the hardware pick one and the whole point here is to look at a chosen
// level of the chain.
layout(set=0, binding = 0) uniform HiZPresentData
{
	vec4 DisplayLevel;
};

// Same curve and ramp as the depth buffer view, so the two can be compared
// directly: a level of the pyramid should look like a blockier version of the
// depth buffer, biased towards the far value because the reduction is a maximum.
const float DEPTH_CONTRAST_POWER = 0.15;

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
	float storedDepth = textureLod(ColorTexture, oUV0, DisplayLevel.x).r;

	// Nothing was drawn anywhere under this texel.
	if (storedDepth >= 0.9999)
	{
		oColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	float nearness = pow(clamp(1.0 - storedDepth, 0.0, 1.0), DEPTH_CONTRAST_POWER);

	oColor = vec4(HeatmapColor(nearness), 1.0);
}
//@end
