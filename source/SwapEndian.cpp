#include "SwapEndian.h"

void swapBytesElfHeader(Elf64_Ehdr* h){
	h -> e_type = swapBytes(h -> e_type);
	h -> e_machine = swapBytes(h -> e_machine);
	h -> e_shstrndx = swapBytes(h -> e_shstrndx);
	h -> e_shnum = swapBytes(h -> e_shnum);
	h -> e_shentsize = swapBytes(h -> e_shentsize);
	h -> e_phnum = swapBytes(h -> e_phnum);
	h -> e_phentsize = swapBytes(h -> e_phentsize);
	h -> e_ehsize = swapBytes(h -> e_ehsize);
	h -> e_flags = swapBytes(h -> e_flags);
	h -> e_shoff = swapBytes(h -> e_shoff);
	h -> e_phoff = swapBytes(h -> e_phoff);
	h -> e_entry = swapBytes(h -> e_entry);
	h -> e_version = swapBytes(h -> e_version);
}

void swapBytesProgHeader(Elf64_Phdr* h){
	h -> p_type = swapBytes(h -> p_type);
	h -> p_flags = swapBytes(h -> p_flags);
	h -> p_offset = swapBytes(h -> p_offset);
	h -> p_vaddr = swapBytes(h -> p_vaddr);
	h -> p_paddr = swapBytes(h -> p_paddr);
	h -> p_filesz = swapBytes(h -> p_filesz);
	h -> p_memsz = swapBytes(h -> p_memsz);
	h -> p_align = swapBytes(h -> p_align);
}

void swapBytesSectionHeader(Elf64_Shdr* h){
	h -> sh_name = swapBytes(h -> sh_name);
	h -> sh_type = swapBytes(h -> sh_type);
	h -> sh_flags = swapBytes(h -> sh_flags);
	h -> sh_addr = swapBytes(h -> sh_addr);
	h -> sh_offset = swapBytes(h -> sh_offset);
	h -> sh_size = swapBytes(h -> sh_size);
	h -> sh_link = swapBytes(h -> sh_link);
	h -> sh_info = swapBytes(h -> sh_info);
	h -> sh_addralign = swapBytes(h -> sh_addralign);
	h -> sh_entsize = swapBytes(h -> sh_entsize);
}

void swapBytesSymHeader(Elf64_Sym* h){
	h -> st_name = swapBytes(h -> st_name);
	h -> st_shndx = swapBytes(h -> st_shndx);
	h -> st_value = swapBytes(h -> st_value);
	h -> st_size = swapBytes(h -> st_size);
}

void swapBytesRela(Elf64_Rela* h){
	h -> r_offset = swapBytes(h -> r_offset);
	h -> r_info = swapBytes(h -> r_info);
	h -> r_addend = swapBytes(h -> r_addend);
}

void swapBytesRel(Elf64_Rel* h){
	h -> r_offset = swapBytes(h -> r_offset);
	h -> r_info = swapBytes(h -> r_info);
}



void swapBytesElfHeader(Elf32_Ehdr* h){
	h -> e_type = swapBytes(h -> e_type);
	h -> e_machine = swapBytes(h -> e_machine);
	h -> e_shstrndx = swapBytes(h -> e_shstrndx);
	h -> e_shnum = swapBytes(h -> e_shnum);
	h -> e_shentsize = swapBytes(h -> e_shentsize);
	h -> e_phnum = swapBytes(h -> e_phnum);
	h -> e_phentsize = swapBytes(h -> e_phentsize);
	h -> e_ehsize = swapBytes(h -> e_ehsize);
	h -> e_flags = swapBytes(h -> e_flags);
	h -> e_shoff = swapBytes(h -> e_shoff);
	h -> e_phoff = swapBytes(h -> e_phoff);
	h -> e_entry = swapBytes(h -> e_entry);
	h -> e_version = swapBytes(h -> e_version);
}

void swapBytesProgHeader(Elf32_Phdr* h){
	h -> p_type = swapBytes(h -> p_type);
	h -> p_flags = swapBytes(h -> p_flags);
	h -> p_offset = swapBytes(h -> p_offset);
	h -> p_vaddr = swapBytes(h -> p_vaddr);
	h -> p_paddr = swapBytes(h -> p_paddr);
	h -> p_filesz = swapBytes(h -> p_filesz);
	h -> p_memsz = swapBytes(h -> p_memsz);
	h -> p_align = swapBytes(h -> p_align);
}

void swapBytesSectionHeader(Elf32_Shdr* h){
	h -> sh_name = swapBytes(h -> sh_name);
	h -> sh_type = swapBytes(h -> sh_type);
	h -> sh_flags = swapBytes(h -> sh_flags);
	h -> sh_addr = swapBytes(h -> sh_addr);
	h -> sh_offset = swapBytes(h -> sh_offset);
	h -> sh_size = swapBytes(h -> sh_size);
	h -> sh_link = swapBytes(h -> sh_link);
	h -> sh_info = swapBytes(h -> sh_info);
	h -> sh_addralign = swapBytes(h -> sh_addralign);
	h -> sh_entsize = swapBytes(h -> sh_entsize);
}

void swapBytesSymHeader(Elf32_Sym* h){
	h -> st_name = swapBytes(h -> st_name);
	h -> st_shndx = swapBytes(h -> st_shndx);
	h -> st_value = swapBytes(h -> st_value);
	h -> st_size = swapBytes(h -> st_size);
}


