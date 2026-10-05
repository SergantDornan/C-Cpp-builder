#include "BuilderFilework.h"
#include "ELFparser.h"
#include "ARparse.h"
#include <thread>

int findLinks(std::vector<std::string>&, const std::vector<binFile>&, const std::vector<std::string>&,
	const std::vector<std::string>&, const std::vector<std::string>&, const std::string&, const bool,
	const int, std::vector<std::string>&);
std::vector<std::string> toLinkList(const std::vector<std::string>&,const std::string&, const bool,
	const std::vector<std::string>&, const std::vector<std::string>&, const int, std::vector<std::string>&);
void OneThreadObjAnal(const std::string&, const std::vector<std::string>&, std::vector<binFile>&);
void LibAnal(const std::string&, binFile&);
std::string link(const std::string&, const std::string&, const std::vector<std::string>&,
	const std::vector<std::string>&,const std::vector<std::string>&,
	const bool, const int, const bool, const bool, const std::vector<std::string>&,
	const std::vector<std::string>&);
void createSymfile(binFile&, const std::string&);
bool readSymfile(binFile&, const std::string&);
void printUnresolved(const std::vector<std::string>&);
