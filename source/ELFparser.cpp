#include "ELFparser.h"

binFile parseELF(unsigned char* elf, const std::string& name){
    uint8_t arch = uint8_t(*(elf + 4)); // 1 - 32 бит, 2 - 64 бит
    if(arch == 1) { 
        Elf32_parse_result result = {name};
        parse32(result, elf);
        binFile binF;
        binF.name = std::move(result.name);
        binF.callSyms = std::move(result.callSyms);
        binF.defSyms = std::move(result.defSyms);
        return binF;           
    }
    else if(arch == 2){
        Elf64_parse_result result = {name};
        parse64(result, elf);
        binFile binF;
        binF.name = std::move(result.name);
        binF.callSyms = std::move(result.callSyms);
        binF.defSyms = std::move(result.defSyms);
        return binF;
    } 
    else{
        std::cout << "======================== ERROR ========================" << std::endl;
        std::cout << "File: " << name << " \nHas weird arch field: " << uint16_t(arch) << std::endl;
        std::cout << std::endl;
        binFile dummy = {"dummy"};
        return dummy;
    }
}


binFile parse_ELF_File(const std::string& path){

	std::ifstream file(path, std::ios::binary);
    unsigned long size = getFileSize(path);
    unsigned char* elf = new unsigned char[size];
    file.read((char*)elf, size);
    file.close();
    bool isElf = (uint8_t(*elf) == 127 && uint8_t(*(elf+1)) == 'E' && uint8_t(*(elf+2)) == 'L' && uint8_t(*(elf+3)) == 'F');
    if(!isElf){
        std::cout << "==================================== ERROR ====================================" << std::endl;
        std::cout << "belder thinks that file: " << path << std::endl;
        std::cout << "is an ELF file but it is not" << std::endl;
        std::cout << "try running belder with -reb flag or with \"clear\" option" << std::endl;
        std::cout << "or maybe you specified some strange file in force link section" << std::endl;
        std::cout << std::endl;
        binFile dummy = {"dummy"};
        return dummy;
    }
    binFile res = parseELF(elf, path);
    delete[] elf;
    return res;
}