#include "BuilderFilework.h"
bool createDepfiles(const std::string&,
	const std::vector<std::string>&,const std::vector<std::string>&,
	const bool);

int UpdateDependencies(const std::vector<std::string>&,
	const std::vector<std::string>&,const std::string&, const std::string&,
    const std::vector<std::string>&,const std::vector<std::string>&);

int updateFiles(std::vector<std::string>&,const std::vector<std::string>&,
	const std::vector<std::string>&);

void updateFile(std::vector<std::string>&,
    const std::string&,std::vector<std::string>&);

void rebuildForSharedLib(const std::string&, const std::string&,
	const std::string&);

void updateSymfiles(const std::string&, const std::vector<std::string>&);