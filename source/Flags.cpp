#include "Flags.h"
#include "algs.h"
bool isLib(const std::string& s){
	if(s.size() < 4) return false;
	if(std::string(s.begin(), s.begin() + 3) != "lib") return false;
	if(getExt(s) != "a" && getExt(s) != "so") return false;
	return true;
}
bool isFlag(const std::string& s){
	return ((s.size() >= 2 && s[0] == '-' && s[1] != '-') ||
	(s.size() >= 3 && s[0] == '-' && s[1] == '-' && s[2] != '-'));	
}
std::string findFile(const std::string& name, const std::string& dir){
	auto dirs = getDirs(dir);
	for(int i = 1; i < dirs.size(); ++i){
		if(name == getName(dirs[i])) return dirs[i];
		if(std::filesystem::is_directory(dirs[i])){
			std::string s = findFile(name,dirs[i]);
			if(s != "-1") return s;
		}
	}
	return "-1";
}
std::vector<std::string> getParameters(std::vector<std::string>& args,
	const std::string& path, const std::string& cd)
{

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

	FindForceLinkUnlink(args, parameters);
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
		args[0] != "status" && args[0] != "help" && !isFlag(args[0])){

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
void getNamesAfterFlag(std::vector<std::string>& args,
	const std::string& flag,std::vector<std::string>& s){

	if(args.size() == 0) return;
	auto it = args.begin();
	bool get = false;
	while(it != args.end()){
		if(*it == flag){
			get = true;
			args.erase(it);
			continue;
		}
		if(get && isFlag(*it)) break;
		if(find(s, *it) == -1 && get) s.push_back(*it);
		if(get) args.erase(it);
		else it++;
	}
}
void FindForceLinkUnlink(std::vector<std::string>& args,
	std::vector<std::string>& parameters)
{
	std::vector<std::string> fLink, fUnlink, defLink;
	if(parameters[3] != "-1") fLink = split(parameters[3]);
	if(parameters[4] != "-1") fUnlink = split(parameters[4]);
	getNamesAfterFlag(args, "--link-force", fLink);
	getNamesAfterFlag(args, "--no-link-force", fUnlink);
	getNamesAfterFlag(args, "--default-link", defLink);

	std::vector<std::string> fLibs, fUnLibs, defLibs;
	if(parameters[2] != "-1") fLibs = split(parameters[2]);
	if(parameters[13] != "-1") fUnLibs = split(parameters[13]);

	auto it = fLink.begin();
	while(it != fLink.end()){
		if(isLib(*it)){
			std::string shortname = getNameNoExt(*it);
			shortname = std::string(shortname.begin() + 3, shortname.end());
			fLibs.push_back(shortname);
			fLink.erase(it);
		}
		else
			it++;
	}
	it = fUnlink.begin();
	while(it != fUnlink.end()){
		if(isLib(*it)){
			std::string shortname = getNameNoExt(*it);
			shortname = std::string(shortname.begin() + 3, shortname.end());
			fUnLibs.push_back(shortname);
			fUnlink.erase(it);
		}
		else
			it++;
	}
	it = defLink.begin();
	while(it != defLink.end()){
		if(isLib(*it)){
			std::string shortname = getNameNoExt(*it);
			shortname = std::string(shortname.begin() + 3, shortname.end());
			defLibs.push_back(shortname);
			defLink.erase(it);
		}
		else
			it++;
	}

	it = args.begin();
	while(it != args.end()){
		if((*it).size() > 2 && isFlag(*it) && (*it)[1] == 'l' && (*it) != "-log"){
			fLibs.push_back(std::string((*it).begin() + 2, (*it).end()));
			args.erase(it);
		}
		else
			it++;
	}

	fLink -= defLink;
	fLink -= fUnlink;

	fUnlink -= defLink;
	fUnlink -= fLink;

	fLibs -= defLibs;
	fLibs -= fUnLibs;

	fUnLibs -= defLibs;
	fUnLibs -= fLibs;

	if(fLink.size() == 0) parameters[3] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fLink.size(); ++i) s += (fLink[i] + " ");
		parameters[3] = s;
	}

	if(fUnlink.size() == 0) parameters[4] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fUnlink.size(); ++i) s += (fUnlink[i] + " ");
		parameters[4] = s;
	}

	if(fLibs.size() == 0) parameters[2] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fLibs.size(); ++i) s += (fLibs[i] + " ");
		parameters[2] = s;
	}

	if(fUnLibs.size() == 0) parameters[13] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fUnLibs.size(); ++i) s += (fUnLibs[i] + " ");
		parameters[13] = s;
	}
}
