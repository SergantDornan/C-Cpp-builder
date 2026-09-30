#include "helpers.h"
#include <fstream>

// ---------------------------------------------------------------------------
// Тесты устойчивости belder к повреждённым служебным файлам (config, .sym)
// и к инъекции команд через имя выходного файла.
//
// belder доверяет собственным файлам кэша/конфига в ~/builder. Но запись в них
// неатомарна: прерывание belder, переполнение диска или ручная правка оставляют
// усечённый/битый файл. На следующем запуске это раньше приводило к выходу за
// границы вектора (SIGSEGV) или необработанному std::stoul (Aborted).
//
// Краш ловится через exitCode == -1 (процесс убит сигналом - см. helpers.h).
// ---------------------------------------------------------------------------

namespace {

// Найти каталог сборки проекта внутри ~/builder по записи "cwd*N" в общем config.
std::string belderBuildDir(const std::string& projectDir) {
    const char* home = getenv("HOME");
    if (!home) return "";
    std::ifstream cfg(std::string(home) + "/builder/config");
    std::string line;
    while (std::getline(cfg, line)) {
        auto star = line.rfind('*');
        if (star == std::string::npos) continue;
        if (line.substr(0, star) == projectDir)
            return std::string(home) + "/builder/" + line.substr(star + 1);
    }
    return "";
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream f(path);
    std::string l;
    while (std::getline(f, l)) lines.push_back(l);
    return lines;
}

void writeLines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream f(path, std::ios::trunc);
    for (const auto& l : lines) f << l << "\n";
}

} // namespace

// ===========================================================================
// Инъекция команд через имя выходного файла (-o)
//
// Пейлоады содержат пробел: runBelder() обернёт такой аргумент в одинарные
// кавычки, поэтому ВНЕШНИЙ sh -c не выполнит метасимволы сам - проверяется
// именно обработка внутри belder. Sentinel-файл не должен появиться.
// ===========================================================================

class InjectionFixture : public BelderFixture {
protected:
    void expectInjectionRejected(const std::string& payload,
                                 const std::string& description) {
        REQUIRE_TOOLS_OR_SKIP({"g++"}, description);
        write("main.cpp", "int main(){return 0;}\n");
        std::string sentinel = tmpDir + "/PWNED";
        // payload содержит маркер %S, заменяем его на путь sentinel
        std::string out = payload;
        auto pos = out.find("%S");
        if (pos != std::string::npos) out.replace(pos, 2, sentinel);

        auto result = runBelder({"main.cpp", "-o", out});

        EXPECT_NE(result.exitCode, 0)
            << "belder should REJECT a dangerous output name, not build it"
            << result.diagnostic(description);
        EXPECT_FALSE(std::filesystem::exists(sentinel))
            << "COMMAND INJECTION EXECUTED via output name: " << out
            << result.diagnostic(description);
    }
};

TEST_F(InjectionFixture, RejectsSemicolonInOutputName) {
    expectInjectionRejected("out; touch %S",
        "Output name with ';command' must not be executed");
}

TEST_F(InjectionFixture, RejectsSubshellInOutputName) {
    expectInjectionRejected("a $(touch %S) b",
        "Output name with $(command) must not be executed");
}

TEST_F(InjectionFixture, RejectsBacktickInOutputName) {
    expectInjectionRejected("a `touch %S` b",
        "Output name with `command` must not be executed");
}

TEST_F(InjectionFixture, AcceptsNormalOutputNames) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Normal output names must still build");
    write("main.cpp", "int main(){return 0;}\n");
    auto r1 = runBelder({"main.cpp", "-o", "my_app-v2"});
    EXPECT_BELDER_OK(r1, "plain alphanumeric/._- output name");
    EXPECT_TRUE(fileExists("my_app-v2"));
}

// ===========================================================================
// Повреждённый config проекта
// ===========================================================================

TEST_F(BelderFixture, TruncatedProjectConfigDoesNotCrash) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Truncated config must not crash belder");
    write("main.cpp", simpleCppMain());
    ASSERT_BELDER_OK(runBelder({"main.cpp", "-o", "out"}), "initial build");

    std::string bdir = belderBuildDir(tmpDir);
    ASSERT_FALSE(bdir.empty()) << "could not locate build dir under ~/builder";

    // Обрезаем config до одной строки (имитация недописанной записи).
    writeLines(belderLastPairConfig(bdir), {"-1"});

    auto r = runBelder({"main.cpp", "-o", "out"});
    EXPECT_NE(r.exitCode, -1)
        << "belder crashed building with a truncated config" << r.diagnostic();

    // status тоже ходит через чтение config.
    writeLines(belderLastPairConfig(bdir), {"-1"});
    auto rs = runBelder({"status"});
    EXPECT_NE(rs.exitCode, -1)
        << "belder crashed on 'status' with a truncated config" << rs.diagnostic();
}

TEST_F(BelderFixture, MalformedCompilersFieldDoesNotCrash) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Malformed compilers field must not crash belder");
    write("main.cpp", simpleCppMain());
    ASSERT_BELDER_OK(runBelder({"main.cpp", "-o", "out"}), "initial build");

    std::string bdir = belderBuildDir(tmpDir);
    ASSERT_FALSE(bdir.empty()) << "could not locate build dir under ~/builder";

    auto lines = readLines(belderLastPairConfig(bdir));
    ASSERT_GE(lines.size(), 6u);
    // Поле компиляторов (индекс 5 = 6-я строка) должно содержать 2 токена.
    // Ломаем: оставляем один токен -> split(...)[1] раньше падал.
    lines[5] = "default";
    writeLines(belderLastPairConfig(bdir), lines);

    auto r = runBelder({"main.cpp", "-o", "out"});
    EXPECT_NE(r.exitCode, -1)
        << "belder crashed with a single-token compilers field" << r.diagnostic();
    auto rs = runBelder({"status"});
    EXPECT_NE(rs.exitCode, -1)
        << "belder crashed on 'status' with malformed compilers field" << rs.diagnostic();
}

// ===========================================================================
// Повреждённый .sym-кэш
// ===========================================================================

TEST_F(BelderFixture, CorruptSymCacheDoesNotCrash) {
    REQUIRE_TOOLS_OR_SKIP({"g++"}, "Corrupt .sym cache must not crash belder");
    write("main.cpp", simpleCppMain());
    ASSERT_BELDER_OK(runBelder({"main.cpp", "-o", "out"}), "initial build");

    std::string bdir = belderBuildDir(tmpDir);
    ASSERT_FALSE(bdir.empty()) << "could not locate build dir under ~/builder";

    auto profiles = belderProfileDirs(bdir);
    ASSERT_EQ(profiles.size(), 1u);
    std::string symDir = profiles[0] + "/sym";
    ASSERT_TRUE(std::filesystem::exists(symDir));

    // Портим каждый .sym, СОХРАНЯЯ первые две строки (путь + время изменения),
    // чтобы файл прошёл проверку updateSymfiles и дошёл до readSymfile, где
    // счётчик символов парсится через std::stoul. Нечисловой счётчик раньше
    // бросал необработанное исключение (Aborted).
    bool patchedAny = false;
    for (const auto& entry : std::filesystem::directory_iterator(symDir)) {
        auto lines = readLines(entry.path().string());
        if (lines.size() < 2) continue;
        writeLines(entry.path().string(),
                   {lines[0], lines[1], "GARBAGE_NOT_A_NUMBER", "0"});
        patchedAny = true;
    }
    ASSERT_TRUE(patchedAny) << "no .sym files to corrupt";

    auto r = runBelder({"main.cpp", "-o", "out"});
    EXPECT_NE(r.exitCode, -1)
        << "belder crashed reading a corrupt .sym cache" << r.diagnostic();
    // Битый кэш должен быть перечитан из объектника, сборка - успешна.
    EXPECT_BELDER_OK(r, "build recovers from corrupt .sym cache");
}
