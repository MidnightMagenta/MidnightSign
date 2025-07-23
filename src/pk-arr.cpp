#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
	std::string varName = "arr";
	std::string path;
	std::string outFile;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--var") == 0) {
			varName = argv[++i];
		} else if (strcmp(argv[i], "-in") == 0) {
			path = argv[++i];
		} else if (strcmp(argv[i], "-out") == 0) {
			outFile = argv[++i];
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			std::cout << std::left << std::setw(20) << "-in"
					  << "Used to specify the binary input file\n";
			std::cout << std::left << std::setw(20) << "-out"
					  << "If this parameter is used, the oputput will be appended to the end of the specified file\n";

			std::cout << std::left << std::setw(20) << "--var [name]"
					  << "Specifies the name of the generated array.\n"
					  << std::setw(20) << ""
					  << "If this argument is not specified, the "
						 "array will be named \"arr\"\n";

			std::cout << std::left << std::setw(20) << "-v or --version"
					  << "Prints the current version of the program\n";
			std::cout << std::left << std::setw(20) << "-h or --help"
					  << "Shows this help message\n";
			std::cout << "\nExample usage: md-keytoarr data.bin > array.h -ver binary_data\n\n";
			return EXIT_SUCCESS;
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			std::cout << "md-keytoarr version 0.1\n";
			return EXIT_SUCCESS;
		} else {
			std::cerr << "Unknown parameter " << argv[i] << "\n";
			return EXIT_FAILURE;
		}
	}

	if (path.empty()) {
		std::cerr << "No input path specified\n";
		return EXIT_FAILURE;
	}

	if (!std::filesystem::exists(path)) {
		std::cerr << "File " << path << " does not exist\n";
		return EXIT_FAILURE;
	}
	if (!std::filesystem::is_regular_file(path)) {
		std::cerr << path << " is not a file\n";
		return EXIT_FAILURE;
	}

	uint8_t *key;
	std::ifstream keyFile(path, std::ios::binary | std::ios::ate);
	size_t fileSz = keyFile.tellg();
	keyFile.seekg(0);
	key = new uint8_t[fileSz];
	keyFile.read((char *) key, fileSz);

	if (outFile.empty()) {
		std::cout << "static const uint8_t " << varName << "[" << fileSz << "] = {";

		for (int i = 0; i < fileSz; i++) {
			if ((i % 8) == 0) { std::cout << "\n\t"; }
			printf("0x%02x, ", key[i]);
		}

		std::cout << "\n};\n";
		delete[] key;
		return EXIT_SUCCESS;
	} else {
		std::ofstream hdr(outFile, std::ios::app);
		hdr << "static const uint8_t " << varName << "[" << fileSz << "] = {";

		for (int i = 0; i < fileSz; i++) {
			if ((i % 8) == 0) { hdr << "\n\t"; }
			hdr << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(key[i]) << ", ";
		}

		hdr << "\n};\n";
		delete[] key;
		return EXIT_SUCCESS;
	}

	delete[] key;
	return EXIT_FAILURE;
}