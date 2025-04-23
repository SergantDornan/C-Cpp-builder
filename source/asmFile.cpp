#include "Linker.h"
//	.type	sum, @function
//	call	mult@PLT
// 	.type	_ZL1y, @object
asmFile::asmFile(const std::string& line){
	name = line;
}
asmFile::asmFile(const asmFile& other){
	name = other.name;
	callFuncs = std::move(other.callFuncs);
	defFuncs = std::move(other.defFuncs);
	callVars = std::move(other.callVars);
	defVars = std::move(other.defVars);
}
void asmFile::add(const std::string& line){
	//std::cout << line << std::endl;
	if(getVarCallName(line) != "-1"){
		std::string s = getVarCallName(line);
		if(find(callVars, s) == -1)
			callVars.push_back(s);

	}
	else if(getVarDefName(line) != "-1"){

		std::string s = getVarDefName(line);
		if(find(defVars, s) == -1)
			defVars.push_back(s);

	}
	else if(getCallName(line) != "-1"){

		std::string s = getCallName(line);
		if(find(callFuncs, s) == -1)
			callFuncs.push_back(s);

	}
	else if(getDefName(line) != "-1"){

		std::string s = getDefName(line);
		if(find(defFuncs, s) == -1)
			defFuncs.push_back(s);

	}
	else if(getAlias(line) != "-1"){

		std::string s = getAlias(line);
		if(find(defFuncs, s) == -1)
			defFuncs.push_back(s);

	}
	//std::cout << "AAAAAAAAAa" << std::endl;
}
//	.set	aboba1,aboba2
std::string getAlias(const std::string& line){
	if(line.find(".set") == std::string::npos)
		return "-1";
	return split(split(line, "\t ")[1], ",")[0];
}
//	movl	externalVar(%rip), %eax
//	movl	%eax, externalVar(%rip)
//	incl	externalVar(%rip)
//	lock addl	$1, externalCounter(%rip)
std::string getVarCallName(const std::string& line){
	if(line.find("(%rip)") == std::string::npos)
		return "-1";
	auto v = split(split(line, "\t")[1]);
	if(v.size() == 1 && v[0].find("(%rip)") != std::string::npos)
		return std::string(v[0].begin(), v[0].end() - 6);
	if(v.size() == 2){
		if(v[0].find("(%rip)") != std::string::npos)
			return std::string(v[0].begin(), v[0].end() - 7);
		else
			return std::string(v[1].begin(), v[1].end() - 6);
	}
	return "SOME ERROR";
}
std::string getVarDefName(const std::string& line){
	if(line.find("@object") == std::string::npos)
		return "-1";
	auto s = split(split(line, "\t")[1]);
	return std::string(s[0].begin(),s[0].end()-1);
}
std::string getCallName(const std::string& line){
	if(line.find("call") == std::string::npos && 
		line.find("jmp") == std::string::npos &&
		line.find("j") == std::string::npos)
		return "-1";
	auto s = split(line, "\t");
	if(s.size() != 2)
		return "-1";
	if(s[1].find("@PLT") == std::string::npos)
		return "-1";
	return std::string(s[1].begin(), s[1].end()-4);
}
std::string getDefName(const std::string& line){
	if(line.find("@function") == std::string::npos)
		return "-1";
	auto s = split(line, "\t ");
	return std::string(s[1].begin(),s[1].end()-1);
}