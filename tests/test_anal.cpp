#include "helpers.h"
#include <map>
#include <set>

namespace {

std::string firstLine(const std::string& file) {
    std::ifstream in(file);
    std::string line;
    std::getline(in, line);
    return line;
}

std::string secondLine(const std::string& file) {
    std::ifstream in(file);
    std::string line;
    std::getline(in, line);
    std::getline(in, line);
    return line;
}

std::string firstSubdirLib(const std::string& dir) {
    if (!std::filesystem::is_directory(dir)) return "";
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) break;
        if (it.depth() < 1 || !it->is_regular_file(ec)) continue;
        std::string name = it->path().filename().string();
        if (name.size() > 2 && name.compare(name.size() - 2, 2, ".a") == 0) return it->path().string();
    }
    return "";
}

}

class AnalFixture : public BelderFixture {
protected:
    std::string home;
    std::string srcDir;

    void SetUp() override {
        BelderFixture::SetUp();
        home = tmpDir + "_home";
        srcDir = tmpDir + "_analsrc";
        makeDir(home + "/builder");
        makeDir(srcDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(home);
        std::filesystem::remove_all(srcDir);
        BelderFixture::TearDown();
    }

    BelderResult belder(const std::vector<std::string>& args) {
        std::string cmd = "HOME='" + home + "' " + std::string(BELDER_BINARY) + " -C '" + tmpDir + "'";
        for (const auto& a : args) cmd += " '" + a + "'";
        return runCommandInDir(cmd, tmpDir);
    }

    std::string analDir() const { return home + "/builder/anal"; }

    std::map<std::string, std::string> symsIn(const std::string& dir, bool skipAnal) const {
        std::map<std::string, std::string> syms;
        if (!std::filesystem::is_directory(dir)) return syms;
        for (auto it = std::filesystem::recursive_directory_iterator(dir); it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (skipAnal && it->is_directory() && it->path().filename() == "anal") {
                it.disable_recursion_pending();
                continue;
            }
            if (it->is_regular_file() && it->path().extension() == ".sym")
                syms[firstLine(it->path().string())] = it->path().string();
        }
        return syms;
    }

    std::map<std::string, std::string> analSyms() const { return symsIn(analDir(), false); }
    std::map<std::string, std::string> projectSyms() const { return symsIn(home + "/builder", true); }

    bool cached(const std::string& file) const { return analSyms().count(file) != 0; }

    void compileTo(const std::string& rel, const std::string& code, const std::string& kind) {
        std::string src = srcDir + "/" + std::to_string(counter++) + ".c";
        std::string obj = src + ".o";
        writeFile(src, code);
        std::string out = (rel[0] == '/') ? rel : path(rel);
        std::filesystem::create_directories(std::filesystem::path(out).parent_path());
        std::string cmd;
        if (kind == "o") cmd = "gcc -c '" + src + "' -o '" + out + "'";
        else if (kind == "a") cmd = "gcc -c '" + src + "' -o '" + obj + "' && rm -f '" + out + "' && ar rcs '" + out + "' '" + obj + "'";
        else cmd = "gcc -shared -fPIC '" + src + "' -o '" + out + "'";
        auto r = runCommand(cmd);
        ASSERT_EQ(r.exitCode, 0) << r.diagnostic();
    }

    int counter = 0;
};

TEST_F(AnalFixture, AbsolutePathCachesLibrariesAndObjects) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    compileTo("libs/libtwo.so", "int two(void){ return 2; }\n", "so");
    compileTo("libs/three.o", "int three(void){ return 3; }\n", "o");
    auto r = belder({"anal", path("libs")});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("files: 3, analyzed: 3, up to date: 0")) << r.diagnostic();
    EXPECT_TRUE(cached(path("libs/libone.a")));
    EXPECT_TRUE(cached(path("libs/libtwo.so")));
    EXPECT_TRUE(cached(path("libs/three.o")));
    std::string sym = readFile(analSyms()[path("libs/libone.a")]);
    EXPECT_NE(sym.find("one"), std::string::npos) << sym;
}

TEST_F(AnalFixture, RelativePathIsResolvedFromC) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    auto r = belder({"anal", "libs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(cached(path("libs/libone.a"))) << r.diagnostic();
}

TEST_F(AnalFixture, RelativePathFromCurrentDirectory) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    auto r = runCommandInDir("HOME='" + home + "' " + std::string(BELDER_BINARY) + " anal ./libs/../libs", tmpDir);
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(cached(path("libs/libone.a"))) << r.diagnostic();
}

TEST_F(AnalFixture, PathAnalysisIsRecursiveAndSkipsGit) {
    compileTo("libs/a/b/c/libdeep.a", "int deep(void){ return 1; }\n", "a");
    compileTo("libs/.git/libhidden.a", "int hidden(void){ return 1; }\n", "a");
    auto r = belder({"anal", "libs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(cached(path("libs/a/b/c/libdeep.a"))) << r.diagnostic();
    EXPECT_FALSE(cached(path("libs/.git/libhidden.a"))) << r.diagnostic();
}

TEST_F(AnalFixture, SeveralPathsInOneCommand) {
    compileTo("x/libx.a", "int xx(void){ return 1; }\n", "a");
    compileTo("y/liby.a", "int yy(void){ return 1; }\n", "a");
    auto r = belder({"anal", "x", path("y")});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(cached(path("x/libx.a")));
    EXPECT_TRUE(cached(path("y/liby.a")));
}

TEST_F(AnalFixture, SingleLibraryFile) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    compileTo("libs/libother.a", "int other(void){ return 1; }\n", "a");
    auto r = belder({"anal", "libs/libone.a"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(cached(path("libs/libone.a")));
    EXPECT_FALSE(cached(path("libs/libother.a")));
}

TEST_F(AnalFixture, EmptyFolder) {
    makeDir(path("empty"));
    auto r = belder({"anal", "empty"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("files: 0")) << r.diagnostic();
}

TEST_F(AnalFixture, MissingPathIsAnError) {
    auto r = belder({"anal", "no_such_folder_xyz"});
    EXPECT_EQ(r.exitCode, 1) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("no_such_folder_xyz is not an existing path or a compiler from PATH")) << r.diagnostic();
    EXPECT_TRUE(analSyms().empty());
}

TEST_F(AnalFixture, ErrorDoesNotAnalyzeOtherArguments) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    auto r = belder({"anal", "libs", "no_such_folder_xyz"});
    EXPECT_EQ(r.exitCode, 1) << r.diagnostic();
    EXPECT_FALSE(cached(path("libs/libone.a")));
}

TEST_F(AnalFixture, NotALibraryFileIsAnError) {
    write("notes.txt", "hello\n");
    auto r = belder({"anal", "notes.txt"});
    EXPECT_EQ(r.exitCode, 1) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("is not a folder, a library, an object file or a compiler")) << r.diagnostic();
}

TEST_F(AnalFixture, NoArgumentsIsAnError) {
    auto r = belder({"anal"});
    EXPECT_EQ(r.exitCode, 1) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("Nothing to analyze")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("belder anal <compiler> [compiler flags]")) << r.diagnostic();
}

TEST_F(AnalFixture, FlagWithoutCompilerIsAnError) {
    makeDir(path("libs"));
    auto r = belder({"anal", "libs", "-m32"});
    EXPECT_EQ(r.exitCode, 1) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("Flag -m32 is not after a compiler")) << r.diagnostic();
}

TEST_F(AnalFixture, SecondRunIsUpToDate) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    compileTo("libs/libtwo.a", "int two(void){ return 2; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", "libs"}), "");
    auto r = belder({"anal", "libs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("files: 2, analyzed: 0, up to date: 2")) << r.diagnostic();
}

TEST_F(AnalFixture, ChangedLibraryIsAnalyzedAgain) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    compileTo("libs/libtwo.a", "int two(void){ return 2; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", "libs"}), "");
    compileTo("libs/libone.a", "int one_changed(void){ return 1; }\n", "a");
    auto r = belder({"anal", "libs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("files: 2, analyzed: 1, up to date: 1")) << r.diagnostic();
    EXPECT_NE(readFile(analSyms()[path("libs/libone.a")]).find("one_changed"), std::string::npos);
}

TEST_F(AnalFixture, SymlinksToOneLibraryAreAllCached) {
    compileTo("libs/libz9.so.1.2", "int z9(void){ return 9; }\n", "so");
    std::filesystem::create_symlink("libz9.so.1.2", path("libs/libz9.so.1"));
    std::filesystem::create_symlink("libz9.so.1", path("libs/libz9.so"));
    auto r = belder({"anal", "libs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    for (const char* name : {"libs/libz9.so.1.2", "libs/libz9.so.1", "libs/libz9.so"}) {
        ASSERT_TRUE(cached(path(name))) << name;
        EXPECT_NE(readFile(analSyms()[path(name)]).find("z9"), std::string::npos) << name;
    }
}

TEST_F(AnalFixture, ClearAndCleanRemoveCache) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    for (const char* word : {"clear", "clean"}) {
        ASSERT_BELDER_OK(belder({"anal", "libs"}), "");
        ASSERT_FALSE(analSyms().empty());
        auto r = belder({"anal", word});
        EXPECT_BELDER_OK(r, r.diagnostic());
        EXPECT_TRUE(r.hasOutput("has been cleared")) << r.diagnostic();
        EXPECT_TRUE(analSyms().empty()) << word;
    }
}

TEST_F(AnalFixture, ClearOnMissingCache) {
    auto r = belder({"anal", "clear"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(std::filesystem::is_directory(analDir()));
}

TEST_F(AnalFixture, ClearDoesNotRemoveProjectBuild) {
    write("main.c", "int main(void){ return 0; }\n");
    ASSERT_BELDER_OK(belder({}), "");
    auto r = belder({"anal", "clear"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    auto again = belder({});
    EXPECT_BELDER_OK(again, again.diagnostic());
    EXPECT_FALSE(again.hasOutput("Compiling")) << again.diagnostic();
}

TEST_F(AnalFixture, ClearThenAnalyzeInOneCommand) {
    compileTo("x/libx.a", "int xx(void){ return 1; }\n", "a");
    compileTo("y/liby.a", "int yy(void){ return 1; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", "x"}), "");
    auto r = belder({"anal", "clear", "y"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(cached(path("x/libx.a")));
    EXPECT_TRUE(cached(path("y/liby.a")));
}

TEST_F(AnalFixture, FolderNamedClearIsAnalyzedWithDotSlash) {
    compileTo("clear/libc1.a", "int c1(void){ return 1; }\n", "a");
    compileTo("y/liby.a", "int yy(void){ return 1; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", "y"}), "");
    auto r = belder({"anal", "./clear"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(cached(path("clear/libc1.a")));
    EXPECT_TRUE(cached(path("y/liby.a")));
}

TEST_F(AnalFixture, FolderWithCompilerNameIsAnalyzedAsFolder) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc";
    compileTo("gcc/libg.a", "int gg(void){ return 1; }\n", "a");
    auto r = belder({"anal", "gcc"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("files: 1,")) << r.diagnostic();
    EXPECT_TRUE(cached(path("gcc/libg.a")));
}

TEST_F(AnalFixture, FirstBuildKeepsCacheMadeBeforeIt) {
    compileTo(srcDir + "/ext/libext.a", "int ext_func(void){ return 5; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", srcDir + "/ext"}), "");
    ASSERT_FALSE(std::filesystem::exists(home + "/builder/config"));
    write("main.c", "int main(void){ return 0; }\n");
    ASSERT_BELDER_OK(belder({}), "");
    EXPECT_TRUE(cached(srcDir + "/ext/libext.a"));
}

TEST_F(AnalFixture, AnalDoesNotTouchCurrentProject) {
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", "libs"}), "");
    std::set<std::string> entries;
    for (const auto& e : std::filesystem::directory_iterator(home + "/builder")) entries.insert(e.path().filename().string());
    EXPECT_EQ(entries, (std::set<std::string>{"anal"}));
}

TEST_F(AnalFixture, CompilerAnalysisIsNotRecursive) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc";
    auto r = belder({"anal", "gcc"});
    ASSERT_BELDER_OK(r, r.diagnostic());
    auto syms = analSyms();
    ASSERT_FALSE(syms.empty());
    bool crt = false;
    for (const auto& s : syms) crt |= (std::filesystem::path(s.first).filename().string().find("crt1.o") != std::string::npos);
    EXPECT_TRUE(crt) << "implicit inputs of the linker must be cached";
    for (const char* dir : {"/usr/lib/x86_64-linux-gnu", "/usr/lib"}) {
        std::string deep = firstSubdirLib(dir);
        if (deep.empty()) continue;
        std::string folder = std::filesystem::path(deep).parent_path().string();
        bool searched = false;
        for (const auto& s : syms) searched |= (std::filesystem::path(s.first).parent_path().string() == folder);
        EXPECT_FALSE(searched) << "belder went into subfolder " << folder;
    }
}

TEST_F(AnalFixture, CompilerFromPathAndByFullPath) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++";
    auto r = belder({"anal", "g++"});
    ASSERT_BELDER_OK(r, r.diagnostic());
    bool stdcpp = false;
    for (const auto& s : analSyms()) stdcpp |= (std::filesystem::path(s.first).filename().string().rfind("libstdc++", 0) == 0);
    EXPECT_TRUE(stdcpp);
    auto which = runCommand("command -v g++");
    std::string full = which.stdout_str.substr(0, which.stdout_str.find('\n'));
    auto again = belder({"anal", full});
    EXPECT_BELDER_OK(again, again.diagnostic());
    EXPECT_TRUE(again.hasOutput("analyzed: 0,")) << again.diagnostic();
}

TEST_F(AnalFixture, NotACompilerIsAnError) {
    if (!toolExists("true")) GTEST_SKIP() << "true";
    auto r = belder({"anal", "true"});
    EXPECT_EQ(r.exitCode, 1) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("Cannot get linker search folders from: true")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("probe")) << r.diagnostic();
}

TEST_F(AnalFixture, ProbeFileIsRemoved) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc";
    ASSERT_BELDER_OK(belder({"anal", "gcc"}), "");
    for (const auto& e : std::filesystem::directory_iterator(analDir()))
        EXPECT_EQ(e.path().extension(), ".sym") << e.path();
}

TEST_F(AnalFixture, ClangNeedsExistingProbe) {
    if (!toolExists("clang")) GTEST_SKIP() << "clang";
    auto r = belder({"anal", "clang"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(analSyms().empty()) << r.diagnostic();
}

TEST_F(AnalFixture, CompilerFlagsSelectMultilib) {
    if (!toolExists("arm-none-eabi-gcc") || !std::filesystem::exists("/usr/lib/arm-none-eabi"))
        GTEST_SKIP() << "arm-none-eabi toolchain";
    auto r = belder({"anal", "arm-none-eabi-gcc", "-mcpu=cortex-m4", "-mthumb"});
    ASSERT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("belder anal: arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb")) << r.diagnostic();
    bool nofp = false, plain = false;
    for (const auto& s : analSyms()) {
        nofp |= (s.first.find("thumb/v7e-m/nofp/libm.a") != std::string::npos);
        plain |= (s.first == "/usr/lib/arm-none-eabi/lib/libm.a");
    }
    EXPECT_TRUE(nofp);
    EXPECT_FALSE(plain) << "only one configuration must be analyzed";
}

TEST_F(AnalFixture, SeveralCompilersAndPathsInOneCommand) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc";
    compileTo("libs/libone.a", "int one(void){ return 1; }\n", "a");
    auto r = belder({"anal", "gcc", "-O2", "libs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("belder anal: gcc -O2")) << r.diagnostic();
    EXPECT_TRUE(cached(path("libs/libone.a")));
}

TEST_F(AnalFixture, SystemLibraryIsNotFoundWithoutAnal) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    write("main.c", "#include <stdio.h>\n#include <math.h>\n#include <zlib.h>\n"
                    "int main(void){ volatile double x = 1.0; printf(\"_%.0f_%d_\\n\", sin(x) * 100, zlibVersion()[0] != 0); return 0; }\n");
    auto r = belder({"run"});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("belder did not find these symbols")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("zlibVersion")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("sin")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libz")) << r.diagnostic();
}

TEST_F(AnalFixture, LibmIsNotFoundWithoutAnal) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc";
    write("main.c", "#include <stdio.h>\n#include <math.h>\n"
                    "int main(void){ volatile double x = 2.0; printf(\"_%.0f_\\n\", sqrt(x) * 1000); return 0; }\n");
    auto r = belder({"run"});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("sqrt")) << r.diagnostic();
    ASSERT_BELDER_OK(belder({"anal", "gcc"}), "");
    auto after = belder({"run"});
    EXPECT_BELDER_OK(after, after.diagnostic());
    EXPECT_TRUE(after.hasOutput("Linking lib: libm")) << after.diagnostic();
    EXPECT_TRUE(after.hasOutput("_1414_")) << after.diagnostic();
}

TEST_F(AnalFixture, SystemLibraryIsFoundAfterAnal) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    ASSERT_BELDER_OK(belder({"anal", "gcc"}), "");
    write("main.c", "#include <stdio.h>\n#include <math.h>\n#include <zlib.h>\n"
                    "int main(void){ volatile double x = 1.0; printf(\"_%.0f_%d_\\n\", sin(x) * 100, zlibVersion()[0] != 0); return 0; }\n");
    auto r = belder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_84_1_")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("Linking lib: libz")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("Linking lib: libm")) << r.diagnostic();
}

TEST_F(AnalFixture, AnalOfOneSystemLibraryFindsOnlyIt) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    auto which = runCommand("gcc -print-file-name=libz.so");
    std::string libz = which.stdout_str.substr(0, which.stdout_str.find('\n'));
    if (libz.empty() || libz[0] != '/') GTEST_SKIP() << "libz.so";
    libz = std::filesystem::canonical(std::filesystem::path(libz).parent_path()).string() + "/libz.so";
    ASSERT_BELDER_OK(belder({"anal", libz}), "");
    write("main.c", "#include <stdio.h>\n#include <zlib.h>\nint main(void){ printf(\"_%d_\\n\", zlibVersion()[0] != 0); return 0; }\n");
    auto r = belder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_1_")) << r.diagnostic();
    write("main.c", "#include <stdio.h>\n#include <math.h>\n#include <zlib.h>\n"
                    "int main(void){ volatile double x = 1.0; printf(\"_%.0f_%d_\\n\", sin(x) * 100, zlibVersion()[0] != 0); return 0; }\n");
    auto m = belder({"run"});
    EXPECT_EQ(m.exitCode, 3) << m.diagnostic();
    EXPECT_TRUE(m.hasOutput("sin")) << m.diagnostic();
    EXPECT_FALSE(m.hasOutput("zlibVersion")) << m.diagnostic();
}

TEST_F(AnalFixture, BuildDoesNotFillSharedCache) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    write("main.c", "#include <zlib.h>\nint main(void){ return zlibVersion()[0] == 0; }\n");
    auto first = belder({});
    EXPECT_EQ(first.exitCode, 3) << first.diagnostic();
    EXPECT_TRUE(analSyms().empty());
    auto second = belder({});
    EXPECT_EQ(second.exitCode, 3) << second.diagnostic();
}

TEST_F(AnalFixture, SystemLibraryFromIWorksWithoutAnal) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    auto which = runCommand("gcc -print-file-name=libz.so");
    std::string libz = which.stdout_str.substr(0, which.stdout_str.find('\n'));
    if (libz.empty() || libz[0] != '/') GTEST_SKIP() << "libz.so";
    write("main.c", "#include <zlib.h>\nint main(void){ return zlibVersion()[0] == 0; }\n");
    auto r = belder({"-I" + std::filesystem::canonical(std::filesystem::path(libz).parent_path()).string()});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("Linking lib: libz")) << r.diagnostic();
    EXPECT_TRUE(analSyms().empty());
}

TEST_F(AnalFixture, BuildAfterAnalReusesCache) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    ASSERT_BELDER_OK(belder({"anal", "gcc"}), "");
    std::map<std::string, std::string> times;
    for (const auto& s : analSyms())
        if (std::filesystem::path(s.first).filename().string().rfind("libz.so", 0) == 0) times[s.second] = secondLine(s.second) + std::to_string(std::filesystem::last_write_time(s.second).time_since_epoch().count());
    write("main.c", "#include <zlib.h>\nint main(void){ return zlibVersion()[0] == 0; }\n");
    auto r = belder({});
    ASSERT_BELDER_OK(r, r.diagnostic());
    ASSERT_FALSE(times.empty());
    for (const auto& s : analSyms())
        if (times.count(s.second))
            EXPECT_EQ(times[s.second], secondLine(s.second) + std::to_string(std::filesystem::last_write_time(s.second).time_since_epoch().count())) << s.second;
}

TEST_F(AnalFixture, AnalClearForgetsSystemLibraries) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    write("main.c", "#include <zlib.h>\nint main(void){ return zlibVersion()[0] == 0; }\n");
    ASSERT_BELDER_OK(belder({"anal", "gcc"}), "");
    auto warm = belder({});
    ASSERT_BELDER_OK(warm, warm.diagnostic());
    EXPECT_TRUE(warm.hasOutput("Linking lib: libz")) << warm.diagnostic();
    ASSERT_BELDER_OK(belder({"anal", "clear"}), "");
    auto cold = belder({"-rel"});
    EXPECT_EQ(cold.exitCode, 3) << cold.diagnostic();
    EXPECT_TRUE(cold.hasOutput("zlibVersion")) << cold.diagnostic();
}

TEST_F(AnalFixture, AnalyzedFolderIsNotUsedWithoutI) {
    compileTo(srcDir + "/ext/libext.a", "int ext_func(void){ return 5; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", srcDir + "/ext"}), "");
    write("main.c", "int ext_func(void);\nint main(void){ return ext_func(); }\n");
    auto r = belder({});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    auto withI = belder({"-I" + srcDir + "/ext"});
    EXPECT_BELDER_OK(withI, withI.diagnostic());
}

TEST_F(AnalFixture, ProjectLibraryBeatsSystemLibrary) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    ASSERT_BELDER_OK(belder({"anal", "gcc"}), "");
    compileTo("mylibs/libfakez.a", "const char* zlibVersion(void){ return \"fake\"; }\n", "a");
    write("main.c", "#include <stdio.h>\nconst char* zlibVersion(void);\nint main(void){ printf(\"_%s_\\n\", zlibVersion()); return 0; }\n");
    auto r = belder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_fake_")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libz")) << r.diagnostic();
}

TEST_F(AnalFixture, NoStdlibDoesNotUseLinkerFolders) {
    if (!toolExists("gcc") || !std::filesystem::exists("/usr/include/zlib.h")) GTEST_SKIP() << "zlib";
    ASSERT_BELDER_OK(belder({"anal", "gcc"}), "");
    write("main.c", "const char* zlibVersion(void);\nvoid _start(void){ zlibVersion(); for(;;); }\n");
    auto r = belder({"--link-flags", "-nostdlib"});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("Linking lib: libz")) << r.diagnostic();
}

TEST_F(AnalFixture, StaleCachedLibraryIsReanalyzedDuringBuild) {
    compileTo(srcDir + "/ext/libext.a", "int ext_old(void){ return 5; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", srcDir + "/ext"}), "");
    compileTo(srcDir + "/ext/libext.a", "int ext_new(void){ return 6; }\n", "a");
    write("main.c", "int ext_new(void);\nint main(void){ return ext_new() != 6; }\n");
    auto r = belder({"-I" + srcDir + "/ext", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    std::string sym = readFile(analSyms()[srcDir + "/ext/libext.a"]);
    EXPECT_NE(sym.find("ext_new"), std::string::npos) << sym;
    EXPECT_TRUE(projectSyms().count(srcDir + "/ext/libext.a") == 0);
}

TEST_F(AnalFixture, BrokenCachedFileDoesNotBreakBuild) {
    compileTo(srcDir + "/ext/libext.a", "int ext_func(void){ return 5; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", srcDir + "/ext"}), "");
    std::string symFile = analSyms()[srcDir + "/ext/libext.a"];
    std::string lines = firstLine(symFile) + "\n" + secondLine(symFile) + "\ngarbage\n";
    writeFile(symFile, lines);
    write("main.c", "int ext_func(void);\nint main(void){ return ext_func() != 5; }\n");
    auto r = belder({"-I" + srcDir + "/ext", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_NE(readFile(symFile).find("ext_func"), std::string::npos);
}

TEST_F(AnalFixture, RemovedCachedLibraryDoesNotBreakBuild) {
    compileTo(srcDir + "/ext/libgone.a", "int gone(void){ return 5; }\n", "a");
    compileTo(srcDir + "/ext/libstay.a", "int stay(void){ return 0; }\n", "a");
    ASSERT_BELDER_OK(belder({"anal", srcDir + "/ext"}), "");
    std::filesystem::remove(srcDir + "/ext/libgone.a");
    write("main.c", "int stay(void);\nint main(void){ return stay(); }\n");
    auto r = belder({"-I" + srcDir + "/ext", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
}

TEST_F(AnalFixture, ParallelAnalOfOneCompiler) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc";
    std::string cmd = "HOME='" + home + "' " + std::string(BELDER_BINARY) + " anal gcc";
    auto r = runCommand("(" + cmd + " > /dev/null) & (" + cmd + " > /dev/null) & (" + cmd + " > /dev/null) & wait");
    EXPECT_EQ(r.exitCode, 0) << r.diagnostic();
    auto again = belder({"anal", "gcc"});
    EXPECT_BELDER_OK(again, again.diagnostic());
    EXPECT_TRUE(again.hasOutput("analyzed: 0,")) << again.diagnostic();
    for (const auto& e : std::filesystem::directory_iterator(analDir()))
        EXPECT_EQ(e.path().extension(), ".sym") << e.path();
}

TEST_F(AnalFixture, HelpShowsAnal) {
    auto r = belder({"help"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("anal [path] [path] [path]")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("anal [compiler] [compiler flags]")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("anal clear, anal clean")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("without -I")) << r.diagnostic();
}
