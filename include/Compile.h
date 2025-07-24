#include "BuilderFilework.h"
#include "DepFiles.h"
#include <thread>
#include <future>
#include <mutex>
const size_t numThreads = std::thread::hardware_concurrency();
std::vector<std::string> compile(const std::string&,const std::vector<std::string>&,
	const std::vector<std::string>&,const std::vector<std::string>&,
	const bool, const bool);
void compileFile(const std::string&, const std::vector<std::string>&,
	const std::string&,const bool,const std::vector<std::string>&);
void oneThreadCompile(const std::vector<std::string>&, 
	const std::vector<std::string>&,const std::string&,const bool,
	const std::vector<std::string>&);