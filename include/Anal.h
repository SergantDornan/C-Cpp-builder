#ifndef BELDER_ANAL_H
#define BELDER_ANAL_H

#include "ELFparser.h"
#include <string>
#include <vector>

#define ANAL_DIR "anal"

int anal(const std::vector<std::string>&);
void getLinkerLibs(std::vector<std::string>&, const std::vector<std::string>&,
	const std::vector<std::string>&, const std::vector<std::string>&);
std::string symfilePath(const std::string&, const std::string&);
bool readFreshSymfile(binFile&, const std::string&);
void saveSymfile(binFile&, const std::string&);

#endif
