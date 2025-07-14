#include "ELFparser.h"


binFile parse_ELF_File(const std::string& path){

	std::ifstream file(path, std::ios::binary);
    unsigned long size = getFileSize(path);
    unsigned char* elf = new unsigned char[size];
    file.read((char*)elf, size);
    file.close();

    uint8_t arch = uint8_t(*(elf + 4)); // 1 - 32 бит, 2 - 64 бит
    if(arch == 2) {
    	Elf64_parse_result result = {getNameNoExt(path)};
    	parse64(result, elf);
        binFile binF;
        binF.name = std::move(result.name);
        binF.callSyms = std::move(result.callSyms);
        binF.defSyms = std::move(result.defSyms);
    	return binF;
    }
   	else {
   		Elf32_parse_result result = {getNameNoExt(path)};
   		parse32(result, elf);
   		binFile binF;
        binF.name = std::move(result.name);
        binF.callSyms = std::move(result.callSyms);
        binF.defSyms = std::move(result.defSyms);
        return binF;
   	} 
    delete[] elf;
}