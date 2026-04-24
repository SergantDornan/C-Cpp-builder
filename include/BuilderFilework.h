#ifndef BUILDERFILEWORK
#define BUILDERFILEWORK
#include "algs.h"
#include "filework.h"
#include "Mapping.h"

#define OUTPUT_CONFIGS_FOLDER "outputConfigs"
#define CONFIG_FILE "config"

#define HEADERS_DIR "headers"
#define SOURCE_DIR "source"
#define SYM_DIR "sym"

#define DEPS_DIR "deps"
#define OBJECTS_DIR "objects"

extern const std::string root;
extern std::string cd;
extern const bool pocket;

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
void clearAllDepFiles(const std::string&);
#endif