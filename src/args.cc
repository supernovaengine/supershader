#include "supershader.h"

#include "argparse.h"
#include <sstream>

#ifndef SUPERSHADER_VERSION
#define SUPERSHADER_VERSION ""
#endif

using namespace supershader;

const char kPathSeparator =
#ifdef _WIN32
'\\';
#else
'/';
#endif

static std::string trim(const std::string &s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
 
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
 
    return std::string(start, end + 1);
}

static std::vector<define_t> parse_defines(const char *defines){
    std::stringstream ss(defines);
    std::vector<define_t> result;

    while( ss.good() ){
        std::string substr;
        getline( ss, substr, ';' );

        substr = trim(substr);

        std::string def = substr;
        std::string value = "";

        const size_t equal = substr.find_first_of("=");
        if (equal != substr.npos){
            def = substr.substr (0, equal);
            value = substr.substr (equal+1);
        }

        result.push_back( {def, value} );
    }

    return result;
}

static std::string get_directory(const char* path) {
    std::string dir = path;
    size_t last = dir.find_last_of("/\\");
    return last == std::string::npos ? "." : dir.substr(0, last);
}

static std::string get_filename(const char* path){
    std::string filename = path;

    // Remove directory if present
    const size_t last_slash_idx = filename.find_last_of("\\/");
    if (std::string::npos != last_slash_idx){
        filename.erase(0, last_slash_idx + 1);
    }

    // Remove extension if present
    const size_t period_idx = filename.rfind('.');
    if (std::string::npos != period_idx) {
        filename.erase(period_idx);
    }

    return filename;
}

args_t supershader::parse_args(int argc, const char **argv){
    args_t args;
    args.vert_file = "";
    args.frag_file = "";
    args.lang = LANG_GLSL;
    args.version = 0;
    args.es = false;
    args.platform = PLATFORM_DEFAULT;
    args.output_basename = "";
    args.output_dir = "";
    args.output_type = OUTPUT_JSON;
    args.include_dir = "";
    args.defines.clear();
    args.list_includes = false;
    args.optimization = true;

    const char *vert_file = NULL;
    const char *frag_file = NULL;
    const char *lang = NULL;
    const char *output = NULL;
    const char *output_type = NULL;
    const char *include_dir = NULL;
    const char *defines = NULL;
    int list_includes = 0;
    int disable_optimization = 0;

    static const char *const usage[] = {
    "supershader --vert <vertex shader> [[--] args]",
    "supershader --frag <fragment shader> [[--] args]",
    "supershader --vert <vertex shader> --frag <fragment shader> [[--] args]",
    NULL,
    };

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('v', "vert", &vert_file, "vertex shader input file"),
        OPT_STRING('f', "frag", &frag_file, "fragment shader input file"),
        OPT_STRING('l', "lang", &lang, "<see below> shader language output"),
        OPT_STRING('o', "output", &output, "output file template (extension is ignored)"),
        OPT_STRING('t', "output-type", &output_type, "output in json or binary shader format"),
        OPT_STRING('I', "include-dir", &include_dir, "include search directory"),
        OPT_STRING('D', "defines", &defines, "preprocessor definitions, seperated by ';'"),
        OPT_BOOLEAN('L', "list-includes", &list_includes, "print included files"),
        OPT_BOOLEAN('d', "disable-optimization", &disable_optimization, "disable shader lang optimizations"),
        OPT_END(),
    };

    std::string description = "\nSupershader "+std::string(SUPERSHADER_VERSION)+"\nhttps://github.com/supernovaengine/supershader";

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, description.c_str(), 
                                "\nCurrent supported shader stages:"
                                "\n  - Vertex shader (--vert)"
                                "\n  - Fragment shader (--frag)"
                                "\n"
                                "\nCurrent supported shader langs:"
                                "\n  - glsl430: desktop (default)"
                                "\n  - glsl410: desktop"
                                "\n  - glsl330: desktop"
                                "\n  - glsl100: GLES2 / WebGL"
                                "\n  - glsl300es: GLES3 / WebGL2"
                                "\n  - hlsl4: D3D11"
                                "\n  - hlsl5: D3D11"
                                "\n  - msl12macos: Metal for MacOS"
                                "\n  - msl21macos: Metal for MacOS"
                                "\n  - msl12ios: Metal for iOS"
                                "\n  - msl21ios: Metal for iOS"
                                "\n"
                                "\nOutput format types:"
                                "\n  - json"
                                "\n  - binary (SBS file)");
    argc = argparse_parse(&argparse, argc, argv);

    args.isValid = true;

    if (!vert_file && !frag_file){
        fprintf( stderr, "Missing vertex or fragment shader input\n");
        args.isValid = false;
    }

    if (vert_file){
        args.vert_file = vert_file;
    }

    if (frag_file){
        args.frag_file = frag_file;
    }

    if (lang){
        std::string templang = lang;
        if (templang == "glsl330"){
            args.lang = LANG_GLSL;
            args.version = 330;
        }else if (templang == "glsl410"){
            args.lang = LANG_GLSL;
            args.version = 410;
        }else if (templang == "glsl430"){
            args.lang = LANG_GLSL;
            args.version = 430;
        }else if (templang == "glsl100"){
            args.lang = LANG_GLSL;
            args.version = 100;
            args.es = true;
        }else if (templang == "glsl300es"){
            args.lang = LANG_GLSL;
            args.version = 300;
            args.es = true;
        }else if (templang == "hlsl4"){
            args.lang = LANG_HLSL;
            args.version = 40;
        }else if (templang == "hlsl5"){
            args.lang = LANG_HLSL;
            args.version = 50;
        }else if (templang == "msl12macos"){
            args.lang = LANG_MSL;
            args.version = 10200;
            args.platform = PLATFORM_MACOS;
        }else if (templang == "msl21macos"){
            args.lang = LANG_MSL;
            args.version = 20100;
            args.platform = PLATFORM_MACOS;
        }else if (templang == "msl12ios"){
            args.lang = LANG_MSL;
            args.version = 10200;
            args.platform = PLATFORM_IOS;
        }else if (templang == "msl21ios"){
            args.lang = LANG_MSL;
            args.version = 20100;
            args.platform = PLATFORM_IOS;
        }else{
            fprintf( stderr, "Unsupported shader output language: %s\n", lang);
            args.isValid = false;
        }
    } else {
        args.lang = LANG_GLSL;
        args.version = 430;
        fprintf( stdout, "Not defined shader output language, using: glsl430\n");
    }

    if (output){
        args.output_basename = get_filename(output);
        args.output_dir = get_directory(output) + kPathSeparator;
    }else{
        args.output_basename = "output";
    }

    if (output_type){
        std::string temptype = output_type;
        if (temptype == "json"){
            args.output_type = OUTPUT_JSON;
        } else if (temptype == "binary"){
            args.output_type = OUTPUT_BINARY;
        }else{
            fprintf( stderr, "Unsupported output type: %s\n", output_type);
            args.isValid = false;
        }
    }else{
        args.output_type = OUTPUT_JSON;
    }

    if (include_dir){
        args.include_dir = include_dir;
    }

    if (defines){
        args.defines = parse_defines(defines);
    }

    if (list_includes != 0){
        args.list_includes = true;
    }

    if (disable_optimization != 0){
        args.optimization = false;
    }

    return args;
}