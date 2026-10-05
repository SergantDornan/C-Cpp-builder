#include "helpers.h"

namespace {

std::string mainCalling(const std::vector<std::string>& funcs) {
    std::string code = "#include <iostream>\n";
    for (const auto& f : funcs) code += "int " + f + "();\n";
    code += "int main(){ std::cout << \"_res_\"";
    for (const auto& f : funcs) code += " << " + f + "() << \"_\"";
    code += " << std::endl; return 0; }\n";
    return code;
}

size_t countOccurrences(const std::string& text, const std::string& what) {
    size_t count = 0;
    for (size_t pos = text.find(what); pos != std::string::npos; pos = text.find(what, pos + 1)) ++count;
    return count;
}

std::string bigEndian(uint64_t value, int width) {
    std::string out(width, '\0');
    for (int i = width - 1; i >= 0; --i) {
        out[i] = char(value & 0xff);
        value >>= 8;
    }
    return out;
}

uint64_t readBigEndian(const std::string& data, size_t pos, int width) {
    uint64_t value = 0;
    for (int i = 0; i < width; ++i) value = (value << 8) | (unsigned char)data[pos + i];
    return value;
}

std::string padRight(const std::string& s, size_t width) {
    return s + std::string(width - s.size(), ' ');
}

std::string convertToSym64(const std::string& archive) {
    size_t size = std::stoul(archive.substr(8 + 48, 10));
    std::string data = archive.substr(68, size);
    std::string rest = archive.substr(68 + size + (size & 1));
    uint64_t count = readBigEndian(data, 0, 4);
    std::string names = data.substr(4 + 4 * count);
    size_t newSize = 8 + 8 * count + names.size();
    long delta = long(newSize + (newSize & 1)) - long(size + (size & 1));
    std::string newData = bigEndian(count, 8);
    for (uint64_t i = 0; i < count; ++i)
        newData += bigEndian(readBigEndian(data, 4 + 4 * i, 4) + delta, 8);
    newData += names;
    std::string header = padRight("/SYM64/", 16) + padRight("0", 12) + padRight("0", 6) + padRight("0", 6) +
                         padRight("0", 8) + padRight(std::to_string(newSize), 10) + "`\n";
    return "!<arch>\n" + header + newData + ((newSize & 1) ? "\n" : "") + rest;
}

}

class LibSelectFixture : public BelderFixture {
protected:
    std::string srcDir;

    void SetUp() override {
        BelderFixture::SetUp();
        srcDir = tmpDir + "_libsrc";
        makeDir(srcDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(srcDir);
        BelderFixture::TearDown();
    }

    std::string object(const std::string& name, const std::string& code, bool pic = false) {
        std::string src = srcDir + "/" + name + ".cpp";
        std::string obj = srcDir + "/" + name + ".o";
        writeFile(src, code);
        auto r = runCommand("g++ -c " + std::string(pic ? "-fPIC " : "") + "'" + src + "' -o '" + obj + "'");
        EXPECT_EQ(r.exitCode, 0) << r.diagnostic();
        return obj;
    }

    void archive(const std::string& rel, const std::vector<std::pair<std::string, std::string>>& members,
                 const std::string& flags = "rcs") {
        std::filesystem::create_directories(std::filesystem::path(path(rel)).parent_path());
        std::filesystem::remove(path(rel));
        std::string cmd = "ar " + flags + " '" + path(rel) + "'";
        for (const auto& m : members) cmd += " '" + object(m.first, m.second) + "'";
        auto r = runCommand(cmd);
        ASSERT_EQ(r.exitCode, 0) << r.diagnostic();
    }

    void shared(const std::string& rel, const std::string& name, const std::string& code) {
        std::filesystem::create_directories(std::filesystem::path(path(rel)).parent_path());
        std::string obj = object(name, code, true);
        auto r = runCommand("g++ -shared '" + obj + "' -o '" + path(rel) + "'");
        ASSERT_EQ(r.exitCode, 0) << r.diagnostic();
    }

    std::vector<std::string> profiles() {
        return belderProfileDirs(belderProjectDir(tmpDir));
    }
};

TEST_F(LibSelectFixture, PicksTheOnlyArchiveThatHasAllNeededSymbols) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "lib1/lib2 choice";
    archive("libs/liba1.a", {{"l1", "int s1(){return 11;}\nint s2(){return 12;}\nint s3(){return 13;}\n"}});
    archive("libs/liba2.a", {{"l2", "int s1(){return 21;}\nint s2(){return 22;}\nint s4(){return 24;}\n"}});
    write("main.cpp", mainCalling({"s1", "s4"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_21_24_")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("Linking lib: liba2.a")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: liba1.a")) << r.diagnostic();
}

TEST_F(LibSelectFixture, GreedyChoicePrefersLibraryCoveringMoreSymbols) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "coverage choice";
    archive("libs/libpa.a", {{"pa", "int fa(){return 1;}\n"}});
    archive("libs/libpb.a", {{"pb", "int fb(){return 2;}\n"}});
    archive("libs/libpc.a", {{"pc", "int fc(){return 3;}\n"}});
    archive("libs/libpbig.a", {{"pbig", "int fa(){return 4;}\nint fb(){return 5;}\nint fc(){return 6;}\n"}});
    write("main.cpp", mainCalling({"fa", "fb", "fc"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_4_5_6_")) << r.diagnostic();
    EXPECT_EQ(countOccurrences(r.stdout_str, "Linking lib:"), 1u) << r.diagnostic();
}

TEST_F(LibSelectFixture, ChainOfArchives) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "chain of archives";
    archive("libs/libch1.a", {{"ch1", "int f2();\nint f1(){return f2() + 1;}\n"}});
    archive("libs/libch2.a", {{"ch2", "int f3();\nint f2(){return f3() + 1;}\n"}});
    archive("libs/libch3.a", {{"ch3", "int f3(){return 40;}\n"}});
    write("main.cpp", mainCalling({"f1"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_42_")) << r.diagnostic();
    EXPECT_EQ(countOccurrences(r.stdout_str, "Linking lib:"), 3u) << r.diagnostic();
}

TEST_F(LibSelectFixture, CycleBetweenArchivesIsLinkedThroughGroup) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "cycle between archives";
    archive("libs/libcya.a", {{"cya1", "int cb();\nint ca(){return cb() + 1;}\n"},
                              {"cya2", "int ca2(){return 7;}\n"}});
    archive("libs/libcyb.a", {{"cyb", "int ca2();\nint cb(){return ca2() * 10;}\n"}});
    write("main.cpp", mainCalling({"ca"}));
    auto r = runBelder({"-log", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_71_")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("-Wl,--start-group")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("-Wl,--end-group")) << r.diagnostic();
}

TEST_F(LibSelectFixture, WeakDefinitionsInTwoArchivesDoNotConflict) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "weak symbols";
    archive("libs/libwa.a", {{"wa", "__attribute__((weak)) int w(){return 1;}\nint wa(){return w();}\n"}});
    archive("libs/libwb.a", {{"wb", "__attribute__((weak)) int w(){return 2;}\nint wb(){return 3;}\n"}});
    write("main.cpp", mainCalling({"wa", "wb"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("multiple definition")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SameSymbolInTwoSharedLibrariesIsNotAConflict) {
    if (!toolExists("g++")) GTEST_SKIP() << ".so against .so";
    shared("libs/libsa.so", "sa", "int s1(){return 1;}\nint sa(){return 10;}\n");
    shared("libs/libsb.so", "sb", "int s1(){return 2;}\nint sb(){return 20;}\n");
    write("main.cpp", mainCalling({"sa", "sb"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_10_20_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, RealConflictBetweenPulledMembersIsReported) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "real conflict";
    archive("libs/libca.a", {{"ca", "int dup(){return 1;}\nint ca(){return dup();}\n"}});
    archive("libs/libcb.a", {{"cb", "int dup(){return 2;}\nint cb(){return dup();}\n"}});
    write("main.cpp", mainCalling({"ca", "cb"}));
    auto r = runBelder({"run"});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("conflict")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("dup()")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("_res_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SameSymbolInUnpulledMemberIsNotAConflict) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "unpulled member";
    archive("libs/libm1.a", {{"m1a", "int s1(){return 1;}\n"}, {"m1b", "int s2(){return 2;}\n"}});
    archive("libs/libm2.a", {{"m2c", "int s2(){return 3;}\nint s4(){return 4;}\n"}});
    write("main.cpp", mainCalling({"s1", "s4"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_1_4_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, UnusedMemberDoesNotPullItsDependencies) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "unused member dependencies";
    archive("libs/libmix.a", {{"used", "int usedf(){return 5;}\n"},
                              {"unused", "int trig();\nint unusedf(){return trig();}\n"}});
    archive("libs/libt1.a", {{"t1", "int trig(){return 1;}\n"}});
    archive("libs/libt2.a", {{"t2", "int trig(){return 2;}\n"}});
    write("main.cpp", mainCalling({"usedf"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_5_")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libt")) << r.diagnostic();
}

TEST_F(LibSelectFixture, DependencyBetweenMembersOfOneArchive) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "members of one archive";
    archive("libs/libdep.a", {{"depa", "int fb();\nint fa(){return fb() + 1;}\n"},
                              {"depb", "int fb(){return 8;}\n"}});
    write("main.cpp", mainCalling({"fa"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_9_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, ArchiveWithLongMemberNames) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "long member names";
    archive("libs/liblong.a", {{"a_really_long_member_name_number_one", "int lf1(){return 1;}\n"},
                               {"a_really_long_member_name_number_two", "int lf1b();\nint lf2(){return lf1b();}\n"},
                               {"a_really_long_member_name_number_three", "int lf1b(){return 2;}\n"}});
    write("main.cpp", mainCalling({"lf1", "lf2"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_1_2_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, ThinArchive) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "thin archive";
    archive("libs/libthin.a", {{"thin1", "int th2();\nint th1(){return th2() + 1;}\n"},
                               {"thin2", "int th2(){return 30;}\n"}}, "rcs --thin");
    write("main.cpp", mainCalling({"th1"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_31_")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("Linking lib: libthin.a")) << r.diagnostic();
}

TEST_F(LibSelectFixture, ArchiveWithoutSymbolTable) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "archive without armap";
    archive("libs/libnomap.a", {{"nomap", "int nm(){return 6;}\n"}}, "rcS");
    write("main.cpp", mainCalling({"nm"}));
    auto r = runBelder();
    EXPECT_TRUE(r.hasOutput("Linking lib: libnomap.a")) << r.diagnostic();
}

TEST_F(LibSelectFixture, ArchiveWith64BitSymbolTable) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "/SYM64/ armap";
    archive("libs/libsym64.a", {{"s64a", "int b64();\nint a64(){return b64() + 1;}\n"},
                                {"s64b", "int b64(){return 63;}\n"}});
    std::string converted = convertToSym64(readFile(path("libs/libsym64.a")));
    std::ofstream(path("libs/libsym64.a"), std::ios::binary) << converted;
    auto nm = runCommand("nm -s '" + path("libs/libsym64.a") + "'");
    ASSERT_EQ(nm.exitCode, 0) << nm.diagnostic();
    write("main.cpp", mainCalling({"a64"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_64_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SymlinksToOneSharedLibraryAreOneLibrary) {
    if (!toolExists("g++")) GTEST_SKIP() << "symlinked .so";
    shared("libs/libsym.so.1.0", "symlib", "int symf(){return 77;}\n");
    std::filesystem::create_symlink("libsym.so.1.0", path("libs/libsym.so.1"));
    std::filesystem::create_symlink("libsym.so.1", path("libs/libsym.so"));
    write("main.cpp", mainCalling({"symf"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_77_")) << r.diagnostic();
    EXPECT_EQ(countOccurrences(r.stdout_str, "Linking lib: libsym"), 1u) << r.diagnostic();
}

TEST_F(LibSelectFixture, SharedLibraryIsPreferredOverArchiveWithSameName) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << ".a and .so of one library";
    archive("libs/libboth.a", {{"botha", "int both(){return 1;}\n"}});
    shared("libs/libboth.so", "boths", "int both(){return 2;}\n");
    write("main.cpp", mainCalling({"both"}));
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("Linking lib: libboth.so")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libboth.a")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_res_2_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SymbolsOfImplicitLibrariesAreNotTakenFromProjectLibraries) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "implicit libraries";
    archive("libs/libfakeio.a", {{"fakeio", "extern \"C\" int puts(const char*){ return 0; }\n"
                                            "extern \"C\" double sqrt(double){ return 0; }\n"}});
    write("main.cpp", "#include <cstdio>\n#include <cmath>\n#include <iostream>\n"
                      "int main(int argc, char**){ puts(\"_real_puts_\"); std::cout << sqrt(16.0 + argc - 1) << std::endl; }\n");
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_real_puts_")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("4")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libfakeio.a")) << r.diagnostic();
}

TEST_F(LibSelectFixture, LinkForceOverridesAutomaticChoice) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "--link-force against automatic choice";
    archive("libs/libfa.a", {{"fa", "int fx(){return 1;}\n"}});
    archive("libs/libfb.a", {{"fb", "int fx(){return 2;}\n"}});
    write("main.cpp", mainCalling({"fx"}));
    auto r1 = runBelder({"run"});
    EXPECT_BELDER_OK(r1, r1.diagnostic());
    EXPECT_TRUE(r1.hasOutput("_res_1_")) << r1.diagnostic();
    auto r2 = runBelder({"--link-force", "libs/libfb.a", "run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_TRUE(r2.hasOutput("_res_2_")) << r2.diagnostic();
    EXPECT_FALSE(r2.hasOutput("Linking lib: libfa.a")) << r2.diagnostic();
}

TEST_F(LibSelectFixture, ProjectFunctionReplacesLibraryAfterEdit) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "link record fast path";
    archive("libs/libfoo.a", {{"foo", "int foo(){return 1;}\n"}});
    write("f.cpp", "int other(){return 0;}\n");
    write("main.cpp", mainCalling({"foo"}));
    auto r1 = runBelder({"run"});
    EXPECT_BELDER_OK(r1, r1.diagnostic());
    EXPECT_TRUE(r1.hasOutput("_res_1_")) << r1.diagnostic();
    EXPECT_TRUE(r1.hasOutput("Linking lib: libfoo.a")) << r1.diagnostic();

    auto r2 = runBelder({"run"});
    EXPECT_TRUE(r2.hasOutput("nothing to link")) << r2.diagnostic();

    write("f.cpp", "int other(){return 0;}\nint foo(){return 2;}\n");
    auto r3 = runBelder({"run"});
    EXPECT_BELDER_OK(r3, r3.diagnostic());
    EXPECT_TRUE(r3.hasOutput("_res_2_")) << r3.diagnostic();
    EXPECT_TRUE(r3.hasOutput("Linking file: f.cpp")) << r3.diagnostic();
    EXPECT_FALSE(r3.hasOutput("Linking lib: libfoo.a")) << r3.diagnostic();
}

TEST_F(LibSelectFixture, MissingSymbolIsReportedAfterLinkError) {
    if (!toolExists("g++")) GTEST_SKIP() << "unresolved symbols hint";
    write("main.cpp", mainCalling({"nowhere_to_be_found"}));
    auto r = runBelder({"run"});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("belder did not find these symbols")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("nowhere_to_be_found()")) << r.diagnostic();
}

TEST_F(LibSelectFixture, LibraryAddedLaterIsFound) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "index: new library";
    write("main.cpp", mainCalling({"later"}));
    auto r1 = runBelder();
    EXPECT_EQ(r1.exitCode, 3) << r1.diagnostic();
    archive("libs/liblater.a", {{"later", "int later(){return 12;}\n"}});
    auto r2 = runBelder({"run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_TRUE(r2.hasOutput("_res_12_")) << r2.diagnostic();
}

TEST_F(LibSelectFixture, ChangedLibraryIsRelinked) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "index: changed library";
    archive("libs/libval.a", {{"val", "int val(){return 1;}\n"}});
    write("main.cpp", mainCalling({"val"}));
    auto r1 = runBelder({"run"});
    EXPECT_TRUE(r1.hasOutput("_res_1_")) << r1.diagnostic();
    archive("libs/libval.a", {{"val", "int val(){return 2;}\nint extra(){return 0;}\n"}});
    auto r2 = runBelder({"run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_FALSE(r2.hasOutput("nothing to link")) << r2.diagnostic();
    EXPECT_TRUE(r2.hasOutput("_res_2_")) << r2.diagnostic();
}

TEST_F(LibSelectFixture, RemovedLibraryGivesLinkError) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "index: removed library";
    archive("libs/libgone.a", {{"gone", "int gone(){return 1;}\n"}});
    write("main.cpp", mainCalling({"gone"}));
    ASSERT_BELDER_OK(runBelder(), "build with library");
    std::filesystem::remove(path("libs/libgone.a"));
    auto r = runBelder({"run"});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("gone()")) << r.diagnostic();
}

TEST_F(LibSelectFixture, CorruptIndexIsRebuilt) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "index: corrupt file";
    archive("libs/libidx.a", {{"idx", "int idx(){return 3;}\n"}});
    write("main.cpp", mainCalling({"idx"}));
    ASSERT_BELDER_OK(runBelder(), "first build");
    auto dirs = profiles();
    ASSERT_EQ(dirs.size(), 1u);
    ASSERT_TRUE(std::filesystem::exists(dirs[0] + "/symindex"));
    std::ofstream(dirs[0] + "/symindex", std::ios::binary) << "garbage garbage garbage";
    write("main.cpp", mainCalling({"idx"}) + "\n");
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_res_3_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, LinkerScriptNamedLikeLibraryIsIgnored) {
    if (!toolExists("g++")) GTEST_SKIP() << "linker script";
    write("libs/libscript.so", "/* GNU ld script */\nGROUP ( /nonexistent/libx.so.6 )\n");
    write("main.cpp", simpleCppMain());
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(r.hasOutput("Linking lib: libscript.so")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SystemZlibFoundThroughIncludeOfSystemLibDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "zlib from /usr/lib";
    const std::string libDir = "/usr/lib/x86_64-linux-gnu";
    if (!std::filesystem::exists("/usr/include/zlib.h") || !std::filesystem::exists(libDir + "/libz.so"))
        GTEST_SKIP() << "zlib is not installed";
    write("main.cpp", "#include <zlib.h>\n#include <iostream>\n"
                      "int main(){ std::cout << \"_zlib_\" << compressBound(100) << std::endl; }\n");
    auto r = runBelder({"-I" + libDir, "-log", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_zlib_")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("libz.so")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("-rpath," + libDir)) << r.diagnostic();
    auto again = runBelder({"run"});
    EXPECT_TRUE(again.hasOutput("nothing to link")) << again.diagnostic();
    EXPECT_TRUE(again.hasOutput("_zlib_")) << again.diagnostic();
}

TEST_F(LibSelectFixture, SymbolsFromLinkerScriptWithRelativeNamesAreImplicit) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "libgcc_s through linker script";
    archive("libs/libfakeunwind.a", {{"fakeunwind", "extern \"C\" void _Unwind_Resume(void*){}\n"
                                                    "extern \"C\" int _Unwind_RaiseException(void*){ return 0; }\n"}});
    write("main.cpp", "#include <iostream>\n#include <stdexcept>\n#include <string>\n"
                      "int main(int argc, char**){ std::string s(\"_exc_\"); try { if(argc > 0) throw std::runtime_error(s); }"
                      " catch(const std::exception& e){ std::cout << e.what() << std::endl; } }\n");
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_exc_")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libfakeunwind.a")) << r.diagnostic();
}

namespace {

const std::string SYSTEM_LIB_DIR = "/usr/lib/x86_64-linux-gnu";

bool systemLibraryInstalled(const std::string& header, const std::string& lib) {
    return std::filesystem::exists("/usr/include/" + header) && std::filesystem::exists(SYSTEM_LIB_DIR + "/" + lib);
}

}

TEST_F(LibSelectFixture, SystemMathLibraryIsFoundForCProgram) {
    if (!toolExists("gcc") || !systemLibraryInstalled("math.h", "libm.so.6")) GTEST_SKIP() << "libm";
    write("main.c", "#include <math.h>\n#include <stdio.h>\n"
                    "int main(int argc, char** argv){ printf(\"_cos_%.0f\\n\", cos(argc - 1.0) * 5); return 0; }\n");
    auto r = runBelder({"-I" + SYSTEM_LIB_DIR, "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("Linking lib: libm")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_cos_5")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SystemUuidLibrary) {
    if (!toolExists("g++") || !systemLibraryInstalled("uuid/uuid.h", "libuuid.so")) GTEST_SKIP() << "libuuid";
    write("main.cpp", "#include <uuid/uuid.h>\n#include <iostream>\n#include <string>\n"
                      "int main(){ uuid_t u; uuid_generate(u); char s[37]; uuid_unparse(u, s);"
                      " std::cout << \"_uuid_\" << std::string(s).size() << std::endl; }\n");
    auto r = runBelder({"-I" + SYSTEM_LIB_DIR, "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("Linking lib: libuuid.so")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_uuid_36")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SeveralSystemLibrariesAtOnce) {
    if (!toolExists("g++") || !systemLibraryInstalled("zlib.h", "libz.so") ||
        !systemLibraryInstalled("uuid/uuid.h", "libuuid.so") || !systemLibraryInstalled("expat.h", "libexpat.so"))
        GTEST_SKIP() << "zlib, libuuid or expat";
    write("main.cpp", "#include <zlib.h>\n#include <uuid/uuid.h>\n#include <expat.h>\n#include <iostream>\n"
                      "int main(){ uuid_t u; uuid_generate(u); XML_Parser p = XML_ParserCreate(nullptr); XML_ParserFree(p);"
                      " std::cout << \"_three_\" << compressBound(10) << std::endl; }\n");
    auto r = runBelder({"-I" + SYSTEM_LIB_DIR, "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_EQ(countOccurrences(r.stdout_str, "Linking lib:"), 3u) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_three_")) << r.diagnostic();
}

TEST_F(LibSelectFixture, SystemDirDoesNotAddLibrariesForStdThread) {
    if (!toolExists("g++") || !std::filesystem::exists(SYSTEM_LIB_DIR)) GTEST_SKIP() << "system lib dir";
    write("main.cpp", "#include <thread>\n#include <iostream>\n"
                      "int main(){ int x = 0; std::thread t([&]{ x = 5; }); t.join(); std::cout << \"_thr_\" << x << std::endl; }\n");
    auto r = runBelder({"-I" + SYSTEM_LIB_DIR, "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(r.hasOutput("Linking lib:")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_thr_5")) << r.diagnostic();
}

TEST_F(LibSelectFixture, ProjectLibraryWinsOverSystemLibraryOnTie) {
    if (!toolExists("g++") || !toolExists("ar") || !systemLibraryInstalled("zlib.h", "libz.so")) GTEST_SKIP() << "zlib";
    archive("libs/libmyz.a", {{"myz", "unsigned long compressBound(unsigned long n){ return n + 1; }\n"}});
    write("main.cpp", "#include <iostream>\nunsigned long compressBound(unsigned long);\n"
                      "int main(){ std::cout << \"_mine_\" << compressBound(5) << std::endl; }\n");
    auto r = runBelder({"-I" + SYSTEM_LIB_DIR, "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("Linking lib: libmyz.a")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libz")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_mine_6")) << r.diagnostic();
}

TEST_F(LibSelectFixture, ClangImplicitLibrariesAreRecognized) {
    if (!toolExists("clang") || !toolExists("clang++") || !toolExists("ar")) GTEST_SKIP() << "clang";
    archive("libs/libfakeio.a", {{"fakeio", "extern \"C\" int puts(const char*){ return 0; }\n"
                                            "extern \"C\" void _Unwind_Resume(void*){}\n"}});
    write("main.c", "#include <stdio.h>\nint main(void){ puts(\"_clang_c_\"); return 0; }\n");
    write("main2.cpp", "#include <iostream>\n#include <stdexcept>\n"
                       "int main(int argc, char**){ try { if(argc > 0) throw std::runtime_error(\"_clang_cpp_\"); }"
                       " catch(const std::exception& e){ std::cout << e.what() << std::endl; } }\n");
    auto r1 = runBelder({"main.c", "-o", "outc", "--CC", "clang", "run"});
    EXPECT_BELDER_OK(r1, r1.diagnostic());
    EXPECT_TRUE(r1.hasOutput("_clang_c_")) << r1.diagnostic();
    EXPECT_FALSE(r1.hasOutput("Linking lib:")) << r1.diagnostic();
    auto r2 = runBelder({"main2.cpp", "-o", "outcpp", "--CXX", "clang++", "run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_TRUE(r2.hasOutput("_clang_cpp_")) << r2.diagnostic();
    EXPECT_FALSE(r2.hasOutput("Linking lib:")) << r2.diagnostic();
}
