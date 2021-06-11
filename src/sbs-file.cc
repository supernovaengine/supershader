//
// (c) 2021 Eduardo Doria.
//

#include "supershader.h"

#include <fstream>
#include <cstring>

using namespace supershader;

#define makefourcc(_a, _b, _c, _d) (((uint32_t)(_a) | ((uint32_t)(_b) << 8) | ((uint32_t)(_c) << 16) | ((uint32_t)(_d) << 24)))

#define SBS_VERSION 100

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

#define SBS_TEXTURE_2D          makefourcc('2', 'D', ' ', ' ')
#define SBS_TEXTURE_3D          makefourcc('3', 'D', ' ', ' ')
#define SBS_TEXTURE_CUBE        makefourcc('C', 'U', 'B', 'E')
#define SBS_TEXTURE_ARRAY       makefourcc('A', 'R', 'R', 'A')

#define SBS_SAMPLERTYPE_FLOAT   makefourcc('T', 'F', 'L', 'T')
#define SBS_SAMPLERTYPE_SINT    makefourcc('T', 'I', 'N', 'T')
#define SBS_SAMPLERTYPE_UINT    makefourcc('T', 'U', 'I', 'T')

struct sbs_chunk {
    uint32_t sbs_version;
    uint32_t lang;
    uint32_t profile_version;
    uint16_t es;
};

// STAG
struct sbs_stage {
    uint32_t type;
};

// REFL
struct sbs_chunk_refl {
    char     name[32];
    uint32_t num_inputs;
    uint32_t num_textures;
    uint32_t num_uniform_blocks;
};

struct sbs_refl_input {
    char     name[32];
    int32_t  location;
    char     semantic_name[32];
    uint32_t semantic_index;
    uint32_t type;
};

struct sbs_refl_texture {
    char     name[32];
    uint32_t set;
    int32_t  binding;
    uint32_t type;
    uint32_t sampler_type;
}; 

struct sbs_refl_uniformblock {
    char     name[32];
    uint32_t set;
    int32_t  binding;
    uint32_t size_bytes;
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
        return SBS_SAMPLERTYPE_FLOAT;
    }else if (basetype == texture_samplertype_t::SINT){
        return SBS_SAMPLERTYPE_SINT;
    }else if (basetype == texture_samplertype_t::UINT){
        return SBS_SAMPLERTYPE_UINT;
    }

    return 0;
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
    sbs.profile_version = args.profile;
    sbs.es = args.es;
    ofs.write((char *) &sbs, sizeof(sbs_chunk));

    for (int i = 0; i < spirvcrossvec.size(); i++){
        size_t num_inputs = ( (spirvcrossvec[i].stage_type==STAGE_VERTEX)? spirvcrossvec[i].inputs.size() : 0 );
        size_t num_ubs = spirvcrossvec[i].uniform_blocks.size();
        size_t num_textures = spirvcrossvec[i].textures.size();

        const uint32_t code_size = spirvcrossvec[i].source.size();

        const uint32_t refl_size = 
            sizeof(sbs_chunk_refl) + 
            sizeof(sbs_refl_input) * num_inputs +
            sizeof(sbs_refl_uniformblock) * num_ubs +
            sizeof(sbs_refl_texture) * num_textures;

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
        strcpy(refl.name, args.output_basename.substr(0, 32).c_str());
        refl.num_inputs = num_inputs;
        refl.num_uniform_blocks = num_ubs;
        refl.num_textures = num_textures;

        ofs.write((char *) &refl, sizeof(sbs_chunk_refl));

        for (int a = 0; a < num_inputs; a++){
            sbs_refl_input refl_input;
            strcpy(refl_input.name, spirvcrossvec[i].inputs[a].name.substr(0, 32).c_str());
            strcpy(refl_input.semantic_name, spirvcrossvec[i].inputs[a].semantic_name.substr(0, 32).c_str());
            refl_input.location = spirvcrossvec[i].inputs[a].location;
            refl_input.semantic_index = spirvcrossvec[i].inputs[a].semantic_index;
            refl_input.type = get_vertex_type(spirvcrossvec[i].inputs[a].type);

            ofs.write((char *) &refl_input, sizeof(sbs_refl_input));
        }

        for (int a = 0; a < num_ubs; a++){
            sbs_refl_uniformblock refl_uniformbuffer;
            strcpy(refl_uniformbuffer.name, spirvcrossvec[i].uniform_blocks[a].name.substr(0, 32).c_str());
            refl_uniformbuffer.set = spirvcrossvec[i].uniform_blocks[a].set;
            refl_uniformbuffer.binding = spirvcrossvec[i].uniform_blocks[a].binding;
            refl_uniformbuffer.size_bytes = spirvcrossvec[i].uniform_blocks[a].size_bytes;

            ofs.write((char *) &refl_uniformbuffer, sizeof(sbs_refl_uniformblock));
        }

        for (int a = 0; a < num_textures; a++){
            sbs_refl_texture refl_texture;
            strcpy(refl_texture.name, spirvcrossvec[i].textures[a].name.substr(0, 32).c_str());
            refl_texture.set = spirvcrossvec[i].textures[a].set;
            refl_texture.binding = spirvcrossvec[i].textures[a].binding;
            refl_texture.type = get_texture_format(spirvcrossvec[i].textures[a].type);
            refl_texture.sampler_type = get_texture_samplertype(spirvcrossvec[i].textures[a].sampler_type);

            ofs.write((char *) &refl_texture, sizeof(sbs_refl_texture));
        }

    }

    ofs.close();
    if(!ofs.good()) {
        fprintf(stderr, "Writing to file %s failed\n", filename.c_str());
        return false;
    }

    return true;
}