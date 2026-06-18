#ifndef BELDER_PROCESS_H
#define BELDER_PROCESS_H

#include <string>
#include <vector>

// Запуск внешних программ БЕЗ участия shell.
//
// В отличие от system(), который всегда поднимает /bin/sh -c "<строка>" и отдаёт
// строку на разбор оболочке (отсюда инъекция команд через метасимволы + лишний
// процесс на каждый вызов), здесь программа запускается напрямую через
// posix_spawn: argv[0] - имя программы (ищется в PATH), остальные элементы -
// аргументы, передаваемые ДОСЛОВНО. Никакого разбора метасимволов, никакого
// shell - имя файла вида "out; rm -rf ~" становится просто одним аргументом.

// Запускает argv[0] с аргументами argv. stdout/stderr наследуются от belder.
// Возвращает код возврата программы, либо -1 если запустить не удалось
// (нет такой программы) или процесс был убит сигналом.
int runProcess(const std::vector<std::string>& argv);

// То же, но stdout и stderr перенаправляются в /dev/null (аналог "... >/dev/null 2>&1").
int runProcessQuiet(const std::vector<std::string>& argv);

// Разбивает строку s по пробельным символам и добавляет непустые токены в argv.
// Нужно для полей, где несколько флагов хранятся одной строкой ("-O2 -g3").
void appendArgs(std::vector<std::string>& argv, const std::string& s);

// Собирает argv обратно в читаемую строку (для вывода в режиме -log).
std::string joinArgs(const std::vector<std::string>& argv);

#endif // BELDER_PROCESS_H
