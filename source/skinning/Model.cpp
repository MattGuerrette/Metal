////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "Model.hpp"

#include <stdexcept>
#include <fmt/core.h>

#include "FileUtil.hpp"


#define gltf_model_load_attribute(accessor, component_count, type, dest)                     \
  {                                                                                               \
    int n = 0;                                                                                    \
    type *buffer = (type *) accessor->buffer_view->buffer->data +                                 \
                   accessor->buffer_view->offset / sizeof (type) +                                \
                   accessor->offset / sizeof (type);                                              \
    for (unsigned int k = 0; k < accessor->count; k++)                                            \
      {                                                                                           \
        for (int l = 0; l < component_count; l++)                                                 \
          {                                                                                       \
            dest[component_count * k + l] = buffer[n + l];                                        \
          }                                                                                       \
        n += (int) (accessor->stride / sizeof (type));                                            \
      }                                                                                           \
  }


static int32_t
primitive_get_accessor_for_type(const cgltf_primitive* primitive,
	cgltf_attribute_type attribute_type)
{
	for (int32_t i = 1; i < primitive->attributes_count; i++)
	{
		const cgltf_attribute* attribute = &primitive->attributes[i];
		if (attribute->type == attribute_type)
		{
			return i;
		}
	}

	return -1;
}

static void
mesh_calculate_vertex_index_count(const cgltf_mesh* mesh,
	uint32_t* vertex_count,
	uint32_t* uv_count,
	uint32_t* index_count)
{
	*vertex_count = 0;
	*uv_count = 0;
	*index_count = 0;

	for (int32_t i = 0; i < mesh->primitives_count; i++)
	{
		const cgltf_primitive* primitive = &mesh->primitives[i];
		*index_count += primitive->indices->count;

		for (int32_t j = 0; j < primitive->attributes_count; j++)
		{
			const cgltf_attribute* attribute = &primitive->attributes[j];
			if (attribute->type == cgltf_attribute_type_position)
			{
				*vertex_count += attribute->data->count;
			}
			else if (attribute->type == cgltf_attribute_type_texcoord)
			{
				*uv_count += attribute->data->count;
			}
		}
	}
}

Model::Model(const std::string& fileName)
{
	auto filePath = FileUtil::PathForResource(fileName);
	NS::String* nsFilePath = NS::String::string(filePath.c_str(), NS::UTF8StringEncoding);
	NS::Data* data = NS::Data::data(nsFilePath);
	nsFilePath->release();
	if (data == nullptr)
	{
		throw std::runtime_error(fmt::format("Failed to load file: {}", fileName));
	}

	cgltf_options options = {};
	cgltf_data* model_data = nullptr;
	cgltf_result result = cgltf_parse(&options, data->bytes(), data->length(), &model_data);
	if (result != cgltf_result_success)
	{
		throw std::runtime_error(fmt::format("Failed to parse GLTF file: {}", fileName));
	}

	result = cgltf_load_buffers(&options, model_data, NULL);
	if (result != cgltf_result_success)
	{
		throw std::runtime_error("Failed to load GLTF buffers");
	}


	for (int32_t i = 0; i < model_data->meshes_count; i++)
	{
		const cgltf_mesh* mesh = &model_data->meshes[i];

		uint32_t vertex_count = 0;
		uint32_t uv_count = 0;
		mesh_calculate_vertex_index_count(mesh, &vertex_count, &uv_count,
			&priv->meshes[i].index_count);

		float* positions = new float[vertex_count * 3];
		float* texcoords = new float[uv_count * 2];
		uint16_t* indices = new uint16_t[priv->meshes[i].index_count);

		/*
		 * Access the primitive attributes to get accessor indices
		 */
		for (gint j = 0; j < mesh->primitives_count; j++)
		{
			const cgltf_primitive* primitive = &mesh->primitives[j];
			if (primitive->type != cgltf_primitive_type_triangles)
				continue;

			const cgltf_accessor* accessor = NULL;
			for (gint attr = 0; attr < primitive->attributes_count; attr++)
			{
				const cgltf_attribute* attribute = &primitive->attributes[attr];
				accessor = attribute->data;
				if (attribute->type == cgltf_attribute_type_position)
				{
					g_assert(accessor->type == cgltf_type_vec3 &&
						accessor->component_type == cgltf_component_type_r_32f);
					gk_vulkan_model_load_attribute(accessor, 3, float, positions);
				}
				else if (attribute->type == cgltf_attribute_type_normal)
				{
				}
				else if (attribute->type == cgltf_attribute_type_texcoord)
				{

					g_assert(accessor->type == cgltf_type_vec2 &&
						accessor->component_type == cgltf_component_type_r_32f);
					gk_vulkan_model_load_attribute(accessor, 2, float, texcoords);
				}
			}

			if (primitive->indices != NULL)
			{
				accessor = primitive->indices;
				g_assert(accessor->type == cgltf_type_scalar &&
					accessor->component_type == cgltf_component_type_r_16u);
				gk_vulkan_model_load_attribute(accessor, 1, uint16_t, indices);
			}
		}

		struct Vertex* vertices = g_new0(
		struct Vertex, vertex_count);
		gint32 vertex_index = 0;
		gint32 uv_index = 0;
		for (uint32_t j = 0; j < vertex_count; j++)
		{
			const gint32 v0 = vertex_index;
			const gint32 v1 = vertex_index + 1;
			const gint32 v2 = vertex_index + 2;

			const gint32 uv0 = uv_index;
			const gint32 uv1 = uv_index + 1;

			float posX = positions[v0];
			float posY = positions[v1];
			float posZ = positions[v2];
			graphene_vec3_init(&vertices[j].pos, posX, posY, posZ);
			graphene_vec3_init(&vertices[j].color, 1.0f, 0.0f, 0.0f);

			float uvx = texcoords[uv0];
			float uvy = texcoords[uv1];
			graphene_vec2_init(&vertices[j].uv, uvx, uvy);

			vertex_index += 3;
			uv_index += 2;
		}
		g_free(positions);
		g_free(texcoords);

		const VkDeviceSize vbuffer_size = sizeof(struct Vertex) * vertex_count;
		GKVulkanBuffer* staging_vertex_buffer =
			gk_vulkan_buffer_new(priv->context, vbuffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY);

		void* vertex_data;
		gk_vulkan_buffer_map(staging_vertex_buffer, &vertex_data);
		memcpy(vertex_data, vertices, vbuffer_size);
		gk_vulkan_buffer_unmap(staging_vertex_buffer);

		priv->meshes[i].vertex_buffer = gk_vulkan_buffer_new(priv->context, vbuffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		gk_vulkan_buffer_copy(priv->meshes[i].vertex_buffer, staging_vertex_buffer, command_pool);
		g_clear_object(&staging_vertex_buffer);

		const VkDeviceSize index_buffer_size = sizeof(uint16_t) * priv->meshes[i].index_count;
		GKVulkanBuffer* staging_index_buffer =
			gk_vulkan_buffer_new(priv->context, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY);

		void* index_data;
		gk_vulkan_buffer_map(staging_index_buffer, &index_data);
		memcpy(index_data, indices, index_buffer_size);
		gk_vulkan_buffer_unmap(staging_index_buffer);

		priv->meshes[i].index_buffer = gk_vulkan_buffer_new(priv->context, index_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		gk_vulkan_buffer_copy(priv->meshes[i].index_buffer, staging_index_buffer, command_pool);
		g_clear_object(&staging_index_buffer);
		g_free(indices);
	}

	/*
	 * Load texture images, this assumes the equivalent KTX image is available
	 * TODO: Support multiple texture images
	 */
	priv->texture = gk_vulkan_texture_new(priv->context, "Helmet");
}
