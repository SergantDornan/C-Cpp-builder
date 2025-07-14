#ifndef BUILDERFILEWORK
#define BUILDERFILEWORK
#include "algs.h"
#include "filework.h"
extern const std::vector<std::string> reqFolders, subFolders;
extern const std::string root, cd, configFile;
extern const bool pocket;

void getAllheaders(std::vector<std::string>&,const std::string&,const std::string&);
void getAllsource(std::vector<std::string>&,const std::string&,const std::string&);
void getAllLibs(std::vector<std::string>&,const std::string&,const std::string&);

#endif