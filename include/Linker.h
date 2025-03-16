#include "BuilderFilework.h"
#include <thread>
const std::string postSharedLink = "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH"; //(Добавлять папки как ..._PATH:./aboba:./boba)
const size_t numT = std::thread::hardware_concurrency();
std::vector<std::string> getCallFuncs(const std::string&);
std::vector<std::string> getDefFuncs(const std::string&);
void getAllLibs(std::vector<std::string>&,std::vector<std::string>&,
	const std::string&);
std::string getCallName(const std::string&);
std::string getDefName(const std::string&);
std::string getVarCallName(const std::string&);
std::string getVarDefName(const std::string&);
std::string getAlias(const std::string&);
class asmFile{
public:
	asmFile(const std::string&);
	asmFile(const asmFile&);
	std::string name;
	std::vector<std::string> callFuncs, defFuncs,
							callVars, defVars;
	void add(const std::string&);
};
int findLinks(std::vector<std::string>&, const std::vector<asmFile>&,
	const asmFile&, const std::vector<std::string>&,
	std::vector<std::pair<std::string,std::string>>&, 
	std::vector<std::pair<std::string,std::string>>&);
int pairFind(const std::vector<std::pair<std::string,std::string>>&, const std::string&);
std::vector<std::string> toLinkList(const std::vector<std::string>&,const std::string&);
std::vector<std::string> AllLinkedSource(const std::vector<std::string>&,
	const std::string&, const bool);
void OneThreadAsmAnal(const std::string&,asmFile&,const std::vector<std::string>&,
	std::vector<asmFile>&);
std::string link(const std::string&, const std::vector<std::string>&,
	const std::vector<std::string>&,const std::vector<std::string>&,
	const bool, const int, const bool);