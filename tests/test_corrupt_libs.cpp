#include "helpers.h"
#include <cstdint>

// ---------------------------------------------------------------------------
// Тесты устойчивости парсеров ELF/AR к пустым и битым библиотекам.
//
// belder рекурсивно находит в проекте файлы вида lib*.a / lib*.so и парсит
// КАЖДУЮ найденную библиотеку (ELF/AR), чтобы собрать таблицу символов.
// Поэтому достаточно положить битый файл рядом с main.cpp и запустить сборку:
// парсер библиотеки будет вызван гарантированно.
//
// Ключевая проверка - belder не должен падать (segfault и т.п.). В helpers.h
// runCommandInDir() запускает belder через fork/exec; если процесс убит
// сигналом, WIFEXITED() == false и exitCode выставляется в -1. Значит
// EXPECT_NE(exitCode, -1) ловит любое аварийное завершение.
//
// Дополнительно: битая библиотека не определяет нужных main символов, поэтому
// в финальную линковку не попадает - сборка самого main.cpp должна проходить.
// ---------------------------------------------------------------------------

namespace {

// Записать произвольные (в т.ч. бинарные, с нулевыми) байты в файл.
void writeBytes(const std::string& path, const std::string& bytes) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream f(path, std::ios::binary);
    f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void putU16(std::string& s, uint16_t v) {
    for (int i = 0; i < 2; ++i) s.push_back(char((v >> (8 * i)) & 0xff));
}
void putU32(std::string& s, uint32_t v) {
    for (int i = 0; i < 4; ++i) s.push_back(char((v >> (8 * i)) & 0xff));
}
void putU64(std::string& s, uint64_t v) {
    for (int i = 0; i < 8; ++i) s.push_back(char((v >> (8 * i)) & 0xff));
}

// Минимальный 64-битный little-endian ELF с таблицей из 2 секций, где секция
// типа SYMTAB указывает sh_link далеко за пределы таблицы секций. Без проверки
// sh_link < e_shnum это приводит к чтению sectionHeaders[57005] - выход за
// границы буфера и почти гарантированный segfault.
std::string makeElfWithBadSectionLink() {
    std::string elf;
    // e_ident[16]
    elf.push_back(0x7f); elf += "ELF";
    elf.push_back(2);  // EI_CLASS = ELFCLASS64
    elf.push_back(1);  // EI_DATA  = ELFDATA2LSB (little-endian)
    elf.push_back(1);  // EI_VERSION
    while (elf.size() < 16) elf.push_back('\0');
    putU16(elf, 1);     // e_type = ET_REL
    putU16(elf, 0x3e);  // e_machine = x86-64
    putU32(elf, 1);     // e_version
    putU64(elf, 0);     // e_entry
    putU64(elf, 0);     // e_phoff
    putU64(elf, 64);    // e_shoff  - таблица секций сразу после заголовка
    putU32(elf, 0);     // e_flags
    putU16(elf, 64);    // e_ehsize
    putU16(elf, 0);     // e_phentsize
    putU16(elf, 0);     // e_phnum
    putU16(elf, 64);    // e_shentsize = sizeof(Elf64_Shdr)
    putU16(elf, 2);     // e_shnum = 2 секции
    putU16(elf, 0);     // e_shstrndx
    // elf.size() == 64

    // Секция 0: SHT_NULL (64 нулевых байта)
    elf.append(64, '\0');

    // Секция 1: SHT_SYMTAB с заведомо некорректным sh_link
    std::string sh;
    putU32(sh, 0);       // sh_name
    putU32(sh, 2);       // sh_type = SHT_SYMTAB
    putU64(sh, 0);       // sh_flags
    putU64(sh, 0);       // sh_addr
    putU64(sh, 0);       // sh_offset
    putU64(sh, 0);       // sh_size = 0 (нет символов, но проверка sh_link все равно достигается)
    putU32(sh, 57005);   // sh_link - вне таблицы секций (e_shnum = 2)
    putU32(sh, 0);       // sh_info
    putU64(sh, 0);       // sh_addralign
    putU64(sh, 0);       // sh_entsize
    elf += sh;           // sh.size() == 64

    return elf;          // итого 192 байта
}

} // namespace

// Общая проверка: положить битую библиотеку в проект, собрать main.cpp,
// убедиться что belder не упал и собрал исполняемый файл.
class CorruptLibFixture : public BelderFixture {
protected:
    void buildWithCorruptLib(const std::string& libName,
                             const std::string& libBytes,
                             const std::string& description) {
        REQUIRE_TOOLS_OR_SKIP({"g++"}, description);
        write("main.cpp", simpleCppMain());
        writeBytes(path(libName), libBytes);

        auto result = runBelder({"main.cpp", "-o", "out"});

        // Главное: процесс не убит сигналом (нет segfault/abort).
        EXPECT_NE(result.exitCode, -1)
            << "belder crashed on corrupt library '" << libName << "'"
            << result.diagnostic(description);
        // Битая библиотека не нужна main - сборка должна пройти успешно.
        EXPECT_BELDER_OK(result, description);
        EXPECT_TRUE(fileExists("out"))
            << "output not produced" << result.diagnostic(description);
    }
};

TEST_F(CorruptLibFixture, EmptyStaticArchiveIsIgnored) {
    buildWithCorruptLib("libempty.a", "",
        "Empty (0-byte) static archive must not crash the AR parser");
}

TEST_F(CorruptLibFixture, EmptySharedLibraryIsIgnored) {
    buildWithCorruptLib("libempty.so", "",
        "Empty (0-byte) shared library must not crash the ELF parser");
}

TEST_F(CorruptLibFixture, TruncatedArchiveMagicDoesNotCrash) {
    // Меньше 8 байт - не дотягивает даже до AR-магии.
    buildWithCorruptLib("libtrunc.a", "!<ar",
        "Static archive shorter than AR magic must be rejected gracefully");
}

TEST_F(CorruptLibFixture, GarbageSharedLibraryDoesNotCrash) {
    // 7 байт мусора: не ELF и короче ELF-заголовка.
    buildWithCorruptLib("libjunk.so", "garbage",
        "Tiny non-ELF shared library must not crash the ELF parser");
}

TEST_F(CorruptLibFixture, ShortNonElfSharedLibraryDoesNotCrash) {
    // Ровно правдоподобная длина, но без ELF-магии (51 байт, < 52).
    buildWithCorruptLib("libnotelf.so", std::string(51, 'X'),
        "Non-ELF shared library just under header size must not crash");
}

TEST_F(CorruptLibFixture, ArchiveMagicOnlyNoMembers) {
    // Валидная AR-магия (8 байт), но без членов архива.
    buildWithCorruptLib("libonlymagic.a", "!<arch>\n",
        "Static archive with valid magic but no members must parse cleanly");
}

TEST_F(CorruptLibFixture, ArchiveWithCorruptMemberSize) {
    // Валидная магия + заголовок члена, у которого поле размера - мусор.
    // sscanf("%lu") не распарсит -> парсер должен корректно остановиться.
    std::string ar = "!<arch>\n";
    ar += "garbage_name/   ";        // 16 байт: имя
    ar += "0           ";            // 12 байт: mtime
    ar += "0     ";                  // 6 байт: uid
    ar += "0     ";                  // 6 байт: gid
    ar += "100644  ";                // 8 байт: mode
    ar += "NOTANUMBER";              // 10 байт: размер (невалидный)
    ar += "`\n";                     // 2 байта: магия конца заголовка
    buildWithCorruptLib("libbadsize.a", ar,
        "AR member with non-numeric size field must not crash the parser");
}

TEST_F(CorruptLibFixture, ElfWithOutOfRangeSectionLinkDoesNotCrash) {
    // Регрессия: SYMTAB с sh_link за пределами таблицы секций.
    // Без проверки sh_link < e_shnum это вызывало чтение за границей буфера.
    buildWithCorruptLib("libbadlink.so", makeElfWithBadSectionLink(),
        "ELF with SYMTAB sh_link beyond the section table must not crash");
}
