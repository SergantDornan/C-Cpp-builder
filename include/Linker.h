#include "BuilderFilework.h"
#include "ELFparser.h"
#include "ARparse.h"
#include <thread>
const std::string postSharedLink = "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH"; //(Добавлять папки как ..._PATH:./aboba:./boba)
const size_t numT = std::thread::hardware_concurrency();
int findLinks(std::vector<std::string>&, const std::vector<binFile>&,
	const binFile&,std::map<std::string,std::string>&, const bool, std::vector<binFile>&);
std::vector<std::string> toLinkList(const std::vector<std::string>&,const std::string&, const bool,
	const std::vector<std::string>&);
std::vector<std::string> AllLinkedSource(const std::vector<std::string>&,
	const std::string&, const bool);
void OneThreadObjAnal(const std::string&,binFile&,const std::vector<std::string>&,
	const std::vector<std::string>&, std::vector<binFile>&);
std::string link(const std::string&, const std::vector<std::string>&,
	const std::vector<std::string>&,const std::vector<std::string>&,
	const bool, const int, const bool, const bool, const std::vector<std::string>&);