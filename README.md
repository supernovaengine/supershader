## Supershader: Cross-compiler for shader language

**Supershader** is a command line tool that converts GLSL code to other GLSL versions, HLSL and MSL.
It is used in [Supernova](https://github.com/supernovaengine/supernova) engine as shader creation tool.

### Build
Supershader uses CMake.

```bash
cmake -S $SOURCE_DIR -B $BUILD_DIR

cmake --build $BUILD_DIR --config Release
# "Release" (for --config) could also be "Debug", "MinSizeRel", or "RelWithDebInfo"
```

### Usage
```bash
./supershader --vert=shader.vert --frag=shader.frag ---defines "USE_UV=1; HAS_TEXTURE" --lang glsl330 --output shaderoutput --json
```

#### Arguments
```
    -h, --help                show this help message and exit
    -v, --vert=<str>          vertex shader input file
    -f, --frag=<str>          fragment shader input file
    -l, --lang=<str>          <see below> shader language output
    -o, --output=<str>        output file template (extension is ignored)
    -I, --include-dir=<str>   include search directory
    -D, --defines=<str>       preprocessor definitions, seperated by ';'
    -L, --list-includes       print included files
    -J, --json                output in json and bare shader format
```

#### Current supported shader stages:
- Vertex shader (--vert)
- Fragment shader (--frag)

#### Current supported shader langs:
- glsl330: desktop (default)
- glsl100: GLES2 / WebGL
- glsl300es: GLES3 / WebGL2
- hlsl4: D3D11
- hlsl5: D3D11

#### Output
For default Supershader output format is a .sbs binary file.

```bash
./supershader --vert=shader.vert --frag=shader.frag --output shaderoutput  --lang glsl330
```
Generates ```shaderoutput.sbs``` file

With ```--json``` argument we have reflection info in json and bare shader output.

```bash
./supershader --vert=shader.vert --frag=shader.frag --output shaderoutput  --lang glsl330 --json
```
Generates ```shaderoutput_glsl.json```, ```shaderoutput_vs.glsl```, ```shaderoutput_fs.glsl``` files

### SBS file format
Inspired on septag work file format: [sgs-file.h](https://github.com/septag/glslcc/blob/master/src/sgs-file.h).
Each block header is 8 bytes ( uint32_t fourcc code + uint32_t for size).
- `SBS` block
	- `struct sbs_chunk`
	- `STAG` blocks: defines each shader stage (vs or fs)
        - `struct sbs_stage`
		- [`CODE` | `DATA`] block: actual code or binary (byte-code) data for the shader stage
		- `REFL` block: Reflection data for the shader stage
			- `struct sbs_chunk_refl`: reflection data header
			- `struct sbs_refl_input[]`: array of vertex-shader input attributes (see `sbs_chunk_refl` for number of inputs)
			- `struct sbs_refl_uniform_buffer[]`: array of uniform buffer objects (see `sbs_chunk_refl` for number of uniform buffers)
			- `struct sbs_refl_texture[]`: array of texture objects (see `sbs_chunk_refl` for number of textures)


### Inspired by
- sokol-shdc (https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md)
- glslcc (https://github.com/septag/glslcc)

### External libraries
- glslang (https://github.com/KhronosGroup/glslang)
- SPIRV-Cross (https://github.com/KhronosGroup/SPIRV-Cross)
- nlohmann json (https://github.com/nlohmann/json)
- argparse (https://github.com/cofyc/argparse)