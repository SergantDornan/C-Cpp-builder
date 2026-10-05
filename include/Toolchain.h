#ifndef BELDER_TOOLCHAIN_H
#define BELDER_TOOLCHAIN_H

#include <string>
#include <vector>

std::string linkCompiler(const std::vector<std::string>&);
void linkUserArgs(const std::vector<std::string>&, std::vector<std::string>&);
void implicitLinkInputs(const std::string&, const std::vector<std::string>&, bool, const std::string&,
	std::vector<std::string>&, std::vector<std::string>&);
void standardLibDirs(const std::string&, std::vector<std::string>&);
bool isLinkerDefinedSymbol(const std::string&);
bool isStandardLibDir(const std::string&, const std::vector<std::string>&);

#endif
