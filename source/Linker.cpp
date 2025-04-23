#include "Linker.h"
int pairFind(const std::vector<std::pair<std::string,std::string>>& v, const std::string& s){
	for(int i = 0; i < v.size(); ++i){
		if(v[i].first == s)
			return i;
	}
	return -1;
}
std::vector<std::string> toLinkList(const std::vector<std::string>& parameters,
	const std::string& wd){

	std::string asmFolder = wd + "/" + reqFolders[1] + "/" + subFolders[1];
	auto allAsm = getDirs(asmFolder);
	allAsm.erase(allAsm.begin());
	std::vector<std::string> toLink;
	toLink.push_back(getName(parameters[0]));
	std::vector<asmFile> filesInfo;
	asmFile mainAsm("main");
	int m = (allAsm.size() / numT) + 1;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> multiLink;
    //
    multiLink.push_back(std::vector<std::string>(allAsm.begin(), allAsm.end()));
    OneThreadAsmAnal(parameters[0],mainAsm,multiLink[0],filesInfo);

    //


    
    // for(int i = 0; i < numT && i*m < allAsm.size(); ++i)
    //     multiLink.push_back(std::vector<std::string>(allAsm.begin() + i*m,
    //      allAsm.begin() + (((i+1)*m < allAsm.size()) ? (i+1)*m : allAsm.size())));
    // for (int i = 0; i < multiLink.size(); ++i)
    //     threads.push_back(std::thread(OneThreadAsmAnal,
    //     	std::ref(parameters[0]),std::ref(mainAsm),
    //     	std::ref(multiLink[i]),std::ref(filesInfo)));
    // for (auto& thread : threads) {
    //     if (thread.joinable())
    //         thread.join(); 
    // }
    std::vector<std::string> fUnlink;
    if(parameters[4] != "-1")
    	fUnlink = split(parameters[4]);
	std::vector<std::pair<std::string,std::string>> funcs,vars;
	int code = findLinks(toLink, filesInfo, mainAsm, fUnlink, funcs, vars);
	if(code == 0)
		return toLink;
	else
		return std::vector<std::string>{};
}
void OneThreadAsmAnal(const std::string& name,asmFile& mainAsm,
	const std::vector<std::string>& dirs,
	std::vector<asmFile>& filesInfo){

	std::string onlyName = getName(name);
	for(int i = 0; i < dirs.size(); ++i){
		asmFile newfile(getNameNoExt(dirs[i]));
		std::string line;
		std::ifstream in(dirs[i]);
		while(std::getline(in, line)) newfile.add(line);
		in.close();
		filesInfo.push_back(newfile);
		if(newfile.name == onlyName)
			mainAsm = newfile;
	}
}
int findLinks(std::vector<std::string>& toLink, const std::vector<asmFile>& filesInfo,
	const asmFile& file, const std::vector<std::string>& fUnlink, 
	std::vector<std::pair<std::string,std::string>>& funcs,
	std::vector<std::pair<std::string,std::string>>& vars){
	for(int i = 0; i < file.callFuncs.size(); ++i){
		for(int j = 0; j < filesInfo.size(); ++j){
			if(find(filesInfo[j].defFuncs, file.callFuncs[i]) != -1 &&
				find(fUnlink,filesInfo[j].name) == -1){
				if(pairFind(funcs, file.callFuncs[i]) != -1){
					std::cout << "=================== ERROR ===================" << std::endl;
					std::cout << "multiple definition of: " << std::endl;
					std::cout << file.callFuncs[i] << std::endl;
					std::cout << std::endl;
					std::cout << "First definition in file: " << funcs[pairFind(funcs, file.callFuncs[i])].second << ".asm" << std::endl;
					std::cout << "Second definition in file: " << filesInfo[j].name << ".asm" << std::endl;
					std::cout << std::endl;
					std::cout << "You can choose not to link files forcibly by using the flag: --no-link-force [filename]" << std::endl;
					return -1; 
				}
				if(find(toLink, filesInfo[j].name) == -1){
					toLink.push_back(filesInfo[j].name);
					funcs.push_back(std::pair<std::string,std::string>{file.callFuncs[i],filesInfo[j].name});
					findLinks(toLink,filesInfo,filesInfo[j],fUnlink,funcs,vars);
				}
			}
		}
	}
	for(int i = 0; i < file.callVars.size(); ++i){
		for(int j = 0; j < filesInfo.size(); ++j){
			if(find(filesInfo[j].defVars, file.callVars[i]) != -1 &&
				find(fUnlink,filesInfo[j].name) == -1){

				if(pairFind(vars, file.callVars[i]) != -1){
					std::cout << "=================== ERROR ===================" << std::endl;
					std::cout << "multiple definition of: " << std::endl;
					std::cout << file.callVars[i] << std::endl;
					std::cout << std::endl;
					std::cout << "First definition in file: " << vars[pairFind(vars,file.callVars[i])].second << ".asm" << std::endl;
					std::cout << "Second definition in file: " << filesInfo[j].name << ".asm" << std::endl;
					std::cout << std::endl;
					std::cout << "You can choose not to link files forcibly by using the flag: --no-link-force [filename]" << std::endl;
					return -1; 
				}
				if(find(toLink, filesInfo[j].name) == -1){
					toLink.push_back(filesInfo[j].name);
					vars.push_back(std::pair<std::string,std::string>{file.callVars[i], filesInfo[j].name});
					findLinks(toLink,filesInfo,filesInfo[j],fUnlink,funcs,vars);
				}
			}
		}
	}
	return 0;
}	
void getAllLibs(std::vector<std::string>& libDirs,
	std::vector<std::string>& libsToLink, 
	const std::string& path){

	auto dirs = getDirs(path);
	for(int i = 1; i < dirs.size(); ++i){
		if(std::filesystem::is_directory(dirs[i]))
			getAllLibs(libDirs,libsToLink,dirs[i]);
		else{
			std::string name = getNameNoExt(dirs[i]);
			while(getExt(name) != "-1")
				name = getNameNoExt(name);
			if(name.size() < 4)continue;
			if(std::string(name.begin(), name.begin() + 3) != "lib")continue;
			std::string shortName(name.begin() + 3, name.end());
			if(find(libsToLink, shortName) != -1){
				std::string folder = getFolder(dirs[i]);
				if(find(libDirs, folder) == -1)
					libDirs.push_back(folder);
			}
		}
	}
}
std::string link(const std::string& wd, 
	const std::vector<std::string>& parameters,
	const std::vector<std::string>& includes, 
	const std::vector<std::string>& toCompile,
	const bool log, const int linkType, const bool relink){

	if(toCompile.size() == 0 && exists(parameters[1]))
		return "nothing to link";
	std::string flags = " ";
	std::vector<std::string> libDirs;
	if(parameters[2] != "-1"){
		std::vector<std::string> libsToLink = split(parameters[2]);
		getAllLibs(libDirs,libsToLink,cd);
		auto addDirs = split(parameters[6]);
		for(int i = 0; i < addDirs.size(); ++i)
			getAllLibs(libDirs,libsToLink,addDirs[i]);
		for(int i = 0; i < libDirs.size(); ++i)
			flags += ("-L" + libDirs[i] + " ");
		for(int i = 0; i < libsToLink.size(); ++i)
			flags += ("-l" + libsToLink[i] + " ");
	}
	std::vector<std::string> toLink = toLinkList(parameters,wd);
	if(toLink.size() == 0)
		return "nothing to link";
	std::vector<std::string> fLink, fUnlink;
	if(parameters[3] != "-1")
		fLink = split(parameters[3]);
	if(parameters[4] != "-1")
		fUnlink = split(parameters[4]);
	auto it = toLink.begin();
	while(it != toLink.end()){
		if(find(fUnlink, *it) != -1)
			toLink.erase(it);
		else
			it++;
	}
	for(int i = 0; i < fLink.size(); ++i){
		if(find(toLink, fLink[i]) == -1){
			std::string path = wd + "/" + reqFolders[1] + "/" + subFolders[2] + "/" + fLink[i] + ".o";
			if(exists(path))
				toLink.push_back(fLink[i]);
			else{
				std::cout << "======================== ERROR ========================" << std::endl;
				std::cout << "Cannot find file: " << fLink[i] << std::endl;
				std::cout << "You specified it as a file for forced linking" << std::endl;
				return "error";
			}
		}
	}
	bool linking = false;
	for(int i = 0; i < toLink.size(); ++i){
		for(int j = 0; j < toCompile.size(); ++j){
			if(getName(toCompile[j]) == toLink[i]){
				linking = true;
				break;
			}
		}
		if(linking)
			break;
	}
	if(!exists(parameters[1]))
		linking = true;
	if(toLink.size() == 0)
		linking = false;
	if(relink)
		linking = true;
	if(!linking)
		return "nothing to link";

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

	it = toLink.begin();
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
		if(!erase)
			it++;
	}
	for(int i = 0; i < toLink.size(); ++i){
		std::string path = wd + "/" + reqFolders[1] + "/" + subFolders[2] + "/" + toLink[i] + ".o";
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
		cmd += (flags + " ");
		cmd += (" -o " + parameters[1]);
		if(log){
			std::cout << std::endl;
			std::cout << "Linking" << std::endl;
			std::cout << cmd << std::endl;
		}
		system(cmd.c_str());
	}
	else if(linkType == 1){
		std::string cmd = "ar rc " + parameters[1] + " ";
		for(int i = 0; i < toLink.size(); ++i)
			cmd += (toLink[i] + " ");
		if(log){
			std::cout << std::endl;
			std::cout << "Linking" << std::endl;
			std::cout << cmd << std::endl;
		}
		system(cmd.c_str());
	}
	if(parameters[2] != "-1" && libDirs.size() > 0){
		std::string cmd = postSharedLink;
		for(int i = 0; i < libDirs.size(); ++i)
			cmd += (":" + libDirs[i]);
		if(log){
			std::cout << std::endl;
			std::cout << cmd << std::endl;
		}
		system(cmd.c_str());
	}
	return "succes";
}