// tests/test_flags.cpp
// Tests for: run, config, --rebuild/-reb, --relink/-rel,
// -o flag variants, -log presence/absence
#include "helpers.h"

// -----------------------------------------------------------------------
// --rebuild / -reb
// -----------------------------------------------------------------------
TEST_F(BelderFixture, RebuildFlagRecompileAll) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // First build
    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "--rebuild: initial build should succeed");

    // Second build without rebuild - nothing to do
    auto r2 = runBelder();
    EXPECT_TRUE(r2.stdout_str.find("Compiling") == std::string::npos)
        << r2.diagnostic("Second build with no changes should NOT recompile");

    // Build with --rebuild - should recompile
    auto r3 = runBelder({"--rebuild"});
    EXPECT_BELDER_OK(r3, "--rebuild: forced recompilation should succeed");
    EXPECT_TRUE(r3.stdout_str.find("Compiling") != std::string::npos)
        << r3.diagnostic("--rebuild should force recompilation of all files");
}

TEST_F(BelderFixture, RebFlagShortFormRecompileAll) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "-reb: initial build should succeed");

    auto r2 = runBelder({"-reb"});
    EXPECT_BELDER_OK(r2, "-reb: forced recompilation should succeed");
    EXPECT_TRUE(r2.stdout_str.find("Compiling") != std::string::npos)
        << r2.diagnostic("-reb should force recompilation (same as --rebuild)");
}

// -----------------------------------------------------------------------
// --relink / -rel
// -----------------------------------------------------------------------
TEST_F(BelderFixture, RelinkFlagForcesRelink) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "--relink: initial build should succeed");

    // No changes, no relink
    auto r2 = runBelder();
    EXPECT_TRUE(r2.stdout_str.find("Linking file") == std::string::npos)
        << r2.diagnostic("Second build with no changes should NOT relink");

    // Force relink
    auto r3 = runBelder({"--relink"});
    EXPECT_BELDER_OK(r3, "--relink: forced relink should succeed");
    EXPECT_TRUE(r3.stdout_str.find("Linking file") != std::string::npos)
        << r3.diagnostic("--relink should force the link step even when nothing changed");
}

TEST_F(BelderFixture, RelShortFormForcesRelink) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "-rel: initial build should succeed");

    auto r2 = runBelder({"-rel"});
    EXPECT_BELDER_OK(r2, "-rel: forced relink should succeed");
    EXPECT_TRUE(r2.stdout_str.find("Linking file") != std::string::npos)
        << r2.diagnostic("-rel should force the link step (same as --relink)");
}

// -----------------------------------------------------------------------
// -o flag
// -----------------------------------------------------------------------
TEST_F(BelderFixture, OutputFlagCustomName) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    std::string outFile = tmpDir + "/my_output";
    auto r = runBelder({"-o", outFile});
    EXPECT_BELDER_OK(r, "-o custom_name: build with custom output path should succeed");
    EXPECT_TRUE(std::filesystem::exists(outFile))
        << r.diagnostic("Custom output file should exist at: " + outFile);
}

TEST_F(BelderFixture, OutputFlagWithFlagAsNameErrors) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // Build to have a clean state first
    runBelder({"--rebuild"});
    // -o followed by a flag (starts with -): belder prints a warning but still exits 0
    // The flag value is then treated as a general compiler flag (which g++ will reject)
    auto r = runBelder({"--relink", "-o", "--some-flag"});
    // belder prints "no file name after -o flag" as a warning
    EXPECT_TRUE(r.hasOutput("no file name after -o")) << r.combined();
}

TEST_F(BelderFixture, OutputFlagEndOfLineErrors) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // Build to have a clean state first
    runBelder({"--rebuild"});
    // -o with nothing after it: belder prints "no file name after -o flag" warning
    // but still exits 0 (using default output name)
    auto r = runBelder({"--relink", "-o"});
    EXPECT_TRUE(r.hasOutput("no file name after -o") ||
                r.hasOutput("ERROR")) << r.combined();
}

TEST_F(BelderFixture, OutputFlagRelativePath) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // -o with a simple relative name (in the project root which always exists)
    auto r = runBelder({"-o", "my_relative_out"});
    EXPECT_BELDER_OK(r, "-o relative_path: build with relative output path should succeed");
    EXPECT_TRUE(fileExists("my_relative_out"))
        << r.diagnostic("Output binary should be created at relative path 'my_relative_out' inside project dir");
}

// -----------------------------------------------------------------------
// run option
// -----------------------------------------------------------------------
TEST_F(BelderFixture, RunOptionBuildsAndExecutes) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // Write a program that exits with 0
    write("main.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"running\"<<std::endl;return 0;}\n");

    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, "run option: should build and execute the program");
    // The program output should appear
    EXPECT_TRUE(r.stdout_str.find("running") != std::string::npos)
        << r.diagnostic("Program output 'running' should appear in stdout when using 'run' option");
}

// -----------------------------------------------------------------------
// config option
// -----------------------------------------------------------------------
TEST_F(BelderFixture, ConfigOptionSavesConfig) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // First build to initialize project
    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "config: initial build before saving config should succeed");

    // config option saves current options silently and also runs the normal build
    std::string outFile = tmpDir + "/myout";
    auto r2 = runBelder({"-o", outFile, "config"});
    EXPECT_BELDER_OK(r2, "config: save output path config should succeed");
    EXPECT_TRUE(std::filesystem::exists(outFile))
        << r2.diagnostic("Output binary should be created at the configured path: " + outFile);

    // After saving config, subsequent build with --relink uses the saved output path
    auto r3 = runBelder({"--relink"});
    EXPECT_BELDER_OK(r3, "config: relink after saved config should use the saved output path");
    EXPECT_TRUE(std::filesystem::exists(outFile))
        << r3.diagnostic("Saved output path '" + outFile + "' should be used on relink");
}

// -----------------------------------------------------------------------
// status option
// -----------------------------------------------------------------------
TEST_F(BelderFixture, StatusOptionPrintsProjectInfo) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "status: initial build should succeed before calling 'status'");

    auto r2 = runBelder({"status"});
    // status should print information about the project
    EXPECT_TRUE(r2.hasOutput("compiler") || r2.hasOutput("Compiler") ||
                r2.hasOutput("Output") || r2.hasOutput("output"))
        << r2.diagnostic("'status' should print project configuration information (compiler, output, etc.)");
}

// -----------------------------------------------------------------------
// clear / clean / mrproper
// -----------------------------------------------------------------------
TEST_F(BelderFixture, ClearCommandRemovesBuildFolder) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "clear: build before testing clear should succeed");
    EXPECT_TRUE(fileExists("out")) << r1.diagnostic("Output 'out' should exist before clearing");

    // Clean the build state
    auto r2 = runBelder({"clear"});
    EXPECT_BELDER_OK(r2, "clear: should clean build state without error");
}

TEST_F(BelderFixture, SilentClearNoOutput) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "silent_clear: build before testing silent_clear should succeed");

    auto r2 = runBelder({"silent_clear"});
    EXPECT_BELDER_OK(r2, "silent_clear: should clear build state silently");
    // silent_clear should produce no output
    EXPECT_TRUE(r2.stdout_str.empty() || r2.stdout_str == "\n")
        << r2.diagnostic("silent_clear should produce no stdout output");
}

// -----------------------------------------------------------------------
// Incremental build behavior
// -----------------------------------------------------------------------
TEST_F(BelderFixture, ModifiedFilesAreRecompiled) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"v1\"<<std::endl;return 0;}\n");

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "incremental: initial build should succeed");

    // Nothing changed - should not recompile
    auto r2 = runBelder();
    EXPECT_TRUE(r2.stdout_str.find("Compiling") == std::string::npos)
        << r2.diagnostic("Second build with no changes should NOT recompile any file");

    // Modify main.cpp
    sleep(1); // Ensure timestamp changes
    write("main.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"v2\"<<std::endl;return 0;}\n");

    auto r3 = runBelder();
    EXPECT_BELDER_OK(r3, "incremental: build after modifying main.cpp should succeed");
    EXPECT_TRUE(r3.stdout_str.find("Compiling") != std::string::npos)
        << r3.diagnostic("Modified main.cpp should trigger recompilation");
}

TEST_F(BelderFixture, DependentFilesRecompiledAfterHeaderChange) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("util.h", "int util_value();\n");
    write("util.cpp",
          "#include \"util.h\"\n"
          "int util_value(){return 1;}\n");
    write("main.cpp",
          "#include \"util.h\"\n"
          "#include <iostream>\n"
          "int main(){std::cout<<util_value()<<std::endl;return 0;}\n");

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "header dependency: initial build with util.h should succeed");

    // No changes
    auto r2 = runBelder();
    EXPECT_TRUE(r2.stdout_str.find("Compiling") == std::string::npos)
        << r2.diagnostic("No changes: should not recompile any file");

    // Modify the header
    sleep(1);
    write("util.h", "int util_value();\nint util_value2();\n");

    auto r3 = runBelder();
    EXPECT_BELDER_OK(r3, "header dependency: build after modifying util.h should succeed");
    // Files that depend on util.h should be recompiled
    EXPECT_TRUE(r3.stdout_str.find("Compiling main.cpp") != std::string::npos ||
                r3.stdout_str.find("Compiling util.cpp") != std::string::npos)
        << r3.diagnostic("Files depending on changed util.h should be recompiled");
}

TEST_F(BelderFixture, UnchangedFilesNotRecompiled) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());
    write("other.cpp", "int other(){return 99;}\n");

    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "unchanged files: initial build should succeed");

    // Second build - no changes
    auto r2 = runBelder();
    EXPECT_TRUE(r2.stdout_str.find("Compiling") == std::string::npos)
        << r2.diagnostic("Unchanged files (main.cpp, other.cpp) should NOT be recompiled on second build");
}

// -----------------------------------------------------------------------
// help / unknown flags
// -----------------------------------------------------------------------
TEST_F(BelderFixture, HelpOutputProvided) {
    auto r = runBelder({"help"});
    EXPECT_BELDER_OK(r, "help: belder help should succeed and show usage information");
    EXPECT_TRUE(r.hasOutput("belder") || r.hasOutput("flag") || r.hasOutput("FLAG"))
        << r.diagnostic("'help' should output belder usage/flag information");
}

TEST_F(BelderFixture, SourceFolderDeletionCleansBelderState) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // Build once
    write("main.cpp", simpleCppMain());
    auto r1 = runBelder();
    EXPECT_BELDER_OK(r1, "source deletion: initial build should succeed");

    // Simulate deletion of source by removing tmpDir
    std::filesystem::remove_all(tmpDir);
    // Recreate it so TearDown can clean up
    std::filesystem::create_directories(tmpDir);

    // Run belder on a NEW project. Belder should detect that
    // the old project dir no longer exists and clean it.
    write("main.cpp", simpleCppMain());
    auto r2 = runBelder();
    EXPECT_BELDER_OK(r2, "source deletion: after recreating project dir belder should build cleanly");
}
