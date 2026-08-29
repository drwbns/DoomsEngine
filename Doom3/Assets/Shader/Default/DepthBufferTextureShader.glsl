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

// Deliberately no Global uniform block here.
//
// This shader used to linearise depth against camNear and camFar from that
// block. It had no callers, and when it was finally given one the whole view
// came out black: the engine already warns that it cannot find
// ambientLightIntensity in Global, so the block the engine fills and the block
// this shader declared did not agree, and camFar read as zero. Dividing by it
// produced a NaN for every pixel.
//
// Depth is expanded with a fixed curve instead, which needs no uniforms at all
// and so cannot fall out of step with the engine again.

// Applied to the distance from the far plane, not to the depth itself.
//
// A perspective depth buffer spends nearly all of its range close to the
// camera, so everything in a scene like this lands between 0.999 and 1.0.
// Raising the depth to a power cannot separate values that close together --
// it just maps them all to one end. Taking a low power of (1 - depth) does,
// because it stretches numbers near zero apart rather than squashing them:
// 0.0005 and 0.01 are a factor of twenty apart and end up a third of the ramp
// apart, instead of both reading as the same blue.
const float DEPTH_CONTRAST_POWER = 0.15;

// Cold to hot, matching the occlusion and overdraw views.
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
	float rawDepth = texture(ColorTexture, oUV0).r;

	// Nothing was drawn here: the buffer is still at its cleared far value.
	if (rawDepth >= 0.9999)
	{
		oColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	// Near reads hot, which is the same sense as the tile depth debugger.
	float nearness = pow(clamp(1.0 - rawDepth, 0.0, 1.0), DEPTH_CONTRAST_POWER);

	oColor = vec4(HeatmapColor(nearness), 1.0);
}
//@end
