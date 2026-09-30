#ifndef BELDER_CONFIGS_H
#define BELDER_CONFIGS_H
#include "BuilderFilework.h"

int selectPair(std::vector<std::string>&, const std::string&, const std::string&, std::string&);
std::string profileKey(const std::vector<std::string>&);
void removeUnusedProfiles(const std::string&);
std::string selectProfile(const std::string&, const std::string&, std::vector<std::string>&);
void printConfigs(const std::string&, const std::string&);
#endif
