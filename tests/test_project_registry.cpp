#include "helpers.h"
#include <fstream>
#include <set>

// ---------------------------------------------------------------------------
// Тесты общего реестра проектов ~/builder/config (строки "cwd*N").
//
// 1. Если каталог проекта исчез, belder удаляет его запись и папку сборки.
//    Раньше папка бралась из итератора ПОСЛЕ erase -> удалялась папка
//    соседнего (следующего) проекта, а для последней записи - UB.
// 2. Новый проект получал индекс size()+1. После удаления любой записи этот
//    индекс совпадал с индексом живого проекта -> два проекта в одной папке.
// ---------------------------------------------------------------------------

namespace {

std::string builderRoot() {
    const char* home = getenv("HOME");
    return home ? std::string(home) + "/builder" : "";
}

// Найти каталог сборки проекта внутри ~/builder по записи "cwd*N" в общем config.
std::string belderBuildDir(const std::string& projectDir) {
    std::ifstream cfg(builderRoot() + "/config");
    std::string line;
    while (std::getline(cfg, line)) {
        auto star = line.rfind('*');
        if (star == std::string::npos) continue;
        if (line.substr(0, star) == projectDir)
            return builderRoot() + "/" + line.substr(star + 1);
    }
    return "";
}

std::string makeTmpProject() {
    char tmpl[] = "/tmp/btest_XXXXXX";
    char* d = mkdtemp(tmpl);
    if (!d) return "";
    writeFile(std::string(d) + "/main.cpp", "int main(){return 0;}\n");
    return d;
}

// Регистрирует проект в ~/builder без компиляции
BelderResult registerProject(const std::string& dir) {
    return runCommandInDir(std::string(BELDER_BINARY) + " -C '" + dir + "' status", dir);
}

void dropProject(const std::string& dir) {
    if (dir.empty()) return;
    if (std::filesystem::exists(dir))
        runCommandInDir(std::string(BELDER_BINARY) + " -C '" + dir + "' silent_clear", dir);
    std::filesystem::remove_all(dir);
}

} // namespace

class RegistryFixture : public BelderFixture {
protected:
    std::vector<std::string> extra;

    std::string newProject() {
        std::string d = makeTmpProject();
        extra.push_back(d);
        return d;
    }

    void TearDown() override {
        for (const auto& d : extra) dropProject(d);
        BelderFixture::TearDown();
    }
};

// Исчезнувший проект стоит в реестре ПЕРЕД живым: должна удалиться только
// его папка сборки, папка соседа обязана остаться.
TEST_F(RegistryFixture, StaleEntryRemovalKeepsNextProjectBuildDir) {
    std::string a = newProject(), b = newProject();
    ASSERT_FALSE(a.empty()); ASSERT_FALSE(b.empty());
    ASSERT_BELDER_OK(registerProject(a), "register project A");
    ASSERT_BELDER_OK(registerProject(b), "register project B");

    std::string dirA = belderBuildDir(a), dirB = belderBuildDir(b);
    ASSERT_FALSE(dirA.empty()); ASSERT_FALSE(dirB.empty());
    ASSERT_TRUE(std::filesystem::exists(belderLastPairConfig(dirB)));

    std::filesystem::remove_all(a); // A исчез
    write("main.cpp", "int main(){return 0;}\n");
    auto r = runBelder({"status"}); // любой запуск чистит устаревшие записи
    EXPECT_BELDER_OK(r, "run belder after project A directory was deleted");

    EXPECT_EQ(belderBuildDir(a), "") << "stale entry of A must be removed from registry";
    EXPECT_FALSE(std::filesystem::exists(dirA)) << "build dir of deleted A must be removed: " << dirA;
    EXPECT_EQ(belderBuildDir(b), dirB);
    EXPECT_TRUE(std::filesystem::exists(belderLastPairConfig(dirB)))
        << "build dir of live project B was removed instead of A's: " << dirB;
}

// Исчезнувший проект - последняя запись реестра: раньше разыменовывался end().
TEST_F(RegistryFixture, StaleLastEntryRemovedWithoutCrash) {
    // Проект фикстуры регистрируем заранее, чтобы следующий запуск не занял
    // освободившийся индекс A и не пересоздал его папку
    write("main.cpp", "int main(){return 0;}\n");
    ASSERT_BELDER_OK(runBelder({"status"}), "register fixture project");
    std::string a = newProject();
    ASSERT_FALSE(a.empty());
    ASSERT_BELDER_OK(registerProject(a), "register project A");
    std::string dirA = belderBuildDir(a);
    ASSERT_FALSE(dirA.empty());

    std::filesystem::remove_all(a);
    auto r = runBelder({"status"});
    EXPECT_BELDER_OK(r, "run belder after last registered project was deleted");

    EXPECT_EQ(belderBuildDir(a), "");
    EXPECT_FALSE(std::filesystem::exists(dirA)) << dirA;
}

// После удаления записи новый проект не должен получить индекс живого проекта.
TEST_F(RegistryFixture, NewProjectDoesNotReuseLiveIndexAfterClear) {
    std::string a = newProject(), b = newProject();
    ASSERT_FALSE(a.empty()); ASSERT_FALSE(b.empty());
    ASSERT_BELDER_OK(registerProject(a), "register project A");
    ASSERT_BELDER_OK(registerProject(b), "register project B");
    std::string dirB = belderBuildDir(b);
    ASSERT_FALSE(dirB.empty());

    auto clr = runCommandInDir(std::string(BELDER_BINARY) + " -C '" + a + "' silent_clear", a);
    ASSERT_BELDER_OK(clr, "clear project A");
    ASSERT_EQ(belderBuildDir(a), "");

    write("main.cpp", "int main(){return 0;}\n");
    auto r = runBelder({"config", "-o", "fixtureOut"});
    ASSERT_BELDER_OK(r, "register fixture project after A was cleared");

    std::string dirC = belderBuildDir(tmpDir);
    ASSERT_FALSE(dirC.empty());
    EXPECT_NE(dirC, dirB) << "new project got the same build dir as live project B";

    // Конфиг B не должен быть перезаписан новым проектом
    std::string cfgB = readFile(belderLastPairConfig(dirB));
    EXPECT_EQ(cfgB.find("fixtureOut"), std::string::npos)
        << "config of project B was overwritten:\n" << cfgB;

    // Все индексы в реестре уникальны
    std::ifstream cfg(builderRoot() + "/config");
    std::string line;
    std::set<std::string> seen;
    while (std::getline(cfg, line)) {
        auto star = line.rfind('*');
        if (star == std::string::npos) continue;
        std::string idx = line.substr(star + 1);
        EXPECT_TRUE(seen.insert(idx).second) << "duplicate index " << idx << " in registry";
    }
}
