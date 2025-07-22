#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "Invalid arguments\n";
		return -1;
	}
	std::string path = argv[1];
	if (!std::filesystem::exists(path)) {
		std::cerr << "File " << path << " does not exist\n";
		return -1;
	}
	if (!std::filesystem::is_regular_file(path)) {
		std::cerr << path << " is not a file\n";
		return -1;
	}

	std::string outFile;

	if (argc > 3) {
		if (strcmp(argv[2], ">") == 0) { outFile = argv[3]; }
		if (!std::filesystem::exists(outFile)) {
			std::cerr << "File " << outFile << " does not exist\n";
			return -1;
		}
		if (!std::filesystem::is_regular_file(outFile)) {
			std::cerr << outFile << " is not a file\n";
			return -1;
		}
	}

	std::string varName = "arr";
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--var") == 0) { varName = argv[++i]; }
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			std::cout << "md-keytoarr converts a binary file into a C style array of bytes. The first argument must be "
						 "the path to the binary data.\n"
					  << "The following sequence may be used to write the array directly to a file [key] > [out]\n"
					  << "The name of the array may be changed with --ver [name]\n";
		}
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			std::cout << "md-keytoarr version 0.1\n";
		}
	}

	uint8_t *key;
	std::ifstream keyFile(path, std::ios::binary | std::ios::ate);
	size_t fileSz = keyFile.tellg();
	keyFile.seekg(0);
	key = new uint8_t[fileSz];
	keyFile.read((char *) key, 32);

	if (outFile.empty()) {
		std::cout << "static const uint8_t " << varName << "[" << fileSz << "] = {";

		for (int i = 0; i < fileSz; i++) {
			if ((i % 8) == 0) { std::cout << "\n\t"; }
			printf("0x%02x, ", key[i]);
		}

		std::cout << "\n};\n";
		return 0;
	} else {
		std::ofstream hdr(outFile, std::ios::ate);
		hdr << "static const uint8_t " << varName << "[" << fileSz << "] = {";

		for (int i = 0; i < fileSz; i++) {
			if ((i % 8) == 0) { hdr << "\n\t"; }
			std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(key[i]) << ", ";
		}

		hdr << "\n};\n";
		return 0;
	}
}