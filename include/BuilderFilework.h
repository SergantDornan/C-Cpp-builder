#ifndef BUILDERFILEWORK
#define BUILDERFILEWORK
#include "algs.h"
#include "filework.h"
#include "Mapping.h"

extern const std::vector<std::string> reqFolders, subFolders;
extern const std::string root, configFile, outputFilesConfig;
extern std::string cd;
extern const bool pocket;

#define OUTPUT_CONFIGS_FOLDER "outputConfigs"

void getAllheaders(std::vector<std::string>&,const std::string&,const std::vector<std::string>&,
	const std::vector<std::string>&);
void getAllsource(std::vector<std::string>&,const std::string&,const std::vector<std::string>&,
	const std::vector<std::string>&);
void getAllLibs(std::vector<std::string>&,const std::string&,const std::vector<std::string>&,
	const std::vector<std::string>&);
void getIncludes(std::vector<std::string>&,
    std::vector<std::string>&,
    const std::vector<FileNode>&,
    const std::vector<int>&,
    const std::string& ,bool all = false);
std::string convertPathToName(const std::string&, const char ch = '_');
bool checkOutputFiles(const std::string&, const std::string&, bool);
void clearAllDepFiles(const std::string&);
#endif