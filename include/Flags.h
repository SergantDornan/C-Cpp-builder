#include "BuilderFilework.h"
const std::vector<std::string> possibleFlags = {"--rebuild", "-reb", 
	"-o", "--no-link-force", "--link-force", "--default-link",
	"--no-link-lib", "--CC", "--CXX", "--compile-flags", "--link-flags", "--clear-flags",
	"--preproc"};
const std::vector<std::string> switchFlags = {"--compile-flags", "--link-flags"};
bool isFlag(const std::string&);
std::string findFile(const std::string&,const std::string&);
std::vector<std::string> getParameters(std::vector<std::string>&,
	const std::string& , const std::string&);
void getIdirs(const std::vector<std::string>&, std::string&);
void findEntryFile(const std::vector<std::string>&, std::string&,
	const std::string&);
void getNameAfterFlag(const std::vector<std::string>&,
	const std::string&,std::string&);
void getRestFlags(const std::vector<std::string>&, std::string&);
void getSpecFlags(std::vector<std::string>&, std::string&, const std::string&);
bool isOpt(const std::string&);
bool isDebug(const std::string&);
bool isStandart(const std::string&);