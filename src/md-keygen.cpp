#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <monocypher.h>
#include <string>
int main(int argc, char **argv) {
	bool overwriteFlag = false;
	bool printKeys = false;
	std::string outPathPk;
	std::string outPathSk;

	for (size_t i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-pk") == 0) { outPathPk = std::string(argv[++i]); }
		if (strcmp(argv[i], "-sk") == 0) { outPathSk = std::string(argv[++i]); }
		if (strcmp(argv[i], "--print") == 0) { printKeys = true; }
		if (strcmp(argv[i], "--overwrite") == 0) { overwriteFlag = true; }

		if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			std::cout << "Midnight's Dumb Keygen Tool version 0.1\n";
			return 0;
		}
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			std::cout << "To specify the private key output file, use -pk [path]\n"
					  << "To specify the secret key output file use -sk [path]\n"
					  << "If the files are being overwritten, use --overwrite\n"
					  << "Use --print to print the keys to the terminal during generation\n";
			return 0;
		}
	}

	if (outPathPk.empty() || outPathSk.empty()) {
		std::cout << "No output path specified\n";
		return -1;
	}

	uint8_t seed[32];
	uint8_t sk[64];
	uint8_t pk[32];

	std::ifstream rnd = std::ifstream("/dev/urandom", std::ios::binary);
	rnd.read((char *) seed, 32);
	rnd.close();

	crypto_eddsa_key_pair(sk, pk, seed);

	if (printKeys) {
		std::cout << "Public key: 0x";
		for (size_t i = 0; i < 32; i++) { printf("%02x", pk[i]); }
		std::cout << "\nSecret key: 0x";
		for (size_t i = 0; i < 64; i++) { printf("%02x", sk[i]); }
		std::cout << "\n";
	}

	if (std::filesystem::exists(outPathPk)) {
		if (!overwriteFlag) {
			std::cout << outPathPk << " already exists. Use --overwrite if you want to overwrite the file.\n";
			return -1;
		}
		std::cout << "Overwriting " << outPathPk << "\n";
		std::filesystem::remove(outPathPk);
	}
	if (std::filesystem::exists(outPathSk)) {
		if (!overwriteFlag) {
			std::cout << outPathSk << " already exists. Use --overwrite if you want to overwrite the file.\n";
			return -1;
		}
		std::cout << "Overwriting " << outPathSk << "\n";
		std::filesystem::remove(outPathSk);
	}

	std::ofstream pkOut(outPathPk, std::ios::binary);
	std::ofstream skOut(outPathSk, std::ios::binary);
	pkOut.write((char *) pk, 32);
	skOut.write((char *) sk, 64);
	return 0;
}