#include "Linker.h"

std::vector<std::string> toLinkList(const std::vector<std::string>& parameters,
	const std::string& wd,const bool idgaf, const std::vector<std::string>& allLibs){

	std::string objFolder = wd + "/" + reqFolders[1] + "/" + subFolders[1];
	auto allObj = getDirs(objFolder);
	allObj.erase(allObj.begin());
	std::vector<std::string> toLink;
	toLink.push_back(getName(parameters[0]));
	std::vector<binFile> filesInfo;
	binFile mainObj = {"main"};

	// SOME LIBRARY ACTION

	std::vector<std::string> forceLinkLibs, forceUnlinkLibs;
	if(parameters[2] != "-1") forceLinkLibs = split(parameters[2]);
	if(parameters[13] != "-1") forceUnlinkLibs = split(parameters[13]);
	for(int i = 0; i < forceLinkLibs.size(); ++i){
		std::string longName = ("lib" + forceLinkLibs[i] + ".so");
		for(int i = 0; i < allLibs.size(); ++i){
			if(getName(allLibs[i]) == longName){
				forceLinkLibs[i] = longName;
				continue;	
			}
		}
		longName = ("lib" + forceLinkLibs[i] + ".a");
		for(int i = 0; i < allLibs.size(); ++i){
			if(getName(allLibs[i]) == longName){
				forceLinkLibs[i] = longName;
				continue;
			}
		}
		std::cout << "==================== ERROR ====================" << std::endl;
		std::cout << "Cannot find library: " << forceLinkLibs[i] << std::endl;
		std::cout << "You specified it as a force link library" << std::endl;
		std::cout << std::endl;
		return std::vector<std::string>{};
	}
	for(int i = 0; i < forceUnlinkLibs.size(); ++i){
		std::string longName = ("lib" + forceUnlinkLibs[i] + ".so");
		for(int i = 0; i < allLibs.size(); ++i){
			if(getName(allLibs[i]) == longName){
				forceUnlinkLibs[i] = longName;
				continue;	
			}
		}
		longName = ("lib" + forceUnlinkLibs[i] + ".a");
		for(int i = 0; i < allLibs.size(); ++i){
			if(getName(allLibs[i]) == longName){
				forceUnlinkLibs[i] = longName;
				continue;	
			}
		}
		std::cout << "==================== ERROR ====================" << std::endl;
		std::cout << "Cannot find library: " << forceUnlinkLibs[i] << std::endl;
		std::cout << "You specified it as a force unlink library" << std::endl;
		std::cout << std::endl;
		return std::vector<std::string>{};
	}

	// END SOME LIBRARY ACTION ---------------

    OneThreadObjAnal(parameters[0],mainObj,allObj,allLibs,filesInfo);
    std::map<std::string, std::string> syms;
	std::vector<std::string> fLink, fUnlink;
	if(parameters[3] != "-1") fLink = split(parameters[3]); 
	int code = findLinks(toLink, filesInfo, mainObj, syms, idgaf);
	for(int i = 0; i < fLink.size(); ++i){
		bool b = true;
		binFile file;
		for(int j = 0; j < filesInfo.size(); ++j){
			if(filesInfo[j].name == fLink[i]){
				file = filesInfo[j];
				b = false;
			}
		}
		if(b){
			std::cout << "====================== ERROR ======================" << std::endl;
			std::cout << "Cannot find file: " << fLink[i] << std::endl;
			std::cout << "You specified this file as force link" << std::endl;
			std::cout << "===================================================" << std::endl;
			return std::vector<std::string>{};
		}
		code |= findLinks(toLink, filesInfo, file, syms,idgaf);
	}
	if(parameters[4] != "-1") fUnlink = split(parameters[4]);
	toLink -= fUnlink;
	if(code == 0) return toLink;
	else return std::vector<std::string>{};
}
void OneThreadObjAnal(const std::string& name,binFile& mainObj,
	const std::vector<std::string>& dirs,const std::vector<std::string>& allLibs,
	std::vector<binFile>& filesInfo){

	// ------------- OBJ ANAL -------------
	std::string onlyName = getName(name);
	for(int i = 0; i < dirs.size(); ++i){
		binFile newfile = parse_ELF_File(dirs[i]);
		filesInfo.push_back(newfile);
		if(newfile.name == onlyName){
			mainObj.name = std::move(newfile.name);
			mainObj.callSyms = std::move(newfile.callSyms);
			mainObj.defSyms = std::move(newfile.defSyms);
		}
	}
	// ------------- LIB ANAL -------------
	for(int i = 0; i < allLibs.size(); ++i){
		std::string ext = getExt(allLibs[i]);
		if(ext == "so") filesInfo.push_back(parse_ELF_File(allLibs[i]));
		else if(ext == "a"){
			std::cout << "ABOBA ABOBA" << std::endl;
			std::cout << std::endl;
		}
		else{
			std::cout << "==================== ERROR ====================" << std::endl;
			std::cout << "UNKNOWN LIB EXT: " << ext << std::endl;
			std::cout << std::endl;
		}
	}

}
int findLinks(std::vector<std::string>& toLink, const std::vector<binFile>& filesInfo,
	const binFile& file, std::map<std::string,std::string>& syms, const bool idgaf)
{

	// syms : <sym_name, file_name>
	// syms служит для отслеживания конфликтов
	for(int i = 0; i < file.callSyms.size(); ++i){
		for(int j = 0; j < filesInfo.size(); ++j){
			if(find(filesInfo[j].defSyms, file.callSyms[i]) != -1){ // Нашли совпадение
				if(syms.find(file.callSyms[i]) != syms.end() &&
					syms[file.callSyms[i]] != filesInfo[j].name && !idgaf)
				{ // Уже такой был => уже определили => конфликт
					std::cout << "=================== ERROR ===================" << std::endl;
					std::cout << "multiple definition of symbol: " << std::endl;
					std::cout << file.callSyms[i] << std::endl;
					std::cout << std::endl;
					std::cout << "First definition in file: " << syms[file.callSyms[i]] << ".o" << std::endl;
					std::cout << "Second definition in file: " << filesInfo[j].name << ".o" << std::endl;
					std::cout << std::endl;
					std::cout << "You can choose not to link files forcibly by using the flag: --no-link-force [filename]" << std::endl;
					std::cout << "Or you can run builder with --idgaf flag to ignore this error" << std::endl;
					std::cout << std::endl;
					std::cout << "=============================================" << std::endl;
					return -1;
				}
				// Конфликтов нет, либо мы их игнорируем:
				if(find(toLink, filesInfo[j].name) == -1){
					toLink.push_back(filesInfo[j].name);
					syms[file.callSyms[i]] = filesInfo[j].name;
					findLinks(toLink, filesInfo, filesInfo[j], syms, idgaf);
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

	if(toCompile.size() == 0 && exists(parameters[1]) && !relink)
		return "nothing to link";

	std::vector<std::string> toLink = toLinkList(parameters,wd,idgaf, allLibs);
	if(toLink.size() == 0)
		return "nothing to link";
	
	bool linking = false;
	for(int i = 0; i < toLink.size(); ++i){
		for(int j = 0; j < toCompile.size(); ++j){
			if(getName(toCompile[j]) == toLink[i]){
				linking = true;
				break;
			}
		}
		if(linking) break;
	}
	if(!exists(parameters[1]) || relink) linking = true;
	if(!linking) return "nothing to link";

	if(exists(parameters[1])){
		std::string cmd = "rm " + parameters[1];
		system(cmd.c_str());
	}
	if(!log){
		std::cout << std::endl;
		for(int i = 0; i < toLink.size(); ++i)
			std::cout << "Linking file: " << toLink[i] << ".o" << std::endl;
		std::cout << std::endl;
	}

	auto it = toLink.begin();
	while(it != toLink.end()){
		bool erase = false;
		for(int i = 0; i < includes.size(); ++i){
			if((*it) == getName(includes[i]) && getExt(includes[i]) != "h" &&
				getExt(includes[i]) != "hpp"){
				toLink.erase(it);
				erase = true;
				break;
			}
		}
		if(!erase) it++;
	}

	for(int i = 0; i < toLink.size(); ++i){
		std::string path = wd + "/" + reqFolders[1] + "/" + subFolders[1] + "/" + toLink[i] + ".o";
		toLink[i] = path;
	}

	if(linkType == 0){
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
		for(int i = 11; i <= 12; ++i){
			if(parameters[i] != "-1")
				cmd += (parameters[i] + " ");
		}
		for(int i = 0; i < toLink.size(); ++i)
			cmd += (toLink[i] + " ");
		//cmd += (libFlags + " ");
		cmd += (" -o " + parameters[1]);
		if(log){
			std::cout << std::endl;
			std::cout << "Linking" << std::endl;
			std::cout << cmd << std::endl;
		}
		system(cmd.c_str());
	}
	// else if(linkType == 1){
	// 	std::string cmd = "ar rc " + parameters[1] + " ";
	// 	for(int i = 0; i < toLink.size(); ++i)
	// 		cmd += (toLink[i] + " ");
	// 	if(log){
	// 		std::cout << std::endl;
	// 		std::cout << "Linking" << std::endl;
	// 		std::cout << cmd << std::endl;
	// 	}
	// 	system(cmd.c_str());
	// } // TODO

	// if(parameters[2] != "-1" && libDirs.size() > 0){
	// 	std::string cmd = postSharedLink;
	// 	for(int i = 0; i < libDirs.size(); ++i)
	// 		cmd += (":" + libDirs[i]);
	// 	if(log){
	// 		std::cout << std::endl;
	// 		std::cout << cmd << std::endl;
	// 	}
	// 	system(cmd.c_str());
	// }
	return "succes";
}