#ifndef BELDER_PROCESS_H
#define BELDER_PROCESS_H

#include <string>
#include <vector>
int runProcess(const std::vector<std::string>& argv);
int runProcessQuiet(const std::vector<std::string>& argv);
void appendArgs(std::vector<std::string>& argv, const std::string& s);
std::string joinArgs(const std::vector<std::string>& argv);
#endif 
