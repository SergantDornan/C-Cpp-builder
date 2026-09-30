#include "helpers.h"
#include <thread>
#include <set>

namespace {

std::string printingMain(const std::string& text) {
    return "#include <iostream>\n"
           "int main(){std::cout << \"" + text + "\" << std::endl; return 0;}\n";
}

}

class RegressionFixture : public BelderFixture {
protected:
    std::vector<std::string> extraDirs;

    std::string extraDir() {
        char tmpl[] = "/tmp/btest_extra_XXXXXX";
        char* d = mkdtemp(tmpl);
        extraDirs.push_back(d ? d : "");
        return extraDirs.back();
    }

    void TearDown() override {
        for (const auto& d : extraDirs) {
            if (d.empty()) continue;
            runCommandInDir(std::string(BELDER_BINARY) + " silent_clear", d);
            std::filesystem::remove_all(d);
        }
        BelderFixture::TearDown();
    }
};

TEST_F(RegressionFixture, EntryAfterConfigKeywordIsNotAFlag) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "entry after keyword");
    write("a.cpp", printingMain("_a_"));
    ASSERT_BELDER_OK(runBelder({"config", "a.cpp", "-o", "oa", "-O2"}), "config with entry");
    EXPECT_FALSE(fileExists("oa"));
    auto s = runBelder({"status"});
    EXPECT_TRUE(s.hasOutput("Entry file: " + path("a.cpp"))) << s.diagnostic();
    EXPECT_FALSE(s.hasOutput("Other Flags")) << s.diagnostic();
    auto r = runBelder({"run", "a.cpp", "-o", "oa"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_a_")) << r.diagnostic();
}

TEST_F(RegressionFixture, LinkErrorExitsWithCodeAndDoesNotRunStaleBinary) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "link error");
    write("main.cpp", printingMain("_old_"));
    ASSERT_BELDER_OK(runBelder({"-o", "out"}), "first build");
    write("main.cpp", "int missing();\nint main(){ return missing(); }\n");
    auto r = runBelder({"run"});
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("belder: link error")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("_old_")) << r.diagnostic();
}

TEST_F(RegressionFixture, SymbolConflictIsLinkError) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "symbol conflict");
    write("impl1.cpp", "int shared_func(){return 1;}\n");
    write("impl2.cpp", "int shared_func(){return 2;}\n");
    write("main.cpp", "int shared_func();\nint main(){ return shared_func(); }\n");
    auto r = runBelder();
    EXPECT_EQ(r.exitCode, 3) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("multiple definition")) << r.diagnostic();
}

TEST_F(RegressionFixture, RelativeDirectoryWithParentStepsFromRoot) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "-C with .. up to root");
    write("main.cpp", printingMain("_c_"));
    auto r = runBelderFrom("/tmp", {"-C", ".." + tmpDir, "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_c_")) << r.diagnostic();
    std::ifstream cfg(std::string(getenv("HOME")) + "/builder/config");
    std::string line;
    while (std::getline(cfg, line)) EXPECT_NE(line.rfind("-1*", 0), 0u) << line;
}

TEST_F(RegressionFixture, AbsoluteOutputPathWithParentStep) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "absolute path with ..");
    write("main.cpp", printingMain("_abs_"));
    makeDir(path("sub"));
    ASSERT_BELDER_OK(runBelder({"-o", path("sub/../out")}), "build");
    EXPECT_TRUE(fileExists("out"));
    auto s = runBelder({"status"});
    EXPECT_TRUE(s.hasOutput("Output file: " + path("out") + "\n")) << s.diagnostic();
}

TEST_F(RegressionFixture, ForceLinkedLibraryOutsideProject) {
    if (!toolExists("g++") || !toolExists("ar")) GTEST_SKIP() << "g++/ar not found";
    std::string ext = extraDir();
    writeFile(ext + "/extf.cpp", "int extf(){ return 99; }\n");
    ASSERT_BELDER_OK(runCommandInDir(std::string(BELDER_BINARY) + " extf.cpp -o libextf.a", ext), "external lib");
    write("main.cpp", "#include <iostream>\nint extf();\nint main(){ std::cout << \"ext\" << extf() << std::endl; }\n");
    auto r = runBelder({"main.cpp", "-o", "out", "--link-force", ext + "/libextf.a", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("ext99")) << r.diagnostic();
}

TEST_F(RegressionFixture, EditInSameSecondIsNoticed) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "sub-second edits");
    write("main.cpp", printingMain("_v1_"));
    ASSERT_BELDER_OK(runBelder(), "first build");
    auto before = std::filesystem::last_write_time(path("main.cpp"));
    write("main.cpp", printingMain("_v2_"));
    std::filesystem::last_write_time(path("main.cpp"), before + std::chrono::nanoseconds(1));
    auto r = runBelder({"run"});
    EXPECT_TRUE(r.hasOutput("_v2_")) << r.diagnostic();
}

TEST_F(RegressionFixture, OutputDirectoryIsCreated) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "missing output dir");
    write("main.cpp", printingMain("_dir_"));
    auto r = runBelder({"-o", "deep/er/bin/app", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_dir_")) << r.diagnostic();
}

TEST_F(RegressionFixture, TruncatedDepfileDoesNotCrash) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "truncated depfile");
    write("main.cpp", printingMain("_dep_"));
    write("h.cpp", "int h(){ return 1; }\n");
    ASSERT_BELDER_OK(runBelder(), "first build");
    auto profiles = belderProfileDirs(belderProjectDir(tmpDir));
    ASSERT_EQ(profiles.size(), 1u);
    for (const auto& entry : std::filesystem::directory_iterator(profiles[0] + "/source/deps")) {
        std::ifstream in(entry.path());
        std::string first;
        std::getline(in, first);
        in.close();
        writeFile(entry.path().string(), first + "\n");
    }
    auto r = runBelder({"run"});
    EXPECT_NE(r.exitCode, -1) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_dep_")) << r.diagnostic();
}

TEST_F(RegressionFixture, CorruptedLastFileKeepsLastPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "corrupted last");
    write("a.cpp", printingMain("_a_"));
    write("b.cpp", printingMain("_b_"));
    ASSERT_BELDER_OK(runBelder({"a.cpp", "-o", "oa"}), "pair a");
    sleep(1);
    ASSERT_BELDER_OK(runBelder({"b.cpp", "-o", "ob"}), "pair b");
    writeFile(belderProjectDir(tmpDir) + "/last", "garbage\n");
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_b_")) << r.diagnostic();
}

TEST_F(RegressionFixture, CompileFlagsPersistBetweenRuns) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "compile flags persist");
    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "#ifdef PERSIST\n"
        "std::cout << \"_on_\" << std::endl;\n"
        "#endif\n"
        "}\n");
    ASSERT_BELDER_OK(runBelder({"--compile-flags", "-DPERSIST"}), "set flags");
    auto r = runBelder({"-reb", "run"});
    EXPECT_TRUE(r.hasOutput("_on_")) << r.diagnostic();
    auto s = runBelder({"status"});
    EXPECT_TRUE(s.hasOutput("-DPERSIST")) << s.diagnostic();
}

TEST_F(RegressionFixture, LinkFlagsBeforeCompileFlags) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "flag lists order");
    write("main.cpp", printingMain("_f_"));
    ASSERT_BELDER_OK(runBelder({"--link-flags", "-s", "--compile-flags", "-DCF"}), "build");
    auto s = runBelder({"status"});
    EXPECT_TRUE(s.hasOutput("Compile flags")) << s.diagnostic();
    EXPECT_TRUE(s.hasOutput("-DCF")) << s.diagnostic();
    EXPECT_FALSE(s.hasOutput("Other Flags")) << s.diagnostic();
}

TEST_F(RegressionFixture, BelderOptionEndsCompileFlags) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "option after compile flags");
    write("main.cpp", printingMain("_o_"));
    write("bad.cpp", "garbage\n");
    auto r = runBelder({"--compile-flags", "-DCF", "-o", "custom", "--no-link-force", "bad.cpp", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(fileExists("custom"));
    EXPECT_TRUE(r.hasOutput("_o_")) << r.diagnostic();
}

TEST_F(RegressionFixture, OutputCannotOverwriteSourcesOrDirectories) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "dangerous outputs");
    write("main.cpp", printingMain("_keep_"));
    write("h.h", "#define H 1\n");
    makeDir(path("outdir"));
    for (const std::string& out : {"main.cpp", "h.h", "outdir"}) {
        auto r = runBelder({"main.cpp", "-o", out});
        EXPECT_EQ(r.exitCode, 1) << r.diagnostic();
    }
    EXPECT_EQ(readFile(path("main.cpp")), printingMain("_keep_"));
    EXPECT_EQ(readFile(path("h.h")), "#define H 1\n");
    EXPECT_TRUE(std::filesystem::is_directory(path("outdir")));
}

TEST_F(RegressionFixture, ObjectNamesDoNotCollide) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "object name collision");
    write("x_y/z.cpp", "int from_xy_z(){ return 1; }\n");
    write("x/y_z.cpp", "int from_x_yz(){ return 2; }\n");
    write("main.cpp",
        "#include <iostream>\n"
        "int from_xy_z(); int from_x_yz();\n"
        "int main(){ std::cout << \"sum\" << from_xy_z() + from_x_yz() << std::endl; }\n");
    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("sum3")) << r.diagnostic();
}

TEST_F(RegressionFixture, RemovedHeaderRebuildsDependents) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "removed header");
    write("h.h", "#define HV 1\n");
    write("main.cpp", "#include \"h.h\"\n#include <iostream>\nint main(){ std::cout << HV << std::endl; }\n");
    ASSERT_BELDER_OK(runBelder(), "first build");
    std::filesystem::rename(path("h.h"), path("h.bak"));
    auto r = runBelder();
    EXPECT_EQ(r.exitCode, 2) << r.diagnostic();
    std::filesystem::rename(path("h.bak"), path("h.h"));
    auto r2 = runBelder({"run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
}

TEST_F(RegressionFixture, ParallelRegistrationGivesUniqueIndexes) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "parallel registration");
    for (int round = 0; round < 3; ++round) {
        std::vector<std::string> dirs;
        for (int i = 0; i < 16; ++i) {
            dirs.push_back(extraDir());
            writeFile(dirs.back() + "/main.cpp", printingMain("_p_"));
        }
        std::vector<std::thread> threads;
        for (const auto& d : dirs)
            threads.emplace_back([d]{ runCommandInDir(std::string(BELDER_BINARY) + " status", d); });
        for (auto& t : threads) t.join();
        std::set<std::string> buildDirs;
        for (const auto& d : dirs) {
            std::string b = belderProjectDir(d);
            EXPECT_FALSE(b.empty()) << d;
            buildDirs.insert(b);
        }
        EXPECT_EQ(buildDirs.size(), dirs.size());
    }
}
