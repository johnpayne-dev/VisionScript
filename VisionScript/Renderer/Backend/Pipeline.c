#include <stdio.h>
#include <stdlib.h>
#include <shaderc/shaderc.h>
#include "Pipeline.h"
#include "Graphics.h"

Shader ShaderCompile(ShaderType type, const char * source)
{
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	
	shaderc_shader_kind stage = 0;
	switch (type)
	{
		case ShaderTypeVertex: stage = shaderc_vertex_shader; break;
		case ShaderTypeFragment: stage = shaderc_fragment_shader; break;
		case ShaderTypeCompute: stage = shaderc_compute_shader; break;
		default: break;
	}
	shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, source, strlen(source), stage, "", "main", NULL);
	shaderc_compilation_status status = shaderc_result_get_compilation_status(result);
	if (status != shaderc_compilation_status_success)
	{
		printf("[Fatal] failed to compile shader: %s\n", shaderc_result_get_error_message(result));
	}
	
	Shader shader =
	{
		.type = type,
		.codeSize = shaderc_result_get_length(result),
	};
	shader.code = malloc(shader.codeSize);
	memcpy(shader.code, shaderc_result_get_bytes(result), shader.codeSize);
	
	shaderc_result_release(result);
	shaderc_compiler_release(compiler);
	return shader;
}

void ShaderCodeFree(Shader shader)
{
	free(shader.code);
}

static void CreateReflectModules(Pipeline * pipeline, PipelineConfig config)
{
	// allocate and set the information for each stage in the pipeline
	pipeline->stageCount = config.shaderCount;
	pipeline->stages = malloc(pipeline->stageCount * sizeof(struct PipelineStage));
	for (int32_t i = 0; i < pipeline->stageCount; i++)
	{
		pipeline->stages[i].shaderType = config.shaders[i].type;
		spvReflectCreateShaderModule(config.shaders[i].codeSize, config.shaders[i].code, &pipeline->stages[i].module);
	}
}

static void CreateDescriptorLayout(Pipeline * pipeline, int32_t * uboCount, int32_t * samplerCount, int32_t * storageCount)
{
	// go through each pipeline stage to look for a descriptor set (currently only supports handling 1 descriptor set)
	uint32_t bindingCount = 0;
	pipeline->usesDescriptors = false;
	for (int32_t i = 0; i < pipeline->stageCount; i++)
	{
		struct PipelineStage * stage = pipeline->stages + i;
		stage->bindingCount = 0;
		
		// get a list of descriptor sets
		uint32_t setCount = 0;
		spvReflectEnumerateDescriptorSets(&stage->module, &setCount, NULL);
		SpvReflectDescriptorSet * sets = malloc(setCount * sizeof(SpvReflectDescriptorSet));
		spvReflectEnumerateDescriptorSets(&stage->module, &setCount, &sets);
		
		// grab the first descriptor set and record the number of bindings
		if (setCount > 0)
		{
			if (setCount > 1) { printf("[Fatal] backend currently only supports 1 descriptor set in shaders\n"); }
			pipeline->usesDescriptors = true;
			stage->descriptorInfo = sets[0];
			stage->bindingCount = stage->descriptorInfo.binding_count;
			bindingCount += stage->descriptorInfo.binding_count;
		}
	}
	
	if (pipeline->usesDescriptors)
	{
		// go through each stage and fill out the information for each binding
		VkDescriptorSetLayoutBinding * layoutBindings = malloc(bindingCount * sizeof(VkDescriptorSetLayoutBinding));
		for (int32_t i = 0, c = 0; i < pipeline->stageCount; i++)
		{
			struct PipelineStage * stage = pipeline->stages + i;
			
			// get the list of descriptors sets again
			uint32_t setCount = 0;
			spvReflectEnumerateDescriptorSets(&stage->module, &setCount, NULL);
			SpvReflectDescriptorSet * sets = malloc(setCount * sizeof(SpvReflectDescriptorSet));
			spvReflectEnumerateDescriptorSets(&stage->module, &setCount, &sets);
			
			// if this stage has descriptors then go though each binding
			if (setCount > 0)
			{
				for (int j = 0; j < stage->descriptorInfo.binding_count; j++, c++)
				{
					// setup the VkDescriptorSetLayoutBinding from each binding in the stage
					SpvReflectDescriptorBinding * binding = stage->descriptorInfo.bindings[j];
					layoutBindings[c] = (VkDescriptorSetLayoutBinding)
					{
						.binding = binding->binding,
						.descriptorCount = binding->count,
						.descriptorType = (VkDescriptorType)binding->descriptor_type,
						.stageFlags = stage->shaderType,
					};
					if (binding->descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) { (*samplerCount) += binding->count; }
					if (binding->descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) { (*uboCount) += binding->count; }
					if (binding->descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) { (*storageCount) += binding->count; }
				}
			}
		}
		
		// create the VkDescriptorSetLayout
		VkDescriptorSetLayoutCreateInfo layoutInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindingCount,
			.pBindings = layoutBindings,
		};
		vkCreateDescriptorSetLayout(graphics.device, &layoutInfo, NULL, &pipeline->descriptorLayout);
		free(layoutBindings);
	}
}

static void CreateDescriptorPool(Pipeline * pipeline, int32_t uboCount, int32_t samplerCount, int32_t storageCount)
{
	if (pipeline->usesDescriptors)
	{
		// go through each binding type and allocate the pool size for it
		VkDescriptorPoolSize poolSizes[3];
		int32_t c = 0;
		
		if (uboCount > 0)
		{
			poolSizes[c++] = (VkDescriptorPoolSize){ .descriptorCount = uboCount * FRAMES_IN_FLIGHT, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
		}
		if (samplerCount > 0)
		{
			poolSizes[c++] = (VkDescriptorPoolSize){ .descriptorCount = samplerCount * FRAMES_IN_FLIGHT, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		}
		if (storageCount > 0)
		{
			poolSizes[c++] = (VkDescriptorPoolSize){ .descriptorCount = storageCount * FRAMES_IN_FLIGHT, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
		}
		
		// create the descriptor pool
		VkDescriptorPoolCreateInfo poolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = FRAMES_IN_FLIGHT,
			.poolSizeCount = c,
			.pPoolSizes = poolSizes,
		};
		vkCreateDescriptorPool(graphics.device, &poolInfo, NULL, &pipeline->descriptorPool);
	}
}

static void CreateDescriptorSets(Pipeline * pipeline)
{
	if (pipeline->usesDescriptors)
	{
		// create a descriptor set for each frame in flight
		pipeline->descriptorSet = malloc(FRAMES_IN_FLIGHT * sizeof(VkDescriptorSet));
		for (int32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorSetAllocateInfo allocateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = pipeline->descriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &pipeline->descriptorLayout,
			};
			vkAllocateDescriptorSets(graphics.device, &allocateInfo, pipeline->descriptorSet + i);
		}
	}
}

static void CreatePipelineLayout(Pipeline * pipeline)
{
	// create the pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = pipeline->usesDescriptors ? 1 : 0,
		.pSetLayouts = &pipeline->descriptorLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	};
	vkCreatePipelineLayout(graphics.device, &pipelineLayoutCreateInfo, NULL, &pipeline->layout);
}

static void CreateLayout(Pipeline * pipeline, PipelineConfig config)
{
	CreateReflectModules(pipeline, config);
	int32_t samplerCount = 0, uboCount = 0, storageCount = 0;
	CreateDescriptorLayout(pipeline, &uboCount, &samplerCount, &storageCount);
	CreateDescriptorPool(pipeline, uboCount, samplerCount, storageCount);
	CreatePipelineLayout(pipeline);
	CreateDescriptorSets(pipeline);
}

Pipeline PipelineCreate(PipelineConfig config)
{
	Pipeline pipeline =
	{
		.isCompute = false,
		.vertexLayout = config.vertexLayout,
	};
	
	// go through each shader stage and grab the module information
	VkPipelineShaderStageCreateInfo shaderInfos[5];
	VkShaderModule modules[5];
	for (int32_t i = 0; i < config.shaderCount; i++)
	{
		VkShaderModuleCreateInfo moduleInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = config.shaders[i].codeSize,
			.pCode = config.shaders[i].code,
		};
		VkResult result = vkCreateShaderModule(graphics.device, &moduleInfo, NULL, modules + i);
		if (result != VK_SUCCESS) { printf("[Fatal] failed to create shader module: %i\n", result); }
		
		shaderInfos[i] = (VkPipelineShaderStageCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = (VkShaderStageFlagBits)config.shaders[i].type,
			.module = modules[i],
			.pName = "main",
		};
	}
	
	// setup the pipeline configuration
	VkPipelineVertexInputStateCreateInfo vertexInput =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &config.vertexLayout.binding,
		.vertexAttributeDescriptionCount = config.vertexLayout.attributeCount,
		.pVertexAttributeDescriptions = config.vertexLayout.attributes,
	};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = (VkPrimitiveTopology)config.primitive,
		.primitiveRestartEnable = VK_FALSE,
	};
	VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = graphics.swapchain.extent.width,
		.height = graphics.swapchain.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	VkRect2D scissor =
	{
		.offset = { 0, 0 },
		.extent = graphics.swapchain.extent,
	};

	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};
	VkPipelineRasterizationStateCreateInfo rasterizationState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = (VkPolygonMode)config.polygonMode,
		.lineWidth = 1.0f,
		.cullMode = (VkCullModeFlagBits)config.cullMode,
		.frontFace = config.cullClockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
	};
	VkPipelineMultisampleStateCreateInfo multisampleState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
	{
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = config.alphaBlend ? VK_TRUE : VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
	};
	VkPipelineColorBlendStateCreateInfo colorBlendState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
	};
	VkPipelineDepthStencilStateCreateInfo depthStencilState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = config.depthTest ? VK_TRUE : VK_FALSE,
		.depthWriteEnable = config.depthWrite ? VK_TRUE : VK_FALSE,
		.depthCompareOp = (VkCompareOp)config.depthCompare,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};
	VkPipelineDynamicStateCreateInfo dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = (VkDynamicState[]){ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR },
	};

	// create the VkPipeline
	CreateLayout(&pipeline, config);
	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = config.shaderCount,
		.pStages = shaderInfos,
		.pVertexInputState = &vertexInput,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = pipeline.layout,
		.renderPass = graphics.swapchain.renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};
	VkResult result = vkCreateGraphicsPipelines(graphics.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline.instance);
	if (result != VK_SUCCESS) { printf("[Fatal] unable to create graphics pipeline: %i\n", result); }
	
	for (int i = 0; i < config.shaderCount; i++)
	{
		vkDestroyShaderModule(graphics.device, modules[i], NULL);
	}
	return pipeline;
}

void PipelineFree(Pipeline pipeline)
{
	vkDeviceWaitIdle(graphics.device);
	vkDestroyPipelineLayout(graphics.device, pipeline.layout, NULL);
	if (pipeline.usesDescriptors)
	{
		free(pipeline.descriptorSet);
		vkDestroyDescriptorSetLayout(graphics.device, pipeline.descriptorLayout, NULL);
		vkDestroyDescriptorPool(graphics.device, pipeline.descriptorPool, NULL);
	}
	for (int32_t i = 0; i < pipeline.stageCount; i++) { spvReflectDestroyShaderModule(&pipeline.stages[i].module); }
	free(pipeline.stages);
	vkDestroyPipeline(graphics.device, pipeline.instance, NULL);
}
