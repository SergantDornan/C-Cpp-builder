#include "BuilderFilework.h"
#include "Mapping.h"
bool createDepfiles(const std::string&,
	const std::vector<std::string>&,const std::vector<std::string>&,
	const bool);

void UpdateDependencies(const std::vector<std::string>&,
	const std::vector<std::string>&,const std::string&, const std::string&,
    const std::vector<FileNode>&, const std::vector<int>&);

int updateFiles(std::vector<std::string>&,const std::vector<std::string>&,
	const std::vector<std::string>&);

void updateFile(std::vector<std::string>&,
    const std::string&,std::vector<std::string>&);

void rebuildForSharedLib(const std::string&, const std::string&,
	const std::string&);

void updateSymfiles(const std::string&, const std::vector<std::string>&);