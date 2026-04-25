#include "ARparse.h"
#include <cstdio>
#include <cstring>

void parse_ARLIB(binFile& newfile){
	std::ifstream file(newfile.name, std::ios::binary);
    unsigned long size = getFileSize(newfile.name);
    
    if(size < 8){
    	std::cerr << "==================================== ERROR ====================================" << std::endl;
        std::cerr << "belder thinks that file: " << newfile.name << std::endl;
        std::cerr << "is an AR (static lib) file but it is too small" << std::endl;
        std::cerr << std::endl;
        return;
    }
    
    unsigned char* ar = new unsigned char[size];
    file.read((char*)ar, size);
    file.close();

	bool isAR = false;
    if(size >= 8){
        isAR |= (uint8_t(*ar) != '!');
        isAR |= (uint8_t(*(ar + 1)) != '<');
        isAR |= (uint8_t(*(ar + 2)) != 'a');
        isAR |= (uint8_t(*(ar + 3)) != 'r');
        isAR |= (uint8_t(*(ar + 4)) != 'c');
        isAR |= (uint8_t(*(ar + 5)) != 'h');
        isAR |= (uint8_t(*(ar + 6)) != '>');
        isAR |= (uint8_t(*(ar + 7)) != '\n');
    } else {
        isAR = true;
    }
    isAR = !isAR;
    
    if(!isAR){
    	// std::cerr << "==================================== ERROR ====================================" << std::endl;
        // std::cerr << "belder thinks that file: " << newfile.name << std::endl;
        // std::cerr << "is an AR (static lib) file but it is not" << std::endl;
        // std::cerr << "try running belder with -reb flag or with \"clear\" option" << std::endl;
        // std::cerr << "or maybe you specified some strange file in force link section" << std::endl;
        // std::cerr << std::endl;
    	delete[] ar;
    	return;
    }
    
    unsigned char* ptr = ar + 8;
    unsigned char* endptr = ar + size;
    
    while(ptr < endptr){
        if(endptr - ptr < 60){
            break;
        }
        
        unsigned char* member_header = ptr;
        
        char size_str[11];
        memcpy(size_str, (char*)(member_header + 48), 10);
        size_str[10] = '\0';
        
        unsigned long member_size = 0;
        if(sscanf(size_str, "%lu", &member_size) != 1){
            break;
        }
        
        ptr += 60;
        
        if(ptr + member_size > endptr){
            break;
        }
        
        bool isElf = (ptr + 4 <= endptr) && 
                     (uint8_t(*ptr) == 127 && uint8_t(*(ptr+1)) == 'E' && 
                      uint8_t(*(ptr+2)) == 'L' && uint8_t(*(ptr+3)) == 'F');
        
        if(isElf && ptr + 52 <= endptr){
            binFile parseFile;
            parseELF(ptr, parseFile, member_size);
            newfile.callSyms += parseFile.callSyms;
            newfile.defSyms += parseFile.defSyms;
        }
        
        ptr += member_size;
        if(member_size % 2 == 1 && ptr < endptr){
            ptr++;
        }
    }
    
    delete[] ar;
}