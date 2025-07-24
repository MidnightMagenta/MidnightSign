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

	for (size_t i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-pk") == 0) {
			outPathPk = std::string(argv[++i]);
		} else if (strcmp(argv[i], "-sk") == 0) {
			outPathSk = std::string(argv[++i]);
		} else if (strcmp(argv[i], "--print") == 0) {
			printKeys = true;
		} else if (strcmp(argv[i], "--overwrite") == 0) {
			overwriteFlag = true;
		}

		else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			std::cout << "md-keygen version " << VERSION << "\n";
			return EXIT_SUCCESS;
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			std::cout << std::left << std::setw(20) << "-pk [path]"
					  << "Specifies the path to a file into which the public key will be written\n";
			std::cout << std::left << std::setw(20) << "-sk [path]"
					  << "Specifies the path to a file into which the secret/private key will be written\n";

			std::cout << std::left << std::setw(20) << "--overwrite"
					  << "If used, the files specified by -pk and -sk parameters will be overwritten.\n"
					  << std::setw(20) << ""
					  << "If this is not "
						 "passed, the files will never be overwritten\n";
			std::cout << std::left << std::setw(20) << "--print"
					  << "Used to print the generated keys in the terminal.\n"
					  << std::setw(20) << ""
					  << "\033[1;41;97m!!!This option makes key generation not "
						 "secure!!!\033[0m\n";

			std::cout << "\nA -pk and -sk parameter must be passed in order for the program to be able to verify the "
						 "signatures\n\n";

			std::cout << std::left << std::setw(20) << "-v or --version"
					  << "Prints the current version of the program\n";
			std::cout << std::left << std::setw(20) << "-h or --help"
					  << "Shows this help message\n";
			std::cout << "\nExample usage: md-keygen -pk public.key -sk secret.key --overwrite\n\n";
			return EXIT_SUCCESS;
		} else {
			std::cout << "Unknown argument: " << argv[i] << "\n";
			return EXIT_FAILURE;
		}
	}

	if (outPathPk.empty() || outPathSk.empty()) {
		std::cout << "No output path specified\n";
		return EXIT_FAILURE;
	}

	uint8_t seed[32];
	uint8_t sk[64];
	uint8_t pk[32];

	std::ifstream rnd = std::ifstream("/dev/urandom", std::ios::binary);
	rnd.read((char *) seed, 32);
	rnd.close();

	crypto_eddsa_key_pair(sk, pk, seed);

	if (printKeys) {
		std::cout << "\033[1;41;97mWARNING: Keys are not secure!!!\033[0m\n";
		std::cout << "\033[32mPublic key:\033[0m 0x";
		for (size_t i = 0; i < 32; i++) { printf("%02x", pk[i]); }
		std::cout << "\n\033[31mSecret key:\033[0m 0x";
		for (size_t i = 0; i < 64; i++) {
			if (i == 32) { printf("\n              "); }
			printf("%02x", sk[i]);
		}
		std::cout << "\n";
	}

	if (std::filesystem::exists(outPathPk)) {
		if (!overwriteFlag) {
			std::cout << outPathPk << " already exists. Use --overwrite if you want to overwrite the file.\n";
			crypto_wipe(sk, 64);
			return -1;
		}
		std::cout << "Overwriting " << outPathPk << "\n";
		std::filesystem::remove(outPathPk);
	}
	if (std::filesystem::exists(outPathSk)) {
		if (!overwriteFlag) {
			std::cout << outPathSk << " already exists. Use --overwrite if you want to overwrite the file.\n";
			crypto_wipe(sk, 64);
			return EXIT_FAILURE;
		}
		std::cout << "Overwriting " << outPathSk << "\n";
		std::filesystem::remove(outPathSk);
	}

	std::ofstream pkOut(outPathPk, std::ios::binary);
	std::ofstream skOut(outPathSk, std::ios::binary);
	pkOut.write((char *) pk, 32);
	skOut.write((char *) sk, 64);

	crypto_wipe(sk, 64);
	return EXIT_SUCCESS;
}