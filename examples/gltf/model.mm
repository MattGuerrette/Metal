//
//  model.m
//  Metal
//
//  Created by Matthew Guerrette on 8/10/22.
//

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <vector>

#include "model.h"

#import <MetalKit/MetalKit.h>

using namespace DirectX;

static void cgltf_query_mesh_vertices_indices(const cgltf_mesh* mesh,
                                              std::vector<Vertex>& vertices,
                                              std::vector<uint16_t>& indices)
{
    for(uint32_t i = 0; i < mesh->primitives_count; i++)
    {
        // Acces each primitive
        const cgltf_primitive* prim = &mesh->primitives[i];
        if(prim->type != cgltf_primitive_type_triangles)
            continue;
        
        const cgltf_accessor* accessor = nullptr;
        
        // Access primtive indices
        if (prim->indices != nullptr)
        {
            accessor = prim->indices;
            assert(accessor->type == cgltf_type_scalar && accessor->component_type == cgltf_component_type_r_16u);

            NSString* indexBinaryPath = [[[NSBundle mainBundle] resourcePath]
                stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:accessor->buffer_view->buffer->uri]];
            NSData* indexBinaryData = [NSData dataWithContentsOfFile:indexBinaryPath];
            
            
            uint16_t* buffer = (uint16_t*)indexBinaryData.bytes +
            (accessor->buffer_view->offset / sizeof(uint16_t)) + (accessor->offset / sizeof(uint16_t));

            int n = 0;
            for(uint32_t k = 0; k < accessor->count; k++)
            {
                indices.push_back(buffer[n]);
                n += (int)(accessor->stride / sizeof(uint16_t));
            }
        }
        
        // Check all attributes for each primitive
        for(uint32_t j = 0; j < prim->attributes_count; j++)
        {
            const cgltf_attribute* attrib = &prim->attributes[j];
            accessor = attrib->data;
            if(attrib->type == cgltf_attribute_type_position)
            {
                // Attribute should be per vertex. So we can safely resize
                // the vertex array to match.
                vertices.resize(accessor->count);
                
                assert(accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f);
                
                
                NSString* vertexBinaryPath = [[[NSBundle mainBundle] resourcePath]
                    stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:accessor->buffer_view->buffer->uri]];
                NSData* vertexBinaryData = [NSData dataWithContentsOfFile:vertexBinaryPath];
                
                // Access positional data
                float* buffer = (float*)vertexBinaryData.bytes +
                (accessor->buffer_view->offset / sizeof(float)) + (accessor->offset / sizeof(float));
                
                int n = 0;
                for(uint32_t k = 0; k < accessor->count; k++)
                {
                    Vertex* v = &vertices[k];
                    v->Position = XMFLOAT4(buffer[n+0], buffer[n+1], buffer[n+2], 1.0f);
                    n += (int)(accessor->stride / sizeof(float));
                }
            }
            else if (attrib->type == cgltf_attribute_type_texcoord)
            {
                assert(accessor->type == cgltf_type_vec2 && accessor->component_type == cgltf_component_type_r_32f);
                NSString* vertexBinaryPath = [[[NSBundle mainBundle] resourcePath]
                    stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:accessor->buffer_view->buffer->uri]];
                NSData* vertexBinaryData = [NSData dataWithContentsOfFile:vertexBinaryPath];
                
                // Acces UV data
                float* buffer = (float*)vertexBinaryData.bytes +
                (accessor->buffer_view->offset / sizeof(float)) + (accessor->offset / sizeof(float));
                
                int n = 0;
                for(uint32_t k = 0; k < accessor->count; k++)
                {
                    Vertex* v = &vertices[k];
                    v->UV = XMFLOAT2(buffer[n+0], buffer[n+1]);
                    n += (int)(accessor->stride / sizeof(float));
                }
            }
        }
    }
}

Model::Model(std::string file, id<MTLDevice> device)
{
    cgltf_options options{};
    cgltf_data* modelData{};
    
    NSString* modelPath = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:file.c_str()]];
    NSData* modelFileData = [NSData dataWithContentsOfFile:modelPath];
    
    cgltf_result result = cgltf_parse(&options, [modelFileData bytes], [modelFileData length], &modelData);
    assert(result == cgltf_result_success);
    
    const auto meshCount = modelData->meshes_count;
    assert(meshCount == 1);
    for(auto i = 0; i < meshCount; i++)
    {
        // Access each mesh
        const cgltf_mesh* mesh = &modelData->meshes[i];
        cgltf_query_mesh_vertices_indices(mesh, Vertices, Indices);
    }
    
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
    
    const auto textureCount = modelData->textures_count;
    for(auto i = 0; i < textureCount; i++)
    {
        const cgltf_texture* texture = &modelData->textures[i];
        const cgltf_image* image = texture->image;
        
        // Load image from URI
        if(image->uri != nullptr && !strcmp(image->uri, "Default_albedo.jpg"))
        {
            NSString* imageResourcePath = [[[NSBundle mainBundle] resourcePath]
                stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:image->uri]];
            NSURL* url = [[NSURL alloc] initFileURLWithPath:imageResourcePath];
            NSError* error = nil;
            
            NSDictionary *textureLoaderOptions = @{
                  MTKTextureLoaderOptionSRGB: @NO,
                  MTKTextureLoaderOptionTextureUsage       : @(MTLTextureUsageShaderRead),
                  MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
                };
            Texture = [textureLoader newTextureWithContentsOfURL:url options:textureLoaderOptions error:&error];
        }
    }
    
    cgltf_free(modelData);
}

Model::~Model()
{
    
}

const std::vector<Vertex>& Model::GetVertices() const
{
    return Vertices;
}

const std::vector<uint16_t>& Model::GetIndices() const
{
    return Indices;
}

id<MTLTexture> Model::GetTexture() const
{
    return Texture;
}
