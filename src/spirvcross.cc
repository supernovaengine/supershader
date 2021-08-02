//
// (c) 2021 Eduardo Doria.
//

#include "supershader.h"

#include "spirv_cross.hpp"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_parser.hpp"

#include <memory>

using namespace supershader;

static attribute_type_t spirtype_to_attribute_type(const spirv_cross::SPIRType& type) {
    if (type.basetype == spirv_cross::SPIRType::Float) {
        if (type.columns == 1) {
            switch (type.vecsize) {
                case 1: return attribute_type_t::FLOAT;
                case 2: return attribute_type_t::FLOAT2;
                case 3: return attribute_type_t::FLOAT3;
                case 4: return attribute_type_t::FLOAT4;
            }
        }
    } else if (type.basetype == spirv_cross::SPIRType::Int) {
        if (type.columns == 1) {
            switch (type.vecsize) {
                case 1: return attribute_type_t::INT;
                case 2: return attribute_type_t::INT2;
                case 3: return attribute_type_t::INT3;
                case 4: return attribute_type_t::INT4;
            }
        }
    }

    return attribute_type_t::INVALID;
}

static uniform_type_t spirtype_to_uniform_type(const spirv_cross::SPIRType& type) {
    if (type.basetype == spirv_cross::SPIRType::Float) {
        if (type.columns == 1) {
            switch (type.vecsize) {
                case 1: return uniform_type_t::FLOAT;
                case 2: return uniform_type_t::FLOAT2;
                case 3: return uniform_type_t::FLOAT3;
                case 4: return uniform_type_t::FLOAT4;
            }
        }
        else {
            if ((type.vecsize == 3) && (type.columns == 3)) {
                return uniform_type_t::MAT3;
            } else if ((type.vecsize == 4) && (type.columns == 4)) {
                return uniform_type_t::MAT4;
            }
        }
    } else if (type.basetype == spirv_cross::SPIRType::Int) {
        if (type.columns == 1) {
            switch (type.vecsize) {
                case 1: return uniform_type_t::INT;
                case 2: return uniform_type_t::INT2;
                case 3: return uniform_type_t::INT3;
                case 4: return uniform_type_t::INT4;
            }
        }
    }

    return uniform_type_t::INVALID;
}

static texture_type_t spirtype_to_image_type(const spirv_cross::SPIRType& type) {
    if (type.image.arrayed) {
        if (type.image.dim == spv::Dim2D) {
            return texture_type_t::TEXTURE_ARRAY;
        }
    }
    else {
        switch (type.image.dim) {
            case spv::Dim2D:    return texture_type_t::TEXTURE_2D;
            case spv::DimCube:  return texture_type_t::TEXTURE_CUBE;
            case spv::Dim3D:    return texture_type_t::TEXTURE_3D;
            default: break;
        }
    }

    return texture_type_t::INVALID;
}

static texture_samplertype_t spirtype_to_image_samplertype(const spirv_cross::SPIRType& type) {
    switch (type.basetype) {
        case spirv_cross::SPIRType::Int:
        case spirv_cross::SPIRType::Short:
        case spirv_cross::SPIRType::SByte:
            return texture_samplertype_t::SINT;
        case spirv_cross::SPIRType::UInt:
        case spirv_cross::SPIRType::UShort:
        case spirv_cross::SPIRType::UByte:
            return texture_samplertype_t::UINT;
        default:
            return texture_samplertype_t::FLOAT;
    }
}

static bool parse_reflection(spirvcross_t& spirvcross, const spirv_cross::Compiler* compiler) {

    spirv_cross::ShaderResources shd_resources = compiler->get_shader_resources();

    // Stage
    switch (compiler->get_execution_model()) {
        case spv::ExecutionModelVertex:   spirvcross.stage_type = STAGE_VERTEX; break;
        case spv::ExecutionModelFragment: spirvcross.stage_type = STAGE_FRAGMENT; break;
        default: fprintf(stderr, "INVALID Stage\n"); return false; break;
    }

    // Entry function
    const auto entry_points = compiler->get_entry_points_and_stages();
    for (const auto& item: entry_points) {
        if (compiler->get_execution_model() == item.execution_model) {
            spirvcross.entry_point = item.name;
            break;
        }
    }

    // Stage inputs and outputs
    for (const spirv_cross::Resource& res_attr: shd_resources.stage_inputs) {
        s_attr_t attr;
        const uint32_t loc = compiler->get_decoration(res_attr.id, spv::DecorationLocation);
        const spirv_cross::SPIRType& type = compiler->get_type(res_attr.type_id);

        attr.location = loc;
        attr.name = res_attr.name;
        attr.semantic_name = k_attrib_sem_names[loc];
        attr.semantic_index = k_attrib_sem_indices[loc];
        attr.type = spirtype_to_attribute_type(type);

        spirvcross.inputs.push_back(attr);
    }
    for (const spirv_cross::Resource& res_attr: shd_resources.stage_outputs) {
        s_attr_t attr;
        const uint32_t loc = compiler->get_decoration(res_attr.id, spv::DecorationLocation);
        const spirv_cross::SPIRType& type = compiler->get_type(res_attr.type_id);
        
        attr.location = loc;
        attr.name = res_attr.name;
        attr.semantic_name = k_attrib_sem_names[loc];
        attr.semantic_index = k_attrib_sem_indices[loc];
        attr.type = spirtype_to_attribute_type(type);

        spirvcross.outputs.push_back(attr);
    }

    // uniform blocks
    for (const spirv_cross::Resource& ub_res: shd_resources.uniform_buffers) {
        s_uniform_block_t ub;

        const spirv_cross::SPIRType& ub_type = compiler->get_type(ub_res.base_type_id);

        ub.name = ub_res.name;
        ub.set = compiler->get_decoration(ub_res.id, spv::DecorationDescriptorSet);
        ub.binding = compiler->get_decoration(ub_res.id, spv::DecorationBinding);
        ub.size_bytes = (int) compiler->get_declared_struct_size(ub_type);

        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
            s_uniform_t uniform;
            const spirv_cross::SPIRType& m_type = compiler->get_type(ub_type.member_types[m_index]);

            uniform.name = compiler->get_member_name(ub_res.base_type_id, m_index);
            uniform.offset = compiler->type_struct_member_offset(ub_type, m_index);
            uniform.type = spirtype_to_uniform_type(m_type);            
            if (m_type.array.size() > 0) {
                uniform.array_count = m_type.array[0];
            }

            ub.uniforms.push_back(uniform);
        }

        spirvcross.uniform_blocks.push_back(ub);
    }

    // images
    for (const spirv_cross::Resource& img_res: shd_resources.sampled_images) {
        s_texture_t image;

        const spirv_cross::SPIRType& img_type = compiler->get_type(img_res.type_id);

        image.name = img_res.name;
        image.set = compiler->get_decoration(img_res.id, spv::DecorationDescriptorSet);
        image.binding = compiler->get_decoration(img_res.id, spv::DecorationBinding);
        image.type = spirtype_to_image_type(img_type);
        image.sampler_type = spirtype_to_image_samplertype(compiler->get_type(img_type.image.type));

        spirvcross.textures.push_back(image);
    }

    return true;
}

//
// From https://github.com/floooh/sokol-tools
//
/*
    for "Vulkan convention", fragment shader uniform block bindings live in the same
    descriptor set as vertex shader uniform blocks, but are offset by 4:

    set=0, binding=0..3:    vertex shader uniform blocks
    set=0, binding=4..7:    fragment shader uniform blocks
*/
static const uint32_t vk_fs_ub_binding_offset = 4;

static void fix_bind_slots(spirv_cross::Compiler* compiler, stage_type_t stage, bool is_vulkan) {
    /*
        This overrides all bind slots like this:

        Target is not WebGPU:
            - both vertex shader and fragment shader:
                - uniform blocks go into set=0 and start at binding=0
                - images go into set=1 and start at binding=0
        Target is WebGPU:
            - uniform blocks go into set=0
                - vertex shader uniform blocks start at binding=0
                - fragment shader uniform blocks start at binding=1
            - vertex shader images go into set=1, start at binding=0
            - fragment shader images go into set=2, start at binding=0

        NOTE that any existing binding definitions are always overwritten,
        this differs from previous behaviour which checked if explicit
        bindings existed.
    */
    spirv_cross::ShaderResources res = compiler->get_shader_resources();
    uint32_t ub_slot = 0;
    if (is_vulkan) {
        ub_slot = (stage == STAGE_VERTEX) ? 0 : vk_fs_ub_binding_offset;
    }
    for (const spirv_cross::Resource& ub_res: res.uniform_buffers) {
        compiler->set_decoration(ub_res.id, spv::DecorationDescriptorSet, 0);
        compiler->set_decoration(ub_res.id, spv::DecorationBinding, ub_slot++);
    }

    uint32_t img_slot = 0;
    uint32_t img_set = (stage == STAGE_VERTEX) ? 1 : 2;
    for (const spirv_cross::Resource& img_res: res.sampled_images) {
        compiler->set_decoration(img_res.id, spv::DecorationDescriptorSet, img_set);
        compiler->set_decoration(img_res.id, spv::DecorationBinding, img_slot++);
    }
}

bool supershader::compile_to_lang(std::vector<spirvcross_t>& spirvcrossvec, const std::vector<spirv_t>& spirvvec, const std::vector<input_t>& inputs, const args_t& args){
    for (int i = 0; i < inputs.size(); i++){
        spirv_cross::Parser spirv_parser(move(spirvvec[i].bytecode));
	    spirv_parser.parse();

        std::unique_ptr<spirv_cross::CompilerGLSL> compiler;
        // Use spirv-cross to convert to other types of shader
        if (args.lang == LANG_GLSL) {
            compiler.reset(new spirv_cross::CompilerGLSL(std::move(spirv_parser.get_parsed_ir())));
        } else if (args.lang == LANG_MSL) {
            compiler.reset(new spirv_cross::CompilerMSL(std::move(spirv_parser.get_parsed_ir())));
        } else if (args.lang == LANG_HLSL) {
            compiler.reset(new spirv_cross::CompilerHLSL(std::move(spirv_parser.get_parsed_ir())));
        } else {
            fprintf(stderr, "Language not implemented");
            return false;
        }

        spirv_cross::CompilerGLSL::Options opts = compiler->get_common_options();
        
        opts.flatten_multidimensional_arrays = true;

        if (args.lang == LANG_GLSL) {
            opts.emit_line_directives = false;
            opts.enable_420pack_extension = false;
            opts.es = args.es;
            opts.version = args.version;
        } else if (args.lang == LANG_HLSL) {
            spirv_cross::CompilerHLSL* hlsl = (spirv_cross::CompilerHLSL*)compiler.get();
            spirv_cross::CompilerHLSL::Options hlsl_opts = hlsl->get_hlsl_options();

            opts.emit_line_directives = true;

            hlsl_opts.shader_model = args.version;
            hlsl_opts.point_size_compat = true;
            hlsl_opts.point_coord_compat = true;

            hlsl->set_hlsl_options(hlsl_opts);
        } else if (args.lang == LANG_MSL) {
            spirv_cross::CompilerMSL* msl = (spirv_cross::CompilerMSL*)compiler.get();
            spirv_cross::CompilerMSL::Options msl_opts = msl->get_msl_options();

            opts.emit_line_directives = true;

            if (args.platform == PLATFORM_MACOS){
                msl_opts.platform = spirv_cross::CompilerMSL::Options::macOS;
            }else if (args.platform == PLATFORM_IOS){
                msl_opts.platform = spirv_cross::CompilerMSL::Options::iOS;
            }
            msl_opts.enable_decoration_binding = true;
            msl_opts.msl_version = args.version;

            msl->set_msl_options(msl_opts);
        }

        compiler->set_common_options(opts);

        // Vertex attribute remap for HLSL
        if (args.lang == LANG_HLSL) {
            spirv_cross::CompilerHLSL* hlsl_compiler = (spirv_cross::CompilerHLSL*)compiler.get();
            for (int i = 0; i < VERTEX_ATTRIB_COUNT; i++) {
                spirv_cross::HLSLVertexAttributeRemap remap = { (uint32_t)i, k_attrib_names[i] };
                hlsl_compiler->add_vertex_attribute_remap(remap);
            }
        }

        fix_bind_slots(compiler.get(), inputs[i].stage_type, false);

        // GL/GLES always flatten UBs to use only one glUniform4fv call
        // TODO: Not for Vulkan
        if (args.lang == LANG_GLSL) {
            spirv_cross::ShaderResources res = compiler->get_shader_resources();
            for (const spirv_cross::Resource& ub_res: res.uniform_buffers) {
                compiler->flatten_buffer_block(ub_res.id);
            }
        }
        
        spirvcrossvec[i].source = compiler->compile();

        //print_reflection(compiler.get());

        if (!parse_reflection(spirvcrossvec[i], compiler.get()))
            return false;
    }
    return true;
}