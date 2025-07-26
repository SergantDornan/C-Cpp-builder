#include "Flags.h"
#include "algs.h"
bool isLib(const std::string& s0){
	std::string s = getName(s0);
	if(s.size() < 4) return false;
	if(std::string(s.begin(), s.begin() + 3) != "lib") return false;
	if(getExt(s) != "a" && getExt(s) != "so") return false;
	return true;
}
bool isFlag(const std::string& s){
	return ((s.size() >= 2 && s[0] == '-' && s[1] != '-') ||
	(s.size() >= 3 && s[0] == '-' && s[1] == '-' && s[2] != '-'));	
}

std::vector<std::string> getParameters(std::vector<std::string>& args,
	const std::string& path, const std::string& cd,
	const std::string& prInName)
{

	std::vector<std::string> parameters;
	std::ifstream in(path);
	std::string line;
	while(std::getline(in, line)) parameters.push_back(line);
	in.close();
	bool clearFlags = (find(args, "--clear-flags") != -1 || find(args, "--clean-flags") != -1 ||
		find(args, "--flags-clear") != -1 || find(args, "--flags-clean") != -1);
	bool clearOptions = (find(args, "--clean-options") != -1 || find(args, "--clear-options") != -1);
	bool isFoundEntry = (findEntryFile(args,cd, parameters) == 0);
	if(prInName != parameters[0] && prInName != "-1" && isFoundEntry){
		std::cout << std::endl;
		std::cout << "------- Change of entry file, clearing all previous options -------" << std::endl;
		clearOptions = true;
	}
	if(clearFlags || clearOptions)
	{
		for(int i = 7; i <= 12; ++i)
			parameters[i] = "-1";
	}
	if(clearOptions){
		parameters[2] = "-1";
		parameters[3] = "-1";
		parameters[4] = "-1";
		parameters[6] = "-1";
		parameters[13] = "-1";
		parameters[14] = "-1";
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
	getAddDirs(args, parameters);
	getSpecFlags(args, parameters[10], "--compile-flags");
	getSpecFlags(args, parameters[11], "--link-flags");
	FindForceLinkUnlink(args, parameters);
	getRestFlags(args, parameters[12]);
	auto compilers = split(parameters[5]);
	getNameAfterFlag(args, "--CC", compilers[0]);
	getNameAfterFlag(args, "--CXX", compilers[1]);
	parameters[5] = "";
	for(int i = 0; i < compilers.size(); ++i)
		parameters[5] += (compilers[i] + " ");
	getNameAfterFlag(args, "-o", parameters[1]);
	if(getFolder(parameters[1]) == "")
		parameters[1] = (cd + "/" + parameters[1]);
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
		if(!get || find(keyWords, (*it)) != -1 || find(possibleFlags, (*it)) != -1){
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
		if(find(possibleFlags, args[i]) == -1 && find(keyWords, args[i]) == -1 && 
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
void getAddDirs(std::vector<std::string>& args, std::vector<std::string>& parameters){
	
	std::vector<std::string> AddInc, fUnInc, defInc;
	if(parameters[6] != "-1") AddInc = split(parameters[6]);
	if(parameters[14] != "-1") fUnInc = split(parameters[14]);
	auto it = args.begin();
	while(it != args.end()){
		if(isFlag(*it) && (*it)[1] == 'I'){
			std::string folder((*it).begin() + 2, (*it).end());
			std::string fullpath = getFullPath(cd, folder);
			if(fullpath != "-1" && find(AddInc, fullpath) == -1) AddInc.push_back(fullpath);
			args.erase(it);
		}
		else it++;
	}
	std::vector<std::string> newfUnInc;
	getNamesAfterFlag(args, "--no-include", newfUnInc);
	for(int i = 0; i < newfUnInc.size(); ++i)
		newfUnInc[i] = getFullPath(cd, newfUnInc[i]);
	fUnInc += newfUnInc;

	getNamesAfterFlag(args, "--default-include", defInc);
	for(int i = 0; i < defInc.size(); ++i)
		defInc[i] = getFullPath(cd, defInc[i]);

	AddInc -= defInc;
	fUnInc -= defInc;

	AddInc -= fUnInc;
	fUnInc -= AddInc;

	if(AddInc.size() > 0){
		parameters[6] = "";
		for(int i = 0; i < AddInc.size(); ++i)
			parameters[6] += (AddInc[i] + " ");
	}
	else parameters[6] = "-1";

	if(fUnInc.size() > 0){
		parameters[14] = "";
		for(int i = 0; i < fUnInc.size(); ++i)
			parameters[14] += (fUnInc[i] + " ");
	}
	else parameters[14] = "-1";
}
int findEntryFile(const std::vector<std::string>& args,
	const std::string& cd, std::vector<std::string>& parameters){

	std::vector<std::string> AddInc, fUnInc;
	if(parameters[6] != "-1") AddInc = split(parameters[6]);
	if(parameters[14] != "-1") fUnInc = split(parameters[14]);
	if(args.size() != 0 && (find(keyWords, args[0]) == -1) && !isFlag(args[0])){
		std::vector<std::string> mainFile;
		findFile(mainFile, args[0], cd, AddInc, fUnInc);
		if(mainFile.size() == 0){
			std::cerr << "================== ERROR ==================" << std::endl;
			std::cerr << "Cannot find file: " << args[0] << std::endl;
			return 1;
		}
		else if(mainFile.size() > 1){
			std::cerr << "================== ERROR ==================" << std::endl;
			std::cerr << "multiple files matching \"" << args[0] << "\" found:" << std::endl;
			for(int i = 0; i < mainFile.size(); ++i)
				std::cerr << '\t' << mainFile[i] << std::endl;
			return 1; 
		}
		parameters[0] = mainFile[0];
	}
	if(args.size() == 0 || (args.size() != 0 && (isFlag(args[0]) || find(keyWords, args[0]) != -1))){
		if(parameters[0] == "-1"){
			std::vector<std::string> mainFile;
			std::string s0 = "main.cpp";
			findFile(mainFile, s0, cd, AddInc, fUnInc);
			if(mainFile.size() == 0) {
				s0 = "main.c";
				findFile(mainFile, s0, cd, AddInc, fUnInc);
			}
			if(mainFile.size() == 0){
				std::cerr << "================== ERROR ==================" << std::endl;
				std::cerr << "Cannot find entry file" << std::endl;
				return 1;
			}
			else if(mainFile.size() > 1){
				std::cerr << "================== ERROR ==================" << std::endl;
				std::cerr << "multiple files matching \"" << s0 << "\" found:" << std::endl;
				for(int i = 0; i < mainFile.size(); ++i)
					std::cerr << '\t' << mainFile[i] << std::endl;
				return 1; 
			}
			parameters[0] = mainFile[0];
		}
	}
	return 0;
}
void getNameAfterFlag(const std::vector<std::string>& args,
	const std::string& flag,std::string& s){
	int index = find(args, flag);
	if(index != -1){
		if((index + 1) >= args.size() || ((index + 1) < args.size() &&
			isFlag(args[index + 1]))){
			std::cerr << "=================== ERROR ===================" << std::endl;
			std::cerr << "no file name after " << flag << " flag" << std::endl;
			return;
		}
		s = args[index + 1];
	}
}
int getNamesAfterFlag(std::vector<std::string>& args,
	const std::string& flag,std::vector<std::string>& s){

	int size = 0;
	if(args.size() == 0) return 0;
	auto it = args.begin();
	bool get = false;
	while(it != args.end()){
		if(*it == flag){
			get = true;
			args.erase(it);
			continue;
		}
		if(get && (isFlag(*it) || find(keyWords, *it) != -1)) get = false;
		if(get && find(s, *it) == -1){
			s.push_back(*it);
			size++;
		}
		if(get) args.erase(it);
		else it++;
	}
	return size;
}
void FindForceLinkUnlink(std::vector<std::string>& args,
	std::vector<std::string>& parameters)
{
	// Считываение новых имен
	std::vector<std::string> fLink, fUnlink, defLink;
	if(parameters[3] != "-1") fLink = split(parameters[3]);
	if(parameters[4] != "-1") fUnlink = split(parameters[4]);
	std::vector<std::string> newv;
	int newfLinkSize = getNamesAfterFlag(args, "--link-force", newv);
	auto it = args.begin();
	while(it != args.end()){
		if((*it).size() > 2 && isFlag(*it) && (*it)[1] == 'l' && (*it) != "-log"){
			std::string shortname = std::string((*it).begin() + 2, (*it).end());
			if(find(newv, shortname) == -1) {
				newv.push_back(shortname);
				newfLinkSize++;
			}
			args.erase(it);
		}
		else it++;
	}
	int newfUnlinkSize = getNamesAfterFlag(args, "--no-link-force", newv);
	int newdefLinkSize = getNamesAfterFlag(args, "--default-link", newv);
	std::vector<std::string> fLibs, fUnLibs, defLibs;
	if(parameters[2] != "-1") fLibs = split(parameters[2]);
	if(parameters[13] != "-1") fUnLibs = split(parameters[13]);

	// Преобразование всех новых имен в полные пути
	std::vector<std::string> AddInc, fUnInc;
	if(parameters[6] != "-1") AddInc = split(parameters[6]);
	if(parameters[14] != "-1") fUnInc = split(parameters[14]);
	for(int i = 0; i < newv.size(); ++i){
		if(getExt(newv[i]) != ""){
			std::vector<std::string> result;
			findFile(result, newv[i], cd, AddInc, fUnInc);
			if(result.size() == 0){
				std::cerr << "======================== ERROR ========================" << std::endl;
				std::cerr << "Cannot find file: " << newv[i] << std::endl;
				std::cerr << "You specified it in ";
				if(i < newfLinkSize) std::cerr << "force-link ";
				else if(i < newfLinkSize + newfUnlinkSize) std::cerr << "force-unlink ";
				else std::cerr << "default-link ";
				std::cerr << "list" << std::endl;
				return;
			}
			else if(result.size() > 1){
				std::cerr << "======================== ERROR ========================" << std::endl;
				std::cerr << "multiple files matching \"" << newv[i] << "\" found:" << std::endl;
				for(int j = 0; j < result.size(); ++j)
					std::cerr << '\t' << result[j] << std::endl;
				std::cerr << "You specified it in ";
				if(i < newfLinkSize) std::cerr << "force-link ";
				else if(i < newfLinkSize + newfUnlinkSize) std::cerr << "force-unlink ";
				else std::cerr << "default-link ";
				std::cerr << "list" << std::endl;
				return;
			}
			newv[i] = result[0];
		}
		else{
			std::vector<std::string> result;
			if(i < newfLinkSize){
				bool erase = false;
				findFile(result, ("lib" + newv[i] + ".so"), cd, AddInc, fUnInc);
				if(result.size() == 0) findFile(result, ("lib" + newv[i] + ".a"), cd, AddInc, fUnInc);
				if(result.size() == 0){
					// std::cerr << "======================== ERROR ========================" << std::endl;
					// std::cerr << "Cannot find files lib" << newv[i] << ".so or lib" << newv[i] << ".a" << std::endl;
					// std::cerr << "You specified \"" << newv[i] << "\" in force-link list, belder thinks it is a library";
					// return;
					parameters[11] += (" -l" + newv[i]);
					erase = true;
				}
				else if(result.size() > 1){
					std::cerr << "======================== ERROR ========================" << std::endl;
					std::cerr << "multiple files matching \"" << newv[i] << "\" found:" << std::endl;
					for(int j = 0; j < result.size(); ++j)
						std::cerr << '\t' << result[j] << std::endl;
					std::cerr << "You specified it in force-link list, belder thinks it is a library" << std::endl;
					return;
				}
				if(!erase) newv[i] = result[0];
				else newv[i] = "-1";
			}
			else{
				findFile(result, ("lib" + newv[i] + ".so"), cd, AddInc, fUnInc);
				findFile(result, ("lib" + newv[i] + ".a"), cd, AddInc, fUnInc);
				if(result.size() == 0){
					std::cerr << "======================== ERROR ========================" << std::endl;
					std::cerr << "Cannot find files lib" << newv[i] << ".so or lib" << newv[i] << ".a" << std::endl;
					std::cerr << "You specified \"" << newv[i] << "\" in ";
					if(i < newfLinkSize + newfUnlinkSize) std::cerr << "force-unlink ";
					else std::cerr << "default-link ";
					std::cerr << "list, belder thinks it is a library" << std::endl;
					return;
				}
				else if((result.size() > 2) || 
					(result.size() == 2 && !(getExt(result[0]) == "so" && getExt(result[1]) == "a")))
				{
					std::cerr << "======================== ERROR ========================" << std::endl;
					std::cerr << "multiple files matching \"" << newv[i] << "\" found:" << std::endl;
					for(int j = 0; j < result.size(); ++j)
						std::cerr << '\t' << result[j] << std::endl;
					std::cerr << "You specified it in force-link list, belder thinks it is a library" << std::endl;
					return;
				}
				if(result.size() == 1) newv[i] = result[0];
				else newv[i] = (result[0] + "*" + result[1]);
			}
		}
	}
	it = newv.begin();
	while(it != newv.end()){
		if(*it == "-1") newv.erase(it);
		else it++;
	}

	for(int i = 0; i < newv.size(); ++i){
		if(i < newfLinkSize){
			if(isLib(newv[i])){
				if(find(fLibs, newv[i]) == -1)
					fLibs.push_back(newv[i]);
			}
			else{
				if(find(fLink, newv[i]) == -1)
					fLink.push_back(newv[i]);
			}
		}
		else if(i < newfLinkSize + newfUnlinkSize){
			if(isLib(newv[i])){
				if(newv[i].find("*") != std::string::npos){
					auto spl = split(newv[i], "*");
					fUnLibs += spl;
				}
				else{
					if(find(fUnLibs, newv[i]) == -1)
						fUnLibs.push_back(newv[i]);
				}
			}
			else{
				if(find(fUnlink, newv[i]) == -1)
					fUnlink.push_back(newv[i]);
			}
		}
		else{
			if(isLib(newv[i])){
				if(newv[i].find("*") != std::string::npos){
					auto spl = split(newv[i], "*");
					defLibs += spl;
				}
				else{
					if(find(defLibs, newv[i]) == -1)
						defLibs.push_back(newv[i]);
				}
			}
			else{
				if(find(defLink, newv[i]) == -1)
					defLink.push_back(newv[i]);
			}
		}
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
