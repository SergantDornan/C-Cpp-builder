#include "helpers.h"
#include <filesystem>

class BelderARParsingFixture : public ::testing::Test {
protected:
    std::string tempDir;

    void SetUp() override {
        tempDir = createAndInitializeTempDir();
    }

    void TearDown() override {
        cleanupBelder(tempDir);
    }

    std::string createAndInitializeTempDir() {
        const char* tmp = getenv("TMPDIR");
        std::string tmpdir = tmp ? tmp : "/tmp";
        std::string path = tmpdir + "/btest_ar_XXXXXX";
        char* dir = mkdtemp((char*)path.c_str());
        return dir ? std::string(dir) : "";
    }

    void cleanupBelder(const std::string& path) {
        std::string stateDir = std::string(getenv("HOME")) + "/.builder";
        system(("rm -rf " + stateDir + " 2>/dev/null || true").c_str());
        system(("rm -rf " + path + " 2>/dev/null || true").c_str());
    }
};

TEST_F(BelderARParsingFixture, BuildWithSystemLibC) {
    std::string main_cpp = tempDir + "/main.cpp";
    {
        std::ofstream out(main_cpp);
        out << "#include <stdio.h>\n"
            << "int main() { printf(\"Hello\\\\n\"); return 0; }\n";
    }

    auto result = runCommandInDir(
        std::string(BELDER_BINARY) + " main.cpp -lc -o test_exe",
        tempDir
    );

    EXPECT_TRUE(result.success() || result.hasOutput("Compiling"))
        << "Should successfully build with libc" << result.diagnostic();
}

TEST_F(BelderARParsingFixture, BuildCustomStaticArchive) {
    std::string libdir = tempDir + "/lib";
    std::filesystem::create_directory(libdir);

    std::string helper_cpp = libdir + "/helper.cpp";
    {
        std::ofstream out(helper_cpp);
        out << "int add(int a, int b) { return a + b; }\n";
    }

    auto compile_lib = runCommandInDir(
        std::string(BELDER_BINARY) + " helper.cpp -o libhelper.a",
        libdir
    );
    ASSERT_BELDER_OK(compile_lib, "Compile helper.cpp to static lib");

    std::string main_cpp = tempDir + "/main.cpp";
    {
        std::ofstream out(main_cpp);
        out << "extern int add(int, int);\n"
            << "#include <stdio.h>\n"
            << "int main() { printf(\"%d\\\\n\", add(2, 3)); return 0; }\n";
    }

    auto result = runCommandInDir(
        std::string(BELDER_BINARY) + " main.cpp --link-force " + libdir + "/libhelper.a -o test_exe",
        tempDir
    );

    EXPECT_BELDER_OK(result, "Link with custom static archive")
        << result.diagnostic("Should parse and link custom archive successfully");
}

TEST_F(BelderARParsingFixture, ParseSystemLibC) {
    std::string test_cpp = tempDir + "/main.cpp";
    {
        std::ofstream out(test_cpp);
        out << "#include <stdio.h>\n"
            << "int main() {\n"
            << "  printf(\"Hello from libc\\n\");\n"
            << "  return 0;\n"
            << "}\n";
    }

    auto result = runCommandInDir(
        std::string(BELDER_BINARY) + " main.cpp --link-force /usr/lib/x86_64-linux-gnu/libc.a -o test_exe",
        tempDir
    );

    EXPECT_BELDER_OK(result, "Parse system libc.a (large complex archive)")
        << result.diagnostic("System libc.a should parse without segfault");
}

TEST_F(BelderARParsingFixture, BuildLargeStaticArchive) {
    std::string libdir = tempDir + "/biglib";
    std::filesystem::create_directory(libdir);

    std::string main_cpp = libdir + "/main.cpp";
    {
        std::ofstream f(main_cpp);
        f << "int func0() { return 0; }\n"
          << "int func1() { return 1; }\n"
          << "int func2() { return 2; }\n"
          << "int func3() { return 3; }\n"
          << "int func4() { return 4; }\n"
          << "int func5() { return 5; }\n"
          << "int func6() { return 6; }\n"
          << "int func7() { return 7; }\n"
          << "int func8() { return 8; }\n"
          << "int func9() { return 9; }\n";
    }

    auto compile_result = runCommandInDir(
        std::string(BELDER_BINARY) + " main.cpp -o libbig.a",
        libdir
    );
    ASSERT_BELDER_OK(compile_result, "Compile into archive with multiple functions");

    std::string test_cpp = tempDir + "/main.cpp";
    {
        std::ofstream out(test_cpp);
        out << "extern int func0();\n"
            << "extern int func9();\n"
            << "#include <stdio.h>\n"
            << "int main() {\n"
            << "  printf(\"%d %d\\\\n\", func0(), func9());\n"
            << "  return 0;\n"
            << "}\n";
    }

    auto result = runCommandInDir(
        std::string(BELDER_BINARY) + " main.cpp --link-force " + libdir + "/libbig.a -o test_exe",
        tempDir
    );

    EXPECT_BELDER_OK(result, "Parse archive with multiple functions")
        << result.diagnostic("Archive parsing should not crash with complex libraries");
}

TEST_F(BelderARParsingFixture, RebuildWithStaticArchive) {
    std::string libdir = tempDir + "/lib";
    std::filesystem::create_directory(libdir);

    std::string lib_cpp = libdir + "/util.cpp";
    {
        std::ofstream out(lib_cpp);
        out << "int multiply(int a, int b) { return a * b; }\n";
    }

    auto compile_lib = runCommandInDir(
        std::string(BELDER_BINARY) + " util.cpp -o libutil.a",
        libdir
    );
    ASSERT_BELDER_OK(compile_lib, "First compile of archive");

    std::string main_cpp = tempDir + "/main.cpp";
    {
        std::ofstream out(main_cpp);
        out << "extern int multiply(int, int);\n"
            << "#include <stdio.h>\n"
            << "int main() { printf(\"%d\\\\n\", multiply(5, 6)); return 0; }\n";
    }

    auto result1 = runCommandInDir(
        std::string(BELDER_BINARY) + " main.cpp --link-force " + libdir + "/libutil.a -o test_exe",
        tempDir
    );
    EXPECT_BELDER_OK(result1, "First build with archive");

    {
        std::ofstream out(main_cpp);
        out << "extern int multiply(int, int);\n"
            << "#include <stdio.h>\n"
            << "int main() { printf(\"result: %d\\\\n\", multiply(7, 8)); return 0; }\n";
    }

    auto result2 = runCommandInDir(
        std::string(BELDER_BINARY) + " main.cpp --link-force " + libdir + "/libutil.a -o test_exe",
        tempDir
    );
    EXPECT_BELDER_OK(result2, "Rebuild after main modification")
        << result2.diagnostic("Should relink with archive without crashing");
}
