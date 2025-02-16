#include "BuilderFilework.h"
#include <thread>
#include <future>
const std::string Cppstandart = "-std=c++23";
const std::string Cstandart = "-std=c2x";
const std::string opt = "-O3";
const std::string debug = "-g3";
extern std::string otherFlags;
extern std::string externCompileFlags;
const size_t numThreads = std::thread::hardware_concurrency();
std::vector<std::string> compile(const std::string&,const std::vector<std::string>&,
	const std::vector<std::string>&,const std::vector<std::string>&,
	const bool, const bool);
void getIncludes(std::vector<std::string>&,const std::vector<std::string>&,
	const std::vector<std::string>&,const std::string&, bool all = false);
void UpdateDependencies(const std::string&, const std::string&,
    const std::vector<std::string>&,const std::vector<std::string>&);
void updateFile(std::vector<std::string>&,
    const std::string&,std::vector<std::string>&);
void compileFile(const std::string&, const std::vector<std::string>&,
	const std::string&,const bool,const std::vector<std::string>&);
void oneThreadCompile(const std::vector<std::string>&, 
	const std::vector<std::string>&,const std::string&,const bool,
	const std::vector<std::string>&);
bool checkProgram(const std::string&);