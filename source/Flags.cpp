#include "Flags.h"
#include "algs.h"
bool isFlag(const std::string& s){
	return ((s.size() >= 2 && s[0] == '-' && s[1] != '-') ||
	(s.size() >= 3 && s[0] == '-' && s[1] == '-' && s[2] != '-'));	
}
std::string findFile(const std::string& name, const std::string& dir){
	auto dirs = getDirs(dir);
	for(int i = 1; i < dirs.size(); ++i){
		if(name == getName(dirs[i]))
			return dirs[i];
		if(std::filesystem::is_directory(dirs[i])){
			std::string s = findFile(name,dirs[i]);
			if(s != "-1")
				return s;
		}
	}
	return "-1";
}
std::vector<std::string> getParameters(std::vector<std::string>& args,
	const std::string& path, const std::string& cd){

	std::vector<std::string> parameters;
	std::ifstream in(path);
	std::string line;
	while(std::getline(in, line)) parameters.push_back(line);
	in.close();
	if(find(args, "--clear-flags") != -1){
		for(int i = 7; i <= 12; ++i)
			parameters[i] = "-1";
	}
	auto it = args.begin();
	while(it != args.end()){
		if(isStandart(*it)){
			parameters[7] = *it;
			args.erase(it);
		}
		else if(isOpt(*it)){
			parameters[8] = *it;
			args.erase(it);
		}
		else if(isDebug(*it)){
			parameters[9] = *it;
			args.erase(it);
		}
		else
			it++;
	}
	getSpecFlags(args, parameters[10], "--compile-flags");
	getSpecFlags(args, parameters[11], "--link-flags");
	getRestFlags(args, parameters[12]);
	findEntryFile(args,parameters[0],cd);
	auto compilers = split(parameters[5]);
	getNameAfterFlag(args, "--CC", compilers[0]);
	getNameAfterFlag(args, "--CXX", compilers[1]);
	getNameAfterFlag(args, "--preproc", compilers[2]);
	parameters[5] = "";
	for(int i = 0; i < compilers.size(); ++i)
		parameters[5] += (compilers[i] + " ");
	getNameAfterFlag(args, "-o", parameters[1]);
	if(getFolder(parameters[1]) == "")
		parameters[1] = (cd + "/" + parameters[1]);
	getIdirs(args, parameters[6]);
	return parameters;
}
bool isStandart(const std::string& s){
	return (s.size() >= 5 && std::string(s.begin(), s.begin() + 5) == "-std=");
}
bool isDebug(const std::string& s){
	return (s.size() == 3 && s[0] == '-' && s[1] == 'g');
}
bool isOpt(const std::string& s){
	return (s.size() == 3 && s[0] == '-' && s[1] == 'O');
}
void getSpecFlags(std::vector<std::string>& args, std::string& s, const std::string& switchFlag){
	auto it = args.begin();
	bool get = false;
	while(it != args.end()){
		if(*it == switchFlag){
			get = true;
			args.erase(it);
			continue;
		}
		if(find(switchFlags, *it) != -1)
			break;
		if(!get){
			it++;
			continue;
		}
		if(s == "-1") s = "";
		if(s.find(*it) == std::string::npos)
			s += ((*it) + " ");
		args.erase(it);
	}
}
void getRestFlags(const std::vector<std::string>& args, std::string& s){
	for(int i = 0; i < args.size(); ++i){
		if(find(possibleFlags, args[i]) == -1 && 
			!(args[i].size() >= 2 && args[i][0] == '-' && (args[i][1] == 'I' || args[i][1] == 'l')))
		{
			if(isFlag(args[i]) || (i > 0 && find(possibleFlags, args[i-1]) == -1)){
				if(s == "-1") s = "";
				if(s.find(args[i]) == std::string::npos)
					s += (args[i] + " ");
			}
		}
	}
}
void getIdirs(const std::vector<std::string>& args, std::string& s){
	std::vector<std::string> AddInc;
	if (s != "-1") AddInc = split(s);
	for(int i = 0; i < args.size(); ++i){
		if(isFlag(args[i]) && args[i][1] == 'I'){
			std::string folder(args[i].begin() + 2, args[i].end());
			if(find(AddInc, folder) == -1) AddInc.push_back(folder);
		}
	}
	std::vector<std::string> NoInc;
	for(int i = 0; i < args.size(); ++i){
		if(args[i] == "--no-include"){
			if((i + 1) >= args.size() || ((i + 1) < args.size() &&
			isFlag(args[i + 1]))){
				std::cout << "======================== ERROR ========================" << std::endl;
				std::cout << "no folder after --no-include flag" << std::endl;
				return;
			}
			else if(find(NoInc, args[i+1]) == -1) NoInc.push_back(args[i+1]);
		}
	}
	auto it = AddInc.begin();
	while(it != AddInc.end()){
		if(find(NoInc, *it) != -1)
			AddInc.erase(it);
		else
			it++;
	}
	if(AddInc.size() == 0)
		s = "-1";
	else{
		s = "";
		for(int i = 0; i < AddInc.size(); ++i)
			s += (AddInc[i] + " ");
	}
}
void findEntryFile(const std::vector<std::string>& args, std::string& s,
	const std::string& cd){
	if(args.size() != 0 && args[0] != "run" && args[0] != "config" && 
		args[0] != "status" && !isFlag(args[0])){

		std::string mainFile = findFile(args[0], cd);
		if(mainFile == "-1"){
			std::cout << "================== ERROR ==================" << std::endl;
			std::cout << "Cannot find file: " << args[0] << std::endl;
			return;
		}
		s = mainFile;
	}
	if(args.size() == 0 || (args.size() != 0 && isFlag(args[0]))){
		if(s == "-1"){
			std::string mainFile = findFile("main.cpp", cd);
			if(mainFile == "-1")
				mainFile = findFile("main.c", cd);
			if(mainFile == "-1"){
				std::cout << "================== ERROR ==================" << std::endl;
				std::cout << "Cannot find entry file" << std::endl;
				return;
			}
			s = mainFile;
		}
	}
}
void getNameAfterFlag(const std::vector<std::string>& args,
	const std::string& flag,std::string& s){
	int index = find(args, flag);
	if(index != -1){
		if((index + 1) >= args.size() || ((index + 1) < args.size() &&
			isFlag(args[index + 1]))){
			std::cout << "=================== ERROR ===================" << std::endl;
			std::cout << "no file name after " << flag << " flag" << std::endl;
			return;
		}
		s = args[index + 1];
	}
}