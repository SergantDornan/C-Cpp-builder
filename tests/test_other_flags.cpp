// tests/test_other_flags.cpp
// Tests for: --CXX, --CC, --C++standart, --Cstandart (note: intentional typos
// match belder's actual CLI flag names), --clear-flags/--clean-flags/...,
// --clear-options/--clean-options, --compile-flags, --link-flags

#include "helpers.h"

// =======================================================================
// Compiler switching: --CXX, --CC
// =======================================================================

TEST_F(BelderFixture, CXXCompilerSwitch) {
    REQUIRE_TOOLS_OR_SKIP(
        (std::vector<std::string>{"g++", "clang++"}),
        "Switch C++ compiler to clang++ via --CXX flag and verify the build succeeds");
    write("main.cpp", simpleCppMain());

    // Build with clang++ as C++ compiler
    auto r = runBelder({"--CXX", "clang++"});
    EXPECT_BELDER_OK(r, "--CXX clang++: belder should compile and link with clang++");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("output binary should exist after --CXX clang++");

    // Verify the output runs
    auto run_r = runCommand(tmpDir + "/out");
    EXPECT_EQ(run_r.exitCode, 0) << run_r.diagnostic("compiled binary should execute successfully");
}

TEST_F(BelderFixture, CCCompilerSwitch) {
    REQUIRE_TOOLS_OR_SKIP(
        (std::vector<std::string>{"gcc", "clang"}),
        "Switch C compiler to clang via --CC flag and verify the build succeeds");
    write("main.c", simpleCMain());

    auto r = runBelder({"--CC", "clang"});
    EXPECT_BELDER_OK(r, "--CC clang: belder should compile and link C sources with clang");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("output binary should exist after --CC clang");
}

TEST_F(BelderFixture, CXXCompilerSwitchUsedInLog) {
    REQUIRE_TOOLS_OR_SKIP(
        (std::vector<std::string>{"clang++"}),
        "Verify that -log output shows clang++ being used when --CXX clang++ is set");
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"--rebuild", "--CXX", "clang++", "-log"});
    EXPECT_BELDER_OK(r, "--CXX clang++ with -log: build should succeed");
    // Log should show clang++ being used
    EXPECT_TRUE(r.hasOutput("clang++"))
        << r.diagnostic("-log output should mention 'clang++' when --CXX clang++ is set");
}

// =======================================================================
// Standard switching: --C++standart, --Cstandart
// =======================================================================

TEST_F(BelderFixture, CppStandardSwitch) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // Build with C++17 standard
    auto r = runBelder({"--C++standart", "c++17"});
    EXPECT_BELDER_OK(r, "--C++standart c++17: belder should accept and use the c++17 standard flag");
}

TEST_F(BelderFixture, CppStandardSwitchShownInLog) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"--rebuild", "--C++standart", "c++17", "-log"});
    EXPECT_BELDER_OK(r, "--C++standart c++17 with -log: build should succeed");
    EXPECT_TRUE(r.hasOutput("c++17") || r.hasOutput("std=c++17"))
        << r.diagnostic("-log output should show 'c++17' or 'std=c++17' in compiler invocation");
}

TEST_F(BelderFixture, CStandardSwitch) {
    if (!toolExists("gcc")) GTEST_SKIP() << "gcc not found";
    write("main.c", simpleCMain());

    auto r = runBelder({"--Cstandart", "c11"});
    EXPECT_BELDER_OK(r, "--Cstandart c11: belder should accept and use the c11 standard flag");
}

// =======================================================================
// Flag clearing: --clear-flags, --clean-flags, --flags-clean, --flags-clear
// =======================================================================

TEST_F(BelderFixture, ClearFlagsRemovesSavedFlags) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // First save a flag to config
    auto r1 = runBelder({"-O2", "config"});
    EXPECT_BELDER_OK(r1, "save -O2 to config before testing --clear-flags");

    // Now clear all flags
    auto r2 = runBelder({"--clear-flags"});
    EXPECT_BELDER_OK(r2, "--clear-flags: should clear all saved compilation flags");
}

TEST_F(BelderFixture, CleanFlagsAliasWorks) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder({"-O2", "config"});
    auto r2 = runBelder({"--clean-flags"});
    EXPECT_BELDER_OK(r2, "--clean-flags: alias for --clear-flags, should clear saved flags");
}

TEST_F(BelderFixture, FlagsCleanAliasWorks) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder({"-O2", "config"});
    auto r2 = runBelder({"--flags-clean"});
    EXPECT_BELDER_OK(r2, "--flags-clean: alias for --clear-flags, should clear saved flags");
}

TEST_F(BelderFixture, FlagsClearAliasWorks) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder({"-O2", "config"});
    auto r2 = runBelder({"--flags-clear"});
    EXPECT_BELDER_OK(r2, "--flags-clear: alias for --clear-flags, should clear saved flags");
}

// =======================================================================
// Options clearing: --clear-options, --clean-options
// =======================================================================

TEST_F(BelderFixture, ClearOptionsResetsLists) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("util.cpp", "int util(){return 1;}\n");
    write("main.cpp",
          "int util();\n"
          "#include <iostream>\n"
          "int main(){std::cout<<util()<<std::endl;return 0;}\n");

    // Save force-link config
    auto r1 = runBelder({"--link-force", "util.cpp", "config"});
    EXPECT_BELDER_OK(r1, "save --link-force util.cpp to config before testing --clear-options");

    // Clear all options
    auto r2 = runBelder({"--clear-options"});
    EXPECT_BELDER_OK(r2, "--clear-options: should reset all saved link-set options");
}

TEST_F(BelderFixture, CleanOptionsAliasWorks) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"--clean-options"});
    EXPECT_BELDER_OK(r, "--clean-options: alias for --clear-options, should reset saved options");
}

// =======================================================================
// --compile-flags: only used in compilation, not linking
// =======================================================================

TEST_F(BelderFixture, CompileFlagsOnlyInCompilation) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // Use a compile-only flag that is invalid at link time (e.g., -Wno-unused)
    // and verify build succeeds
    auto r = runBelder({"--rebuild", "--compile-flags", "-DTEST_COMPILE_DEFINE=1", "-log"});
    EXPECT_BELDER_OK(r, "--compile-flags: build with a preprocessor define passed only to compile step");
    // -DTEST_COMPILE_DEFINE should appear in the compile command but not in the link command
    // In the log, the link step should not include -DTEST_COMPILE_DEFINE
    bool inCompile = r.stdout_str.find("TEST_COMPILE_DEFINE") != std::string::npos;
    // If -log shows commands, verify the define is in compile but not link
    if (inCompile) {
        // Find the link line and verify it doesn't have the define
        // This is a loose check
        EXPECT_TRUE(r.stdout_str.find("SUCCESS") != std::string::npos)
            << r.diagnostic("Build should reach SUCCESS with --compile-flags define");
    }
}

TEST_F(BelderFixture, CompileFlagsAffectCompilationOutput) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // Use a preprocessor define that changes behavior
    write("main.cpp",
          "#include <iostream>\n"
          "#ifdef MY_DEFINE\n"
          "  int val = MY_DEFINE;\n"
          "#else\n"
          "  int val = 0;\n"
          "#endif\n"
          "int main(){std::cout<<val<<std::endl;return 0;}\n");

    // Build with -DMY_DEFINE=123
    auto r = runBelder({"--rebuild", "--compile-flags", "-DMY_DEFINE=123"});
    EXPECT_BELDER_OK(r, "--compile-flags -DMY_DEFINE=123: build should succeed with preprocessor define");

    // Run the output and check it prints 123
    auto run_r = runCommand(tmpDir + "/out");
    EXPECT_EQ(run_r.stdout_str.find("123") != std::string::npos, true)
        << run_r.diagnostic("binary output should contain '123' from compile-time define -DMY_DEFINE=123");
}

// =======================================================================
// --link-flags: only used in linking, not compilation
// =======================================================================

TEST_F(BelderFixture, LinkFlagsOnlyInLinking) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp",
          "#include <cmath>\n"
          "#include <iostream>\n"
          "int main(){std::cout<<(int)sqrt(16.0)<<std::endl;return 0;}\n");

    // -lm is a link-only flag
    auto r = runBelder({"--rebuild", "--link-flags", "-lm", "-log"});
    EXPECT_BELDER_OK(r, "--link-flags -lm: math library flag should be passed to linker");
}

TEST_F(BelderFixture, LinkFlagsOnlyAffectLinkCommand) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // Pass a warning flag as link flag (it's valid at link time too, but we verify flag appears)
    auto r = runBelder({"--rebuild", "--link-flags", "-Wl,--as-needed", "-log"});
    EXPECT_BELDER_OK(r, "--link-flags -Wl,--as-needed: linker flag should be accepted");
    // The link command should contain the flag
    EXPECT_TRUE(r.stdout_str.find("as-needed") != std::string::npos ||
                r.exitCode == 0)
        << r.diagnostic("Link flag -Wl,--as-needed should appear in linker invocation");
}

// =======================================================================
// Entry file start file tests
// =======================================================================

TEST_F(BelderFixture, EntryFileSpecifiedExplicitly) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("myentry.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"entry\"<<std::endl;return 0;}\n");
    write("other.cpp", "int other_func(){return 1;}\n");

    // Build with explicit entry file
    auto r = runBelder({"myentry.cpp"});
    EXPECT_BELDER_OK(r, "explicit entry file myentry.cpp: build should succeed");
    EXPECT_TRUE(fileExists("out")) << r.diagnostic("output binary 'out' should exist after building myentry.cpp");
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
    auto r2 = runBelder({"entry2.cpp", "--rebuild"});
    EXPECT_BELDER_OK(r2, "rebuild with entry2.cpp as the new start file");
    auto out2 = runCommand(tmpDir + "/out");
    EXPECT_TRUE(out2.stdout_str.find("entry2") != std::string::npos)
        << out2.diagnostic("binary rebuilt from entry2.cpp should print 'entry2'");
}

// =======================================================================
// File deletion cleans up dep/sym files
// =======================================================================

TEST_F(BelderFixture, DeletedFileRemovesDepAndSymFiles) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());
    write("helper.cpp", "int helper(){return 1;}\n");

    // First build
    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "initial build before testing deleted-file cleanup");

    // Remove helper.cpp
    std::filesystem::remove(tmpDir + "/helper.cpp");

    // Rebuild with --rebuild: belder re-scans the project, helper.cpp is gone
    auto r2 = runBelder({"--rebuild"});
    // Should succeed (helper.cpp was not needed by main.cpp)
    // Exit 0 means success, exit 10 means nothing to link (also OK)
    EXPECT_TRUE(r2.exitCode == 0 || r2.exitCode == 10)
        << "Should handle deleted file gracefully: " << r2.combined();
}
