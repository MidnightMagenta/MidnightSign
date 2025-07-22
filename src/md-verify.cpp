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

int verify_elf64(std::ifstream &file, std::string filePath, size_t fileSz, const Elf64_Ehdr &ehdr, uint8_t *key) {
	Elf64_Shdr *shdrs = new Elf64_Shdr[ehdr.e_shnum];

	file.seekg(ehdr.e_shoff);
	file.read((char *) shdrs, ehdr.e_shnum * sizeof(Elf64_Shdr));

	Elf64_Shdr shstrtab_hdr = shdrs[ehdr.e_shstrndx];
	char *shstrtab = new char[shstrtab_hdr.sh_size];
	file.seekg(shstrtab_hdr.sh_offset);
	file.read(shstrtab, shstrtab_hdr.sh_size);

	uint8_t *signatures = nullptr;
	size_t sigSize = 0;

	for (int i = 0; i < ehdr.e_shnum; i++) {
		const char *name = &shstrtab[shdrs[i].sh_name];
		if (strcmp(name, ".note.sig") == 0) {
			signatures = new uint8_t[shdrs[i].sh_size];
			sigSize = shdrs[i].sh_size / 64;
			file.seekg(shdrs[i].sh_offset);
			file.read((char *) signatures, shdrs[i].sh_size);
			break;
		}
	}

	if (signatures == nullptr) {
		std::cerr << "Failed to loacate .note.sig\n";
		return EXIT_FAILURE;
	}

	Elf64_Phdr *phdrs = new Elf64_Phdr[ehdr.e_phnum];
	file.seekg(ehdr.e_phoff);
	file.read((char *) phdrs, ehdr.e_phnum * sizeof(Elf64_Phdr));

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

	if (sigSize < sections.size()) {
		delete[] signatures;
		delete[] shdrs;
		delete[] phdrs;
		delete[] shstrtab;
		for (size_t i = 0; i < sections.size(); i++) { delete[] sections[i].second; }
		return EXIT_FAILURE;
	}

	int res = EXIT_SUCCESS;

	for (size_t i = 0; i < sections.size(); i++) {
		uint8_t *sig = signatures + (i * 64);
		if (crypto_eddsa_check(sig, key, sections[i].second, sections[i].first) != 0) {
			std::cout << "Failed to verify signature for section " << i << "\n";
			res = EXIT_FAILURE;
			break;
		}
	}

	if (res == EXIT_SUCCESS) { std::cout << "Signatures sucessfully verified\n"; }
	delete[] signatures;
	delete[] shdrs;
	delete[] phdrs;
	delete[] shstrtab;
	for (size_t i = 0; i < sections.size(); i++) { delete[] sections[i].second; }
	return res;
}

int verify_elf32(std::ifstream &file, std::string filePath, size_t fileSz, const Elf32_Ehdr &ehdr, uint8_t *key) {
	std::cerr << "32 bit ELF not supported\n";
	return EXIT_FAILURE;
}

int main(int argc, char **argv) {
	std::string keyPath;
	std::string filePath;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-k") == 0) {
			keyPath = argv[++i];
		} else if (strcmp(argv[i], "-f") == 0) {
			filePath = argv[++i];
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			std::cout << std::left << std::setw(20) << "-k [path]"
					  << "Specifies the path to a file containing the public key\n";
			std::cout << std::left << std::setw(20) << "-f [path]"
					  << "Specifies the path to a file to be signed\n";

			std::cout << "\nA -k and -f parameter must be passed in order for the program to be able to verify the "
						 "signatures\n\n";

			std::cout << std::left << std::setw(20) << "-v or --version"
					  << "Prints the current version of the program\n";
			std::cout << std::left << std::setw(20) << "-h or --help"
					  << "Shows this help message\n";
			std::cout << "\nExample usage: md-verify -k public.key -f executable.elf\n\n";
			return EXIT_SUCCESS;
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			std::cout << "md-verify version 0.1\n";
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

	uint8_t key[32];
	std::ifstream keyFile(keyPath, std::ios::binary | std::ios::ate);
	if (keyFile.tellg() < 32) {
		std::cout << "Invalid key file " << keyPath << "\n";
		return EXIT_FAILURE;
	}
	keyFile.seekg(0);
	keyFile.read((char *) key, 32);
	keyFile.close();

	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	size_t fileSize = file.tellg();
	file.seekg(0);

	Elf64_Ehdr ehdr;
	file.read((char *) &ehdr, sizeof(Elf64_Ehdr));

	int res = EXIT_SUCCESS;
	if (memcmp(&ehdr.e_ident[EI_MAG0], ELFMAG, SELFMAG) == 0) {
		if (ehdr.e_ident[EI_CLASS] == ELFCLASS64) {
			res = verify_elf64(file, filePath, fileSize, ehdr, key);
		} else if (ehdr.e_ident[EI_CLASS] == ELFCLASS32) {
			Elf32_Ehdr ehdr32;
			file.seekg(0);
			file.read((char *) &ehdr32, sizeof(Elf32_Ehdr));

			res = verify_elf32(file, filePath, fileSize, ehdr32, key);
		} else {
			std::cerr << "Invalid file format\n";
			return EXIT_FAILURE;
		}
	} else {
		std::cerr << "Invalid file format\n";
		return EXIT_FAILURE;
	}

	return res;
}