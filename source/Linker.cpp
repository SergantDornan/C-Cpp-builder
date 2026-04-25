#include "Linker.h"

void readSymfile(binFile& newfile, const std::string& symFile){
	std::ifstream file(symFile);
	unsigned long callNum = 0, defNum = 0;
	std::string line;
	for(int i = 0; i < 3; ++i) std::getline(file, line);
	callNum = std::stoul(line);
	std::getline(file, line);
	defNum = std::stoul(line);
	for(unsigned long j = 0; j < callNum; ++j){
		std::getline(file, line);
		newfile.callSyms.push_back(line);
	}
	for(unsigned long j = 0; j < defNum; ++j){
		std::getline(file, line);
		newfile.defSyms.push_back(line);
	}
	file.close();
}

void createSymfile(binFile& newfile, const std::string& path){
	std::ofstream out(path);
	out << newfile.name << std::endl;
	out << getChangeTime(newfile.name) << std::endl;
	out << newfile.callSyms.size() << std::endl;
	out << newfile.defSyms.size() << std::endl;
	for(int i = 0; i < newfile.callSyms.size(); ++i)
		out << newfile.callSyms[i] << std::endl;
	for(int i = 0; i < newfile.defSyms.size(); ++i)
		out << newfile.defSyms[i] << std::endl;
	out.close();
}

std::vector<std::string> toLinkList(const std::vector<std::string>& parameters,
	const std::string& wd,const bool idgaf, const std::vector<std::string>& allLibs){

	std::string objFolder = wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR;
	auto allObj = getDirs(objFolder);
	allObj.erase(allObj.begin());
	std::vector<std::string> toLink;
	std::vector<binFile> filesInfo;
	binFile mainObj = {"main"};
	toLink.push_back(objFolder + "/" + convertPathToName(parameters[0]) + ".o");
	std::vector<std::string> forceLinkLibs, fLink;
	if(parameters[2] != "-1") forceLinkLibs = split(parameters[2]);
	if(parameters[3] != "-1") fLink = split(parameters[3]);
    OneThreadObjAnal(wd,(objFolder + "/" + convertPathToName(parameters[0]) + ".o"),
    	mainObj,allObj,allLibs,filesInfo);
    unsigned long x = 0;
    std::map<std::string, std::string> syms;
    std::vector<binFile> binLink;
    binLink.push_back(mainObj); // Эта штука нужна только для повторной проверки на конфликты
	int code = findLinks(toLink, filesInfo, mainObj, syms, idgaf, binLink,wd);
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
		code |= findLinks(toLink, filesInfo, filesInfo[index], syms,idgaf,binLink,wd);
	}
	for(int i = 0; i < forceLinkLibs.size(); ++i){
		int index = -1;
		for(int j = 0; j < filesInfo.size(); ++j){
			if(filesInfo[j].name == forceLinkLibs[i]){
				index = j;
				break;
			}
		}
		if(index == -1){
			std::cerr << "====================== ERROR ======================" << std::endl;
			std::cerr << "Cannot find file: " << forceLinkLibs[i] << std::endl;
			std::cerr << "You specified this file as force link" << std::endl;
			std::cerr << "You can add directories with -I flag" << std::endl;
			std::cerr << "===================================================" << std::endl;
			return std::vector<std::string>{};
		}
		if(find(toLink, filesInfo[index].name) == -1) toLink.push_back(filesInfo[index].name);
		code |= findLinks(toLink, filesInfo, filesInfo[index], syms, idgaf,binLink,wd);
	}

	if(code != 0) return std::vector<std::string>{};
	// ЕЩЕ ОДНА ПРОВЕРКА НА КОНФЛИКТЫ, очень нужная (первую не уберу потому что страшно)
	for(int i = 0; i < binLink.size()-1; ++i){
		for(int j = i + 1; j < binLink.size(); ++j){
			for(int h = 0; h < binLink[j].defSyms.size(); ++h){
				if(find(binLink[i].defSyms, binLink[j].defSyms[h]) != -1 && !idgaf){
					std::cerr << "=================== ERROR ===================" << std::endl;
					std::cerr << "multiple definition of symbol: " << std::endl;
					std::cerr << binLink[j].defSyms[h] << std::endl;
					std::cerr << std::endl;
					std::ifstream file(wd + "/" + SOURCE_DIR + "/" + DEPS_DIR + "/" + getNameNoExt(binLink[i].name));
					std::string line;
					std::getline(file, line);
					file.close();
					std::cerr << "First definition in file: " << getName(line) <<  std::endl;
					std::ifstream file1(wd + "/" + SOURCE_DIR + "/" + DEPS_DIR + "/" + getNameNoExt(binLink[j].name));
					std::getline(file1, line);
					file1.close();
					std::cerr << "Second definition in file: " << getName(line) << std::endl;
					std::cerr << std::endl;
					std::cerr << "You can choose not to link files forcibly by using the flag: --no-link-force [filename]" << std::endl;
					std::cerr << "Or you can run builder with --idgaf flag to ignore this error" << std::endl;
					std::cerr << std::endl;
					std::cerr << std::endl;
					return std::vector<std::string>{};
				}
			}
		}
	}
	return toLink;
}
void OneThreadObjAnal(const std::string& wd, const std::string& name,binFile& mainObj,
	const std::vector<std::string>& dirs,const std::vector<std::string>& allLibs,
	std::vector<binFile>& filesInfo){

	// ------------- OBJ ANAL -------------
	for(int i = 0; i < dirs.size(); ++i){
		std::string symFile = (wd + "/" + SYM_DIR + "/" + getNameNoExt(dirs[i]) + ".sym");
		binFile newfile = {dirs[i]};
		if(exists(symFile)) readSymfile(newfile, symFile);
		else{
			parse_ELF_File(newfile);
			createSymfile(newfile, symFile);
		}
		filesInfo.push_back(newfile);
		if(newfile.name == name){
			mainObj.name = std::move(newfile.name);
			mainObj.callSyms = std::move(newfile.callSyms);
			mainObj.defSyms = std::move(newfile.defSyms);
		}
	}
	// ------------- LIB ANAL -------------
	for(int i = 0; i < allLibs.size(); ++i){
		std::string symFile = (wd + "/" + SYM_DIR + "/" + convertPathToName(allLibs[i]) + ".sym");
		
		binFile newfile = {allLibs[i]};
		if(exists(symFile)) readSymfile(newfile, symFile);
		else{
			std::string ext = getExt(allLibs[i]);
			if(ext == "so") parse_ELF_File(newfile);
			else if(ext == "a") parse_ARLIB(newfile);
			createSymfile(newfile, symFile);
		}
		filesInfo.push_back(newfile);
	}
}
int findLinks(std::vector<std::string>& toLink, const std::vector<binFile>& filesInfo,
	const binFile& file, std::map<std::string,std::string>& syms, const bool idgaf,
	std::vector<binFile>& binLink, const std::string& wd)
{

	// syms : <sym_name, file_name>
	// syms служит для отслеживания конфликтов
	for(int i = 0; i < file.callSyms.size(); ++i){
		for(int j = 0; j < filesInfo.size(); ++j){
			if(find(filesInfo[j].defSyms, file.callSyms[i]) != -1){ // Нашли совпадение
				if(syms.find(file.callSyms[i]) != syms.end() &&
					syms[file.callSyms[i]] != filesInfo[j].name && !idgaf)
				{ // Уже такой был => уже определили => конфликт
					std::cerr << "=================== ERROR ===================" << std::endl;
					std::cerr << "multiple definition of symbol: " << std::endl;
					std::cerr << file.callSyms[i] << std::endl;
					std::cerr << std::endl;
					std::ifstream file0(wd + "/" + SOURCE_DIR + "/" + DEPS_DIR + "/" + getNameNoExt(syms[file.callSyms[i]]));
					std::string line;
					std::getline(file0, line);
					file0.close();
					std::cerr << "First definition in file: " << getName(line) << std::endl;
					std::ifstream file1(wd + "/" + SOURCE_DIR + "/" + DEPS_DIR + "/" + getNameNoExt(filesInfo[j].name));
					std::getline(file1, line);
					file1.close();
					std::cerr << "Second definition in file: " << getName(line) << std::endl;
					std::cerr << std::endl;
					std::cerr << "You can choose not to link files forcibly by using the flag: --no-link-force [filename]" << std::endl;
					std::cerr << "Or you can run builder with --idgaf flag to ignore this error" << std::endl;
					std::cerr << std::endl;
					std::cerr << std::endl;
					return 1;
				}
				// Конфликтов нет, либо мы их игнорируем:
				if(find(toLink, filesInfo[j].name) == -1){
					toLink.push_back(filesInfo[j].name);
					binLink.push_back(filesInfo[j]);
					syms[file.callSyms[i]] = filesInfo[j].name;
					findLinks(toLink, filesInfo, filesInfo[j], syms, idgaf, binLink,wd);
				}
			}
		}
	}
	return 0;
}	
std::string link(const std::string& wd, 
	const std::vector<std::string>& parameters,
	const std::vector<std::string>& includes, 
	const std::vector<std::string>& toCompile,
	const bool log, const int linkType, const bool relink,
	const bool idgaf, const std::vector<std::string>& allLibs)
{
	if(toCompile.size() != 0 && toCompile[0] == "-1")
		return "compilation error";
	//if(toCompile.size() == 0 && exists(parameters[1]) && !relink)
	//	return "nothing to link";
	std::vector<std::string> toLink = toLinkList(parameters,wd,idgaf,allLibs);
	std::vector<std::string> libsToLink, sharedLibDirs;
	auto iter = toLink.begin();
	while(iter != toLink.end()){
		if(getExt(*iter) == "so" || getExt(*iter) == "a") {
			libsToLink.push_back(*iter);
			toLink.erase(iter);
		}
		else iter++;
	}
	if(toLink.size() == 0) return "nothing to link";

	bool linking = updateOutputFiles(toCompile, wd, parameters, toLink) | relink; 
	if(!linking) return "nothing to link";

	std::string objFolder = wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR;
	if(exists(parameters[1])) removeFile(parameters[1]);
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
		std::vector<std::string> compilers = split(parameters[5]);
		if(getExt(parameters[0]) == "cpp"){
			if(compilers[1] == "default") compiler = "g++ ";
			else compiler = (compilers[1] + " ");
		}
        else{
        	if(compilers[0] == "default") compiler = "gcc ";
			else compiler = (compilers[0] + " ");
		}
		std::string cmd = compiler;
		if(linkType == 2) cmd += "-shared ";
		for(int i = 0; i < toLink.size(); ++i) cmd += (toLink[i] + " ");
		for(int i = 11; i <= 12; ++i){
			if(parameters[i] != "-1")
				cmd += (parameters[i] + " ");
		}
		for(int i = 0; i < libsToLink.size(); ++i) cmd += (libsToLink[i] + " ");
		cmd += (" -o " + parameters[1]);
		if(log) std::cout << cmd << std::endl;
		code = system(cmd.c_str());
	}
	else if(linkType == 1){ // статическая библиотека
		std::string cmd = "ar rcs " + parameters[1] + " ";
		for(int i = 0; i < toLink.size(); ++i) cmd += (toLink[i] + " ");
		if(log) std::cout << cmd << std::endl;
		code = system(cmd.c_str());
	}
	else{
		std::cerr << "===================== ERROR =====================" << std::endl;
		std::cerr << "WTF unknown linkType: " << linkType << std::endl;
		std::cerr << "This is internal belder error, recompile belder" << std::endl;
		std::cerr << std::endl;
		return "nothing to link";
	}

	const std::string curr_config = wd + "/" + OUTPUT_CONFIGS_FOLDER + "/" + convertPathToName(parameters[0]) + "_" + convertPathToName(parameters[1]);
	std::ofstream out(curr_config);
	out << ((code == 0) ? "true" : "false") << std::endl;
	for(int i = 0; i < parameters.size(); ++i)
		out << parameters[i] << std::endl;
	out.close();

	if(sharedLibDirs.size() > 0){
		std::string cmd = POST_SHARED_LINK;
		for(int i = 0; i < sharedLibDirs.size(); ++i)
			cmd += (":" + sharedLibDirs[i]);
		if(log){
			std::cout << std::endl;
			std::cout << cmd << std::endl;
		}
		system(cmd.c_str());
	}
	return "success";
}

bool updateOutputFiles(const std::vector<std::string>& toCompile,
					   const std::string& wd, 
					   const std::vector<std::string>& curr_parameters,
					   const std::vector<std::string>& curr_toLink){

	
	const std::string output_configs_folder = wd + "/" + OUTPUT_CONFIGS_FOLDER;
	if(!exists(output_configs_folder))
		createDirectory(output_configs_folder);
	auto configs = getDirs(output_configs_folder);

	// ----- обработка текущего файла
	std::string is_updated;
	const std::string curr_config = output_configs_folder + "/" + convertPathToName(curr_parameters[0]) + "_" + convertPathToName(curr_parameters[1]);
	int index = -1;
	for(int i = 1; i < configs.size(); ++i){
		if(configs[i] == curr_config){
			index = i;
			break;
		}
	}
	if(index == -1)
		is_updated = "false";
	else{
		std::ifstream in(curr_config);
		std::getline(in, is_updated);
		in.close();
	}
	const std::string objFolder = wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR; 
	for(int i = 0; i < curr_toLink.size(); ++i){
		for(int j = 0; j < toCompile.size(); ++j){
			if((objFolder + "/" + getName(toCompile[j]) + ".o") == curr_toLink[i]){
				is_updated = "false";
				break;
			}
		}
		if(is_updated == "false") break;
	}
	bool res = (is_updated == "false");

	// true / false (is up to date)
	// parameters

	for(int i = 1; i < configs.size(); ++i){
		if(configs[i] == curr_config) continue;
		std::string new_is_updated;
		std::vector<std::string> parameters, allLibs;
		std::ifstream in(configs[i]);
		std::getline(in, new_is_updated);
		std::string line;
		while(std::getline(in,line)) parameters.push_back(line);
		in.close();

		std::vector<std::string> toLink = toLinkList(parameters, wd, true, std::vector<std::string>{});
		for(int i = 0; i < toLink.size(); ++i){
			for(int j = 0; j < toCompile.size(); ++j){
				if((objFolder + "/" + getName(toCompile[j]) + ".o") == toLink[i]){
					new_is_updated = "false";
					break;
				}
			}
			if(new_is_updated == "false") break;
		}

		std::ofstream out(configs[i]);
		out << new_is_updated << std::endl;
		for(int i = 0; i < parameters.size(); ++i)
			out << parameters[i] << std::endl;
		out.close();
	}
	return res;
}