#include "Flags.h"
#include "algs.h"
#include "ConfigIndex.h"
bool isLib(const std::string& s0){
	return getLibType(s0) != "";
}
bool isFlag(const std::string& s){
	return ((s.size() >= 2 && s[0] == '-' && s[1] != '-') ||
	(s.size() >= 3 && s[0] == '-' && s[1] == '-' && s[2] != '-'));	
}

// Дефолтная раскладка project config (совпадает с тем, что пишет createEssentials).
static std::vector<std::string> defaultConfig(){
	std::vector<std::string> p(CFG_COUNT, "-1");
	p[CFG_OUTPUT] = "out";
	p[CFG_COMPILERS] = "default default";
	return p;
}

std::vector<std::string> getParameters(std::vector<std::string>& args,
	const std::string& projectConfig, const std::string& cd,
	const std::string& prInName, bool& recompile, bool& relink)
{

	std::vector<std::string> parameters;
	std::ifstream in(projectConfig);
	std::string line;
	while(std::getline(in, line)) parameters.push_back(line);
	in.close();

	// Config мог быть повреждён (оборванная запись при прерывании belder,
	// переполнение диска, ручная правка). Без восстановления обращения к
	// parameters[0..CFG_COUNT-1] ниже уходят за границы вектора -> segfault.
	if(parameters.size() < (size_t)CFG_COUNT)
		parameters = defaultConfig();
	// Поле компиляторов обязано содержать ровно 2 токена (C и C++);
	// иначе split(...)[1] в компиляции/линковке выходит за границу.
	if(split(parameters[CFG_COMPILERS]).size() < 2)
		parameters[CFG_COMPILERS] = "default default";

	auto pr_parameters = parameters;

	bool clearFlags = (find(args, "--clear-flags") != -1 || find(args, "--clean-flags") != -1 ||
		find(args, "--flags-clear") != -1 || find(args, "--flags-clean") != -1);
	bool clearOptions = (find(args, "--clean-options") != -1 || find(args, "--clear-options") != -1);
	bool isFoundEntry = (findEntryFile(args,cd, parameters) == 0);

	if(prInName != parameters[CFG_ENTRY] && prInName != "-1" && isFoundEntry){
		std::cout << std::endl;
		std::cout << "------- Change of entry file, clearing all old options -------" << std::endl;
		std::cout << std::endl;
		clearOptions = true;
		//clearFlags = true;
	}

	if(clearFlags || clearOptions)
	{
		for(int i = CFG_CXX_STANDARD; i <= CFG_GENERAL_FLAGS; ++i)
			parameters[i] = "-1";
		parameters[CFG_C_STANDARD] = "-1";
	}
	if(clearOptions){
		parameters[CFG_FORCE_LINK_LIBS] = "-1";
		parameters[CFG_FORCE_LINK] = "-1";
		parameters[CFG_FORCE_UNLINK] = "-1";
		parameters[CFG_ADD_INCLUDE] = "-1";
		parameters[CFG_FORCE_UNLINK_LIBS] = "-1";
		parameters[CFG_FORCE_UNLINK_DIRS] = "-1";
		parameters[CFG_COMPILERS] = "default default";
	}
	auto it = args.begin();
	while(it != args.end()){
		if(isStandart(*it)){
			if((*it).find("c++") != std::string::npos)
				parameters[CFG_CXX_STANDARD] = *it;
			else
				parameters[CFG_C_STANDARD] = *it;
			args.erase(it);
		}
		else if(isOpt(*it)){
			parameters[CFG_OPT] = *it;
			args.erase(it);
		}
		else if(isDebug(*it)){
			parameters[CFG_DEBUG] = *it;
			args.erase(it);
		}
		else
			it++;
	}
	getAddDirs(args,cd, parameters);
	getSpecFlags(args, parameters[CFG_COMPILE_FLAGS], "--compile-flags");
	getSpecFlags(args, parameters[CFG_LINK_FLAGS], "--link-flags");
	// Следующая функция может насрать в Link flags!!!
	FindForceLinkUnlink(args,cd, parameters);
	getRestFlags(args, parameters[CFG_GENERAL_FLAGS]);
	auto compilers = split(parameters[CFG_COMPILERS]);
	getNameAfterFlag(args, "--CC", compilers[0]);
	getNameAfterFlag(args, "--CXX", compilers[1]);
	parameters[CFG_COMPILERS] = "";
	for(int i = 0; i < compilers.size(); ++i)
		parameters[CFG_COMPILERS] += (compilers[i] + " ");
	getNameAfterFlag(args, "-o", parameters[CFG_OUTPUT]);
	if(getFolder(parameters[CFG_OUTPUT]) == "")
		parameters[CFG_OUTPUT] = (cd + "/" + parameters[CFG_OUTPUT]);

	int compile_sensitive_options[7] = {CFG_COMPILERS, CFG_CXX_STANDARD, CFG_OPT,
		CFG_DEBUG, CFG_COMPILE_FLAGS, CFG_GENERAL_FLAGS, CFG_C_STANDARD};
	int link_sensitive_options[5] = {CFG_FORCE_LINK_LIBS, CFG_FORCE_LINK,
		CFG_FORCE_UNLINK, CFG_LINK_FLAGS, CFG_FORCE_UNLINK_LIBS};

	for(int i = 0; i < 7; ++i){
		if(parameters[compile_sensitive_options[i]] != pr_parameters[compile_sensitive_options[i]]) {
			recompile = true;
			break;
		}
	}

	for(int i = 0; i < 5; ++i){
		if(parameters[link_sensitive_options[i]] != pr_parameters[link_sensitive_options[i]]){
			relink = true;
			break;
		}
	}
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
	std::string s0 = "-1";
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
		if(s0 == "-1") s0 = "";
		//if(s.find(*it) == std::string::npos)
			s0 += ((*it) + " ");
		args.erase(it);
	}
	s = s0;
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
void getAddDirs(std::vector<std::string>& args,const std::string& cd, std::vector<std::string>& parameters){
	
	std::vector<std::string> AddInc, fUnInc, defInc;
	if(parameters[CFG_ADD_INCLUDE] != "-1") AddInc = split(parameters[CFG_ADD_INCLUDE]);
	if(parameters[CFG_FORCE_UNLINK_DIRS] != "-1") fUnInc = split(parameters[CFG_FORCE_UNLINK_DIRS]);
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
		parameters[CFG_ADD_INCLUDE] = "";
		for(int i = 0; i < AddInc.size(); ++i)
			parameters[CFG_ADD_INCLUDE] += (AddInc[i] + " ");
	}
	else parameters[CFG_ADD_INCLUDE] = "-1";

	if(fUnInc.size() > 0){
		parameters[CFG_FORCE_UNLINK_DIRS] = "";
		for(int i = 0; i < fUnInc.size(); ++i)
			parameters[CFG_FORCE_UNLINK_DIRS] += (fUnInc[i] + " ");
	}
	else parameters[CFG_FORCE_UNLINK_DIRS] = "-1";
}


int findEntryFile(const std::vector<std::string>& args,
	const std::string& cd, std::vector<std::string>& parameters){

	std::vector<std::string> AddInc, fUnInc;
	if(parameters[CFG_ADD_INCLUDE] != "-1") AddInc = split(parameters[CFG_ADD_INCLUDE]);
	if(parameters[CFG_FORCE_UNLINK_DIRS] != "-1") fUnInc = split(parameters[CFG_FORCE_UNLINK_DIRS]);
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
		parameters[CFG_ENTRY] = mainFile[0];
	}
	if(args.size() == 0 || (args.size() != 0 && (isFlag(args[0]) || find(keyWords, args[0]) != -1))){
		if(parameters[CFG_ENTRY] == "-1"){
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
			parameters[CFG_ENTRY] = mainFile[0];
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
void FindForceLinkUnlink(std::vector<std::string>& args,const std::string& cd,
	std::vector<std::string>& parameters)
{
	// Считываение новых имен
	std::vector<std::string> fLink, fUnlink, defLink;
	if(parameters[CFG_FORCE_LINK] != "-1") fLink = split(parameters[CFG_FORCE_LINK]);
	if(parameters[CFG_FORCE_UNLINK] != "-1") fUnlink = split(parameters[CFG_FORCE_UNLINK]);
	std::vector<std::string> newv;
	int newfLinkSize = getNamesAfterFlag(args, "--link-force", newv);
	auto it = args.begin();
	while(it != args.end()){
		if((*it).size() > 2 && std::string((*it).begin(), (*it).begin() + 2) == "-l" && (*it) != "-log"){
			//std::string shortname = std::string((*it).begin() + 2, (*it).end());
			if(find(newv, *it) == -1) {
				newv.push_back(*it);
				newfLinkSize++;
			}
			args.erase(it);
		}
		else it++;
	}
	int newfUnlinkSize = getNamesAfterFlag(args, "--no-link-force", newv);
	int newdefLinkSize = getNamesAfterFlag(args, "--default-link", newv);
	std::vector<std::string> fLibs, fUnLibs, defLibs;
	if(parameters[CFG_FORCE_LINK_LIBS] != "-1") fLibs = split(parameters[CFG_FORCE_LINK_LIBS]);
	if(parameters[CFG_FORCE_UNLINK_LIBS] != "-1") fUnLibs = split(parameters[CFG_FORCE_UNLINK_LIBS]);

	// Преобразование всех новых имен в полные пути
	std::vector<std::string> AddInc, fUnInc;
	if(parameters[CFG_ADD_INCLUDE] != "-1") AddInc = split(parameters[CFG_ADD_INCLUDE]);
	if(parameters[CFG_FORCE_UNLINK_DIRS] != "-1") fUnInc = split(parameters[CFG_FORCE_UNLINK_DIRS]);
	for(int i = 0; i < newv.size(); ++i){
		if(newv[i].size() < 2 || (newv[i].size() >= 2 && std::string(newv[i].begin(), newv[i].begin() + 2) != "-l")){
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
			std::string shortname = std::string((newv[i]).begin() + 2, (newv[i]).end());
			newv[i] = shortname;
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
					if(parameters[CFG_LINK_FLAGS] == "-1") parameters[CFG_LINK_FLAGS] = (" -l" + newv[i]);
					else if(parameters[CFG_LINK_FLAGS].find("-l" + newv[i]) == std::string::npos) 
						parameters[CFG_LINK_FLAGS] += (" -l" + newv[i]);
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
				else newv[i] = "remove lib";
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
		if(*it == "remove lib") {
			newv.erase(it);
			newfLinkSize--;
		}
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
		else if(i < (newfLinkSize + newfUnlinkSize)){
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

	if(fLink.size() == 0) parameters[CFG_FORCE_LINK] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fLink.size(); ++i) s += (fLink[i] + " ");
		parameters[CFG_FORCE_LINK] = s;
	}

	if(fUnlink.size() == 0) parameters[CFG_FORCE_UNLINK] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fUnlink.size(); ++i) s += (fUnlink[i] + " ");
		parameters[CFG_FORCE_UNLINK] = s;
	}

	if(fLibs.size() == 0) parameters[CFG_FORCE_LINK_LIBS] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fLibs.size(); ++i) s += (fLibs[i] + " ");
		parameters[CFG_FORCE_LINK_LIBS] = s;
	}

	if(fUnLibs.size() == 0) parameters[CFG_FORCE_UNLINK_LIBS] = "-1";
	else{
		std::string s;
		for(int i = 0; i < fUnLibs.size(); ++i) s += (fUnLibs[i] + " ");
		parameters[CFG_FORCE_UNLINK_LIBS] = s;
	}
}