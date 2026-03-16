// tests/test_basic.cpp
// Tests for: -log format, -C flag, basic C/C++/ASM builds,
// link set selection, library builds
#include "helpers.h"

// -----------------------------------------------------------------------
// -log flag format
// -----------------------------------------------------------------------
TEST_F(BelderFixture, LogFlagShowsCompilerCommands) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"--rebuild", "-log"});
    EXPECT_BELDER_OK(r, "-log flag: build with logging should succeed");
    // -log should show the actual g++ compilation command
    EXPECT_TRUE(r.hasOutput("g++")) << r.diagnostic("Expected g++ compiler command in -log output");
    EXPECT_TRUE(r.hasOutput("-c")) << r.diagnostic("Expected '-c' compile flag in -log output");
    EXPECT_TRUE(r.hasOutput("-o")) << r.diagnostic("Expected '-o' flag in -log output");
}

TEST_F(BelderFixture, LogFlagShowsLinkCommand) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"--rebuild", "-log"});
    EXPECT_BELDER_OK(r, "-log flag: link step should show object files");
    EXPECT_TRUE(r.hasOutput(".o")) << r.diagnostic("Expected .o files listed in link command in -log output");
}

TEST_F(BelderFixture, LogFlagNotSetShowsNicePrint) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"--rebuild"});
    EXPECT_BELDER_OK(r, "without -log: build output should show human-readable progress");
    // Without -log, belder prints "Compiling X" not the full command
    EXPECT_TRUE(r.hasOutput("Compiling")) << r.diagnostic("Expected 'Compiling' in human-readable output");
    EXPECT_TRUE(r.hasOutput("Linking file")) << r.diagnostic("Expected 'Linking file' in human-readable output");
    EXPECT_TRUE(r.hasOutput("SUCCESS")) << r.diagnostic("Expected 'SUCCESS' in human-readable output");
}

// -----------------------------------------------------------------------
// -C flag tests
// -----------------------------------------------------------------------
TEST_F(BelderFixture, CFlagAbsolutePathBuildsSuccessfully) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // Run from a different directory using -C with absolute path
    BelderResult r = runBelderFrom("/tmp", {"-C", tmpDir});
    EXPECT_BELDER_OK(r, "-C absolute path: build should succeed when running from /tmp with -C " + tmpDir);
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output file should exist in project dir after -C build");
}

TEST_F(BelderFixture, CFlagNonExistentAbsolutePathErrors) {
    BelderResult r = runBelderFrom("/tmp", {"-C", "/tmp/belder_nonexistent_dir_xyz"});
    EXPECT_NE(r.exitCode, 0) << r.diagnostic("-C with non-existent absolute path should fail");
    EXPECT_TRUE(r.hasOutput("does not exist")) << r.diagnostic("Error output should mention 'does not exist'");
}

TEST_F(BelderFixture, CFlagRelativePath) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // Run from the parent of tmpDir using a relative path
    std::string parent = std::filesystem::path(tmpDir).parent_path().string();
    std::string relName = std::filesystem::path(tmpDir).filename().string();

    BelderResult r = runBelderFrom(parent, {"-C", relName});
    EXPECT_BELDER_OK(r, "-C relative path: build from parent dir with relative project path");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output file should exist after -C relative path build");
}

TEST_F(BelderFixture, CFlagNonExistentRelativePathErrors) {
    BelderResult r = runBelderFrom("/tmp", {"-C", "belder_nonexistent_relative_xyz"});
    EXPECT_NE(r.exitCode, 0) << r.diagnostic("-C with non-existent relative path should fail");
    EXPECT_TRUE(r.hasOutput("does not exist")) << r.diagnostic("Error output should mention 'does not exist'");
}

// -----------------------------------------------------------------------
// Basic language builds
// -----------------------------------------------------------------------
TEST_F(BelderFixture, BasicCppBuild) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "basic C++ build: single main.cpp should compile and link");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output binary 'out' should exist after C++ build");

    // The binary should run
    auto run_r = runCommand(tmpDir + "/out");
    EXPECT_EQ(run_r.exitCode, 0) << run_r.diagnostic("Compiled C++ binary should execute successfully");
}

TEST_F(BelderFixture, BasicCBuild) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc not found";
    write("main.c", simpleCMain());

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "basic C build: single main.c should compile and link");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output binary 'out' should exist after C build");
}

TEST_F(BelderFixture, MixCAndCpp) {
    if (!toolExists("g++") || !toolExists("gcc")) GTEST_SKIP() << "g++/gcc not found";
    write("util.c", "int util_func(void){return 7;}\n");
    write("main.cpp",
          "extern \"C\" int util_func();\n"
          "#include <iostream>\n"
          "int main(){std::cout<<util_func()<<std::endl;return 0;}\n");

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "mixed C+C++ build: util.c + main.cpp should compile and link");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output binary 'out' should exist after C+C++ mix build");
}

TEST_F(BelderFixture, MixCppAndAsm) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("helper.s",
          ".global asm_add\n"
          ".type asm_add, @function\n"
          "asm_add:\n"
          "  mov %rdi,%rax\n"
          "  add %rsi,%rax\n"
          "  ret\n");
    write("main.cpp",
          "extern \"C\" long asm_add(long,long);\n"
          "#include <iostream>\n"
          "int main(){std::cout<<asm_add(3,4)<<std::endl;return 0;}\n");

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "mixed C++/ASM build: helper.s + main.cpp should compile and link");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output binary 'out' should exist after C++/ASM mix build");
}

TEST_F(BelderFixture, MixCAndAsm) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc not found";
    write("helper.s",
          ".global asm_val\n"
          ".type asm_val, @function\n"
          "asm_val:\n"
          "  mov $99,%eax\n"
          "  ret\n");
    write("main.c",
          "extern int asm_val(void);\n"
          "#include <stdio.h>\n"
          "int main(){printf(\"%d\\n\",asm_val());return 0;}\n");

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "mixed C/ASM build: helper.s + main.c should compile and link");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output binary 'out' should exist after C/ASM mix build");
}

TEST_F(BelderFixture, BuildOnlyAsm) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ assembler not found";
    // Write a self-contained ASM program with a main label
    write("main.s",
          ".global main\n"
          ".type main, @function\n"
          "main:\n"
          "  xor %rax,%rax\n"
          "  ret\n");

    auto r = runBelder();
    // Belder requires a C/C++ entry file; pure ASM projects
    // fail with "Cannot find entry file"
    EXPECT_EQ(r.exitCode, 1) << "Pure ASM project should fail (no C/C++ entry file)";
    EXPECT_TRUE(r.hasOutput("Cannot find entry file") ||
                r.hasOutput("entry") || r.hasOutput("ERROR"))
        << "Should report missing entry file: " << r.combined();
}

// -----------------------------------------------------------------------
// Link only needed files
// -----------------------------------------------------------------------
TEST_F(BelderFixture, OnlyNeededFilesLinked) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // foo is called by main, bar is NOT called
    write("foo.cpp", "int foo(){return 1;}\n");
    write("bar.cpp", "int bar(){return 2;}\n");
    write("main.cpp",
          "int foo();\n"
          "#include <iostream>\n"
          "int main(){std::cout<<foo()<<std::endl;return 0;}\n");

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "link selection: only needed files linked (foo called, bar not called)");
    // bar should NOT appear in the linking output
    EXPECT_TRUE(r.stdout_str.find("Linking file: bar.cpp") == std::string::npos)
        << r.diagnostic("bar.cpp should NOT be linked (it defines unused symbol)");
    // foo should appear
    EXPECT_TRUE(r.stdout_str.find("Linking file: foo.cpp") != std::string::npos)
        << r.diagnostic("foo.cpp should be linked (it defines a symbol used by main)");
}

TEST_F(BelderFixture, OnlyNeededAsmFilesLinked) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("asm_used.s",
          ".global asm_used\n"
          ".type asm_used, @function\n"
          "asm_used:\n"
          "  mov $1,%eax\n"
          "  ret\n");
    write("asm_unused.s",
          ".global asm_unused\n"
          ".type asm_unused, @function\n"
          "asm_unused:\n"
          "  mov $2,%eax\n"
          "  ret\n");
    write("main.cpp",
          "extern \"C\" int asm_used();\n"
          "#include <iostream>\n"
          "int main(){std::cout<<asm_used()<<std::endl;return 0;}\n");

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "ASM link selection: only asm_used.s linked (asm_unused.s not referenced)");
    EXPECT_TRUE(r.stdout_str.find("Linking file: asm_unused.s") == std::string::npos)
        << r.diagnostic("asm_unused.s should NOT be linked (its symbol is never called)");
    EXPECT_TRUE(r.stdout_str.find("Linking file: asm_used.s") != std::string::npos)
        << r.diagnostic("asm_used.s should be linked (its symbol is called by main)");
}

// -----------------------------------------------------------------------
// Source file included via #include
// -----------------------------------------------------------------------
TEST_F(BelderFixture, SourceFileIncludedViaHashInclude) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // helper.cpp is #include'd by main.cpp, so it should NOT be in the link list
    write("helper.cpp",
          "int helper_func(){return 42;}\n");
    write("main.cpp",
          "#include \"helper.cpp\"\n"
          "#include <iostream>\n"
          "int main(){std::cout<<helper_func()<<std::endl;return 0;}\n");

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "#include of .cpp file: main.cpp includes helper.cpp - build should succeed");
    // helper.cpp should NOT appear as a separately linked file
    EXPECT_TRUE(r.stdout_str.find("Linking file: helper.cpp") == std::string::npos)
        << r.diagnostic("helper.cpp is #included by main.cpp - it must NOT be linked separately");
}

// -----------------------------------------------------------------------
// Library builds
// -----------------------------------------------------------------------
TEST_F(BelderFixture, StaticLibraryBuild) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "g++/ar not found";
    write("libfoo.cpp", "int foo(){return 1;}\n");
    write("main.cpp",
          "int foo();\n"
          "#include <iostream>\n"
          "int main(){std::cout<<foo()<<std::endl;return 0;}\n");

    auto r = runBelder({"-o", tmpDir + "/libMyLib.a"});
    EXPECT_BELDER_OK(r, "static library build: -o libMyLib.a should produce an ar archive");
    EXPECT_TRUE(std::filesystem::exists(tmpDir + "/libMyLib.a"))
        << r.diagnostic("Static library libMyLib.a should be produced");

    // Verify it's actually an archive
    auto ar_r = runCommand("ar t " + tmpDir + "/libMyLib.a");
    EXPECT_EQ(ar_r.exitCode, 0) << ar_r.diagnostic("libMyLib.a should be a valid ar archive");
}

TEST_F(BelderFixture, SharedLibraryBuild) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("libfoo.cpp", "int foo(){return 1;}\n");
    write("main.cpp",
          "int foo();\n"
          "#include <iostream>\n"
          "int main(){std::cout<<foo()<<std::endl;return 0;}\n");

    auto r = runBelder({"-o", tmpDir + "/libMyLib.so"});
    EXPECT_BELDER_OK(r, "shared library build: -o libMyLib.so should produce an ELF shared object");
    EXPECT_TRUE(std::filesystem::exists(tmpDir + "/libMyLib.so"))
        << r.diagnostic("Shared library libMyLib.so should be produced");

    // Verify it's an ELF shared library
    auto file_r = runCommand("file " + tmpDir + "/libMyLib.so");
    EXPECT_TRUE(file_r.stdout_str.find("shared object") != std::string::npos ||
                file_r.stdout_str.find("ELF") != std::string::npos)
        << file_r.diagnostic("libMyLib.so should be reported as an ELF shared object by 'file'");
}

// -----------------------------------------------------------------------
// Compilation failure stops build
// -----------------------------------------------------------------------
TEST_F(BelderFixture, CompilationFailureStopsLinking) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // bad.cpp has a syntax error - belder will attempt compile, g++ fails,
    // but belder does NOT check g++ exit codes. It still runs the link step.
    // However, the output binary cannot be linked without bad.cpp's .o file.
    write("bad.cpp", "this is not valid c++ code !!!\n");
    write("main.cpp", simpleCppMain());

    auto r = runBelder();
    // Belder currently exits 0 even when g++ fails (known behavior)
    // The g++ error should appear in the output
    EXPECT_TRUE(r.hasOutput("error") || r.hasOutput("Error"))
        << r.diagnostic("Compilation error output should contain 'error' when bad.cpp has syntax errors");
    EXPECT_TRUE(!r.hasOutput("SUCCESS")) << r.diagnostic("Compilation error shoud not contain SUCCESS");
    EXPECT_TRUE(r.hasOutput("belder: compilation error")) << r.diagnostic("Compilation error shoud contain message \"compilation error\"");
    EXPECT_EQ(r.exitCode, 2) << r.diagnostic("Should exit with code 2 - compilation error");    
}

// -----------------------------------------------------------------------
// Entry file not found / multiple matches
// -----------------------------------------------------------------------
TEST_F(BelderFixture, EntryFileNotFound) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // Project with only a helper file and no valid entry point (no main function)
    // AND no file named "main.cpp" or "main.c"
    write("helper.cpp", "int foo(){return 1;}\n");

    auto r = runBelder();
    EXPECT_EQ(r.exitCode, 1) << r.diagnostic("Should exit with code 1 when no entry file is found");
    EXPECT_TRUE(r.hasOutput("Cannot find entry file"))
        << r.diagnostic("Error output should contain 'Cannot find entry file'");
}

TEST_F(BelderFixture, SameFilenameInDifferentPaths) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // Two files named util.cpp in different directories
    write("src/util.cpp", "int util_a(){return 1;}\n");
    write("other/util.cpp", "int util_b(){return 2;}\n");
    write("main.cpp",
          "int util_a();\n"
          "int util_b();\n"
          "#include <iostream>\n"
          "int main(){std::cout<<util_a()+util_b()<<std::endl;return 0;}\n");

    auto r = runBelder();
    EXPECT_BELDER_OK(r, "same filename in different dirs: src/util.cpp and other/util.cpp should both build");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("Output binary should exist when two files share the same name in different dirs");
}


TEST_F(BelderFixture, StartFileChangeProducesNewOutput) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("entry1.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"entry1\"<<std::endl;return 0;}\n");
    write("entry2.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"entry2\"<<std::endl;return 0;}\n");

    // Build with entry1
    auto r1 = runBelder({"entry1.cpp"});
    EXPECT_BELDER_OK(r1, "build with entry1.cpp as start file");
    auto out1 = runCommand(tmpDir + "/out");
    EXPECT_TRUE(out1.stdout_str.find("entry1") != std::string::npos)
        << out1.diagnostic("binary built from entry1.cpp should print 'entry1'");

    // Switch to entry2 - use --rebuild to force recompilation
    auto r2 = runBelder({"entry2.cpp"});
    EXPECT_BELDER_OK(r2, "rebuild with entry2.cpp as the new start file");
    auto out2 = runCommand(tmpDir + "/out");
    EXPECT_TRUE(out2.stdout_str.find("entry2") != std::string::npos)
        << out2.diagnostic("binary rebuilt from entry2.cpp should print 'entry2'");
}

TEST_F(BelderFixture, ComplexTestStaticLib) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("lib.h",
        "template <class T>\n"
        "T func(T a){\n"
        "return ++a;\n"
        "}\n"
        );

    write("lib.cpp",
        "#include \"lib.h\"\n"
        "#include <iostream>\n"
        "void aboba(){\n"
        "std::cout << \"_lib.cpp_\" << std::endl;\n"
        "std::cout << func(4) << std::endl;\n"
        "}\n"
        );

    write("main1.cpp",
        "#include <iostream>\n"
        "void aboba();\n"
        "int main(){\n"
        "std::cout << \"_main1.cpp_\" << std::endl;\n"
        "aboba();\n"
        "}\n"
        );


    auto r1 = runBelder({"lib.cpp", "-o", "libLib1.a"});
    EXPECT_BELDER_OK(r1, "first lib should be built");
    auto r2 = runBelder({"main1.cpp", "-o", "out", "-log", "--no-link-force", "lib.cpp", "run"});
    EXPECT_TRUE(r2.hasOutput("_main1.cpp_") && r2.hasOutput("_lib.cpp_") && r2.hasOutput("5"));
}

TEST_F(BelderFixture, ComplexTestSharedLib) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("lib.h",
        "template <class T>\n"
        "T func(T a){\n"
        "return ++a;\n"
        "}\n"
        );

    write("lib.cpp",
        "#include \"lib.h\"\n"
        "#include <iostream>\n"
        "void aboba(){\n"
        "std::cout << \"_lib.cpp_\" << std::endl;\n"
        "std::cout << func(4) << std::endl;\n"
        "}\n"
        );

    write("main1.cpp",
        "#include <iostream>\n"
        "void aboba();\n"
        "int main(){\n"
        "std::cout << \"_main1.cpp_\" << std::endl;\n"
        "aboba();\n"
        "}\n"
        );


    auto r1 = runBelder({"lib.cpp", "-o", "libLib1.so"});
    EXPECT_BELDER_OK(r1, "first lib should be built");
    auto r2 = runBelder({"main1.cpp", "-o", "out", "-log", "--no-link-force", "lib.cpp", "run"});
    EXPECT_TRUE(r2.hasOutput("_main1.cpp_") && r2.hasOutput("_lib.cpp_") && r2.hasOutput("5"));
}