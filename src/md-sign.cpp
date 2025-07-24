#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <monocypher.h>
#include <string>
#include <utility>
#include <vector>

int verify_elf64(std::ifstream &file, std::string filePath, size_t fileSz, const Elf64_Ehdr &ehdr, uint8_t *key,
				 std::string objcopy) {
	size_t phdrBufferSize = ehdr.e_phnum * ehdr.e_phentsize;
	Elf64_Phdr *phdrs = new Elf64_Phdr[ehdr.e_phnum];
	file.seekg(ehdr.e_phoff);
	file.read((char *) phdrs, phdrBufferSize);

	std::vector<std::pair<size_t, uint8_t *>> sections;

	for (size_t i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type == PT_LOAD && phdrs[i].p_filesz > 0) {
			uint8_t *buffer = new uint8_t[phdrs[i].p_filesz];
			file.seekg(phdrs[i].p_offset);
			file.read((char *) buffer, phdrs[i].p_filesz);
			sections.push_back(std::make_pair(phdrs[i].p_filesz, buffer));
		}
	}
	file.close();

	uint8_t *signatures = new uint8_t[sections.size() * 64];
	for (size_t i = 0; i < sections.size(); i++) {
		uint8_t *sig = signatures + (i * 64);
		crypto_eddsa_sign(sig, key, sections[i].second, sections[i].first);
	}

	std::ofstream sigTmp("./signature.tmp", std::ios::binary);
	sigTmp.write((char *) signatures, sections.size() * 64);
	sigTmp.close();

	std::string cmd = objcopy + " --remove-section .note.sig " + filePath;
	int res = std::system(cmd.data());
	if (res == 0) {
		cmd = objcopy + " --add-section .note.sig=signature.tmp " + filePath;
		int res = std::system(cmd.data());
	}

	for (size_t i = 0; i < sections.size(); i++) { delete[] sections[i].second; }
	delete[] signatures;
	delete[] phdrs;
	std::filesystem::remove("./signature.tmp");

	if (res != 0) {
		std::cerr << "Failed to sign file\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int verify_elf32(std::ifstream &file, std::string filePath, size_t fileSz, const Elf32_Ehdr &ehdr, uint8_t *key,
				 std::string objcopy) {
	std::cerr << "32 bit ELF not supported\n";
	return EXIT_FAILURE;
}

int main(int argc, char **argv) {
	std::string keyPath;
	std::string filePath;
	std::string objcopy = "objcopy";

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-k") == 0) {
			keyPath = argv[++i];
		} else if (strcmp(argv[i], "-f") == 0) {
			filePath = argv[++i];
		} else if (strcmp(argv[i], "--objcpy") == 0) {
			objcopy = argv[++i];
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			std::cout << std::left << std::setw(20) << "-k [path]"
					  << "Specifies the path to a file containing the privvate key\n";
			std::cout << std::left << std::setw(20) << "-f [path]"
					  << "Specifies the path to a file to be signed\n";

			std::cout << "\nA -k and -f parameter must be passed in order for the program to be able to verify the "
						 "signatures\n\n";

			std::cout << std::left << std::setw(20) << "--objcopy"
					  << "Specifies the objcopy command to use\n";

			std::cout << std::left << std::setw(20) << "-v or --version"
					  << "Prints the current version of the program\n";
			std::cout << std::left << std::setw(20) << "-h or --help"
					  << "Shows this help message\n";
			std::cout << "\nExample usage: md-sign -k secret.key -f executable.elf\n\n";
			return EXIT_SUCCESS;
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			std::cout << "md-sign version " << VERSION << "\n";
			return EXIT_SUCCESS;
		} else {
			std::cerr << "Unknown argument: " << argv[i] << "\n";
			return EXIT_FAILURE;
		}
	}

	if (keyPath.empty() || filePath.empty()) {
		if (keyPath.empty()) {
			std::cerr << "The key has not been specified. Use -k [path] to specify the path to the key\n";
		}
		if (filePath.empty()) {
			std::cerr << "The path to a file has not been specified. Use -f [path] to specify the path to a file to "
						 "sign\n";
		}
		return EXIT_FAILURE;
	}

	if (!std::filesystem::exists(keyPath)) {
		std::cerr << "Key file does not exist: " << keyPath << "\n";
		return EXIT_FAILURE;
	}
	if (!std::filesystem::is_regular_file(keyPath)) {
		std::cerr << "Key path does not point to a file: " << keyPath << "\n";
		return EXIT_FAILURE;
	}

	if (!std::filesystem::exists(filePath)) {
		std::cerr << "File does not exist: " << filePath << "\n";
		return EXIT_FAILURE;
	}
	if (!std::filesystem::is_regular_file(filePath)) {
		std::cerr << "File path does not point to a file: " << filePath << "\n";
		return EXIT_FAILURE;
	}

	uint8_t key[64];
	std::ifstream keyFile(keyPath, std::ios::binary | std::ios::ate);
	if (keyFile.tellg() < 64) {
		std::cout << "Invalid key file " << keyPath << "\n";
		return EXIT_FAILURE;
	}
	keyFile.seekg(0);
	keyFile.read((char *) key, 64);
	keyFile.close();

	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	size_t fileSize = file.tellg();
	file.seekg(0);

	Elf64_Ehdr ehdr;
	file.read((char *) &ehdr, sizeof(Elf64_Ehdr));

	int res;
	if (memcmp(&ehdr.e_ident[EI_MAG0], ELFMAG, SELFMAG) == 0) {
		if (ehdr.e_ident[EI_CLASS] == ELFCLASS64) {
			res = verify_elf64(file, filePath, fileSize, ehdr, key, objcopy);
		} else if (ehdr.e_ident[EI_CLASS] == ELFCLASS32) {
			Elf32_Ehdr ehdr32;
			file.seekg(0);
			file.read((char *) &ehdr32, sizeof(Elf32_Ehdr));
			verify_elf32(file, filePath, fileSize, ehdr32, key, objcopy);
		} else {
			crypto_wipe(key, 64);
			std::cerr << "Invalid file format\n";
			return EXIT_FAILURE;
		}
	} else {
		crypto_wipe(key, 64);
		std::cerr << "Invalid file format\n";
		return EXIT_FAILURE;
	}

	crypto_wipe(key, 64);
	return res;
}