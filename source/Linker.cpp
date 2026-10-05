#include "Linker.h"
#include "ConfigIndex.h"
#include "Process.h"
#include "Toolchain.h"
#include "Anal.h"

static bool readSyms(std::ifstream& file, binFile& newfile){
	unsigned long callNum = 0, defNum = 0;
	std::string line;
	if(!std::getline(file, line)) return false;
	try{ callNum = std::stoul(line); }
	catch(const std::exception&){ return false; }
	if(!std::getline(file, line)) return false;
	try{ defNum = std::stoul(line); }
	catch(const std::exception&){ return false; }
	for(unsigned long j = 0; j < callNum + defNum; ++j){
		if(!std::getline(file, line)) return false;
		if(line.size() < 3 || line[1] != ' ' || line[0] < '0' || line[0] > '2') return false;
		if(j < callNum){
			newfile.callStrong.push_back(line[0] - '0');
			newfile.callSyms.push_back(line.substr(2));
		}
		else{
			newfile.defStrong.push_back(line[0] - '0');
			newfile.defSyms.push_back(line.substr(2));
		}
	}
	return true;
}

// Возвращает true, если .sym-кэш прочитан успешно. На любом признаке порчи
// (файл не открылся, оборван, нечисловые счётчики) возвращает false - вызывающий
// код тогда перечитает символы из самого бинарника и перезапишет кэш.
bool readSymfile(binFile& newfile, const std::string& symFile){
	std::ifstream file(symFile);
	if(!file.is_open()) return false;
	unsigned long memberNum = 0;
	std::string line;
	for(int i = 0; i < 3; ++i)
		if(!std::getline(file, line)) return false;
	try{ newfile.arch = std::stoul(line); }
	catch(const std::exception&){ return false; }
	if(!readSyms(file, newfile)) return false;
	if(!std::getline(file, line)) return false;
	try{ memberNum = std::stoul(line); }
	catch(const std::exception&){ return false; }
	for(unsigned long j = 0; j < memberNum; ++j){
		binFile member = {};
		int parsed = 0;
		if(!std::getline(file, member.name) || !std::getline(file, line)) return false;
		std::istringstream in(line);
		if(!(in >> member.offset >> parsed >> member.arch)) return false;
		member.parsed = (parsed == 1);
		if(!readSyms(file, member)) return false;
		newfile.members.push_back(member);
	}
	return true;
}

static void writeSyms(std::ofstream& out, const binFile& newfile){
	out << newfile.callSyms.size() << std::endl;
	out << newfile.defSyms.size() << std::endl;
	for(int i = 0; i < newfile.callSyms.size(); ++i)
		out << int(i < newfile.callStrong.size() ? newfile.callStrong[i] : SYM_UNKNOWN) << " " << newfile.callSyms[i] << std::endl;
	for(int i = 0; i < newfile.defSyms.size(); ++i)
		out << int(i < newfile.defStrong.size() ? newfile.defStrong[i] : SYM_UNKNOWN) << " " << newfile.defSyms[i] << std::endl;
}

void createSymfile(binFile& newfile, const std::string& path){
	std::ofstream out(path);
	out << newfile.name << std::endl;
	out << getChangeTime(newfile.name) << std::endl;
	out << newfile.arch << std::endl;
	writeSyms(out, newfile);
	out << newfile.members.size() << std::endl;
	for(int i = 0; i < newfile.members.size(); ++i){
		out << newfile.members[i].name << std::endl;
		out << newfile.members[i].offset << " " << newfile.members[i].parsed << " " << newfile.members[i].arch << std::endl;
		writeSyms(out, newfile.members[i]);
	}
	out.close();
}

static std::vector<std::string> pairObjects(const std::string& wd, const std::vector<std::string>& pairSource){
	std::string objFolder = wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR;
	std::vector<std::string> allObj;
	for(int i = 0; i < pairSource.size(); ++i){
		std::string obj = objFolder + "/" + convertPathToName(pairSource[i]) + ".o";
		if(exists(obj)) allObj.push_back(obj);
	}
	return allObj;
}

std::vector<std::string> toLinkList(const std::vector<std::string>& parameters,
	const std::string& wd,const bool idgaf, const std::vector<std::string>& allLibs,
	const std::vector<std::string>& pairSource, const int linkType, std::vector<std::string>& unresolved){

	std::string objFolder = wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR;
	std::vector<std::string> allObj = pairObjects(wd, pairSource);
	std::vector<std::string> toLink;
	std::vector<binFile> filesInfo;
	toLink.push_back(objFolder + "/" + convertPathToName(parameters[CFG_ENTRY]) + ".o");
	if(!exists(toLink[0])) return std::vector<std::string>{};
	std::vector<std::string> forceLinkLibs, fLink;
	if(parameters[CFG_FORCE_LINK_LIBS] != "-1") forceLinkLibs = split(parameters[CFG_FORCE_LINK_LIBS]);
	if(parameters[CFG_FORCE_LINK] != "-1") fLink = split(parameters[CFG_FORCE_LINK]);
    OneThreadObjAnal(wd, allObj, filesInfo);
	for(int i = 0; i < fLink.size(); ++i){
		int index = -1;
		for(int j = 0; j < filesInfo.size(); ++j){
			if(filesInfo[j].name == (objFolder + "/" + convertPathToName(fLink[i]) + ".o")){
				index = j;
				break;
			}
		}
		if(index == -1){
			std::cerr << "====================== ERROR ======================" << std::endl;
			std::cerr << "Cannot find file: " << fLink[i] << std::endl;
			std::cerr << "You specified this file as force link" << std::endl;
			std::cerr << "You can add directories with -I flag" << std::endl;
			std::cerr << std::endl;
			return std::vector<std::string>{};
		}
		if(find(toLink, filesInfo[index].name) == -1) toLink.push_back(filesInfo[index].name);
	}
	for(int i = 0; i < forceLinkLibs.size(); ++i){
		if(!exists(forceLinkLibs[i])){
			std::cerr << "====================== ERROR ======================" << std::endl;
			std::cerr << "Cannot find file: " << forceLinkLibs[i] << std::endl;
			std::cerr << "You specified this file as force link" << std::endl;
			std::cerr << "You can add directories with -I flag" << std::endl;
			std::cerr << "===================================================" << std::endl;
			return std::vector<std::string>{};
		}
	}

	int code = findLinks(toLink, filesInfo, forceLinkLibs, allLibs, parameters, wd, idgaf, linkType, unresolved);
	if(code != 0) return std::vector<std::string>{};
	return toLink;
}
void OneThreadObjAnal(const std::string& wd, const std::vector<std::string>& dirs, std::vector<binFile>& filesInfo){

	// ------------- OBJ ANAL -------------
	for(int i = 0; i < dirs.size(); ++i){
		std::string symFile = (wd + "/" + SYM_DIR + "/" + getNameNoExt(dirs[i]) + ".sym");
		binFile newfile = {dirs[i]};
		bool ok = exists(symFile) && readSymfile(newfile, symFile);
		if(!ok){ // кэша нет или он повреждён - перечитываем из объектника
			newfile = {dirs[i]};
			parse_ELF_File(newfile);
			createSymfile(newfile, symFile);
		}
		filesInfo.push_back(newfile);
	}
}
void LibAnal(const std::string& wd, binFile& newfile){
	// ------------- LIB ANAL -------------
	std::string symFile = symfilePath(wd, newfile.name);
	bool ok = readFreshSymfile(newfile, symFile);
	if(!ok){ // кэша нет или он повреждён - перечитываем из библиотеки
		newfile = {newfile.name};
		std::string libType = getLibType(newfile.name);
		if(libType == "a") parse_ARLIB(newfile);
		else parse_ELF_File(newfile);
		saveSymfile(newfile, symFile);
	}
}
static std::vector<std::string> linkState(const std::vector<std::string>& toLink,
	const std::vector<std::string>& libsToLink){

	std::vector<std::string> state;
	for(int i = 0; i < toLink.size(); ++i)
		state.push_back(toLink[i] + " " + getChangeTime(toLink[i]));
	for(int i = 0; i < libsToLink.size(); ++i)
		state.push_back(libsToLink[i] + " " + getChangeTime(libsToLink[i]));
	return state;
}

static bool isLinkUpToDate(const std::string& recordPath, const std::string& output,
	const std::vector<std::string>& objects){

	if(!exists(output)) return false;
	std::vector<std::string> record;
	std::string line;
	std::ifstream in(recordPath);
	while(std::getline(in, line)) record.push_back(line);
	in.close();
	const std::vector<std::string> state = linkState(objects, {});
	if(record.size() < state.size() + 2 || record[0] != getChangeTime(output) ||
		record[1] != std::to_string(objects.size()) ||
		!std::equal(state.begin(), state.end(), record.begin() + 2)) return false;
	for(int i = state.size() + 2; i < record.size(); ++i){
		size_t space = record[i].rfind(' ');
		if(space == std::string::npos) return false;
		std::string lib = record[i].substr(0, space);
		if(!exists(lib) || getChangeTime(lib) != record[i].substr(space + 1)) return false;
	}
	return true;
}

static void writeLinkRecord(const std::string& recordPath, const std::string& output,
	const std::vector<std::string>& state){

	std::ofstream out(recordPath);
	out << getChangeTime(output) << std::endl;
	for(int i = 0; i < state.size(); ++i) out << state[i] << std::endl;
	out.close();
}

std::string link(const std::string& wd, const std::string& pairDir,
	const std::vector<std::string>& parameters,
	const std::vector<std::string>& includes, 
	const std::vector<std::string>& toCompile,
	const bool log, const int linkType, const bool relink,
	const bool idgaf, const std::vector<std::string>& allLibs,
	const std::vector<std::string>& pairSource)
{
	if(toCompile.size() != 0 && toCompile[0] == "-1")
		return "compilation error";
	//if(toCompile.size() == 0 && exists(parameters[CFG_OUTPUT]) && !relink)
	//	return "nothing to link";
	const std::string recordPath = pairDir + "/" + LINK_RECORD_FILE;
	const std::vector<std::string> objects = pairObjects(wd, pairSource);
	if(!relink && isLinkUpToDate(recordPath, parameters[CFG_OUTPUT], objects))
		return "nothing to link";
	std::vector<std::string> unresolved;
	std::vector<std::string> toLink = toLinkList(parameters,wd,idgaf,allLibs,pairSource,linkType,unresolved);
	if(toLink.size() == 0) return "link error";
	std::vector<std::string> libsToLink, sharedLibDirs;
	auto iter = toLink.begin();
	while(iter != toLink.end()){
		std::string libType = getLibType(*iter);
		if(libType != "") {
			libsToLink.push_back(*iter);
			// Разделяемой библиотеке нужен -rpath, иначе динамический загрузчик
			// не найдет .so во время запуска. Собираем уникальные каталоги .so.
			if(libType == "so"){
				std::string dir = getFolder(*iter);
				if(!dir.empty() && find(sharedLibDirs, dir) == -1 && !isStandardLibDir(dir, parameters))
					sharedLibDirs.push_back(dir);
			}
			toLink.erase(iter);
		}
		else iter++;
	}
	if(toLink.size() == 0) return "nothing to link";

	std::string objFolder = wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR;
	if(exists(parameters[CFG_OUTPUT])) removeFile(parameters[CFG_OUTPUT]);
	std::string outputFolder = getFolder(parameters[CFG_OUTPUT]);
	if(!outputFolder.empty() && !exists(outputFolder)) createDirectory(outputFolder);
	auto it = toLink.begin();
	while(it != toLink.end()){
		bool erase = false;
		for(int i = 0; i < includes.size(); ++i){
			if((*it) == (objFolder + "/" + convertPathToName(includes[i]) + ".o") && 
				getExt(includes[i]) != "h" && getExt(includes[i]) != "hpp")
			{
				toLink.erase(it);
				erase = true;
				break;
			}
		}
		if(!erase) it++;
	}

	if(!log){
		std::cout << std::endl;
		for(int i = 0; i < toLink.size(); ++i){ // очень криво
			std::ifstream file(wd + "/" + SOURCE_DIR + "/" + DEPS_DIR + "/" + getNameNoExt(toLink[i]));
			std::string line;
			std::getline(file, line);
			file.close();
			std::cout << "Linking file: " << getName(line) << std::endl;
		}
		for(int i = 0; i < libsToLink.size(); ++i)
			std::cout << "Linking lib: " << getName(libsToLink[i]) << std::endl;
		std::cout << std::endl;
	}

	int code = -1;
	if(linkType == 0 || linkType == 2){
		std::string compiler;
		std::vector<std::string> compilers = split(parameters[CFG_COMPILERS]);
		if(getExt(parameters[CFG_ENTRY]) == "cpp"){
			if(compilers[1] == "default") compiler = "g++";
			else compiler = compilers[1];
		}
        else{
        	if(compilers[0] == "default") compiler = "gcc";
			else compiler = compilers[0];
		}
		std::vector<std::string> argv;
		appendArgs(argv, compiler);
		if(linkType == 2) argv.push_back("-shared");
		for(int i = 0; i < toLink.size(); ++i) argv.push_back(toLink[i]);
		for(int i = CFG_LINK_FLAGS; i <= CFG_GENERAL_FLAGS; ++i)
			if(parameters[i] != "-1") appendArgs(argv, parameters[i]);
		if(libsToLink.size() != 0) argv.push_back("-Wl,--start-group");
		for(int i = 0; i < libsToLink.size(); ++i) argv.push_back(libsToLink[i]);
		if(libsToLink.size() != 0) argv.push_back("-Wl,--end-group");
		// rpath на каталоги подключаемых .so, чтобы загрузчик нашел их в рантайме
		for(int i = 0; i < sharedLibDirs.size(); ++i)
			argv.push_back("-Wl,-rpath," + sharedLibDirs[i]);
		argv.push_back("-o");
		argv.push_back(parameters[CFG_OUTPUT]);
		if(log) std::cout << joinArgs(argv) << std::endl;
		code = runProcess(argv);
	}
	else if(linkType == 1){ // статическая библиотека
		std::vector<std::string> argv = {"ar", "rcs", parameters[CFG_OUTPUT]};
		for(int i = 0; i < toLink.size(); ++i) argv.push_back(toLink[i]);
		if(log) std::cout << joinArgs(argv) << std::endl;
		code = runProcess(argv);
	}
	else{
		std::cerr << "===================== ERROR =====================" << std::endl;
		std::cerr << "WTF unknown linkType: " << linkType << std::endl;
		std::cerr << "This is internal belder error, recompile belder" << std::endl;
		std::cerr << std::endl;
		return "nothing to link";
	}

	if(code != 0){
		removeFile(recordPath);
		printUnresolved(unresolved);
		return "link error";
	}
	std::vector<std::string> state = {std::to_string(objects.size())};
	state += linkState(objects, libsToLink);
	writeLinkRecord(recordPath, parameters[CFG_OUTPUT], state);

	// Каталоги подключаемых .so теперь прописываются в сам бинарник через
	// -Wl,-rpath (см. сборку argv выше), поэтому LD_LIBRARY_PATH больше не нужен.
	return "success";
}
