#include "helpers.h"

namespace {

std::string printingMain(const std::string& text) {
    return "#include <iostream>\n"
           "int main(){std::cout << \"" + text + "\" << std::endl; return 0;}\n";
}

}

class ConfigsFixture : public BelderFixture {
protected:
    void writeTwoMains() {
        write("main1.cpp", printingMain("_main1_"));
        write("main2.cpp", printingMain("_main2_"));
    }

    std::vector<std::string> profiles() {
        return belderProfileDirs(belderProjectDir(tmpDir));
    }
};

TEST_F(ConfigsFixture, EachPairKeepsItsOwnOptions) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Pairs keep separate options");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-O2"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2", "-O1"}), "pair 2");

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1"}), "back to pair 1");
    auto s1 = runBelder({"status"});
    EXPECT_TRUE(s1.hasOutput("Opt: -O2")) << s1.diagnostic();
    EXPECT_FALSE(s1.hasOutput("Opt: -O1")) << s1.diagnostic();

    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2"}), "back to pair 2");
    auto s2 = runBelder({"status"});
    EXPECT_TRUE(s2.hasOutput("Opt: -O1")) << s2.diagnostic();
    EXPECT_FALSE(s2.hasOutput("Opt: -O2")) << s2.diagnostic();
}

TEST_F(ConfigsFixture, OptionsChangeOnlySelectedPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Options go to the selected pair");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2"}), "pair 2");
    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-O3"}), "change pair 1");

    auto s1 = runBelder({"status"});
    EXPECT_TRUE(s1.hasOutput("Opt: -O3")) << s1.diagnostic();

    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2"}), "pair 2 again");
    auto s2 = runBelder({"status"});
    EXPECT_FALSE(s2.hasOutput("Opt:")) << s2.diagnostic();
}

TEST_F(ConfigsFixture, NewPairStartsWithDefaults) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "New pair starts from defaults");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-O2", "-w"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2", "-g3"}), "pair 2");

    auto s = runBelder({"status"});
    EXPECT_TRUE(s.hasOutput("Debug: -g3")) << s.diagnostic();
    EXPECT_FALSE(s.hasOutput("Opt:")) << s.diagnostic();
    EXPECT_FALSE(s.hasOutput("Other Flags")) << s.diagnostic();
}

TEST_F(ConfigsFixture, EntryOnlyTakesOutputOfLastPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Entry without -o uses last output");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-O2"}), "pair 1");
    auto r = runBelder({"main2.cpp", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_main2_")) << r.diagnostic();
    EXPECT_FALSE(fileExists("out")) << r.diagnostic("default output must not be used");

    auto s = runBelder({"status"});
    EXPECT_TRUE(s.hasOutput("Entry file: " + path("main2.cpp"))) << s.diagnostic();
    EXPECT_TRUE(s.hasOutput("Output file: " + path("out1"))) << s.diagnostic();
    EXPECT_FALSE(s.hasOutput("Opt:")) << s.diagnostic("new pair must not inherit options");
}

TEST_F(ConfigsFixture, OutputOnlyTakesEntryOfLastPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "-o without entry uses last entry");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"-o", "out2"}), "pair with new output");
    ASSERT_TRUE(fileExists("out2"));

    auto r = runCommand(path("out2"));
    EXPECT_TRUE(r.hasOutput("_main1_")) << r.diagnostic();
}

TEST_F(ConfigsFixture, NoArgumentsUseLastPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "No arguments use last pair");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2"}), "pair 2");

    auto r = runBelder({"run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("belder: nothing to link")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_main2_")) << r.diagnostic();
}

TEST_F(ConfigsFixture, StatusListsAllPairs) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "status lists pairs");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2"}), "pair 2");

    auto s = runBelder({"status"});
    EXPECT_TRUE(s.hasOutput("Configs in this project")) << s.diagnostic();
    EXPECT_TRUE(s.hasOutput(path("main1.cpp") + " -> " + path("out1"))) << s.diagnostic();
    EXPECT_TRUE(s.hasOutput("* " + path("main2.cpp") + " -> " + path("out2"))) << s.diagnostic();
}

TEST_F(ConfigsFixture, SameProfileSharesObjects) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Pairs with the same flags share objects");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-O2"}), "pair 1");
    auto r = runBelder({"main2.cpp", "-o", "out2", "-O2"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(r.hasOutput("Compiling")) << r.diagnostic();
    EXPECT_TRUE(fileExists("out2")) << r.diagnostic();
    EXPECT_EQ(profiles().size(), 1u);
}

TEST_F(ConfigsFixture, DifferentProfilesDoNotRebuildEachOther) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Switching between profiles does not rebuild");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-O0"}), "pair 1");
    auto r2 = runBelder({"main2.cpp", "-o", "out2", "-O2"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_TRUE(r2.hasOutput("Compiling main2.cpp")) << r2.diagnostic();
    EXPECT_EQ(profiles().size(), 2u);

    auto r3 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link")) << r3.diagnostic();
    EXPECT_FALSE(r3.hasOutput("Compiling")) << r3.diagnostic();

    auto r4 = runBelder({"main2.cpp", "-o", "out2"});
    EXPECT_TRUE(r4.hasOutput("belder: nothing to link")) << r4.diagnostic();
    EXPECT_FALSE(r4.hasOutput("Compiling")) << r4.diagnostic();
}

TEST_F(ConfigsFixture, IncludeDirOrderDoesNotSplitProfile) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Order of -I dirs is not significant");
    writeTwoMains();
    makeDir(path("incA"));
    makeDir(path("incB"));

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-I" + path("incA"), "-I" + path("incB")}), "pair 1");
    auto r = runBelder({"main2.cpp", "-o", "out2", "-I" + path("incB"), "-I" + path("incA")});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(r.hasOutput("Compiling")) << r.diagnostic();
    EXPECT_EQ(profiles().size(), 1u);
}

TEST_F(ConfigsFixture, CompileFlagsOrderSplitsProfile) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Order of compile flags is significant");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "--compile-flags", "-Wall", "-Wextra"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2", "--compile-flags", "-Wextra", "-Wall"}), "pair 2");
    EXPECT_EQ(profiles().size(), 2u);
}

TEST_F(ConfigsFixture, UnusedProfileIsRemoved) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Profile without pairs is removed");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "-O1"}), "first profile");
    ASSERT_EQ(profiles().size(), 1u);

    auto r = runBelder({"main1.cpp", "-o", "out1", "-O2"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("Compiling main1.cpp")) << r.diagnostic();
    auto left = profiles();
    ASSERT_EQ(left.size(), 1u);
    std::ifstream cfg(belderLastPairConfig(belderProjectDir(tmpDir)));
    std::vector<std::string> lines;
    for (std::string line; std::getline(cfg, line);) lines.push_back(line);
    ASSERT_EQ(lines.size(), 17u);
    EXPECT_EQ(lines[8], "-O2");
    EXPECT_EQ(lines[16], std::filesystem::path(left[0]).filename().string());
    EXPECT_FALSE(std::filesystem::exists(left[0] + "/key"));
}

TEST_F(ConfigsFixture, SharedLibraryGetsSeparateProfile) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Shared library objects need -fPIC");
    write("lib.cpp", "int libfunc(){return 7;}\n");
    write("main.cpp", printingMain("_main_"));

    ASSERT_BELDER_OK(runBelder({"main.cpp", "-o", "out"}), "executable");
    auto r = runBelder({"lib.cpp", "-o", "libLib.so", "-log"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("-fPIC")) << r.diagnostic();
    EXPECT_EQ(profiles().size(), 2u);
}

TEST_F(ConfigsFixture, ForceUnlinkedSourceIsNotCompiled) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Force-unlinked source is skipped by the pair");
    writeTwoMains();
    write("broken.cpp", "this is not C++\n");

    auto r = runBelder({"main1.cpp", "-o", "out1", "--no-link-force", "broken.cpp"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(r.hasOutput("Compiling broken.cpp")) << r.diagnostic();
    EXPECT_TRUE(fileExists("out1")) << r.diagnostic();
}

TEST_F(ConfigsFixture, HeaderChangeReachesSourceSkippedByOtherPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Header change is not lost between pairs");
    write("common.h", "#define VALUE 1\n");
    write("helper.cpp", "#include \"common.h\"\nint helper(){return VALUE;}\n");
    write("main1.cpp", printingMain("_main1_"));
    write("main2.cpp",
        "#include <iostream>\n"
        "int helper();\n"
        "int main(){std::cout << \"value=\" << helper() << std::endl; return 0;}\n");

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1", "--no-link-force", "helper.cpp"}), "pair 1");
    auto r1 = runBelder({"main2.cpp", "-o", "out2", "run"});
    EXPECT_TRUE(r1.hasOutput("value=1")) << r1.diagnostic();
    ASSERT_EQ(profiles().size(), 1u);

    sleep(2);
    write("common.h", "#define VALUE 2\n");

    auto r2 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_FALSE(r2.hasOutput("Compiling helper.cpp")) << r2.diagnostic();

    auto r3 = runBelder({"main2.cpp", "-o", "out2", "run"});
    EXPECT_TRUE(r3.hasOutput("Compiling helper.cpp")) << r3.diagnostic();
    EXPECT_TRUE(r3.hasOutput("value=2")) << r3.diagnostic();
}

TEST_F(ConfigsFixture, PairsWritingSameOutputRelink) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Output overwritten by another pair is relinked");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out"}), "pair 1");
    auto r1 = runBelder({"main2.cpp", "run"});
    EXPECT_TRUE(r1.hasOutput("_main2_")) << r1.diagnostic();

    auto r2 = runBelder({"main1.cpp", "run"});
    EXPECT_FALSE(r2.hasOutput("belder: nothing to link")) << r2.diagnostic();
    EXPECT_TRUE(r2.hasOutput("_main1_")) << r2.diagnostic();
    EXPECT_FALSE(r2.hasOutput("_main2_")) << r2.diagnostic();
}

TEST_F(ConfigsFixture, ClearRemovesAllPairs) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "clear removes every pair");
    writeTwoMains();

    ASSERT_BELDER_OK(runBelder({"main1.cpp", "-o", "out1"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"main2.cpp", "-o", "out2"}), "pair 2");
    std::string buildDir = belderProjectDir(tmpDir);
    ASSERT_FALSE(buildDir.empty());

    ASSERT_BELDER_OK(runBelder({"silent_clear"}), "clear");
    EXPECT_FALSE(std::filesystem::exists(buildDir));
    EXPECT_EQ(belderProjectDir(tmpDir), "");
}

TEST_F(ConfigsFixture, LegacyLayoutIsMigrated) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Old single-config layout is migrated");
    write("main.cpp", printingMain("_main_"));
    ASSERT_BELDER_OK(runBelder({"status"}), "register project");
    std::string buildDir = belderProjectDir(tmpDir);
    ASSERT_FALSE(buildDir.empty());

    for (const auto& entry : std::filesystem::directory_iterator(buildDir))
        std::filesystem::remove_all(entry.path());
    makeDir(buildDir + "/headers/deps");
    makeDir(buildDir + "/outputConfigs");
    std::vector<std::string> legacy(16, "-1");
    legacy[0] = path("main.cpp");
    legacy[1] = "out";
    legacy[5] = "default default";
    legacy[8] = "-O2";
    std::string content;
    for (const auto& line : legacy) content += line + "\n";
    writeFile(buildDir + "/config", content);

    auto s = runBelder({"status"});
    EXPECT_BELDER_OK(s, s.diagnostic());
    EXPECT_TRUE(s.hasOutput("Opt: -O2")) << s.diagnostic();
    EXPECT_TRUE(s.hasOutput("Output file: " + path("out"))) << s.diagnostic();
    EXPECT_FALSE(std::filesystem::exists(buildDir + "/headers"));
    EXPECT_FALSE(std::filesystem::exists(buildDir + "/outputConfigs"));
    EXPECT_FALSE(std::filesystem::exists(buildDir + "/config"));
    EXPECT_TRUE(std::filesystem::exists(buildDir + "/pairs/1/config"));

    auto r = runBelder({"run"});
    EXPECT_TRUE(r.hasOutput("_main_")) << r.diagnostic();
}

class ExcludedSourceFixture : public ConfigsFixture {
protected:
    void writeProject(bool in1UsesFile3) {
        write("file1.cpp", "int file1(){ return 1; }\n");
        write("file2.cpp", "int file2(){ return 2; }\n");
        write("file3.cpp", "int file3(){ return 3; }\n");
        write("in1.cpp",
            "#include <iostream>\n"
            "int file1(); int file2(); int file3();\n"
            "int main(){ std::cout << \"in1=\" << file1() + file2()" +
            std::string(in1UsesFile3 ? " + file3()" : "") + " << std::endl; }\n");
        write("in2.cpp",
            "#include <iostream>\n"
            "int file1();\n"
            "int main(){ std::cout << \"in2=\" << file1() << std::endl; }\n");
    }
};

TEST_F(ExcludedSourceFixture, ChangedSourceExcludedByOtherPairIsNotLost) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "source excluded by one pair and changed");
    writeProject(false);

    auto r1 = runBelder({"in1.cpp", "-o", "out1", "run"});
    EXPECT_BELDER_OK(r1, r1.diagnostic());
    EXPECT_TRUE(r1.hasOutput("Compiling file3.cpp")) << r1.diagnostic();
    EXPECT_TRUE(r1.hasOutput("Linking file: file1.cpp") && r1.hasOutput("Linking file: file2.cpp")) << r1.diagnostic();
    EXPECT_FALSE(r1.hasOutput("Linking file: file3.cpp")) << r1.diagnostic();
    EXPECT_TRUE(r1.hasOutput("in1=3")) << r1.diagnostic();

    auto r2 = runBelder({"in2.cpp", "-o", "out2", "--no-link-force", "file3.cpp", "run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_FALSE(r2.hasOutput("Compiling")) << r2.diagnostic();
    EXPECT_TRUE(r2.hasOutput("in2=1")) << r2.diagnostic();
    EXPECT_EQ(profiles().size(), 1u);

    write("file3.cpp", "int file3(){ return 30; }\n");

    auto r3 = runBelder({"in2.cpp", "-o", "out2"});
    EXPECT_BELDER_OK(r3, r3.diagnostic());
    EXPECT_FALSE(r3.hasOutput("Compiling file3.cpp")) << r3.diagnostic();
    EXPECT_TRUE(r3.hasOutput("nothing to link")) << r3.diagnostic();

    auto r4 = runBelder({"in2.cpp", "-o", "out2"});
    EXPECT_FALSE(r4.hasOutput("Compiling")) << r4.diagnostic();

    auto r5 = runBelder({"in1.cpp", "-o", "out1", "run"});
    EXPECT_BELDER_OK(r5, r5.diagnostic());
    EXPECT_TRUE(r5.hasOutput("Compiling file3.cpp")) << r5.diagnostic();
    EXPECT_TRUE(r5.hasOutput("Linking file: in1.cpp")) << r5.diagnostic();
    EXPECT_FALSE(r5.hasOutput("Linking file: file3.cpp")) << r5.diagnostic();
    EXPECT_TRUE(r5.hasOutput("in1=3")) << r5.diagnostic();

    auto r6 = runBelder({"in1.cpp", "-o", "out1"});
    EXPECT_FALSE(r6.hasOutput("Compiling")) << r6.diagnostic();
}

TEST_F(ExcludedSourceFixture, ChangedSourceNeededByOtherPairIsRelinked) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "excluded source used by other pair");
    writeProject(true);

    auto r1 = runBelder({"in1.cpp", "-o", "out1", "run"});
    EXPECT_TRUE(r1.hasOutput("Linking file: file3.cpp")) << r1.diagnostic();
    EXPECT_TRUE(r1.hasOutput("in1=6")) << r1.diagnostic();
    ASSERT_BELDER_OK(runBelder({"in2.cpp", "-o", "out2", "--no-link-force", "file3.cpp"}), "pair 2");

    write("file3.cpp", "int file3(){ return 30; }\n");
    auto r2 = runBelder({"in2.cpp", "-o", "out2"});
    EXPECT_FALSE(r2.hasOutput("Compiling file3.cpp")) << r2.diagnostic();

    auto r3 = runBelder({"in1.cpp", "-o", "out1", "run"});
    EXPECT_BELDER_OK(r3, r3.diagnostic());
    EXPECT_TRUE(r3.hasOutput("Compiling file3.cpp")) << r3.diagnostic();
    EXPECT_TRUE(r3.hasOutput("Linking file: file3.cpp")) << r3.diagnostic();
    EXPECT_TRUE(r3.hasOutput("in1=33")) << r3.diagnostic();
}

TEST_F(ExcludedSourceFixture, BrokenSourceExcludedByOtherPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "broken source excluded by one pair");
    writeProject(false);
    ASSERT_BELDER_OK(runBelder({"in1.cpp", "-o", "out1"}), "pair 1");
    ASSERT_BELDER_OK(runBelder({"in2.cpp", "-o", "out2", "--no-link-force", "file3.cpp"}), "pair 2");

    write("file3.cpp", "this does not compile\n");
    auto r2 = runBelder({"in2.cpp", "-o", "out2", "run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_TRUE(r2.hasOutput("in2=1")) << r2.diagnostic();

    auto r1 = runBelder({"in1.cpp", "-o", "out1"});
    EXPECT_EQ(r1.exitCode, 2) << r1.diagnostic();

    write("file3.cpp", "int file3(){ return 3; }\n");
    auto r3 = runBelder({"in1.cpp", "-o", "out1", "run"});
    EXPECT_BELDER_OK(r3, r3.diagnostic());
    EXPECT_TRUE(r3.hasOutput("in1=3")) << r3.diagnostic();
    auto r4 = runBelder({"in2.cpp", "-o", "out2"});
    EXPECT_FALSE(r4.hasOutput("Compiling")) << r4.diagnostic();
    EXPECT_TRUE(r4.hasOutput("nothing to link")) << r4.diagnostic();
}

TEST_F(ExcludedSourceFixture, HeaderOfExcludedSourceChangedDuringOtherPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "header of source excluded by one pair");
    writeProject(true);
    write("h3.h", "#define F3 3\n");
    write("file3.cpp", "#include \"h3.h\"\nint file3(){ return F3; }\n");

    auto r1 = runBelder({"in1.cpp", "-o", "out1", "run"});
    EXPECT_TRUE(r1.hasOutput("in1=6")) << r1.diagnostic();
    ASSERT_BELDER_OK(runBelder({"in2.cpp", "-o", "out2", "--no-link-force", "file3.cpp"}), "pair 2");

    write("h3.h", "#define F3 30\n");
    auto r2 = runBelder({"in2.cpp", "-o", "out2"});
    EXPECT_FALSE(r2.hasOutput("Compiling file3.cpp")) << r2.diagnostic();
    auto r3 = runBelder({"in2.cpp", "-o", "out2"});
    EXPECT_FALSE(r3.hasOutput("Compiling")) << r3.diagnostic();

    auto r4 = runBelder({"in1.cpp", "-o", "out1", "run"});
    EXPECT_BELDER_OK(r4, r4.diagnostic());
    EXPECT_TRUE(r4.hasOutput("Compiling file3.cpp")) << r4.diagnostic();
    EXPECT_TRUE(r4.hasOutput("in1=33")) << r4.diagnostic();
}

TEST_F(ConfigsFixture, ConfigChangeWithoutBuildForcesRelink) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "config change invalidates link record");
    write("main.cpp", printingMain("_relink_"));
    ASSERT_BELDER_OK(runBelder({"main.cpp", "-o", "out"}), "first build");
    ASSERT_BELDER_OK(runBelder({"config", "--link-flags", "-s"}), "change link flags only");
    auto r = runBelder({"-log"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(r.hasOutput("nothing to link")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput(" -s ")) << r.diagnostic();
    auto again = runBelder();
    EXPECT_TRUE(again.hasOutput("nothing to link")) << again.diagnostic();
}

TEST_F(ConfigsFixture, LinkRecordHoldsOnlyFileState) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "link record content");
    write("main.cpp", "#include <iostream>\nint helper();\nint main(){ std::cout << helper() << std::endl; }\n");
    write("helper.cpp", "int helper(){ return 1; }\n");
    ASSERT_BELDER_OK(runBelder({"main.cpp", "-o", "out", "-O2", "--compile-flags", "-DMARK"}), "build");
    std::string record = readFile(std::filesystem::path(belderLastPairConfig(belderProjectDir(tmpDir))).parent_path().string() + "/link");
    std::vector<std::string> lines;
    std::istringstream in(record);
    for (std::string line; std::getline(in, line);) lines.push_back(line);
    ASSERT_EQ(lines.size(), 4u) << record;
    EXPECT_EQ(record.find("-O2"), std::string::npos) << record;
    EXPECT_EQ(record.find("-DMARK"), std::string::npos) << record;
    EXPECT_EQ(lines[1], "2") << record;
    EXPECT_NE(lines[2].find(".o "), std::string::npos) << record;
    EXPECT_NE(lines[3].find(".o "), std::string::npos) << record;
}

TEST_F(ConfigsFixture, NewPairDoesNotSearchEntryInIncludeDirsOfOtherPair) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "entry lookup for a new pair");
    std::string ext = tmpDir + "_ext";
    makeDir(ext);
    writeFile(ext + "/extmain.cpp", printingMain("_extmain_"));
    write("main.cpp", printingMain("_local_"));
    ASSERT_BELDER_OK(runBelder({"main.cpp", "-o", "o", "-I" + ext}), "pair with -I");

    auto r1 = runBelder({"extmain.cpp", "-o", "o2", "run"});
    EXPECT_EQ(r1.exitCode, 1) << r1.diagnostic();
    EXPECT_TRUE(r1.hasOutput("Cannot find file: extmain.cpp")) << r1.diagnostic();
    auto s1 = runBelder({"status"});
    EXPECT_FALSE(s1.hasOutput("extmain.cpp")) << s1.diagnostic();

    auto r2 = runBelder({"extmain.cpp", "-o", "o2", "-I" + ext, "run"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    EXPECT_TRUE(r2.hasOutput("_extmain_")) << r2.diagnostic();
    std::filesystem::remove_all(ext);
}
