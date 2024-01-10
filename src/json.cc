//
// (c) 2024 Eduardo Doria.
//

#include "supershader.h"

#include "nlohmann/json.hpp"
#include <iomanip>
#include <fstream>

using namespace supershader;

using json = nlohmann::ordered_json;

static std::string lang_to_string(lang_type_t lang){
    if (lang == LANG_GLSL){
        return "glsl";
    } else if (lang == LANG_HLSL){
        return "hlsl";
    } else if (lang == LANG_MSL){
        return "msl";
    }

    return "";
}

static std::string stage_to_string(stage_type_t stage){
    if (stage == STAGE_VERTEX){
        return "vs";
    } else if (stage == STAGE_FRAGMENT){
        return "fs";
    }

    return "";
}

static std::string gen_shader_file(std::string directory, std::string basefilename, stage_type_t stage, lang_type_t lang, std::string source){
    std::string filename = basefilename + "_" + stage_to_string(stage) + "." + lang_to_string(lang);
    std::string path = directory + filename;

    std::ofstream ofs(path);
    if (!ofs) {
        fprintf(stderr, "Cannot open file %s\n", path.c_str());
    }

    ofs << source << std::endl;

    ofs.close();
    if (!ofs.good()) {
        fprintf(stderr, "Writing to file %s failed\n", path.c_str());
    }

    return filename;
}

static std::string get_json_path(std::string directory, std::string basefilename, lang_type_t lang){
    std::string path = directory + basefilename + "_" + lang_to_string(lang) + ".json";

    return path;
}

static std::string attr_type_to_string(attribute_type_t type){
    if (type == attribute_type_t::FLOAT){
        return "float";
    }else if (type == attribute_type_t::FLOAT2){
        return "float2";
    }else if (type == attribute_type_t::FLOAT3){
        return "float3";
    }else if (type == attribute_type_t::FLOAT4){
        return "float4";
    }else if (type == attribute_type_t::INT){
        return "int";
    }else if (type == attribute_type_t::INT2){
        return "int2";
    }else if (type == attribute_type_t::INT3){
        return "int3";
    }else if (type == attribute_type_t::INT4){
        return "int4";
    }else if (type == attribute_type_t::INVALID){
        return "INVALID";
    }
    return "";
}

static std::string uniform_type_to_string(uniform_type_t type){
    if (type == uniform_type_t::FLOAT){
        return "float";
    }else if (type == uniform_type_t::FLOAT2){
        return "float2";
    }else if (type == uniform_type_t::FLOAT3){
        return "float3";
    }else if (type == uniform_type_t::FLOAT4){
        return "float4";
    }else if (type == uniform_type_t::INT){
        return "int";
    }else if (type == uniform_type_t::INT2){
        return "int2";
    }else if (type == uniform_type_t::INT3){
        return "int3";
    }else if (type == uniform_type_t::INT4){
        return "int4";
    }else if (type == uniform_type_t::MAT3){
        return "mat3";
    }else if (type == uniform_type_t::MAT4){
        return "mat4";
    }else if (type == uniform_type_t::INVALID){
        return "INVALID";
    }
    return "";
}

static std::string texture_type_to_string(texture_type_t type){
    if (type == texture_type_t::TEXTURE_2D){
        return "texture_2d";
    }else if (type == texture_type_t::TEXTURE_3D){
        return "texture_3d";
    }else if (type == texture_type_t::TEXTURE_CUBE){
        return "texture_cube";
    }else if (type == texture_type_t::TEXTURE_ARRAY){
        return "texture_array";
    }else if (type == texture_type_t::INVALID){
        return "INVALID";
    }
    return "";
}

static std::string texture_samplertype_to_string(texture_samplertype_t type){
    if (type == texture_samplertype_t::SINT){
        return "sint";
    }else if (type == texture_samplertype_t::UINT){
        return "uint";
    }else if (type == texture_samplertype_t::FLOAT){
        return "float";
    }else if (type == texture_samplertype_t::DEPTH){
        return "depth";
    }else if (type == texture_samplertype_t::INVALID){
        return "INVALID";
    }
    return "";
}

static std::string sampler_type_to_string(sampler_type_t type){
    if (type == sampler_type_t::FILTERING){
        return "filtering";
    }else if (type == sampler_type_t::COMPARISON){
        return "comparison";
    }else if (type == sampler_type_t::INVALID){
        return "INVALID";
    }
    return "";
}


bool supershader::generate_json(const std::vector<spirvcross_t>& spirvcrossvec, const std::vector<input_t>& inputs, const args_t& args){
    json j;

    j["language"] = lang_to_string(args.lang);
    j["version"] = args.version;

    for (int i = 0; i < spirvcrossvec.size(); i++){
        json sj;
        sj["file"] = gen_shader_file(args.output_dir, args.output_basename, inputs[i].stage_type, args.lang, spirvcrossvec[i].source);
        sj["entry_point"] = spirvcrossvec[i].entry_point;

        for (int ia = 0; ia < spirvcrossvec[i].inputs.size(); ia++){
            s_attr_t attr = spirvcrossvec[i].inputs[ia];
            json aj;
            aj["name"] = attr.name;
            aj["location"] = attr.location;
            aj["semantic_name"] = attr.semantic_name;
            aj["semantic_index"] = attr.semantic_index;
            aj["type"] = attr_type_to_string(attr.type);

            sj["inputs"].push_back(aj);
        }

        for (int ia = 0; ia < spirvcrossvec[i].outputs.size(); ia++){
            s_attr_t attr = spirvcrossvec[i].outputs[ia];
            json aj;
            aj["name"] = attr.name;
            aj["location"] = attr.location;
            aj["type"] = attr_type_to_string(attr.type);

            sj["outputs"].push_back(aj);
        }

        for (int it = 0; it < spirvcrossvec[i].textures.size(); it++){
            s_texture_t t = spirvcrossvec[i].textures[it];
            json tj;
            tj["name"] = t.name;
            tj["set"] = t.set;
            tj["binding"] = t.binding;
            tj["type"] = texture_type_to_string(t.type);
            tj["sampler_type"] = texture_samplertype_to_string(t.sampler_type);

            sj["textures"].push_back(tj);
        }

        for (int is = 0; is < spirvcrossvec[i].samplers.size(); is++){
            s_sampler_t sm = spirvcrossvec[i].samplers[is];
            json smj;
            smj["name"] = sm.name;
            smj["binding"] = sm.binding;
            smj["type"] = sampler_type_to_string(sm.type);

            sj["samplers"].push_back(smj);
        }

        for (int its = 0; its < spirvcrossvec[i].texture_samplers.size(); its++){
            s_texture_sampler_t tsm = spirvcrossvec[i].texture_samplers[its];
            json tsmj;
            tsmj["name"] = tsm.name;
            tsmj["texture_name"] = tsm.texture_name;
            tsmj["sampler_name"] = tsm.sampler_name;
            tsmj["binding"] = tsm.binding;

            sj["texture_samplers"].push_back(tsmj);
        }

        for (int iub = 0; iub < spirvcrossvec[i].uniform_blocks.size(); iub++){
            s_uniform_block_t ub = spirvcrossvec[i].uniform_blocks[iub];
            json ubj;
            ubj["name"] = ub.name;
            ubj["inst_name"] = ub.inst_name;
            ubj["set"] = ub.set;
            ubj["binding"] = ub.binding;
            ubj["size_bytes"] = ub.size_bytes;
            ubj["flattened"] = ub.flattened;

            for (int iu = 0; iu < ub.uniforms.size(); iu++){
                s_uniform_t u = ub.uniforms[iu];
                json uj;
                uj["name"] = u.name;
                uj["array_count"] = u.array_count;
                uj["offset"] = u.offset;
                uj["type"] = uniform_type_to_string(u.type);

                ubj["uniforms"].push_back(uj);
            }

            sj["uniform_blocks"].push_back(ubj);
        }


        j[stage_to_string(inputs[i].stage_type)] = sj;
    }


    std::string json_path = get_json_path(args.output_dir, args.output_basename, args.lang);
    std::ofstream ofs(json_path);
    if (!ofs) {
        fprintf(stderr, "Cannot open file %s\n", json_path.c_str());
        return false;
    }
    ofs << j.dump(4) << std::endl;

    ofs.close();
    if (!ofs.good()) {
        fprintf(stderr, "Writing to file %s failed\n", json_path.c_str());
        return false;
    }

    return true;
}