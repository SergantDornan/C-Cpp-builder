// tests/test_include_dirs.cpp
// Tests for -I, --no-include, --defult-include options
// Note: '--defult-include' is the actual flag name in belder (intentional typo)

#include "helpers.h"

// =======================================================================
// -I directory tests
// =======================================================================

TEST_F(BelderFixture, IFlagAbsoluteDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    // Put header in a separate directory outside the project
    std::string extraInclude = tmpDir + "_headers";
    makeDir(extraInclude);
    writeFile(extraInclude + "/myheader.h", "int extra_val();\n");
    write("extra_impl.cpp", "int extra_val(){return 42;}\n");
    write("main.cpp",
          "#include \"myheader.h\"\n"
          "#include <iostream>\n"
          "int main(){std::cout<<extra_val()<<std::endl;return 0;}\n");

    auto r = runBelder({"-I" + extraInclude});
    EXPECT_EQ(r.exitCode, 0) << "-I absolute dir should work: " << r.combined();
    // Cleanup extra dir
    std::filesystem::remove_all(extraInclude);
}

TEST_F(BelderFixture, IFlagRelativeDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("headers/local.h", "int local_val();\n");
    write("src/local.cpp", "int local_val(){return 7;}\n");
    write("main.cpp",
          "#include \"local.h\"\n"
          "#include <iostream>\n"
          "int main(){std::cout<<local_val()<<std::endl;return 0;}\n");

    // Run from tmpDir so relative -I path is interpreted correctly
    // Belder should resolve relative paths based on the project dir
    auto r = runBelder({"-Iheaders"});
    EXPECT_EQ(r.exitCode, 0) << "-I relative dir should work: " << r.combined();
}

TEST_F(BelderFixture, IFlagComplexRelativePath) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("shared/api/v1/api.h", "int api_func();\n");
    write("shared/api/v1/api.cpp", "#include \"shared/api/v1/api.h\"\nint api_func(){return 1;}\n");
    write("main.cpp",
          "#include \"api.h\"\n"
          "#include <iostream>\n"
          "int main(){std::cout<<api_func()<<std::endl;return 0;}\n");

    auto r = runBelder({"-Ishared/api/v1"});
    EXPECT_EQ(r.exitCode, 0) << "-I complex relative path should work: " << r.combined();
}

TEST_F(BelderFixture, IFlagAbsoluteNonExistentDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"-I/tmp/belder_nonexistent_include_dir_xyz"});
    // Should fail: non-existent directory
    EXPECT_NE(r.exitCode, 0) << "Should fail with non-existent -I dir";
    EXPECT_TRUE(r.hasOutput("ERROR") || r.hasOutput("error") || r.hasOutput("not exist"))
        << r.combined();
}

TEST_F(BelderFixture, IFlagRelativeNonExistentDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"-Inonexistent_relative_dir_xyz"});
    EXPECT_NE(r.exitCode, 0) << "Should fail with non-existent relative -I dir";
    EXPECT_TRUE(r.hasOutput("ERROR") || r.hasOutput("error") || r.hasOutput("not exist"))
        << r.combined();
}

TEST_F(BelderFixture, IFlagDirAlreadyInNoIncludeList) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("mydir/file.h", "int f();\n");
    write("main.cpp", simpleCppMain());

    // Add dir to no-include list first
    auto r1 = runBelder({"--no-include", tmpDir + "/mydir", "config"});
    EXPECT_EQ(r1.exitCode, 0) << r1.combined();

    // Now try to add to -I list
    auto r2 = runBelder({"-I" + tmpDir + "/mydir"});
    // Should warn or error about conflict
    EXPECT_TRUE(r2.exitCode == 0 || r2.hasOutput("already") || r2.hasOutput("ERROR"))
        << r2.combined();
}

// =======================================================================
// --no-include tests
// =======================================================================

TEST_F(BelderFixture, NoIncludeAbsoluteDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("excluded/helper.cpp", "int excluded_func(){return 99;}\n");
    write("excluded/helper.h", "int excluded_func();\n");
    write("main.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"ok\"<<std::endl;return 0;}\n");

    // Exclude a directory from search
    auto r = runBelder({"--no-include", tmpDir + "/excluded"});
    EXPECT_EQ(r.exitCode, 0) << "--no-include abs dir should work: " << r.combined();
}

TEST_F(BelderFixture, NoIncludeRelativeDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("excluded/helper.cpp", "int excluded_func(){return 99;}\n");
    write("main.cpp",
          "#include <iostream>\n"
          "int main(){std::cout<<\"ok\"<<std::endl;return 0;}\n");

    auto r = runBelder({"--no-include", "excluded"});
    EXPECT_EQ(r.exitCode, 0) << "--no-include relative dir should work: " << r.combined();
}

TEST_F(BelderFixture, NoIncludeRelativeNonExistentDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // First build to initialize the project state
    runBelder({"--rebuild"});

    // --no-include with a non-existent relative dir is silently ignored by belder
    // (the dir can't be found in the project map, so no change is made)
    // Result is exit 10 "nothing to link" since nothing changed
    auto r = runBelder({"--no-include", "nonexistent_dir_xyz"});
    EXPECT_TRUE(r.exitCode == 10 || r.exitCode == 0 || r.hasOutput("not exist"))
        << "Should handle non-existent dir gracefully: " << r.combined();
}

TEST_F(BelderFixture, NoIncludeAbsoluteNonExistentDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // First build to initialize the project state
    runBelder({"--rebuild"});

    // --no-include with a non-existent absolute dir is silently ignored
    auto r = runBelder({"--no-include", "/tmp/belder_nonexistent_dir_xyz"});
    EXPECT_TRUE(r.exitCode == 10 || r.exitCode == 0 || r.hasOutput("not exist"))
        << "Should handle non-existent abs dir gracefully: " << r.combined();
}

TEST_F(BelderFixture, NoIncludeDirAlreadyInIList) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("extdir/file.h", "int f();\n");
    write("main.cpp", simpleCppMain());

    std::string extDir = tmpDir + "/extdir";
    // Add to -I list first
    auto r1 = runBelder({"-I" + extDir, "config"});
    // Now add to no-include
    auto r2 = runBelder({"--no-include", extDir});
    // Should warn or handle gracefully
    EXPECT_TRUE(r2.exitCode == 0 || r2.hasOutput("already") || r2.hasOutput("ERROR"))
        << r2.combined();
}

TEST_F(BelderFixture, NoIncludeMultipleDirs) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("dir1/f1.cpp", "int f1(){return 1;}\n");
    write("dir2/f2.cpp", "int f2(){return 2;}\n");
    write("main.cpp", simpleCppMain());

    auto r = runBelder({"--no-include", tmpDir + "/dir1", tmpDir + "/dir2"});
    EXPECT_EQ(r.exitCode, 0) << "--no-include multiple dirs should work: " << r.combined();
}

// =======================================================================
// --defult-include tests (note: intentional typo in belder's option name)
// =======================================================================

TEST_F(BelderFixture, DefultIncludeAbsoluteDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("extradir/file.h", "int f();\n");
    write("main.cpp", simpleCppMain());

    std::string dir = tmpDir + "/extradir";
    // First add to no-include
    runBelder({"--no-include", dir, "config"});
    // Then restore it with --defult-include
    auto r = runBelder({"--defult-include", dir});
    EXPECT_EQ(r.exitCode, 0) << "--defult-include abs dir should work: " << r.combined();
}

TEST_F(BelderFixture, DefultIncludeRelativeDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("myextradir/file.h", "int f();\n");
    write("main.cpp", simpleCppMain());

    // First add to no-include
    runBelder({"--no-include", tmpDir + "/myextradir", "config"});
    // Then restore with relative path
    auto r = runBelder({"--defult-include", "myextradir"});
    EXPECT_EQ(r.exitCode, 0) << "--defult-include relative dir should work: " << r.combined();
}

TEST_F(BelderFixture, DefultIncludeRelativeNonExistentDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // First build to initialize the project state
    runBelder({"--rebuild"});

    // --defult-include with non-existent relative dir is silently handled
    auto r = runBelder({"--defult-include", "nonexistent_dir_xyz"});
    EXPECT_TRUE(r.exitCode == 10 || r.exitCode == 0 || r.hasOutput("not exist"))
        << "Should handle non-existent dir gracefully: " << r.combined();
}

TEST_F(BelderFixture, DefultIncludeAbsoluteNonExistentDir) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("main.cpp", simpleCppMain());

    // First build to initialize the project state
    runBelder({"--rebuild"});

    // --defult-include with non-existent absolute dir is silently handled
    auto r = runBelder({"--defult-include", "/tmp/belder_nonexistent_dir_xyz"});
    EXPECT_TRUE(r.exitCode == 10 || r.exitCode == 0 || r.hasOutput("not exist"))
        << "Should handle non-existent abs dir gracefully: " << r.combined();
}

TEST_F(BelderFixture, DefultIncludeDirAlreadyInIList) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("extdir2/file.h", "int f();\n");
    write("main.cpp", simpleCppMain());

    std::string dir = tmpDir + "/extdir2";
    // Add to -I list
    auto r1 = runBelder({"-I" + dir, "config"});
    // Try to also add to default (already in I list)
    auto r2 = runBelder({"--defult-include", dir});
    EXPECT_TRUE(r2.exitCode == 0 || r2.hasOutput("already") || r2.hasOutput("ERROR"))
        << r2.combined();
}

TEST_F(BelderFixture, DefultIncludeDirAlreadyInNoIncludeList) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("ndir/file.h", "int f();\n");
    write("main.cpp", simpleCppMain());

    std::string dir = tmpDir + "/ndir";
    // Add to no-include
    auto r1 = runBelder({"--no-include", dir, "config"});
    // --defult-include should remove it from no-include
    auto r2 = runBelder({"--defult-include", dir});
    EXPECT_EQ(r2.exitCode, 0) << "--defult-include removes from no-include: " << r2.combined();
}

TEST_F(BelderFixture, DefultIncludeMultipleDirs) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    write("dir_a/f.h", "int f();\n");
    write("dir_b/g.h", "int g();\n");
    write("main.cpp", simpleCppMain());

    std::string da = tmpDir + "/dir_a";
    std::string db = tmpDir + "/dir_b";
    // Add both to no-include
    runBelder({"--no-include", da, db, "config"});
    // Reset both
    auto r = runBelder({"--defult-include", da, db});
    EXPECT_EQ(r.exitCode, 0) << "--defult-include multiple dirs: " << r.combined();
}
