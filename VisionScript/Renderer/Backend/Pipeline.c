#include <stdio.h>
#include <stdlib.h>
#include <shaderc/shaderc.h>
#include "Graphics.h"
#include "Pipeline.h"

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

static void FindBindings(Pipeline * pipeline, PipelineConfig config)
{
	// count number of bindings
	pipeline->bindingCount = 0;
	for (int32_t i = 0; i < config.shaderCount; i++)
	{
		SpvReflectShaderModule module;
		spvReflectCreateShaderModule(config.shaders[i].codeSize, config.shaders[i].code, &module);
		pipeline->bindingCount += module.descriptor_sets[0].binding_count;
		spvReflectDestroyShaderModule(&module);
	}
	
	// initialize bindings array
	pipeline->bindings = malloc(pipeline->bindingCount * sizeof(struct PipelineBinding));
	for (int32_t i = 0, b = 0; i < config.shaderCount; i++)
	{
		SpvReflectShaderModule module;
		spvReflectCreateShaderModule(config.shaders[i].codeSize, config.shaders[i].code, &module);
		for (int32_t j = 0; j < module.descriptor_sets[0].binding_count; j++, b++)
		{
			pipeline->bindings[b].info = *module.descriptor_sets[0].bindings[j];
		}
	}
	
	// remove duplicate bindings
	int32_t duplicateCount = 0;
	for (int32_t i = 0; i < pipeline->bindingCount; i++)
	{
		for (int32_t j = 0; j < i; j++)
		{
			if (pipeline->bindings[j].info.binding == pipeline->bindings[i].info.binding) { duplicateCount++; break; }
		}
	}
	struct PipelineBinding * bindings = malloc((pipeline->bindingCount - duplicateCount) * sizeof(struct PipelineBinding));
	for (int32_t i = 0, c = 0; i < pipeline->bindingCount; i++)
	{
		bool duplicate = false;
		for (int32_t j = 0; j < i; j++)
		{
			if (pipeline->bindings[j].info.binding == pipeline->bindings[i].info.binding) { duplicate = true; break; }
		}
		if (!duplicate) { bindings[c++] = pipeline->bindings[i]; }
	}
	free(pipeline->bindings);
	pipeline->bindingCount -= duplicateCount;
	pipeline->bindings = bindings;
}

static void CreateDescriptorSetLayout(Pipeline * pipeline)
{
	if (pipeline->bindingCount == 0) { return; }
	VkDescriptorSetLayoutBinding * bindings = malloc(pipeline->bindingCount * sizeof(VkDescriptorSetLayoutBinding));
	for (int32_t i = 0; i < pipeline->bindingCount; i++)
	{
		bindings[i] = (VkDescriptorSetLayoutBinding)
		{
			.binding = pipeline->bindings[i].info.binding,
			.descriptorCount = pipeline->bindings[i].info.count,
			.descriptorType = (VkDescriptorType)pipeline->bindings[i].info.descriptor_type,
			.stageFlags = VK_SHADER_STAGE_ALL,
		};
	}
	VkDescriptorSetLayoutCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = pipeline->bindingCount,
		.pBindings = bindings,
	};
	VkResult result = vkCreateDescriptorSetLayout(graphics.device, &createInfo, NULL, &pipeline->descriptorSetLayout);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to create pipeline but failed to create VkDescriptorSetLayout: %i", result); }
	free(bindings);
}

static void CreateDescriptorPool(Pipeline * pipeline)
{
	if (pipeline->bindingCount == 0) { return; }
	// count how many of each descriptor type
	int32_t uniformCount = 0, storageCount = 0, samplerCount = 0;
	for (int32_t i = 0; i < pipeline->bindingCount; i++)
	{
		if (pipeline->bindings[i].info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) { uniformCount += pipeline->bindings[i].info.count; }
		if (pipeline->bindings[i].info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) { storageCount += pipeline->bindings[i].info.count; }
		if (pipeline->bindings[i].info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) { samplerCount += pipeline->bindings[i].info.count; }
	}
	
	VkDescriptorPoolSize poolSizes[3];
	int32_t count = 0;
	if (uniformCount > 0)
	{
		poolSizes[count++] = (VkDescriptorPoolSize){ .descriptorCount = uniformCount * FRAMES_IN_FLIGHT, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
	}
	if (samplerCount > 0)
	{
		poolSizes[count++] = (VkDescriptorPoolSize){ .descriptorCount = samplerCount * FRAMES_IN_FLIGHT, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
	}
	if (storageCount > 0)
	{
		poolSizes[count++] = (VkDescriptorPoolSize){ .descriptorCount = storageCount * FRAMES_IN_FLIGHT, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
	}
	
	// create the descriptor pool
	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = FRAMES_IN_FLIGHT,
		.poolSizeCount = count,
		.pPoolSizes = poolSizes,
	};
	VkResult result = vkCreateDescriptorPool(graphics.device, &poolInfo, NULL, &pipeline->descriptorPool);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to create pipeline but failed to create VkDescriptorPool: %i", result); }
}

static void CreatePipelineLayout(Pipeline * pipeline)
{
	// create the pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = pipeline->bindingCount > 0 ? 1 : 0,
		.pSetLayouts = &pipeline->descriptorSetLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	};
	VkResult result = vkCreatePipelineLayout(graphics.device, &pipelineLayoutCreateInfo, NULL, &pipeline->layout);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to create pipeline but failed to create VkPipelineLayout: %i", result); }
}

static void CreateDescriptorSets(Pipeline * pipeline)
{
	if (pipeline->bindingCount == 0) { return; }
	// create a descriptor set for each frame in flight
	for (int32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorSetAllocateInfo allocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = pipeline->descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &pipeline->descriptorSetLayout,
		};
		VkResult result = vkAllocateDescriptorSets(graphics.device, &allocateInfo, pipeline->descriptorSets + i);
		if (result != VK_SUCCESS) { printf("[Fatal] trying to create pipeline but failed to allocate VkDescriptorSet: %i", result); }
	}
}

static void AllocateBindings(Pipeline * pipeline)
{
	for (int32_t i = 0; i < pipeline->bindingCount; i++)
	{
		SpvReflectDescriptorBinding info = pipeline->bindings[i].info;
		if (info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			UniformBuffer uniform = { .size = info.block.size * info.count, };
			VkBufferCreateInfo bufferInfo =
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = uniform.size,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			};
			VmaAllocationCreateInfo allocationInfo =
			{
				.usage = VMA_MEMORY_USAGE_CPU_ONLY,
			};
			vmaCreateBuffer(graphics.allocator, &bufferInfo, &allocationInfo, &uniform.buffer, &uniform.allocation, &(VmaAllocationInfo){ 0 });
			pipeline->bindings[i].uniform = uniform;
			
			// write to each descriptor set
			for (int32_t j = 0; j < FRAMES_IN_FLIGHT; j++)
			{
				// write to each array element
				for (int32_t k = 0; k < info.count; k++)
				{
					VkDescriptorBufferInfo bufferInfo =
					{
						.buffer = uniform.buffer,
						.offset = k * info.block.size,
						.range = info.block.size,
					};
					VkWriteDescriptorSet writeInfo =
					{
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.dstArrayElement = k,
						.dstBinding = info.binding,
						.dstSet = pipeline->descriptorSets[j],
						.pBufferInfo = &bufferInfo,
					};
					vkUpdateDescriptorSets(graphics.device, 1, &writeInfo, 0, NULL);
				}
			}
		}
		if (pipeline->bindings[i].info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			
		}
		if (pipeline->bindings[i].info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			
		}
	}
}

static void CreateLayout(Pipeline * pipeline, PipelineConfig config)
{
	FindBindings(pipeline, config);
	CreateDescriptorSetLayout(pipeline);
	CreateDescriptorPool(pipeline);
	CreatePipelineLayout(pipeline);
	CreateDescriptorSets(pipeline);
	AllocateBindings(pipeline);
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

void PipelineSetUniformMember(Pipeline pipeline, const char * name, int32_t index, const char * member, void * value)
{
	for (int32_t i = 0; i < pipeline.bindingCount; i++)
	{
		SpvReflectDescriptorBinding info = pipeline.bindings[i].info;
		if (strcmp(info.name, name) == 0 && index < info.count)
		{
			for (int32_t j = 0; j < info.block.member_count; j++)
			{
				if (strcmp(info.block.members[j].name, member) == 0)
				{
					void * data;
					vmaMapMemory(graphics.allocator, pipeline.bindings[i].uniform.allocation, &data);
					memcpy(data + info.block.size * index + info.block.members[j].offset, value, info.block.members[j].size);
					vmaUnmapMemory(graphics.allocator, pipeline.bindings[i].uniform.allocation);
				}
			}
		}
	}
}

void PipelineFree(Pipeline pipeline)
{
	vkDeviceWaitIdle(graphics.device);
	vkDestroyPipelineLayout(graphics.device, pipeline.layout, NULL);
	vkDestroyPipeline(graphics.device, pipeline.instance, NULL);
}
