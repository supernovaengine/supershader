//
// (c) 2021 Eduardo Doria.
//

#include "supershader.h"

#include <fstream>
#include <cstring>

using namespace supershader;

#define makefourcc(_a, _b, _c, _d) (((uint32_t)(_a) | ((uint32_t)(_b) << 8) | ((uint32_t)(_c) << 16) | ((uint32_t)(_d) << 24)))

#define SBS_VERSION 110
#define SBS_NAME_SIZE 64

#pragma pack(push, 1)

#define SBS_CHUNK               makefourcc('S', 'B', 'S', ' ')
#define SBS_CHUNK_STAG          makefourcc('S', 'T', 'A', 'G')
#define SBS_CHUNK_CODE          makefourcc('C', 'O', 'D', 'E')
#define SBS_CHUNK_DATA          makefourcc('D', 'A', 'T', 'A')
#define SBS_CHUNK_REFL          makefourcc('R', 'E', 'F', 'L')

#define SBS_STAGE_VERTEX        makefourcc('V', 'E', 'R', 'T')
#define SBS_STAGE_FRAGMENT      makefourcc('F', 'R', 'A', 'G')

#define SBS_LANG_HLSL           makefourcc('H', 'L', 'S', 'L')
#define SBS_LANG_GLSL           makefourcc('G', 'L', 'S', 'L')
#define SBS_LANG_MSL            makefourcc('M', 'S', 'L', ' ')

#define SBS_VERTEXTYPE_FLOAT    makefourcc('F', 'L', 'T', '1')
#define SBS_VERTEXTYPE_FLOAT2   makefourcc('F', 'L', 'T', '2')
#define SBS_VERTEXTYPE_FLOAT3   makefourcc('F', 'L', 'T', '3')
#define SBS_VERTEXTYPE_FLOAT4   makefourcc('F', 'L', 'T', '4')
#define SBS_VERTEXTYPE_INT      makefourcc('I', 'N', 'T', '1')
#define SBS_VERTEXTYPE_INT2     makefourcc('I', 'N', 'T', '2')
#define SBS_VERTEXTYPE_INT3     makefourcc('I', 'N', 'T', '3')
#define SBS_VERTEXTYPE_INT4     makefourcc('I', 'N', 'T', '4')

#define SBS_UNIFORMTYPE_FLOAT    makefourcc('F', 'L', 'T', '1')
#define SBS_UNIFORMTYPE_FLOAT2   makefourcc('F', 'L', 'T', '2')
#define SBS_UNIFORMTYPE_FLOAT3   makefourcc('F', 'L', 'T', '3')
#define SBS_UNIFORMTYPE_FLOAT4   makefourcc('F', 'L', 'T', '4')
#define SBS_UNIFORMTYPE_INT      makefourcc('I', 'N', 'T', '1')
#define SBS_UNIFORMTYPE_INT2     makefourcc('I', 'N', 'T', '2')
#define SBS_UNIFORMTYPE_INT3     makefourcc('I', 'N', 'T', '3')
#define SBS_UNIFORMTYPE_INT4     makefourcc('I', 'N', 'T', '4')
#define SBS_UNIFORMTYPE_MAT3     makefourcc('M', 'A', 'T', '3')
#define SBS_UNIFORMTYPE_MAT4     makefourcc('M', 'A', 'T', '4')

#define SBS_TEXTURE_2D          makefourcc('2', 'D', ' ', ' ')
#define SBS_TEXTURE_3D          makefourcc('3', 'D', ' ', ' ')
#define SBS_TEXTURE_CUBE        makefourcc('C', 'U', 'B', 'E')
#define SBS_TEXTURE_ARRAY       makefourcc('A', 'R', 'R', 'A')

#define SBS_TEXTURE_SAMPLERTYPE_FLOAT   makefourcc('T', 'F', 'L', 'T')
#define SBS_TEXTURE_SAMPLERTYPE_SINT    makefourcc('T', 'I', 'N', 'T')
#define SBS_TEXTURE_SAMPLERTYPE_UINT    makefourcc('T', 'U', 'I', 'T')
#define SBS_TEXTURE_SAMPLERTYPE_DEPTH   makefourcc('T', 'D', 'P', 'H')

#define SBS_SAMPLERTYPE_FILTERING     makefourcc('S', 'F', 'I', 'L')
#define SBS_SAMPLERTYPE_COMPARISON    makefourcc('S', 'C', 'O', 'M')

struct sbs_chunk {
    uint32_t sbs_version;
    uint32_t lang;
    uint32_t version;
    uint16_t es;
};

// STAG
struct sbs_stage {
    uint32_t type;
};

// REFL
struct sbs_chunk_refl {
    char     name[SBS_NAME_SIZE];
    uint32_t num_inputs;
    uint32_t num_textures;
    uint32_t num_samplers;
    uint32_t num_texture_samplers;
    uint32_t num_uniform_blocks;
    uint32_t num_uniforms;
};

struct sbs_refl_input {
    char     name[SBS_NAME_SIZE];
    int32_t  location;
    char     semantic_name[SBS_NAME_SIZE];
    uint32_t semantic_index;
    uint32_t type;
};

struct sbs_refl_texture {
    char     name[SBS_NAME_SIZE];
    uint32_t set;
    int32_t  binding;
    uint32_t type;
    uint32_t sampler_type;
};

struct sbs_refl_sampler {
    char     name[SBS_NAME_SIZE];
    uint32_t set;
    int32_t  binding;
    uint32_t type;
}; 

struct sbs_refl_texture_sampler {
    char     name[SBS_NAME_SIZE];
    char     texture_name[SBS_NAME_SIZE];
    char     sampler_name[SBS_NAME_SIZE];
    int32_t  binding;
}; 

struct sbs_refl_uniformblock {
    uint32_t num_uniforms;
    char     name[SBS_NAME_SIZE];
    char     inst_name[SBS_NAME_SIZE];
    uint32_t set;
    int32_t  binding;
    uint32_t size_bytes;
    bool     flattened;
};

struct sbs_refl_uniform {
    char     name[SBS_NAME_SIZE];
    uint32_t type;
    uint32_t array_count;
    uint32_t offset;
};

#pragma pack(pop)

static uint32_t get_stage(stage_type_t stage){
    if (stage == STAGE_VERTEX){
        return SBS_STAGE_VERTEX;
    }else if (stage == STAGE_FRAGMENT){
        return SBS_STAGE_FRAGMENT;
    }

    return 0;
}

static uint32_t get_lang(lang_type_t lang){
    if (lang == LANG_GLSL){
        return SBS_LANG_GLSL;
    }else if (lang == LANG_HLSL){
        return SBS_LANG_HLSL;
    }else if (lang == LANG_MSL){
        return SBS_LANG_MSL;
    }

    return 0;
}

static uint32_t get_uniform_type(uniform_type_t type){
    if (type == uniform_type_t::FLOAT){
        return SBS_UNIFORMTYPE_FLOAT;
    }else if (type == uniform_type_t::FLOAT2){
        return SBS_UNIFORMTYPE_FLOAT2;
    }else if (type == uniform_type_t::FLOAT3){
        return SBS_UNIFORMTYPE_FLOAT3;
    }else if (type == uniform_type_t::FLOAT4){
        return SBS_UNIFORMTYPE_FLOAT4;
    }else if (type == uniform_type_t::INT){
        return SBS_UNIFORMTYPE_INT;
    }else if (type == uniform_type_t::INT2){
        return SBS_UNIFORMTYPE_INT2;
    }else if (type == uniform_type_t::INT3){
        return SBS_UNIFORMTYPE_INT3;
    }else if (type == uniform_type_t::INT4){
        return SBS_UNIFORMTYPE_INT4;
    }else if (type == uniform_type_t::MAT3){
        return SBS_UNIFORMTYPE_MAT3;
    }else if (type == uniform_type_t::MAT4){
        return SBS_UNIFORMTYPE_MAT4;
    }

    return 0;
}

static uint32_t get_vertex_type(attribute_type_t type){
    if (type == attribute_type_t::FLOAT){
        return SBS_VERTEXTYPE_FLOAT;
    }else if (type == attribute_type_t::FLOAT2){
        return SBS_VERTEXTYPE_FLOAT2;
    }else if (type == attribute_type_t::FLOAT3){
        return SBS_VERTEXTYPE_FLOAT3;
    }else if (type == attribute_type_t::FLOAT4){
        return SBS_VERTEXTYPE_FLOAT4;
    }else if (type == attribute_type_t::INT){
        return SBS_VERTEXTYPE_INT;
    }else if (type == attribute_type_t::INT2){
        return SBS_VERTEXTYPE_INT2;
    }else if (type == attribute_type_t::INT3){
        return SBS_VERTEXTYPE_INT3;
    }else if (type == attribute_type_t::INT4){
        return SBS_VERTEXTYPE_INT4;
    }

    return 0;
}

static uint32_t get_texture_format(texture_type_t type){
    if (type == texture_type_t::TEXTURE_2D){
        return SBS_TEXTURE_2D;
    }else if (type == texture_type_t::TEXTURE_3D){
        return SBS_TEXTURE_3D;
    }else if (type == texture_type_t::TEXTURE_CUBE){
        return SBS_TEXTURE_CUBE;
    }else if (type == texture_type_t::TEXTURE_ARRAY){
        return SBS_TEXTURE_ARRAY;
    }

    return 0;
}

static uint32_t get_texture_samplertype(texture_samplertype_t basetype){
    if (basetype == texture_samplertype_t::FLOAT){
        return SBS_TEXTURE_SAMPLERTYPE_FLOAT;
    }else if (basetype == texture_samplertype_t::SINT){
        return SBS_TEXTURE_SAMPLERTYPE_SINT;
    }else if (basetype == texture_samplertype_t::UINT){
        return SBS_TEXTURE_SAMPLERTYPE_UINT;
    }else if (basetype == texture_samplertype_t::DEPTH){
        return SBS_TEXTURE_SAMPLERTYPE_DEPTH;
    }

    return 0;
}

static uint32_t get_samplertype(sampler_type_t basetype){
    if (basetype == sampler_type_t::FILTERING){
        return SBS_SAMPLERTYPE_FILTERING;
    }else if (basetype == sampler_type_t::COMPARISON){
        return SBS_SAMPLERTYPE_COMPARISON;
    }

    return 0;
}

static void copy_name(char* dest, const std::string& source){
    size_t n = SBS_NAME_SIZE - 1;
    strncpy(dest, source.c_str(), n);
    dest[n] = '\0';
}

bool supershader::generate_sbs(const std::vector<spirvcross_t>& spirvcrossvec, const std::vector<input_t>& inputs, const args_t& args){

    std::string filename = args.output_dir + args.output_basename + ".sbs";

    std::ofstream ofs(filename, std::ios::out | std::ios::binary);
    if(!ofs) {
        fprintf(stderr, "Cannot open file %s\n", filename.c_str());
        return false;
    }

    const uint32_t _sbs = SBS_CHUNK;
    const uint32_t _sbs_size = 0;

    ofs.write((char *) &_sbs, sizeof(uint32_t));
    ofs.write((char *) &_sbs_size, sizeof(uint32_t));

    sbs_chunk sbs;
    sbs.sbs_version = SBS_VERSION;
    sbs.lang = get_lang(args.lang);
    sbs.version = args.version;
    sbs.es = args.es;
    ofs.write((char *) &sbs, sizeof(sbs_chunk));

    for (int i = 0; i < spirvcrossvec.size(); i++){
        size_t num_inputs = ( (spirvcrossvec[i].stage_type==STAGE_VERTEX)? spirvcrossvec[i].inputs.size() : 0 );
        size_t num_ubs = spirvcrossvec[i].uniform_blocks.size();
        size_t num_us = 0;
        for (int ub = 0; ub < num_ubs; ub++){
            num_us += spirvcrossvec[i].uniform_blocks[ub].uniforms.size();
        }
        size_t num_textures = spirvcrossvec[i].textures.size();
        size_t num_samplers = spirvcrossvec[i].samplers.size();
        size_t num_texture_samplers = spirvcrossvec[i].texture_samplers.size();

        const uint32_t code_size = spirvcrossvec[i].source.size();

        const uint32_t refl_size = 
            sizeof(sbs_chunk_refl) + 
            sizeof(sbs_refl_input) * num_inputs +
            sizeof(sbs_refl_texture) * num_textures +
            sizeof(sbs_refl_sampler) * num_samplers +
            sizeof(sbs_refl_texture_sampler) * num_texture_samplers +
            sizeof(sbs_refl_uniformblock) * num_ubs +
            sizeof(sbs_refl_uniform) * num_us;

        const uint32_t stage_size = 
            sizeof(sbs_stage) +
            sizeof(uint32_t) + sizeof(uint32_t) + code_size +
            sizeof(uint32_t) + sizeof(uint32_t) + refl_size;
        
        const uint32_t _stage = SBS_CHUNK_STAG;
        ofs.write((char *) &_stage, sizeof(uint32_t));
        ofs.write((char *) &stage_size, sizeof(uint32_t));

        sbs_stage stage;
        stage.type = get_stage(spirvcrossvec[i].stage_type);
        ofs.write((char *) &stage, sizeof(sbs_stage));

        const uint32_t _code = SBS_CHUNK_CODE;
        ofs.write((char *) &_code, sizeof(uint32_t));
        ofs.write((char *) &code_size, sizeof(uint32_t));
        ofs.write(&(spirvcrossvec[i].source)[0], code_size);

        const uint32_t _refl = SBS_CHUNK_REFL;
        ofs.write((char *) &_refl, sizeof(uint32_t));
        ofs.write((char *) &refl_size, sizeof(uint32_t));

        sbs_chunk_refl refl;
        copy_name(refl.name, args.output_basename);
        refl.num_inputs = num_inputs;
        refl.num_textures = num_textures;
        refl.num_samplers = num_samplers;
        refl.num_texture_samplers = num_texture_samplers;
        refl.num_uniforms = num_us;
        refl.num_uniform_blocks = num_ubs;

        ofs.write((char *) &refl, sizeof(sbs_chunk_refl));

        for (int a = 0; a < num_inputs; a++){
            sbs_refl_input refl_input;
            copy_name(refl_input.name, spirvcrossvec[i].inputs[a].name);
            copy_name(refl_input.semantic_name, spirvcrossvec[i].inputs[a].semantic_name);
            refl_input.location = spirvcrossvec[i].inputs[a].location;
            refl_input.semantic_index = spirvcrossvec[i].inputs[a].semantic_index;
            refl_input.type = get_vertex_type(spirvcrossvec[i].inputs[a].type);

            ofs.write((char *) &refl_input, sizeof(sbs_refl_input));
        }

        for (int a = 0; a < num_textures; a++){
            sbs_refl_texture refl_texture;
            copy_name(refl_texture.name, spirvcrossvec[i].textures[a].name.c_str());
            refl_texture.set = spirvcrossvec[i].textures[a].set;
            refl_texture.binding = spirvcrossvec[i].textures[a].binding;
            refl_texture.type = get_texture_format(spirvcrossvec[i].textures[a].type);
            refl_texture.sampler_type = get_texture_samplertype(spirvcrossvec[i].textures[a].sampler_type);

            ofs.write((char *) &refl_texture, sizeof(sbs_refl_texture));
        }

        for (int a = 0; a < num_samplers; a++){
            sbs_refl_sampler refl_sampler;
            copy_name(refl_sampler.name, spirvcrossvec[i].samplers[a].name.c_str());
            refl_sampler.set = spirvcrossvec[i].samplers[a].set;
            refl_sampler.binding = spirvcrossvec[i].samplers[a].binding;
            refl_sampler.type = get_samplertype(spirvcrossvec[i].samplers[a].type);

            ofs.write((char *) &refl_sampler, sizeof(sbs_refl_sampler));
        }

        for (int a = 0; a < num_texture_samplers; a++){
            sbs_refl_texture_sampler refl_texture_sampler;
            copy_name(refl_texture_sampler.name, spirvcrossvec[i].texture_samplers[a].name.c_str());
            copy_name(refl_texture_sampler.texture_name, spirvcrossvec[i].texture_samplers[a].texture_name.c_str());
            copy_name(refl_texture_sampler.sampler_name, spirvcrossvec[i].texture_samplers[a].sampler_name.c_str());
            refl_texture_sampler.binding = spirvcrossvec[i].samplers[a].binding;

            ofs.write((char *) &refl_texture_sampler, sizeof(sbs_refl_texture_sampler));
        }

        for (int a = 0; a < num_ubs; a++){
            sbs_refl_uniformblock refl_uniformblock;
            refl_uniformblock.num_uniforms = spirvcrossvec[i].uniform_blocks[a].uniforms.size();
            copy_name(refl_uniformblock.name, spirvcrossvec[i].uniform_blocks[a].name);
            copy_name(refl_uniformblock.inst_name, spirvcrossvec[i].uniform_blocks[a].inst_name);
            refl_uniformblock.set = spirvcrossvec[i].uniform_blocks[a].set;
            refl_uniformblock.binding = spirvcrossvec[i].uniform_blocks[a].binding;
            refl_uniformblock.size_bytes = spirvcrossvec[i].uniform_blocks[a].size_bytes;
            refl_uniformblock.flattened = spirvcrossvec[i].uniform_blocks[a].flattened;

            ofs.write((char *) &refl_uniformblock, sizeof(sbs_refl_uniformblock));

            for (int b = 0; b < refl_uniformblock.num_uniforms; b++){
                sbs_refl_uniform refl_uniform;
                copy_name(refl_uniform.name, spirvcrossvec[i].uniform_blocks[a].uniforms[b].name);
                refl_uniform.type = get_uniform_type(spirvcrossvec[i].uniform_blocks[a].uniforms[b].type);
                refl_uniform.array_count = spirvcrossvec[i].uniform_blocks[a].uniforms[b].array_count;
                refl_uniform.offset = spirvcrossvec[i].uniform_blocks[a].uniforms[b].offset;

                ofs.write((char *) &refl_uniform, sizeof(sbs_refl_uniform));
            }
        }
    }

    ofs.close();
    if(!ofs.good()) {
        fprintf(stderr, "Writing to file %s failed\n", filename.c_str());
        return false;
    }

    return true;
}