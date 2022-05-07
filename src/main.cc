//
// (c) 2021 Eduardo Doria.
//

#include <stdio.h>

#include "supershader.h"

using namespace supershader;

int main(int argc, const char **argv){

	args_t args = parse_args(argc, argv);
	if (!args.isValid)
		return EXIT_FAILURE;


	std::vector<input_t> inputs;
	if (!load_input(inputs, args))
		return EXIT_FAILURE;

	std::vector<spirv_t> spirvvec;
	spirvvec.resize(inputs.size());
	if (!compile_to_spirv(spirvvec, inputs, args))
		return EXIT_FAILURE;

	if (spirvvec.size() != inputs.size()){
		fprintf(stderr, "Error in pipeline when compile to SPIRV\n");
		return EXIT_FAILURE;
	}

	std::vector<spirvcross_t> spirvcrossvec;
	spirvcrossvec.resize(inputs.size());
	if (!compile_to_lang(spirvcrossvec, spirvvec, inputs, args))
		return EXIT_FAILURE;

	if (spirvcrossvec.size() != inputs.size()){
		fprintf(stderr, "Error in pipeline when compile to shader lang\n");
		return EXIT_FAILURE;
	}

	if (args.output_type == OUTPUT_JSON){
		if (!generate_json(spirvcrossvec, inputs, args))
			return EXIT_FAILURE;
	}else if (args.output_type == OUTPUT_BINARY){
		if (!generate_sbs(spirvcrossvec, inputs, args))
			return EXIT_FAILURE;
	}


	return 0;
}
