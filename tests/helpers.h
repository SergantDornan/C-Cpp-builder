#pragma once

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <thread>

#ifndef BELDER_BINARY
#define BELDER_BINARY "./belder"
#endif

// ---------------------------------------------------------------------------
// BelderResult: captures command output with full provenance for diagnostics
// ---------------------------------------------------------------------------
struct BelderResult {
    int exitCode  = 0;
    int raw_status = 0;       // raw status from waitpid
    std::string cmd;          // exact shell command that was executed
    std::string cwd;          // working directory used for the command
    std::string stdout_str;
    std::string stderr_str;

    std::string combined() const { return stdout_str + stderr_str; }
    bool success() const { return exitCode == 0; }
    bool hasOutput(const std::string& s) const {
        return stdout_str.find(s) != std::string::npos ||
               stderr_str.find(s) != std::string::npos;
    }

    // Format a multi-section diagnostic report for use in assertion messages
    std::string diagnostic(const std::string& description = "") const {
        std::ostringstream oss;
        oss << "\n" << std::string(60, '=') << "\n";
        if (!description.empty())
            oss << "  Description : " << description << "\n";
        oss << "  Command     : " << cmd << "\n";
        oss << "  CWD         : " << cwd << "\n";
        oss << "  Exit code   : " << exitCode << "\n";
        oss << std::string(60, '-') << "\n";
        oss << "  STDOUT:\n";
        if (stdout_str.empty())
            oss << "  (empty)\n";
        else {
            oss << stdout_str;
            if (stdout_str.back() != '\n') oss << "\n";
        }
        oss << std::string(60, '-') << "\n";
        oss << "  STDERR:\n";
        if (stderr_str.empty())
            oss << "  (empty)\n";
        else {
            oss << stderr_str;
            if (stderr_str.back() != '\n') oss << "\n";
        }
        oss << std::string(60, '=') << "\n";
        return oss.str();
    }
};

// ---------------------------------------------------------------------------
// runCommandInDir: run a shell command in an explicit working directory.
// Uses fork/exec with pipes to capture stdout/stderr reliably without temp
// files, and sets the child's cwd via chdir() before exec.
// ---------------------------------------------------------------------------
inline BelderResult runCommandInDir(const std::string& cmd,
                                    const std::string& cwd = "") {
    BelderResult result;
    result.cmd = cmd;
    result.cwd = cwd.empty() ? "." : cwd;

    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        result.exitCode  = -1;
        result.stderr_str = std::string("pipe() failed: ") + strerror(errno);
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        result.exitCode  = -1;
        result.stderr_st r= std::string("fork() failed: ") + strerror(errno);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child: wire up pipes, set cwd, exec the shell command
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        if (!cwd.empty() && chdir(cwd.c_str()) != 0) {
            const char msg[] = "chdir failed\n";
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            (void)::write(STDERR_FILENO, msg, sizeof(msg) - 1);
            _exit(127);
        }
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127);
    }

    // Parent: close write ends, read both pipes concurrently
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    std::string out_str, err_str;
    auto readFd = [](int fd) -> std::string {
        std::string buf;
        char tmp[4096];
        ssize_t n;
        while ((n = read(fd, tmp, sizeof(tmp))) > 0)
            buf.append(tmp, static_cast<size_t>(n));
        close(fd);
        return buf;
    };
    std::thread t_out([&]() { out_str = readFd(stdout_pipe[0]); });
    std::thread t_err([&]() { err_str = readFd(stderr_pipe[0]); });
    t_out.join();
    t_err.join();

    int status = 0;
    waitpid(pid, &status, 0);
    result.raw_status  = status;
    result.exitCode    = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    result.stdout_str  = std::move(out_str);
    result.stderr_str  = std::move(err_str);
    return result;
}

// Convenience wrapper: run in the current working directory
inline BelderResult runCommand(const std::string& cmd) {
    return runCommandInDir(cmd);
}

// ---------------------------------------------------------------------------
// toolExists: check whether an executable is available on PATH
// ---------------------------------------------------------------------------
inline bool toolExists(const std::string& name) {
    return system(("which " + name + " >/dev/null 2>&1").c_str()) == 0;
}

// ---------------------------------------------------------------------------
// missingToolsMessage: build a detailed, actionable skip message listing
// which tools are absent, how to install them, and how to rerun the test.
// Returns an empty string when all tools are present.
// ---------------------------------------------------------------------------
inline std::string missingToolsMessage(const std::vector<std::string>& tools,
                                       const std::string& purpose) {
    std::vector<std::string> missing;
    for (const auto& t : tools)
        if (!toolExists(t)) missing.push_back(t);
    if (missing.empty()) return "";

    // Map tool names to package names on common distros
    auto pkgName = [](const std::string& tool,
                      const std::string& distro) -> std::string {
        if (tool == "clang++" || tool == "clang") return "clang";
        if (tool == "g++") {
            if (distro == "fedora") return "gcc-c++";
            return "g++";
        }
        if (tool == "gcc") return "gcc";
        if (tool == "ar")  return "binutils";
        return tool;
    };

    std::string deb_pkgs, dnf_pkgs, pac_pkgs;
    std::string verify_cmds;
    for (const auto& t : missing) {
        if (!deb_pkgs.empty()) { deb_pkgs += " "; dnf_pkgs += " "; pac_pkgs += " "; }
        deb_pkgs += pkgName(t, "debian");
        dnf_pkgs += pkgName(t, "fedora");
        pac_pkgs += pkgName(t, "arch");
        verify_cmds += "  which " + t + " && " + t + " --version\n";
    }

    std::ostringstream oss;
    oss << "\n" << std::string(60, '=') << "\n";
    oss << "SKIPPED: " << purpose << "\n";
    oss << std::string(60, '-') << "\n";

    oss << "Missing tool(s): ";
    for (size_t i = 0; i < missing.size(); ++i) {
        if (i) oss << ", ";
        oss << missing[i];
    }
    oss << "\n\n";

    oss << "HOW TO INSTALL:\n";
    oss << "  Debian/Ubuntu : sudo apt-get install " << deb_pkgs << "\n";
    oss << "  Fedora/RHEL   : sudo dnf install "     << dnf_pkgs << "\n";
    oss << "  Arch Linux    : sudo pacman -S "        << pac_pkgs << "\n\n";

    oss << "VERIFY INSTALLATION:\n" << verify_cmds << "\n";

    oss << "RERUN THIS TEST:\n";
    const auto* ti = testing::UnitTest::GetInstance()->current_test_info();
    if (ti)
        oss << "  ./tests/test_runner --gtest_filter="
            << ti->test_suite_name() << "." << ti->name() << "\n";
    else
        oss << "  ./tests/test_runner --gtest_filter=<TestSuite.TestName>\n";

    oss << std::string(60, '=') << "\n";
    return oss.str();
}

// Macro: skip the current test with detailed instructions if any listed tool
// is missing from PATH.
//   REQUIRE_TOOLS_OR_SKIP({"clang++", "clang"}, "reason string");
#define REQUIRE_TOOLS_OR_SKIP(tools, purpose)                        \
    do {                                                             \
        auto _skip_msg_ = missingToolsMessage(tools, purpose);       \
        if (!_skip_msg_.empty()) { GTEST_SKIP() << _skip_msg_; }     \
    } while (false)

// ---------------------------------------------------------------------------
// Assertion macros: print full diagnostic report (cmd/cwd/exit/stdout/stderr)
// when the assertion fails.
// ---------------------------------------------------------------------------
#define EXPECT_BELDER_OK(result, description) \
    EXPECT_EQ((result).exitCode, 0) << (result).diagnostic(description)

#define ASSERT_BELDER_OK(result, description) \
    ASSERT_EQ((result).exitCode, 0) << (result).diagnostic(description)

// ---------------------------------------------------------------------------
// File utilities
// ---------------------------------------------------------------------------

// Write content to a file, creating parent directories as needed
inline void writeFile(const std::string& path, const std::string& content) {
    std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path());
    std::ofstream f(path);
    f << content;
}

// Touch a file to update its modification time
inline void touchFile(const std::string& path) {
    system(("touch " + path).c_str());
}

// Create a directory (and all parents)
inline void makeDir(const std::string& path) {
    std::filesystem::create_directories(path);
}

// Read file content
inline std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    return std::string(std::istreambuf_iterator<char>(f), {});
}

// ---------------------------------------------------------------------------
// BelderFixture: creates a unique temp dir and runs belder in it
// ---------------------------------------------------------------------------
class BelderFixture : public ::testing::Test {
public:
    std::string tmpDir;

    void SetUp() override {
        // Ensure ~/builder exists
        const char* home = getenv("HOME");
        if (home) {
            std::string builderDir = std::string(home) + "/builder";
            std::filesystem::create_directories(builderDir);
        }
        // Create unique temp project dir
        char tmpl[] = "/tmp/btest_XXXXXX";
        char* d = mkdtemp(tmpl);
        ASSERT_NE(d, nullptr) << "mkdtemp failed";
        tmpDir = d;
    }

    void TearDown() override {
        if (!tmpDir.empty()) {
            // Clean belder state for this project
            runBelder({"silent_clear"});
            // Remove temp dir
            std::filesystem::remove_all(tmpDir);
        }
    }

    // Run belder in tmpDir with given extra args.
    // The result includes cmd and cwd for diagnostic reporting.
    BelderResult runBelder(const std::vector<std::string>& args = {}) {
        std::string cmd = std::string(BELDER_BINARY) + " -C '" + tmpDir + "'";
        for (const auto& a : args) {
            if (a.find(' ') != std::string::npos)
                cmd += " '" + a + "'";
            else
                cmd += " " + a;
        }
        return runCommandInDir(cmd, tmpDir);
    }

    // Run belder with a custom working directory (for testing -C itself).
    // cwd is set directly in the child process via chdir().
    BelderResult runBelderFrom(const std::string& cwd,
                               const std::vector<std::string>& args = {}) {
        std::string cmd = std::string(BELDER_BINARY);
        for (const auto& a : args) {
            if (a.find(' ') != std::string::npos)
                cmd += " '" + a + "'";
            else
                cmd += " " + a;
        }
        return runCommandInDir(cmd, cwd);
    }

    // Convenience: write a file relative to tmpDir
    void write(const std::string& relPath, const std::string& content) {
        writeFile(tmpDir + "/" + relPath, content);
    }

    // Return full path for a relative path within tmpDir
    std::string path(const std::string& rel) const {
        return tmpDir + "/" + rel;
    }

    bool fileExists(const std::string& relPath) const {
        return std::filesystem::exists(tmpDir + "/" + relPath);
    }
};

// ---------------------------------------------------------------------------
// Minimal source file generators for use in tests
// ---------------------------------------------------------------------------

inline std::string simpleCppMain() {
    return "#include <iostream>\nint main(){std::cout<<\"ok\"<<std::endl;return 0;}\n";
}

inline std::string simpleCMain() {
    return "#include <stdio.h>\nint main(){printf(\"ok\\n\");return 0;}\n";
}

inline std::string simpleCppFunc(const std::string& name) {
    return "int " + name + "(){return 42;}\n";
}

inline std::string simpleCppDecl(const std::string& name) {
    return "int " + name + "();\n";
}
