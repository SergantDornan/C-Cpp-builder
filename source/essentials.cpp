#include "essentials.h"

#define REQ_FOLDERS_SIZE 3
#define SUB_FOLDERS_SIZE 2

static constexpr const char* reqFolders[] = {HEADERS_DIR, SOURCE_DIR, SYM_DIR};
static constexpr const char* subFolders[] = {DEPS_DIR, OBJECTS_DIR};

void removeBuildFolder(const std::string& currDir, bool silent){
	std::string configPath = root + "/" + CONFIG_FILE;
	std::ifstream config(configPath);
	std::string line;
	std::vector<std::string> lines;
	while(std::getline(config, line))
		lines.push_back(line);
	config.close();
	auto it = lines.begin();
	while(it != lines.end()){
		if(split(*it, "*")[0] == currDir){
			removeDirectory(root + "/" + split(*it, "*")[1]);
			if(!silent) std::cout << root << "/" << split(*it, "*")[1] << " has been removed" << std::endl; 
			lines.erase(it);
			std::ofstream f(configPath);
			for(int i = 0; i < lines.size(); ++i)
				f << lines[i] << std::endl;
			f.close();
			break;
		}
		else
			it++;
	}
}

std::string createEssentials(const bool reb){
	auto mainDirs = getDirs(root);
	bool isConfig = false;
	for(int i = 1; i < mainDirs.size(); ++i){
		std::string name = getName(mainDirs[i]);
		if(name == CONFIG_FILE){
			isConfig = true;
			break;
		}
	}
	if(!isConfig){
		for(int i = 1; i < mainDirs.size(); ++i){
			if(std::filesystem::is_directory(mainDirs[i]))
				removeDirectory(mainDirs[i]);
		}
		createFile(root + "/" + CONFIG_FILE);
	}
	std::string configPath = root + "/" + CONFIG_FILE;
	std::ifstream config(configPath);
	std::string line;
	bool isDir = false;
	std::vector<std::string> projectList;
	while(std::getline(config,line))
		projectList.push_back(line);
	config.close();

	auto it = projectList.begin();
	while(it != projectList.end()){
		std::string p = split(*it, "*")[0];
		if(!exists(p)){
			projectList.erase(it);
			removeDirectory(root + "/" + split(*it, "*")[1]);
			std::ofstream f(configPath);
			for(int i = 0; i < projectList.size(); ++i)
				f << projectList[i] << std::endl;
			f.close();
		}
		else
			it++;
	}

	std::string index;
	for(int i = 0; i < projectList.size(); ++i){
		auto s = split(projectList[i], "*");
		if(s[0] == cd){
			index = s[1];
			std::string folder = root + "/" + index;
			if(!exists(folder)){
				std::cerr << "================= FATAL ERROR =================" << std::endl;
				std::cerr << "Main config file is corrupted and it is probably your fault" << std::endl;
				std::cerr << "rebuilding all projects and configs" << std::endl;
				std::cerr << std::endl;
				removeFile(configPath);
				return createEssentials(false);
			} 
			else
				isDir = true;
			break;
		}
	}
	if(isDir && reb){
		std::vector<std::string> dirs_to_remove;
		auto dirs = getDirs(root + "/" + index);
		for(int i = 1; i < dirs.size(); ++i){
			if(getName(dirs[i]) != CONFIG_FILE && std::filesystem::is_directory(dirs[i]))
				dirs_to_remove.push_back(dirs[i]);
		}
		removeDirectories(dirs_to_remove);
	}
	else if(!isDir){
		index = std::to_string(projectList.size() + 1);
		std::ofstream config(configPath,std::ios::app);
		config << cd << '*' << index << std::endl;
		config.close();
		createDirectory(root + "/" + index);
	}
	
	std::string folder = root + "/" + index;
	auto inDir = getDirs(folder);
	auto inDirNames = inDir;
	for(int i = 0; i < inDirNames.size(); ++i)
		inDirNames[i] = getName(inDirNames[i]);
	bool rebuild = false;
	for(int i = 0; i < REQ_FOLDERS_SIZE; ++i){
		if(find(inDirNames,reqFolders[i]) == -1){
			rebuild = true;
			break;
		}
	}
	if(find(inDirNames, CONFIG_FILE) == -1)
		rebuild = true;
	if(rebuild){

		std::vector<std::string> dirs_to_remove;
		for(int i = 1; i < inDir.size(); ++i){
			if(getName(inDir[i]) != CONFIG_FILE && std::filesystem::is_directory(inDir[i]))
				dirs_to_remove.push_back(inDir[i]);
		}
		removeDirectories(dirs_to_remove);

		std::vector<std::string> dirs_to_create;
		for(int i = 0; i < REQ_FOLDERS_SIZE; ++i)
			dirs_to_create.push_back(folder + "/" + reqFolders[i]);
		createDirectories(dirs_to_create);

		if(!exists(folder + "/" + CONFIG_FILE)){
			std::ofstream out(folder + "/" + CONFIG_FILE);
			out << "-1" << std::endl; // 0 main input
			out << "out" << std::endl; // 1 output name
			out << "-1" << std::endl; // 2 libs linking (force)
			out << "-1" << std::endl; // 3 force link list
			out << "-1" << std::endl; // 4 force unlink list
			out << "default default" << std::endl; // 5 compilers (C, C++)
			out << "-1" << std::endl; // 6 additional -I list
			out << "-1" << std::endl; // 7 C++ standart
			out << "-1" << std::endl; // 8 optimization flag
			out << "-1" << std::endl; // 9 debug flag
			out << "-1" << std::endl; // 10 Flags to compiler
			out << "-1" << std::endl; // 11 Flags to linker
			out << "-1" << std::endl; // 12 general Flags
			out << "-1" << std::endl; // 13 force Unlink Libs
			out << "-1" << std::endl; // 14 force unlink dirs
			out << "-1" << std::endl; // 15 C standart
			out.close();
		}
	}
	for(int i = 0; i < REQ_FOLDERS_SIZE; ++i){
		std::string subFolder = folder + "/" + reqFolders[i];
		auto innerDir = getDirs(subFolder);
		auto innerDirNames = innerDir;
		for(int j = 0; j < innerDirNames.size(); ++j)
			innerDirNames[j] = getName(innerDirNames[j]);
		if(i == 0 && find(innerDirNames, DEPS_DIR) == -1)
			createDirectory(subFolder + "/" + DEPS_DIR);
		else if(i == 1){
			bool rebuildSubFolder = false;
			for(int j = 0; j < SUB_FOLDERS_SIZE; ++j){
				if(find(innerDirNames, subFolders[j]) == -1){
					rebuildSubFolder = true;
					break;
				}
			}
			if(rebuildSubFolder){
				std::vector<std::string> dirs_to_remove;
				for(int j = 1; j < innerDir.size(); ++j)
					dirs_to_remove.push_back(innerDir[j]);
				removeDirectories(dirs_to_remove);
				std::vector<std::string> dirs_to_create;
				for(int j = 0; j < SUB_FOLDERS_SIZE; ++j)
					dirs_to_create.push_back(subFolder + "/" + subFolders[j]);
				createDirectories(dirs_to_create);
			}
		}
	}
	return folder;
}