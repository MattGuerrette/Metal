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
            else if (attrib->type == cgltf_attribute_type_color)
            {
                assert(accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f);
                
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
                    v->Color = XMFLOAT4(buffer[n+0], buffer[n+1], buffer[n+2], buffer[n+3]);
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
            else if (attrib->type == cgltf_attribute_type_joints)
            {
                assert(accessor->type == cgltf_type_vec4 &&
                       (accessor->component_type == cgltf_component_type_r_8u ||
                        accessor->component_type == cgltf_component_type_r_16u));
                
                NSString* vertexBinaryPath = [[[NSBundle mainBundle] resourcePath]
                    stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:accessor->buffer_view->buffer->uri]];
                NSData* vertexBinaryData = [NSData dataWithContentsOfFile:vertexBinaryPath];
                
                // Support uint8_t joint indices
                if(accessor->component_type == cgltf_component_type_r_8u)
                {
                    // Acces joint data
                    uint8_t* buffer = (uint8_t*)vertexBinaryData.bytes +
                    (accessor->buffer_view->offset / sizeof(uint8_t)) + (accessor->offset / sizeof(uint8_t));
                    
                    int n = 0;
                    const auto step = (int)(accessor->stride / sizeof(uint8_t));
                    for(uint32_t k = 0; k < accessor->count; k++)
                    {
                        Vertex* v = &vertices[k];
                        
                        v->Joints = XMUINT4(buffer[n+0], buffer[n+1], buffer[n+2], buffer[n+3]);
                        
                        n += step;
                    }
                }
                else if(accessor->component_type == cgltf_component_type_r_16u)
                {
                    // Acces joint data
                    uint16_t* buffer = (uint16_t*)vertexBinaryData.bytes +
                    (accessor->buffer_view->offset / sizeof(uint16_t)) + (accessor->offset / sizeof(uint16_t));
                    
                    int n = 0;
                    const auto step = (int)(accessor->stride / sizeof(uint16_t));
                    for(uint32_t k = 0; k < accessor->count; k++)
                    {
                        Vertex* v = &vertices[k];
                        
                        v->Joints = XMUINT4(buffer[n+0], buffer[n+1], buffer[n+2], buffer[n+3]);
                        
                        n += step;
                    }
                }
                
            }
            else if (attrib->type == cgltf_attribute_type_weights)
            {
                assert(accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f);
                
                NSString* vertexBinaryPath = [[[NSBundle mainBundle] resourcePath]
                    stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:accessor->buffer_view->buffer->uri]];
                NSData* vertexBinaryData = [NSData dataWithContentsOfFile:vertexBinaryPath];
                
                // Acces joint data
                float* buffer = (float*)vertexBinaryData.bytes +
                (accessor->buffer_view->offset / sizeof(float)) + (accessor->offset / sizeof(float));
                
                int n = 0;
                const auto step = (int)(accessor->stride / sizeof(float));
                for(uint32_t k = 0; k < accessor->count; k++)
                {
                    Vertex* v = &vertices[k];
                    
                    v->Weights = XMFLOAT4(buffer[n+0], buffer[n+1], buffer[n+2], buffer[n+3]);
                    
                    n += step;
                }
            }
        }
    }
}

static void cgltf_query_animation_samplers(const cgltf_animation* animation,
                                           Animation& output)
{
    assert(animation != nullptr);
    
    // Access each sampler
    for(auto i = 0; i < animation->samplers_count; i++)
    {
        AnimationSampler animationSampler;
        
        const cgltf_animation_sampler* sampler = &animation->samplers[i];
        //printf("Test\n");
    }
}

static void cgltf_query_animation_channels(const cgltf_animation* animation,
                                           Animation& output)
{
    assert(animation != nullptr);
    
    // Access each channel
    for(auto i = 0; i < animation->channels_count; i++)
    {
        AnimationChannel animationChannel;
        
        const cgltf_animation_channel* channel = &animation->channels[i];
        //printf("Test\n");
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
    for(auto i = 0; i < meshCount; i++)
    {
        // Access each mesh
        const cgltf_mesh* mesh = &modelData->meshes[i];
        cgltf_query_mesh_vertices_indices(mesh, Vertices, Indices);
    }
    
    // Load skinning data
    const auto skinCount = modelData->skins_count;
    assert(skinCount == 1);
    if(skinCount == 1) {
        Skeleton = std::make_shared<struct Skeleton>();
    }
    
    for(auto i = 0; i < skinCount; i++)
    {
        // Access each skin
        const cgltf_skin* skin = &modelData->skins[i];
        
        // Cache skin name
        if(skin->name != nullptr)
            Skeleton->Name = skin->name;
        
        const auto jointCount = skin->joints_count;
        for(auto j = 0; j < jointCount; j++)
        {
            Joint skeletonJoint{};
            
            const cgltf_node* joint = skin->joints[j];
            
            // Cache joint name
            if(joint->name != nullptr)
                skeletonJoint.Name = joint->name;
            
            printf("Joint: %s\n", skeletonJoint.Name.c_str());
            
            
            // Does this joint have a parent?
            if (j > 0 && joint->parent != nullptr)
            {
                const cgltf_node* parentJoint = joint->parent;
                auto it = std::find_if(Skeleton->Joints.begin(), Skeleton->Joints.end(), [parentJoint](const Joint& _joint) {
                    return _joint.Name == parentJoint->name;
                });
                auto parentIndex = std::distance(Skeleton->Joints.begin(), it);
                skeletonJoint.ParentIndex = parentIndex;
                printf("Parent: %s Index: %ld\n", parentJoint->name, parentIndex);
            }
            
            Skeleton->Joints.push_back(skeletonJoint);
        }
    }
    
    // Load animation data
    const auto animationCount = modelData->animations_count;
    for(auto i = 0; i < animationCount; i++)
    {
        Animation animation;
        
        // Access each animation
        const cgltf_animation* anim = &modelData->animations[i];
        
        // Query all samplers
        cgltf_query_animation_samplers(anim, animation);
        
        // Query all channels
        cgltf_query_animation_channels(anim, animation);
        
    }
    
    
    
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
    const auto textureCount = modelData->textures_count;
    for(auto i = 0; i < textureCount; i++)
    {
        const cgltf_texture* texture = &modelData->textures[i];
        const cgltf_image* image = texture->image;
        
        // Load image from URI
        if(image->uri != nullptr)
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
