#include "Process.h"

#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

extern char** environ;

static int runImpl(const std::vector<std::string>& argv, bool quiet){
    if(argv.empty()) return -1;

    // posix_spawn хочет char* const argv[] с завершающим nullptr.
    // const_cast безопасен: posix_spawn не модифицирует строки.
    std::vector<char*> cargv;
    cargv.reserve(argv.size() + 1);
    for(const auto& a : argv) cargv.push_back(const_cast<char*>(a.c_str()));
    cargv.push_back(nullptr);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_t* actp = nullptr;
    if(quiet){
        if(posix_spawn_file_actions_init(&actions) != 0) return -1;
        posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
        posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
        actp = &actions;
    }

    pid_t pid;
    // posix_spawnp ищет программу в PATH (как execvp), если в имени нет '/'.
    int rc = posix_spawnp(&pid, cargv[0], actp, nullptr, cargv.data(), environ);
    if(actp) posix_spawn_file_actions_destroy(actp);
    if(rc != 0) return -1; // программу не удалось запустить

    int status = 0;
    if(waitpid(pid, &status, 0) < 0) return -1;
    if(WIFEXITED(status)) return WEXITSTATUS(status);
    return -1; // убит сигналом и т.п.
}

int runProcess(const std::vector<std::string>& argv){
    return runImpl(argv, false);
}

int runProcessQuiet(const std::vector<std::string>& argv){
    return runImpl(argv, true);
}

void appendArgs(std::vector<std::string>& argv, const std::string& s){
    std::istringstream iss(s);
    std::string tok;
    while(iss >> tok) argv.push_back(tok); // >> пропускает любые пробелы
}

std::string joinArgs(const std::vector<std::string>& argv){
    std::string res;
    for(size_t i = 0; i < argv.size(); ++i){
        if(i) res += ' ';
        res += argv[i];
    }
    return res;
}
