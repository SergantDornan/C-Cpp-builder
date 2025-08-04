#ifndef FLAGSH
#define FLAGSH

#include "BuilderFilework.h"
const std::vector<std::string> possibleFlags = {"--rebuild", "-reb", 
	"-o", "--no-link-force", "--link-force", "--default-link",
	"--no-link-lib", "--CC", "--CXX", "--compile-flags", "--link-flags", "--clear-flags",
	"--relink", "-rel", "--idgaf", "--no-include", "--clean-flags", "--flags-clear", "--flags-clean",
	"--default-include", "--clear-options", "--clean-options", "-log"};
const std::vector<std::string> switchFlags = {"--compile-flags", "--link-flags"};
const std::vector<std::string> keyWords = {"run", "status", "help", "config"};
bool isFlag(const std::string&);
std::vector<std::string> getParameters(std::vector<std::string>&,
	const std::string& , const std::string&, const std::string&);
void getAddDirs(std::vector<std::string>&, std::vector<std::string>&);
int findEntryFile(const std::vector<std::string>&,
	const std::string&,  std::vector<std::string>&);
void getNameAfterFlag(const std::vector<std::string>&,
	const std::string&,std::string&);
void getRestFlags(const std::vector<std::string>&, std::string&);
void getSpecFlags(std::vector<std::string>&, std::string&, const std::string&);
bool isOpt(const std::string&);
bool isDebug(const std::string&);
bool isStandart(const std::string&);
int getNamesAfterFlag(std::vector<std::string>&,const std::string&,
	std::vector<std::string>&);
void FindForceLinkUnlink(std::vector<std::string>&,std::vector<std::string>&);
bool isLib(const std::string&);
#endif