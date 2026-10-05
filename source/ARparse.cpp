#include "ARparse.h"
#include <cstdio>
#include <cstring>
#include <map>

static std::string memberName(unsigned char* header, const std::string& longNames){
    std::string name((char*)header, 16);
    name.erase(name.find_last_not_of(' ') + 1);
    if(name.size() > 1 && name[0] == '/' && isdigit(name[1])){
        size_t pos = std::stoul(name.substr(1));
        if(pos >= longNames.size()) return name;
        size_t end = longNames.find('\n', pos);
        name = longNames.substr(pos, (end == std::string::npos) ? std::string::npos : end - pos);
    }
    if(name.size() > 1 && name.back() == '/') name.pop_back();
    return name;
}

static unsigned long readBigEndian(unsigned char* ptr, unsigned long width){
    unsigned long value = 0;
    for(unsigned long i = 0; i < width; ++i) value = (value << 8) | ptr[i];
    return value;
}

static uint32_t elfArch(unsigned char* elf, unsigned long size){
    if(size < 20 || elf[0] != 127 || elf[1] != 'E' || elf[2] != 'L' || elf[3] != 'F') return 0;
    uint16_t machine = (elf[5] == 2) ? ((elf[18] << 8) | elf[19]) : ((elf[19] << 8) | elf[18]);
    return (uint32_t(elf[4]) << 16) | machine;
}

static void readArmap(binFile& newfile, unsigned char* ar, unsigned long size, unsigned char* armap,
    unsigned long armap_size, bool armap64, const std::string& longNames)
{
    unsigned long width = armap64 ? 8 : 4;
    if(armap_size < width) return;
    unsigned long count = readBigEndian(armap, width);
    if(count > (armap_size - width) / width) return;
    unsigned char* names = armap + width + count * width;
    unsigned char* endptr = armap + armap_size;
    std::map<unsigned long, binFile> members;
    for(unsigned long i = 0; i < count && names < endptr; ++i){
        unsigned char* zero = (unsigned char*)memchr(names, 0, endptr - names);
        if(zero == nullptr) break;
        std::string name((char*)names, zero - names);
        names = zero + 1;
        unsigned long offset = readBigEndian(armap + width + i * width, width);
        if(offset + 60 > size) continue;
        auto it = members.find(offset);
        if(it == members.end()){
            binFile member = {memberName(ar + offset, longNames)};
            member.offset = offset;
            member.arch = elfArch(ar + offset + 60, size - offset - 60);
            it = members.emplace(offset, member).first;
        }
        it->second.defSyms.push_back(name);
        it->second.defStrong.push_back(SYM_UNKNOWN);
    }
    for(auto& member : members) newfile.members.push_back(std::move(member.second));
}

void parseArchiveMember(binFile& lib, int index){
    binFile& member = lib.members[index];
    member.parsed = true;
    std::ifstream file(lib.name, std::ios::binary);
    if(!file.is_open())
        return;
    char magic[8];
    file.read(magic, 8);
    bool thin = (file.gcount() == 8 && memcmp(magic, "!<thin>\n", 8) == 0);
    unsigned char header[60];
    file.seekg(member.offset);
    file.read((char*)header, 60);
    if(file.gcount() != 60)
        return;
    char size_str[11];
    memcpy(size_str, (char*)(header + 48), 10);
    size_str[10] = '\0';
    unsigned long member_size = 0;
    if(sscanf(size_str, "%lu", &member_size) != 1)
        return;

    std::vector<unsigned char> buffer;
    if(thin){
        std::string path = (!member.name.empty() && member.name[0] == '/') ?
            member.name : getFolder(lib.name) + "/" + member.name;
        long fileSize = getFileSize(path);
        if(fileSize < 0)
            return;
        std::ifstream external(path, std::ios::binary);
        buffer.resize(fileSize);
        external.read((char*)buffer.data(), fileSize);
    }
    else{
        if(member_size > (unsigned long)getFileSize(lib.name))
            return;
        buffer.resize(member_size);
        file.read((char*)buffer.data(), member_size);
        if((unsigned long)file.gcount() != member_size)
            return;
    }
    bool isElf = buffer.size() >= 52 && buffer[0] == 127 && buffer[1] == 'E' && buffer[2] == 'L' && buffer[3] == 'F';
    if(!isElf)
        return;
    binFile parseFile = {member.name};
    parseELF(buffer.data(), parseFile, buffer.size());
    member.callSyms = std::move(parseFile.callSyms);
    member.defSyms = std::move(parseFile.defSyms);
    member.callStrong = std::move(parseFile.callStrong);
    member.defStrong = std::move(parseFile.defStrong);
    member.arch = parseFile.arch;
}

void parse_ARLIB(binFile& newfile){
    long fileSize = getFileSize(newfile.name);
    // size < 8 так же отсекает -1 (файл не открылся), т.к. сравнение знаковое.
    if(fileSize < 8){
    	std::cerr << "==================================== ERROR ====================================" << std::endl;
        std::cerr << "belder thinks that file: " << newfile.name << std::endl;
        std::cerr << "is an AR (static lib) file but it is too small" << std::endl;
        std::cerr << std::endl;
        return;
    }
    unsigned long size = (unsigned long)fileSize;

	std::ifstream file(newfile.name, std::ios::binary);
    if(!file.is_open())
        return;
    std::vector<unsigned char> buffer(size); // RAII вместо ручного new/delete
    file.read((char*)buffer.data(), size);
    file.close();
    unsigned char* ar = buffer.data();

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
    bool thin = (size >= 8 && memcmp(ar, "!<thin>\n", 8) == 0);
    isAR = isAR || thin;
    
    if(!isAR){
    	// std::cerr << "==================================== ERROR ====================================" << std::endl;
        // std::cerr << "belder thinks that file: " << newfile.name << std::endl;
        // std::cerr << "is an AR (static lib) file but it is not" << std::endl;
        // std::cerr << "try running belder with -reb flag or with \"clear\" option" << std::endl;
        // std::cerr << "or maybe you specified some strange file in force link section" << std::endl;
        // std::cerr << std::endl;
    	return;
    }

    unsigned char* ptr = ar + 8;
    unsigned char* endptr = ar + size;
    unsigned char* armap = nullptr;
    unsigned long armap_size = 0;
    bool armap64 = false;
    std::string longNames;
    
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
        
        std::string rawName((char*)member_header, 16);
        rawName.erase(rawName.find_last_not_of(' ') + 1);
        bool special = (rawName == "/" || rawName == "//" || rawName == "/SYM64/");
        if(thin && !special) member_size = 0;
        
        if(ptr + member_size > endptr){
            break;
        }
        
        if(rawName == "/" || rawName == "/SYM64/"){
            armap = ptr;
            armap_size = member_size;
            armap64 = (rawName == "/SYM64/");
        }
        else if(rawName == "//")
            longNames = std::string((char*)ptr, member_size);
        else if(armap != nullptr)
            break;
        else{
            binFile member = {memberName(member_header, longNames)};
            member.offset = member_header - ar;
            member.parsed = true;
            bool isElf = (ptr + 4 <= endptr) && 
                         (uint8_t(*ptr) == 127 && uint8_t(*(ptr+1)) == 'E' && 
                          uint8_t(*(ptr+2)) == 'L' && uint8_t(*(ptr+3)) == 'F');
            if(isElf && ptr + 52 <= endptr)
                parseELF(ptr, member, member_size);
            newfile.members.push_back(member);
            if(thin) parseArchiveMember(newfile, newfile.members.size() - 1);
        }
        
        ptr += member_size;
        if(member_size % 2 == 1 && ptr < endptr){
            ptr++;
        }
    }
    if(armap != nullptr) readArmap(newfile, ar, size, armap, armap_size, armap64, longNames);
}
