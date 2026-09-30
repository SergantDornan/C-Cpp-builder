#include "helpers.h"
#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <sys/file.h>
#include <thread>

namespace {

std::string printingMain(const std::string& text) {
    return "#include <iostream>\n"
           "int main(){std::cout << \"" + text + "\" << std::endl; return 0;}\n";
}

int holdLock(const std::string& path) {
    int fd = open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    if (fd >= 0) flock(fd, LOCK_EX);
    return fd;
}

}

class LockFixture : public BelderFixture {
protected:
    std::vector<std::string> extraDirs;

    std::string extraDir() {
        char tmpl[] = "/tmp/btest_lock_XXXXXX";
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

    void expectWaitsForLock(const std::string& lockPath, const std::string& projectDir,
                            const std::vector<std::string>& args) {
        int fd = holdLock(lockPath);
        ASSERT_GE(fd, 0) << lockPath;
        std::atomic<bool> done{false};
        BelderResult result;
        std::string cmd = std::string(BELDER_BINARY);
        for (const auto& a : args) cmd += " " + a;
        std::thread worker([&]{ result = runCommandInDir(cmd, projectDir); done = true; });
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        EXPECT_FALSE(done) << "belder did not wait for " << lockPath;
        close(fd);
        worker.join();
        EXPECT_TRUE(done);
        EXPECT_BELDER_OK(result, result.diagnostic());
    }
};

TEST_F(LockFixture, BuildWaitsForProjectLock) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "project lock");
    write("main.cpp", printingMain("_lock_"));
    ASSERT_BELDER_OK(runBelder({"status"}), "register project");
    std::string buildDir = belderProjectDir(tmpDir);
    ASSERT_FALSE(buildDir.empty());
    expectWaitsForLock(buildDir + "/.lock", tmpDir, {});
    EXPECT_TRUE(fileExists("out"));
}

TEST_F(LockFixture, StatusWaitsForProjectLock) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "project lock for status");
    write("main.cpp", printingMain("_lock_"));
    ASSERT_BELDER_OK(runBelder({"status"}), "register project");
    expectWaitsForLock(belderProjectDir(tmpDir) + "/.lock", tmpDir, {"status"});
}

TEST_F(LockFixture, NewProjectWaitsForRegistryLock) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "registry lock");
    write("main.cpp", printingMain("_reg_"));
    expectWaitsForLock(std::string(getenv("HOME")) + "/builder/.lock", tmpDir, {"status"});
    EXPECT_FALSE(belderProjectDir(tmpDir).empty());
}

TEST_F(LockFixture, ClearWaitsForRegistryLock) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "registry lock for clear");
    write("main.cpp", printingMain("_clr_"));
    ASSERT_BELDER_OK(runBelder({"status"}), "register project");
    expectWaitsForLock(std::string(getenv("HOME")) + "/builder/.lock", tmpDir, {"silent_clear"});
    EXPECT_EQ(belderProjectDir(tmpDir), "");
}

TEST_F(LockFixture, OtherProjectDoesNotWaitForProjectLock) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "project locks are independent");
    write("main.cpp", printingMain("_a_"));
    ASSERT_BELDER_OK(runBelder({"status"}), "register project");
    std::string other = extraDir();
    writeFile(other + "/main.cpp", printingMain("_b_"));
    int fd = holdLock(belderProjectDir(tmpDir) + "/.lock");
    ASSERT_GE(fd, 0);
    auto r = runCommandInDir(std::string(BELDER_BINARY) + " run", other);
    close(fd);
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_b_")) << r.diagnostic();
}

TEST_F(LockFixture, LeftoverLockFileDoesNotBlock) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "leftover lock files");
    write("main.cpp", printingMain("_left_"));
    ASSERT_BELDER_OK(runBelder({"status"}), "register project");
    writeFile(belderProjectDir(tmpDir) + "/.lock", "garbage\n");
    writeFile(std::string(getenv("HOME")) + "/builder/.lock", "garbage\n");
    auto r = runCommand("timeout 30 " + std::string(BELDER_BINARY) + " -C '" + tmpDir + "' run");
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_left_")) << r.diagnostic();
}

TEST_F(LockFixture, LockIsReleasedBeforeRun) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "lock released before run");
    write("main.cpp",
        "#include <cstdlib>\n"
        "#include <iostream>\n"
        "int main(){\n"
        "  int code = std::system(\"" + std::string(BELDER_BINARY) + " -C " + tmpDir + " status > /dev/null\");\n"
        "  std::cout << \"_inner_\" << code << std::endl;\n"
        "  return 0;\n"
        "}\n");
    auto r = runCommand("timeout 30 " + std::string(BELDER_BINARY) + " -C '" + tmpDir + "' run");
    EXPECT_NE(r.exitCode, 124) << "belder deadlocked on its own project lock" << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_inner_0")) << r.diagnostic();
}

TEST_F(LockFixture, ParallelBuildsOfOneProjectStayConsistent) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "parallel builds in one project");
    for (int i = 0; i < 30; ++i)
        write("src/f" + std::to_string(i) + ".cpp", "int f" + std::to_string(i) + "(){ return " + std::to_string(i) + "; }\n");
    write("common.h", "#define TAG \"_v1_\"\n");
    for (int i = 0; i < 4; ++i)
        write("m" + std::to_string(i) + ".cpp",
            "#include \"common.h\"\n#include <iostream>\nint f" + std::to_string(i) + "();\n"
            "int main(){ std::cout << TAG << f" + std::to_string(i) + "() << std::endl; }\n");

    for (int round = 0; round < 2; ++round) {
        if (round == 1) write("common.h", "#define TAG \"_v2_\"\n");
        std::vector<std::thread> threads;
        std::vector<BelderResult> results(8);
        for (int i = 0; i < 8; ++i) {
            std::vector<std::string> args = {"m" + std::to_string(i % 4) + ".cpp", "-o", "o" + std::to_string(i % 4)};
            if (i % 4 >= 2) args.push_back("-O2");
            threads.emplace_back([this, args, &results, i]{ results[i] = runBelder(args); });
        }
        for (auto& t : threads) t.join();
        for (const auto& r : results) EXPECT_NE(r.exitCode, -1) << r.diagnostic();

        std::string tag = (round == 0) ? "_v1_" : "_v2_";
        for (int i = 0; i < 4; ++i) {
            auto r = runBelder({"m" + std::to_string(i) + ".cpp", "-o", "o" + std::to_string(i), "run"});
            EXPECT_BELDER_OK(r, r.diagnostic());
            EXPECT_TRUE(r.hasOutput("nothing to link")) << r.diagnostic();
            EXPECT_TRUE(r.hasOutput(tag + std::to_string(i))) << r.diagnostic();
        }
    }
    EXPECT_EQ(belderProfileDirs(belderProjectDir(tmpDir)).size(), 2u);
}
