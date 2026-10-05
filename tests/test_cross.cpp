#include "helpers.h"

namespace {

const std::string MATH_C =
    "#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n"
    "int main(void){ double* v = malloc(4 * sizeof(double)); for(int i = 0; i < 4; ++i) v[i] = sin(i * 0.5);\n"
    "printf(\"_math_%.0f\\n\", v[3] * 100); free(v); return 0; }\n";

bool armToolchain() {
    return toolExists("arm-none-eabi-gcc") && std::filesystem::exists("/usr/lib/arm-none-eabi");
}

bool riscvLinuxToolchain() {
    return toolExists("riscv64-linux-gnu-gcc") && std::filesystem::exists("/usr/riscv64-linux-gnu/lib") &&
           std::filesystem::exists("/proc/sys/fs/binfmt_misc/qemu-riscv64");
}

}

class CrossFixture : public BelderFixture {
protected:
    std::string srcDir;

    void SetUp() override {
        BelderFixture::SetUp();
        srcDir = tmpDir + "_crosssrc";
        makeDir(srcDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(srcDir);
        BelderFixture::TearDown();
    }

    void archiveWith(const std::string& compiler, const std::string& flags, const std::string& ar,
                     const std::string& rel, const std::string& name, const std::string& code) {
        std::string src = srcDir + "/" + name + ".c";
        std::string obj = srcDir + "/" + name + ".o";
        writeFile(src, code);
        std::filesystem::create_directories(std::filesystem::path(path(rel)).parent_path());
        auto r = runCommand(compiler + " " + flags + " -c '" + src + "' -o '" + obj + "' && " + ar + " rcs '" + path(rel) + "' '" + obj + "'");
        ASSERT_EQ(r.exitCode, 0) << r.diagnostic();
    }

    BelderResult belderWithEnv(const std::string& env, const std::vector<std::string>& args) {
        std::string cmd = env + " " + std::string(BELDER_BINARY) + " -C '" + tmpDir + "'";
        for (const auto& a : args) cmd += " '" + a + "'";
        return runCommand(cmd);
    }
};

TEST_F(CrossFixture, ArmCortexM4TakesLibmFromItsMultilib) {
    if (!armToolchain()) GTEST_SKIP() << "arm-none-eabi toolchain";
    write("main.c", MATH_C);
    auto r = runBelder({"main.c", "-o", "fw.elf", "--CC", "arm-none-eabi-gcc", "-mcpu=cortex-m4", "-mthumb",
                        "-I/usr/lib/arm-none-eabi", "-log", "--link-flags", "--specs=nosys.specs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("thumb/v7e-m/nofp/libm.a")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("/usr/lib/arm-none-eabi/lib/libm.a")) << r.diagnostic();
}

TEST_F(CrossFixture, ArmHardFloatTakesHardFloatMultilib) {
    if (!armToolchain()) GTEST_SKIP() << "arm-none-eabi toolchain";
    write("main.c", MATH_C);
    auto r = runBelder({"main.c", "-o", "fw.elf", "--CC", "arm-none-eabi-gcc", "-mcpu=cortex-m4", "-mthumb",
                        "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-I/usr/lib/arm-none-eabi", "-log",
                        "--link-flags", "--specs=nosys.specs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("thumb/v7e-m+fp/hard/libm.a")) << r.diagnostic();
}

TEST_F(CrossFixture, LibraryOfOtherArchitectureIsSkipped) {
    if (!armToolchain() || !toolExists("gcc")) GTEST_SKIP() << "arm-none-eabi toolchain";
    const std::string code = "int dual_value(void){ return 42; }\n";
    archiveWith("gcc", "", "ar", "libs/a_host/libdual.a", "dual_host", code);
    archiveWith("arm-none-eabi-gcc", "-mcpu=cortex-m4 -mthumb", "arm-none-eabi-ar", "libs/b_arm/libdual.a", "dual_arm", code);
    write("main.c", "int dual_value(void);\nint main(void){ return dual_value(); }\n");
    auto r = runBelder({"main.c", "-o", "fw.elf", "--CC", "arm-none-eabi-gcc", "-mcpu=cortex-m4", "-mthumb", "-log",
                        "--link-flags", "--specs=nosys.specs"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("b_arm/libdual.a")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("a_host/libdual.a")) << r.diagnostic();
}

TEST_F(CrossFixture, ArmLinkerScriptThroughLinkFlags) {
    if (!armToolchain()) GTEST_SKIP() << "arm-none-eabi toolchain";
    write("fw.ld", "MEMORY { FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K\nRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 128K }\n"
                   "ENTRY(Reset_Handler)\n_estack = ORIGIN(RAM) + LENGTH(RAM);\n"
                   "SECTIONS { .isr_vector : { KEEP(*(.isr_vector)) } > FLASH\n .text : { *(.text*) *(.rodata*) } > FLASH\n"
                   " .bss : { *(.bss*) *(COMMON) } > RAM }\n");
    write("startup.c", "extern unsigned long _estack;\nint main(void);\n"
                       "void Reset_Handler(void){ main(); for(;;); }\n"
                       "__attribute__((section(\".isr_vector\"), used)) void (*const vectors[])(void) = {\n"
                       "  (void (*)(void))&_estack, Reset_Handler };\n");
    write("main.c", "int main(void){ for(;;); }\n");
    auto r = runBelder({"main.c", "-o", "fw.elf", "--CC", "arm-none-eabi-gcc", "-mcpu=cortex-m4", "-mthumb",
                        "--link-force", "startup.c", "--link-flags", "-T", path("fw.ld"), "-nostartfiles", "-nostdlib"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    auto sections = runCommand("arm-none-eabi-readelf -S '" + path("fw.elf") + "'");
    EXPECT_TRUE(sections.hasOutput(".isr_vector")) << sections.diagnostic();
    EXPECT_TRUE(sections.hasOutput("08000000")) << sections.diagnostic();
}

TEST_F(CrossFixture, HostLinkerScriptAndPreprocessorFlagInsideFlagSections) {
    if (!toolExists("g++") || !toolExists("ld")) GTEST_SKIP() << "host toolchain";
    auto script = runCommand("ld --verbose");
    ASSERT_EQ(script.exitCode, 0) << script.diagnostic();
    size_t first = script.stdout_str.find("==================================================");
    ASSERT_NE(first, std::string::npos);
    size_t begin = script.stdout_str.find('\n', first) + 1;
    size_t end = script.stdout_str.find("==================================================", begin);
    ASSERT_NE(end, std::string::npos);
    write("host.ld", script.stdout_str.substr(begin, end - begin));
    write("main.cpp", "#include <iostream>\nint main(){ std::cout << \"_script_\" << std::endl; }\n");
    auto r = runBelder({"--compile-flags", "-C", "--link-flags", "-T", path("host.ld"), "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("_script_")) << r.diagnostic();
    auto threads = runBelder({"-T", "2", "run"});
    EXPECT_BELDER_OK(threads, threads.diagnostic());
}

TEST_F(CrossFixture, RiscvLinuxMathLibraryAndRunUnderQemu) {
    if (!riscvLinuxToolchain()) GTEST_SKIP() << "riscv64-linux-gnu toolchain with qemu";
    write("main.c", MATH_C);
    auto r = belderWithEnv("QEMU_LD_PREFIX=/usr/riscv64-linux-gnu",
                           {"main.c", "-o", "rv", "--CC", "riscv64-linux-gnu-gcc", "-I/usr/riscv64-linux-gnu/lib", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("Linking lib: libm")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_math_100")) << r.diagnostic();
}

TEST_F(CrossFixture, RiscvLinuxIgnoresHostLibrariesInIncludeDirs) {
    if (!riscvLinuxToolchain() || !std::filesystem::exists("/usr/lib/x86_64-linux-gnu/libm.so.6"))
        GTEST_SKIP() << "riscv64-linux-gnu toolchain with qemu";
    write("main.c", MATH_C);
    auto r = belderWithEnv("QEMU_LD_PREFIX=/usr/riscv64-linux-gnu",
                           {"main.c", "-o", "rv", "--CC", "riscv64-linux-gnu-gcc", "-I/usr/lib/x86_64-linux-gnu",
                            "-I/usr/riscv64-linux-gnu/lib", "-log", "run"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_FALSE(r.hasOutput("/usr/lib/x86_64-linux-gnu/libm")) << r.diagnostic();
    EXPECT_TRUE(r.hasOutput("_math_100")) << r.diagnostic();
}

TEST_F(CrossFixture, RiscvBareMetalTakesLibraryOfItsWordSize) {
    if (!toolExists("riscv64-unknown-elf-gcc") || !std::filesystem::exists("/usr/lib/picolibc/riscv64-unknown-elf"))
        GTEST_SKIP() << "riscv64-unknown-elf toolchain with picolibc";
    const std::string code = "int rvd(void){ return 3; }\n";
    archiveWith("riscv64-unknown-elf-gcc", "-mcmodel=medany", "riscv64-unknown-elf-ar", "libs/a64/librvd.a", "rvd64", code);
    archiveWith("riscv64-unknown-elf-gcc", "-march=rv32imac -mabi=ilp32", "riscv64-unknown-elf-ar", "libs/b32/librvd.a", "rvd32", code);
    write("main.c", "int rvd(void);\nint main(void){ return rvd(); }\n");
    auto r = runBelder({"main.c", "-o", "fw.elf", "--CC", "riscv64-unknown-elf-gcc", "-march=rv32imac", "-mabi=ilp32",
                        "--specs=picolibc.specs", "--oslib=semihost", "-log"});
    EXPECT_BELDER_OK(r, r.diagnostic());
    EXPECT_TRUE(r.hasOutput("b32/librvd.a")) << r.diagnostic();
    EXPECT_FALSE(r.hasOutput("a64/librvd.a")) << r.diagnostic();
}

TEST_F(CrossFixture, UnresolvedIncludeDoesNotWaitForInput) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++";
    write("a/b/x.h", "int x();\n");
    write("main.cpp", "#include \"../../../../../q/x.h\"\nint main(){ return 0; }\n");
    std::string fifo = srcDir + "/stdin_fifo";
    auto r = runCommand("mkfifo '" + fifo + "' && timeout 20 " + std::string(BELDER_BINARY) + " -C '" + tmpDir + "' 0<>'" + fifo + "'");
    EXPECT_NE(r.exitCode, 124) << "belder waited for input" << r.diagnostic();
}
