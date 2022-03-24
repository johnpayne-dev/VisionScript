#ifndef Pipeline_h
#define Pipeline_h
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>
#include <stdbool.h>
#include "VertexBuffer.h"

typedef enum ShaderType
{
	ShaderTypeVertex = VK_SHADER_STAGE_VERTEX_BIT,
	ShaderTypeFragment = VK_SHADER_STAGE_FRAGMENT_BIT,
	ShaderTypeCompute = VK_SHADER_STAGE_COMPUTE_BIT,
} ShaderType;

typedef enum PolygonMode
{
	PolygonModeFill = VK_POLYGON_MODE_FILL,
	PolygonModeLine = VK_POLYGON_MODE_LINE,
	PolygonModePoint = VK_POLYGON_MODE_POINT,
} PolygonMode;

typedef enum VertexPrimitive
{
	VertexPrimitiveTriangleList = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VertexPrimitiveTriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
	VertexPrimitiveTriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	VertexPrimitiveLineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	VertexPrimitiveLineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	VertexPrimitivePointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
} VertexPrimitive;

typedef enum CullMode
{
	CullModeNone = VK_CULL_MODE_NONE,
	CullModeFront = VK_CULL_MODE_FRONT_BIT,
	CullModeBack = VK_CULL_MODE_BACK_BIT,
	CullModeFrontAndBack = VK_CULL_MODE_FRONT_AND_BACK,
} CullMode;

typedef enum CompareOperation
{
	CompareOperationAlways = VK_COMPARE_OP_ALWAYS,
	CompareOperationNever = VK_COMPARE_OP_NEVER,
	CompareOperationEqual = VK_COMPARE_OP_EQUAL,
	CompareOperationLess = VK_COMPARE_OP_LESS,
	CompareOperationGreater = VK_COMPARE_OP_GREATER,
	CompareOperationLessOrEqual = VK_COMPARE_OP_LESS_OR_EQUAL,
	CompareOperationGreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
	CompareOperationNotEqual = VK_COMPARE_OP_NOT_EQUAL,
} CompareOperation;

typedef struct ShaderCode
{
	ShaderType type;
	uint64_t codeSize;
	void * code;
} ShaderCode;

ShaderCode ShaderCompile(ShaderType type, const char * source);

typedef struct PipelineConfig
{
	VertexLayout vertexLayout;
	int32_t shaderCount;
	ShaderCode shaders[5];
	VertexPrimitive primitive;
	PolygonMode polygonMode;
	CullMode cullMode;
	bool cullClockwise;
	bool alphaBlend;
	bool depthTest;
	bool depthWrite;
	CompareOperation depthCompare;
} PipelineConfig;

typedef struct Pipeline
{
	bool isCompute;
	VkPipeline instance;
	VkPipelineLayout layout;
	VertexLayout vertexLayout;
	
	int32_t stageCount;
	struct PipelineStage
	{
		ShaderType shaderType;
		SpvReflectShaderModule module;
		int32_t bindingCount;
		SpvReflectDescriptorSet descriptorInfo;
	} * stages;
	bool usesDescriptors;
	VkDescriptorSetLayout descriptorLayout;
	VkDescriptorPool descriptorPool;
	int32_t descriptorPoolCapacity;
	VkDescriptorSet * descriptorSet;
} Pipeline;

Pipeline PipelineCreate(PipelineConfig config);

void PipelineFree(Pipeline pipeline);

typedef Pipeline ComputePipeline;

ComputePipeline ComputePipelineCreate(ShaderCode shader);

void ComputePipelineFree(ComputePipeline pipeline);

#endif
