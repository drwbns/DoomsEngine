#pragma once
#include "Graphics_Core.h"

#include "../API/OpenglAPI.h"

#include "ePrimitiveType.h"
#include <Vector4.h>

#include "DebugGraphics/RenderingDebugger.h"


namespace dooms
{
	class GameCore;
	namespace graphics
	{
		/// <summary>
		/// Graphics Server class
		/// Contain graphics api function wrapper
		/// </summary>
		namespace GraphicsAPI
		{
			enum class D_ENUM eBufferMode : UINT32
			{
				FRONT_LEFT = GL_FRONT_LEFT,
				FRONT_RIGHT = GL_FRONT_RIGHT,
				BACK_LEFT = GL_BACK_LEFT,
				BACK_RIGHT = GL_BACK_RIGHT,
				FRONT = GL_FRONT,
				BACK = GL_BACK,
				LEFT = GL_LEFT,
				RIGHT = GL_RIGHT,
				COLOR_ATTACHMENT0 = GL_COLOR_ATTACHMENT0,
				COLOR_ATTACHMENT1 = GL_COLOR_ATTACHMENT1,
				COLOR_ATTACHMENT2 = GL_COLOR_ATTACHMENT2,
				COLOR_ATTACHMENT3 = GL_COLOR_ATTACHMENT3,
				COLOR_ATTACHMENT4 = GL_COLOR_ATTACHMENT4,
				COLOR_ATTACHMENT5 = GL_COLOR_ATTACHMENT5,
				COLOR_ATTACHMENT6 = GL_COLOR_ATTACHMENT6,
				COLOR_ATTACHMENT7 = GL_COLOR_ATTACHMENT7,
				COLOR_ATTACHMENT8 = GL_COLOR_ATTACHMENT8,
				COLOR_ATTACHMENT9 = GL_COLOR_ATTACHMENT9,
				COLOR_ATTACHMENT10 = GL_COLOR_ATTACHMENT10,
				NONE = GL_NONE
			};

			enum class D_ENUM eCullFaceMode : UINT32
			{
				FRONT = GL_FRONT,
				BACK = GL_BACK,
				FRONT_AND_BACK = GL_FRONT_AND_BACK
			};

			enum class D_ENUM eBufferBitType : UINT32
			{
				COLOR = GL_COLOR_BUFFER_BIT,
				DEPTH = GL_DEPTH_BUFFER_BIT,
				DEPTH_STENCIL = GL_STENCIL_BUFFER_BIT
			};

			FORCE_INLINE extern void ReadBuffer(eBufferMode mode) noexcept
			{
				glReadBuffer(static_cast<UINT32>(mode));
			}
			FORCE_INLINE extern void WriteBuffer(eBufferMode mode) noexcept
			{
				glDrawBuffer(static_cast<UINT32>(mode));
			}
			FORCE_INLINE extern void WriteBuffer(const INT32 count, const eBufferMode* modes) noexcept
			{
				glDrawBuffers(count, reinterpret_cast<const UINT32*>(modes));
			}
			FORCE_INLINE extern void CullFace(eCullFaceMode mode) noexcept
			{
				glCullFace(static_cast<UINT32>(mode));
			}

			FORCE_INLINE extern void DefaultClearColor(FLOAT32 r, FLOAT32 g, FLOAT32 b, FLOAT32 a) noexcept
			{
				glClearColor(r, g, b, a);
			}

			FORCE_INLINE extern void DefaultClearColor(const math::Vector4& color) noexcept
			{
				glClearColor(color.r, color.g, color.b, color.a);
			}

			enum class D_ENUM eBufferType : UINT32
			{
				COLOR = GL_COLOR,
				DEPTH = GL_COLOR,
				DEPTH_STENCIL = GL_COLOR
			};

			FORCE_INLINE extern void ClearSpecificBuffer(const INT32 targetBufferCount, const eBufferType* bufferType, const GraphicsAPI::eBufferMode* targetBuffer, const math::Vector4* color) noexcept
			{
				GraphicsAPI::WriteBuffer(targetBufferCount, targetBuffer);

				for (INT32 i = 0; i < targetBufferCount; i++)
				{
					glClearBufferfv(static_cast<GLenum>(bufferType[i]), i, color[i].data());
				}

			}
						
			FORCE_INLINE extern void ClearDepth() noexcept
			{
				glClearDepth(1);
			}

			FORCE_INLINE extern void ClearDepth(FLOAT64 depth) noexcept
			{
				glClearDepth(depth);
			}

			enum class D_ENUM eDepthFuncType
			{
				ALWAYS = GL_ALWAYS,
				NEVER = GL_NEVER,
				LESS = GL_LESS,
				EQUAL = GL_EQUAL,
				LEQUAL = GL_LEQUAL,
				GREATER = GL_GREATER,
				NOTEQUAL = GL_NOTEQUAL,
				GEQUAL = GL_GEQUAL
			};
			FORCE_INLINE extern void DepthFunc(eDepthFuncType depthFuncType) noexcept
			{
				glDepthFunc(static_cast<UINT32>(depthFuncType));

			}

			FORCE_INLINE extern void DepthMask(const bool isWriteDepthBuffer)
			{
				glDepthMask(static_cast<unsigned char>(isWriteDepthBuffer));
			}

			FORCE_INLINE extern void ClearStencil(UINT32 stencil) noexcept
			{
				glClearStencil(stencil);
			}

			enum class D_ENUM eClearMask : UINT32
			{
				COLOR_BUFFER_BIT = GL_COLOR_BUFFER_BIT,
				DEPTH_BUFFER_BIT = GL_DEPTH_BUFFER_BIT,
				STENCIL_BUFFER_BIT = GL_STENCIL_BUFFER_BIT
			};

			FORCE_INLINE extern void Clear(eClearMask mask1) noexcept
			{
				glClear(static_cast<UINT32>(mask1));
			}
			FORCE_INLINE extern void Clear(eClearMask mask1, eClearMask mask2) noexcept
			{
				glClear(static_cast<UINT32>(mask1) | static_cast<UINT32>(mask2));
			}
			FORCE_INLINE extern void Clear(eClearMask mask1, eClearMask mask2, eClearMask mask3) noexcept
			{
				glClear(static_cast<UINT32>(mask1) | static_cast<UINT32>(mask2) | static_cast<UINT32>(mask3));
			}
			FORCE_INLINE extern void Clear(UINT32 clearBitFlags) noexcept
			{
				glClear(clearBitFlags);
			}


			enum class D_ENUM eCapability
			{
				BLEND = GL_BLEND,
				COLOR_LOGIC_OP = GL_COLOR_LOGIC_OP,
				CULL_FACE = GL_CULL_FACE,
				DEPTH_TEST = GL_DEPTH_TEST,
				DITHER = GL_DITHER,
				LINE_SMOOTH = GL_LINE_SMOOTH,
				MULTISAMPLE = GL_MULTISAMPLE,
				POLYGON_OFFSET_FILL = GL_POLYGON_OFFSET_FILL,
				POLYGON_OFFSET_LINE = GL_POLYGON_OFFSET_LINE,
				POLYGON_OFFSET_POINT = GL_POLYGON_OFFSET_POINT,
				POLYGON_SMOOTH = GL_POLYGON_SMOOTH,
				SAMPLE_ALPHA_TO_COVERAGE = GL_SAMPLE_ALPHA_TO_COVERAGE,
				SAMPLE_ALPHA_TO_ONE = GL_SAMPLE_ALPHA_TO_ONE,
				SAMPLE_COVERAGE = GL_SAMPLE_COVERAGE,
				SCISSOR_TEST = GL_SCISSOR_TEST,
				STENCIL_TEST = GL_STENCIL_TEST,
				TEXTURE_1D = GL_TEXTURE_1D,
				TEXTURE_2D = GL_TEXTURE_2D,
				TEXTURE_3D = GL_TEXTURE_3D,
				TEXTURE_CUBE_MAP = GL_TEXTURE_CUBE_MAP,
				VERTEX_PROGRAM_POINT_SIZE = GL_VERTEX_PROGRAM_POINT_SIZE,
				DEBUG_OUTPUT = GL_DEBUG_OUTPUT,
				DEBUG_OUTPUT_SYNCHRONOUS = GL_DEBUG_OUTPUT_SYNCHRONOUS
			};

			FORCE_INLINE extern void Enable(eCapability c) noexcept
			{
				glEnable(static_cast<UINT32>(c));
			}

			FORCE_INLINE extern void Disable(eCapability c) noexcept
			{
				glDisable(static_cast<UINT32>(c));
			}

			enum class D_ENUM eSourceFactor : UINT32
			{
				ZERO = GL_ZERO,
				ONE = GL_ONE,
				SRC_COLOR = GL_SRC_COLOR,
				ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR,
				DST_COLOR = GL_DST_COLOR,
				ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR,
				SRC_ALPHA = GL_SRC_ALPHA,
				ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA,
				DST_ALPHA = GL_DST_ALPHA,
				ONE_MINUS_DST_ALPHA = GL_ONE_MINUS_DST_ALPHA,
				CONSTANT_COLOR = GL_CONSTANT_COLOR,
				ONE_MINUS_CONSTANT_COLOR = GL_ONE_MINUS_CONSTANT_COLOR,
				CONSTANT_ALPHA = GL_CONSTANT_ALPHA,
				ONE_MINUS_CONSTANT_ALPHA = GL_ONE_MINUS_CONSTANT_ALPHA,
				SRC_ALPHA_SATURATE = GL_SRC_ALPHA_SATURATE,
				SRC1_COLOR = GL_SRC1_COLOR,
				ONE_MINUS_SRC1_COLOR = GL_ONE_MINUS_SRC1_COLOR,
				SRC1_ALPHA = GL_SRC1_ALPHA,
				ONE_MINUS_SRC1_ALPHA = GL_ONE_MINUS_SRC1_ALPHA,

			};

			enum class D_ENUM eDestinationFactor : UINT32
			{
				ZERO = GL_ZERO,
				ONE = GL_ONE,
				SRC_COLOR = GL_SRC_COLOR,
				ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR,
				DST_COLOR = GL_DST_COLOR,
				ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR,
				SRC_ALPHA = GL_SRC_ALPHA,
				ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA,
				DST_ALPHA = GL_DST_ALPHA,
				ONE_MINUS_DST_ALPHA = GL_ONE_MINUS_DST_ALPHA,
				CONSTANT_COLOR = GL_CONSTANT_COLOR,
				ONE_MINUS_CONSTANT_COLOR = GL_ONE_MINUS_CONSTANT_COLOR,
				CONSTANT_ALPHA = GL_CONSTANT_ALPHA,
				ONE_MINUS_CONSTANT_ALPHA = GL_ONE_MINUS_CONSTANT_ALPHA,

			};
			FORCE_INLINE extern void BlendFunc(eSourceFactor sourceFactor, eDestinationFactor destinationFactor) noexcept
			{
				glBlendFunc(static_cast<UINT32>(sourceFactor), static_cast<UINT32>(destinationFactor));
			}

			enum class D_ENUM eFrontFaceMode : UINT32
			{
				CW = GL_CW,
				CCW = GL_CCW

			};

			FORCE_INLINE extern void FrontFace(eFrontFaceMode faceMode) noexcept
			{
				glFrontFace(static_cast<UINT32>(faceMode));
			}

			FORCE_INLINE extern void ViewPort(INT32 x, INT32 y, INT32 width, INT32 height) noexcept
			{
				glViewport(x, y, width, height);
			}

		

			enum class D_ENUM GetIntegerParameter
			{
				ACTIVE_TEXTURE = GL_ACTIVE_TEXTURE,
				ALIASED_LINE_WIDTH_RANGE = GL_ALIASED_LINE_WIDTH_RANGE,
				ARRAY_BUFFER_BINDING = GL_ARRAY_BUFFER_BINDING,
				BLEND = GL_BLEND,
				BLEND_COLOR = GL_BLEND_COLOR,
				BLEND_DST_ALPHA = GL_BLEND_DST_ALPHA,
				BLEND_DST_RGB = GL_BLEND_DST_RGB,
				BLEND_EQUATION_RGB = GL_BLEND_EQUATION_RGB,
				BLEND_EQUATION_ALPHA = GL_BLEND_EQUATION_ALPHA,
				BLEND_SRC_ALPHA = GL_BLEND_SRC_ALPHA,
				BLEND_SRC_RGB = GL_BLEND_SRC_RGB,
				COLOR_CLEAR_VALUE = GL_COLOR_CLEAR_VALUE,
				COLOR_LOGIC_OP = GL_COLOR_LOGIC_OP,
				COLOR_WRITEMASK = GL_COLOR_WRITEMASK,
				COMPRESSED_TEXTURE_FORMATS = GL_COMPRESSED_TEXTURE_FORMATS,
				MAX_COMPUTE_SHADER_STORAGE_BLOCKS = GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,
				MAX_COMBINED_SHADER_STORAGE_BLOCKS = GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,
				MAX_COMPUTE_UNIFORM_BLOCKS = GL_MAX_COMPUTE_UNIFORM_BLOCKS,
				MAX_COMPUTE_TEXTURE_IMAGE_UNITS = GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,
				MAX_COMPUTE_UNIFORM_COMPONENTS = GL_MAX_COMPUTE_UNIFORM_COMPONENTS,
				MAX_COMPUTE_ATOMIC_COUNTERS = GL_MAX_COMPUTE_ATOMIC_COUNTERS,
				MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS = GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS,
				MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS = GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS,
				MAX_COMPUTE_WORK_GROUP_INVOCATIONS = GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
				MAX_COMPUTE_WORK_GROUP_COUNT = GL_MAX_COMPUTE_WORK_GROUP_COUNT,
				MAX_COMPUTE_WORK_GROUP_SIZE = GL_MAX_COMPUTE_WORK_GROUP_SIZE,
				DISPATCH_INDIRECT_BUFFER_BINDING = GL_DISPATCH_INDIRECT_BUFFER_BINDING,
				MAX_DEBUG_GROUP_STACK_DEPTH = GL_MAX_DEBUG_GROUP_STACK_DEPTH,
				DEBUG_GROUP_STACK_DEPTH = GL_DEBUG_GROUP_STACK_DEPTH,
				CONTEXT_FLAGS = GL_CONTEXT_FLAGS,
				CULL_FACE = GL_CULL_FACE,
				CULL_FACE_MODE = GL_CULL_FACE_MODE,
				CURRENT_PROGRAM = GL_CURRENT_PROGRAM,
				DEPTH_CLEAR_VALUE = GL_DEPTH_CLEAR_VALUE,
				DEPTH_FUNC = GL_DEPTH_FUNC,
				DEPTH_RANGE = GL_DEPTH_RANGE,
				DEPTH_TEST = GL_DEPTH_TEST,
				DEPTH_WRITEMASK = GL_DEPTH_WRITEMASK,
				DITHER = GL_DITHER,
				DOUBLEBUFFER = GL_DOUBLEBUFFER,
				DRAW_BUFFER = GL_DRAW_BUFFER,
				DRAW_FRAMEBUFFER_BINDING = GL_DRAW_FRAMEBUFFER_BINDING,
				READ_FRAMEBUFFER_BINDING = GL_READ_FRAMEBUFFER_BINDING,
				ELEMENT_ARRAY_BUFFER_BINDING = GL_ELEMENT_ARRAY_BUFFER_BINDING,
				FRAGMENT_SHADER_DERIVATIVE_HINT = GL_FRAGMENT_SHADER_DERIVATIVE_HINT,
				IMPLEMENTATION_COLOR_READ_FORMAT = GL_IMPLEMENTATION_COLOR_READ_FORMAT,
				IMPLEMENTATION_COLOR_READ_TYPE = GL_IMPLEMENTATION_COLOR_READ_TYPE,
				LINE_SMOOTH = GL_LINE_SMOOTH,
				LINE_SMOOTH_HINT = GL_LINE_SMOOTH_HINT,
				LINE_WIDTH = GL_LINE_WIDTH,
				LAYER_PROVOKING_VERTEX = GL_LAYER_PROVOKING_VERTEX,
				LOGIC_OP_MODE = GL_LOGIC_OP_MODE,
				MAJOR_VERSION = GL_MAJOR_VERSION,
				MAX_3D_TEXTURE_SIZE = GL_MAX_3D_TEXTURE_SIZE,
				MAX_ARRAY_TEXTURE_LAYERS = GL_MAX_ARRAY_TEXTURE_LAYERS,
				MAX_CLIP_DISTANCES = GL_MAX_CLIP_DISTANCES,
				MAX_COLOR_TEXTURE_SAMPLES = GL_MAX_COLOR_TEXTURE_SAMPLES,
				MAX_COMBINED_ATOMIC_COUNTERS = GL_MAX_COMBINED_ATOMIC_COUNTERS,
				MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS = GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS,
				MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS = GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS,
				MAX_COMBINED_TEXTURE_IMAGE_UNITS = GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
				MAX_COMBINED_UNIFORM_BLOCKS = GL_MAX_COMBINED_UNIFORM_BLOCKS,
				MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS = GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS,
				MAX_CUBE_MAP_TEXTURE_SIZE = GL_MAX_CUBE_MAP_TEXTURE_SIZE,
				MAX_DEPTH_TEXTURE_SAMPLES = GL_MAX_DEPTH_TEXTURE_SAMPLES,
				MAX_DRAW_BUFFERS = GL_MAX_DRAW_BUFFERS,
				MAX_DUAL_SOURCE_DRAW_BUFFERS = GL_MAX_DUAL_SOURCE_DRAW_BUFFERS,
				MAX_ELEMENTS_INDICES = GL_MAX_ELEMENTS_INDICES,
				MAX_ELEMENTS_VERTICES = GL_MAX_ELEMENTS_VERTICES,
				MAX_FRAGMENT_ATOMIC_COUNTERS = GL_MAX_FRAGMENT_ATOMIC_COUNTERS,
				MAX_FRAGMENT_SHADER_STORAGE_BLOCKS = GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,
				MAX_FRAGMENT_INPUT_COMPONENTS = GL_MAX_FRAGMENT_INPUT_COMPONENTS,
				MAX_FRAGMENT_UNIFORM_COMPONENTS = GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
				MAX_FRAGMENT_UNIFORM_VECTORS = GL_MAX_FRAGMENT_UNIFORM_VECTORS,
				MAX_FRAGMENT_UNIFORM_BLOCKS = GL_MAX_FRAGMENT_UNIFORM_BLOCKS,
				MAX_FRAMEBUFFER_WIDTH = GL_MAX_FRAMEBUFFER_WIDTH,
				MAX_FRAMEBUFFER_HEIGHT = GL_MAX_FRAMEBUFFER_HEIGHT,
				MAX_FRAMEBUFFER_LAYERS = GL_MAX_FRAMEBUFFER_LAYERS,
				MAX_FRAMEBUFFER_SAMPLES = GL_MAX_FRAMEBUFFER_SAMPLES,
				MAX_GEOMETRY_ATOMIC_COUNTERS = GL_MAX_GEOMETRY_ATOMIC_COUNTERS,
				MAX_GEOMETRY_SHADER_STORAGE_BLOCKS = GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,
				MAX_GEOMETRY_INPUT_COMPONENTS = GL_MAX_GEOMETRY_INPUT_COMPONENTS,
				MAX_GEOMETRY_OUTPUT_COMPONENTS = GL_MAX_GEOMETRY_OUTPUT_COMPONENTS,
				MAX_GEOMETRY_TEXTURE_IMAGE_UNITS = GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,
				MAX_GEOMETRY_UNIFORM_BLOCKS = GL_MAX_GEOMETRY_UNIFORM_BLOCKS,
				MAX_GEOMETRY_UNIFORM_COMPONENTS = GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,
				MAX_INTEGER_SAMPLES = GL_MAX_INTEGER_SAMPLES,
				MIN_MAP_BUFFER_ALIGNMENT = GL_MIN_MAP_BUFFER_ALIGNMENT,
				MAX_LABEL_LENGTH = GL_MAX_LABEL_LENGTH,
				MAX_PROGRAM_TEXEL_OFFSET = GL_MAX_PROGRAM_TEXEL_OFFSET,
				MIN_PROGRAM_TEXEL_OFFSET = GL_MIN_PROGRAM_TEXEL_OFFSET,
				MAX_RECTANGLE_TEXTURE_SIZE = GL_MAX_RECTANGLE_TEXTURE_SIZE,
				MAX_RENDERBUFFER_SIZE = GL_MAX_RENDERBUFFER_SIZE,
				MAX_SAMPLE_MASK_WORDS = GL_MAX_SAMPLE_MASK_WORDS,
				MAX_SERVER_WAIT_TIMEOUT = GL_MAX_SERVER_WAIT_TIMEOUT,
				MAX_SHADER_STORAGE_BUFFER_BINDINGS = GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,
				MAX_TESS_CONTROL_ATOMIC_COUNTERS = GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS,
				MAX_TESS_EVALUATION_ATOMIC_COUNTERS = GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS,
				MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS = GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,
				MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS = GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,
				MAX_TEXTURE_BUFFER_SIZE = GL_MAX_TEXTURE_BUFFER_SIZE,
				MAX_TEXTURE_IMAGE_UNITS = GL_MAX_TEXTURE_IMAGE_UNITS,
				MAX_TEXTURE_LOD_BIAS = GL_MAX_TEXTURE_LOD_BIAS,
				MAX_TEXTURE_SIZE = GL_MAX_TEXTURE_SIZE,
				MAX_UNIFORM_BUFFER_BINDINGS = GL_MAX_UNIFORM_BUFFER_BINDINGS,
				MAX_UNIFORM_BLOCK_SIZE = GL_MAX_UNIFORM_BLOCK_SIZE,
				MAX_UNIFORM_LOCATIONS = GL_MAX_UNIFORM_LOCATIONS,
				MAX_VARYING_COMPONENTS = GL_MAX_VARYING_COMPONENTS,
				MAX_VARYING_VECTORS = GL_MAX_VARYING_VECTORS,
				MAX_VARYING_FLOATS = GL_MAX_VARYING_FLOATS,
				MAX_VERTEX_ATOMIC_COUNTERS = GL_MAX_VERTEX_ATOMIC_COUNTERS,
				MAX_VERTEX_ATTRIBS = GL_MAX_VERTEX_ATTRIBS,
				MAX_VERTEX_SHADER_STORAGE_BLOCKS = GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,
				MAX_VERTEX_TEXTURE_IMAGE_UNITS = GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
				MAX_VERTEX_UNIFORM_COMPONENTS = GL_MAX_VERTEX_UNIFORM_COMPONENTS,
				MAX_VERTEX_UNIFORM_VECTORS = GL_MAX_VERTEX_UNIFORM_VECTORS,
				MAX_VERTEX_OUTPUT_COMPONENTS = GL_MAX_VERTEX_OUTPUT_COMPONENTS,
				MAX_VERTEX_UNIFORM_BLOCKS = GL_MAX_VERTEX_UNIFORM_BLOCKS,
				MAX_VIEWPORT_DIMS = GL_MAX_VIEWPORT_DIMS,
				MAX_VIEWPORTS = GL_MAX_VIEWPORTS,
				MINOR_VERSION = GL_MINOR_VERSION,
				NUM_COMPRESSED_TEXTURE_FORMATS = GL_NUM_COMPRESSED_TEXTURE_FORMATS,
				NUM_EXTENSIONS = GL_NUM_EXTENSIONS,
				NUM_PROGRAM_BINARY_FORMATS = GL_NUM_PROGRAM_BINARY_FORMATS,
				NUM_SHADER_BINARY_FORMATS = GL_NUM_SHADER_BINARY_FORMATS,
				PACK_ALIGNMENT = GL_PACK_ALIGNMENT,
				PACK_IMAGE_HEIGHT = GL_PACK_IMAGE_HEIGHT,
				PACK_LSB_FIRST = GL_PACK_LSB_FIRST,
				PACK_ROW_LENGTH = GL_PACK_ROW_LENGTH,
				PACK_SKIP_IMAGES = GL_PACK_SKIP_IMAGES,
				PACK_SKIP_PIXELS = GL_PACK_SKIP_PIXELS,
				PACK_SKIP_ROWS = GL_PACK_SKIP_ROWS,
				PACK_SWAP_BYTES = GL_PACK_SWAP_BYTES,
				PIXEL_PACK_BUFFER_BINDING = GL_PIXEL_PACK_BUFFER_BINDING,
				PIXEL_UNPACK_BUFFER_BINDING = GL_PIXEL_UNPACK_BUFFER_BINDING,
				POINT_FADE_THRESHOLD_SIZE = GL_POINT_FADE_THRESHOLD_SIZE,
				PRIMITIVE_RESTART_INDEX = GL_PRIMITIVE_RESTART_INDEX,
				PROGRAM_BINARY_FORMATS = GL_PROGRAM_BINARY_FORMATS,
				PROGRAM_PIPELINE_BINDING = GL_PROGRAM_PIPELINE_BINDING,
				PROGRAM_POINT_SIZE = GL_PROGRAM_POINT_SIZE,
				PROVOKING_VERTEX = GL_PROVOKING_VERTEX,
				POINT_SIZE = GL_POINT_SIZE,
				POINT_SIZE_GRANULARITY = GL_POINT_SIZE_GRANULARITY,
				POINT_SIZE_RANGE = GL_POINT_SIZE_RANGE,
				POLYGON_OFFSET_FACTOR = GL_POLYGON_OFFSET_FACTOR,
				POLYGON_OFFSET_UNITS = GL_POLYGON_OFFSET_UNITS,
				POLYGON_OFFSET_FILL = GL_POLYGON_OFFSET_FILL,
				POLYGON_OFFSET_LINE = GL_POLYGON_OFFSET_LINE,
				POLYGON_OFFSET_POINT = GL_POLYGON_OFFSET_POINT,
				POLYGON_SMOOTH = GL_POLYGON_SMOOTH,
				POLYGON_SMOOTH_HINT = GL_POLYGON_SMOOTH_HINT,
				READ_BUFFER = GL_READ_BUFFER,
				RENDERBUFFER_BINDING = GL_RENDERBUFFER_BINDING,
				SAMPLE_BUFFERS = GL_SAMPLE_BUFFERS,
				SAMPLE_COVERAGE_VALUE = GL_SAMPLE_COVERAGE_VALUE,
				SAMPLE_COVERAGE_INVERT = GL_SAMPLE_COVERAGE_INVERT,
				SAMPLER_BINDING = GL_SAMPLER_BINDING,
				SAMPLES = GL_SAMPLES,
				SCISSOR_BOX = GL_SCISSOR_BOX,
				SCISSOR_TEST = GL_SCISSOR_TEST,
				SHADER_COMPILER = GL_SHADER_COMPILER,
				SHADER_STORAGE_BUFFER_BINDING = GL_SHADER_STORAGE_BUFFER_BINDING,
				SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT = GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,
				SHADER_STORAGE_BUFFER_START = GL_SHADER_STORAGE_BUFFER_START,
				SHADER_STORAGE_BUFFER_SIZE = GL_SHADER_STORAGE_BUFFER_SIZE,
				SMOOTH_LINE_WIDTH_RANGE = GL_SMOOTH_LINE_WIDTH_RANGE,
				SMOOTH_LINE_WIDTH_GRANULARITY = GL_SMOOTH_LINE_WIDTH_GRANULARITY,
				STENCIL_BACK_FAIL = GL_STENCIL_BACK_FAIL,
				STENCIL_BACK_FUNC = GL_STENCIL_BACK_FUNC,
				STENCIL_BACK_PASS_DEPTH_FAIL = GL_STENCIL_BACK_PASS_DEPTH_FAIL,
				STENCIL_BACK_PASS_DEPTH_PASS = GL_STENCIL_BACK_PASS_DEPTH_PASS,
				STENCIL_BACK_REF = GL_STENCIL_BACK_REF,
				STENCIL_BACK_VALUE_MASK = GL_STENCIL_BACK_VALUE_MASK,
				STENCIL_BACK_WRITEMASK = GL_STENCIL_BACK_WRITEMASK,
				STENCIL_CLEAR_VALUE = GL_STENCIL_CLEAR_VALUE,
				STENCIL_FAIL = GL_STENCIL_FAIL,
				STENCIL_FUNC = GL_STENCIL_FUNC,
				STENCIL_PASS_DEPTH_FAIL = GL_STENCIL_PASS_DEPTH_FAIL,
				STENCIL_PASS_DEPTH_PASS = GL_STENCIL_PASS_DEPTH_PASS,
				STENCIL_REF = GL_STENCIL_REF,
				STENCIL_TEST = GL_STENCIL_TEST,
				STENCIL_VALUE_MASK = GL_STENCIL_VALUE_MASK,
				STENCIL_WRITEMASK = GL_STENCIL_WRITEMASK,
				STEREO = GL_STEREO,
				SUBPIXEL_BITS = GL_SUBPIXEL_BITS,
				TEXTURE_BINDING_1D = GL_TEXTURE_BINDING_1D,
				TEXTURE_BINDING_1D_ARRAY = GL_TEXTURE_BINDING_1D_ARRAY,
				TEXTURE_BINDING_2D = GL_TEXTURE_BINDING_2D,
				TEXTURE_BINDING_2D_ARRAY = GL_TEXTURE_BINDING_2D_ARRAY,
				TEXTURE_BINDING_2D_MULTISAMPLE = GL_TEXTURE_BINDING_2D_MULTISAMPLE,
				TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY = GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY,
				TEXTURE_BINDING_3D = GL_TEXTURE_BINDING_3D,
				TEXTURE_BINDING_BUFFER = GL_TEXTURE_BINDING_BUFFER,
				TEXTURE_BINDING_CUBE_MAP = GL_TEXTURE_BINDING_CUBE_MAP,
				TEXTURE_BINDING_RECTANGLE = GL_TEXTURE_BINDING_RECTANGLE,
				TEXTURE_COMPRESSION_HINT = GL_TEXTURE_COMPRESSION_HINT,
				TEXTURE_BUFFER_OFFSET_ALIGNMENT = GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT,
				TIMESTAMP = GL_TIMESTAMP,
				TRANSFORM_FEEDBACK_BUFFER_BINDING = GL_TRANSFORM_FEEDBACK_BUFFER_BINDING,
				TRANSFORM_FEEDBACK_BUFFER_START = GL_TRANSFORM_FEEDBACK_BUFFER_START,
				TRANSFORM_FEEDBACK_BUFFER_SIZE = GL_TRANSFORM_FEEDBACK_BUFFER_SIZE,
				UNIFORM_BUFFER_BINDING = GL_UNIFORM_BUFFER_BINDING,
				UNIFORM_BUFFER_OFFSET_ALIGNMENT = GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
				UNIFORM_BUFFER_SIZE = GL_UNIFORM_BUFFER_SIZE,
				UNIFORM_BUFFER_START = GL_UNIFORM_BUFFER_START,
				UNPACK_ALIGNMENT = GL_UNPACK_ALIGNMENT,
				UNPACK_IMAGE_HEIGHT = GL_UNPACK_IMAGE_HEIGHT,
				UNPACK_LSB_FIRST = GL_UNPACK_LSB_FIRST,
				UNPACK_ROW_LENGTH = GL_UNPACK_ROW_LENGTH,
				UNPACK_SKIP_IMAGES = GL_UNPACK_SKIP_IMAGES,
				UNPACK_SKIP_PIXELS = GL_UNPACK_SKIP_PIXELS,
				UNPACK_SKIP_ROWS = GL_UNPACK_SKIP_ROWS,
				UNPACK_SWAP_BYTES = GL_UNPACK_SWAP_BYTES,
				VERTEX_ARRAY_BINDING = GL_VERTEX_ARRAY_BINDING,
				VERTEX_BINDING_DIVISOR = GL_VERTEX_BINDING_DIVISOR,
				VERTEX_BINDING_OFFSET = GL_VERTEX_BINDING_OFFSET,
				VERTEX_BINDING_STRIDE = GL_VERTEX_BINDING_STRIDE,
				VERTEX_BINDING_BUFFER = GL_VERTEX_BINDING_BUFFER,
				MAX_VERTEX_ATTRIB_RELATIVE_OFFSET = GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET,
				MAX_VERTEX_ATTRIB_BINDINGS = GL_MAX_VERTEX_ATTRIB_BINDINGS,
				VIEWPORT = GL_VIEWPORT,
				VIEWPORT_BOUNDS_RANGE = GL_VIEWPORT_BOUNDS_RANGE,
				VIEWPORT_INDEX_PROVOKING_VERTEX = GL_VIEWPORT_INDEX_PROVOKING_VERTEX,
				VIEWPORT_SUBPIXEL_BITS = GL_VIEWPORT_SUBPIXEL_BITS,
				MAX_ELEMENT_INDEX = GL_MAX_ELEMENT_INDEX,


			};

			enum class D_ENUM GetStringParameter
			{
				VENDOR = GL_VENDOR,
				RENDERER = GL_VENDOR,
				VERSION = GL_VENDOR,
				SHADING_LANGUAGE_VERSION = GL_VENDOR,
			};
			
			FORCE_INLINE extern int64_t GetInteger64v(const GetIntegerParameter pname)
			{
				int64_t value{ 0 };
				glGetInteger64v(static_cast<const UINT32>(pname), &value);
				return value;
			}

			FORCE_INLINE extern const char* GetString(const GetStringParameter pname)
			{
				return reinterpret_cast<const char*>(glGetString(static_cast<const UINT32>(pname)));
			}

			
			FORCE_INLINE extern void DrawArray(const ePrimitiveType mode, const INT32 first, const INT32 count)
			{
				glDrawArrays(static_cast<const UINT32>(mode), first, count);

				INCREMENT_DRAWCALL_COUNTER;
			}

			FORCE_INLINE extern void DrawElement(const ePrimitiveType mode, const INT32 count, const UINT32 type, const void* indices)
			{
				glDrawElements(static_cast<const UINT32>(mode), count, type, indices);

				INCREMENT_DRAWCALL_COUNTER;
			}

			FORCE_INLINE extern void DrawArraysInstanced(ePrimitiveType mode, INT32 first, UINT32 count, INT32 instancecount)
			{
				glDrawArraysInstanced(static_cast<UINT32>(mode), first, count, instancecount);

				INCREMENT_DRAWCALL_COUNTER;
			}

			FORCE_INLINE extern void DrawElementsInstanced(ePrimitiveType mode, INT32 count, UINT32 type, const void* indices, INT32 instancecount)
			{
				glDrawElementsInstanced(static_cast<UINT32>(mode), count, type, indices, instancecount);

				INCREMENT_DRAWCALL_COUNTER;
			}

// 			enum class D_ENUM eQueryType
// 			{
// 				SAMPLES_PASSED = GL_SAMPLES_PASSED,
// 				ANY_SAMPLES_PASSED = GL_ANY_SAMPLES_PASSED,
// 				ANY_SAMPLES_PASSED_CONSERVATIVE = GL_ANY_SAMPLES_PASSED_CONSERVATIVE,
// 				PRIMITIVES_GENERATED = GL_PRIMITIVES_GENERATED,
// 				TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN = GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
// 				TIME_ELAPSED = GL_TIME_ELAPSED
// 			};
// 
// 			FORCE_INLINE extern void GenQueries(INT32 size, UINT32* ids)
// 			{
// 				glGenQueries(size, ids);
// 			}
// 
// 			FORCE_INLINE extern void BeginQuery(eQueryType queryType, UINT32 id)
// 			{
// 				glBeginQuery(static_cast<UINT32>(queryType), id);
// 			}
// 			FORCE_INLINE extern void EndQuery(eQueryType queryType)
// 			{
// 				glEndQuery(static_cast<UINT32>(queryType));
// 			}

		
		};
	}
}