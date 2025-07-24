#include "ARparse.h"

binFile parse_ARLIB(const std::string& path){
	std::ifstream file(path, std::ios::binary);
    unsigned long size = getFileSize(path);
    unsigned char* ar = new unsigned char[size];
    file.read((char*)ar, size);
    file.close();

	bool isAR = false;
    isAR |= (uint8_t(*(ar + 66)) != 96);
    isAR |= (uint8_t(*(ar + 67)) != 10);
    isAR |= (uint8_t(*ar) != '!');
    isAR |= (uint8_t(*(ar + 1)) != '<');
    isAR |= (uint8_t(*(ar + 2)) != 'a');
    isAR |= (uint8_t(*(ar + 3)) != 'r');
    isAR |= (uint8_t(*(ar + 4)) != 'c');
    isAR |= (uint8_t(*(ar + 5)) != 'h');
    isAR |= (uint8_t(*(ar + 6)) != '>');
    isAR |= (uint8_t(*(ar + 7)) != '\n');
    isAR = !isAR;
    if(!isAR){
    	std::cout << "==================================== ERROR ====================================" << std::endl;
        std::cout << "belder thinks that file: " << path << std::endl;
        std::cout << "is an AR (static lib) file but it is not" << std::endl;
        std::cout << "try running belder with -reb flag or with \"clear\" option" << std::endl;
        std::cout << "or maybe you specified some strange file in force link section" << std::endl;
        std::cout << std::endl;
    	binFile dummy = {"dummy"};
		return dummy;
    }
    binFile result = {path};
    unsigned char* ptr = ar;
    unsigned char* endptr = ar + size;
    while(ptr != endptr){
        bool isElf = (uint8_t(*ptr) == 127 && uint8_t(*(ptr+1)) == 'E' && uint8_t(*(ptr+2)) == 'L' && uint8_t(*(ptr+3)) == 'F');
        if(isElf){
            binFile newfile = parseELF(ptr);
            result.callSyms += newfile.callSyms;
            result.defSyms += newfile.defSyms;
        }
        ptr++;
    }
    delete[] ar;
    return result;
}