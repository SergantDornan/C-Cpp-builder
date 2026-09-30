#include "essentials.h"

#include "Flags.h"
#include "ConfigIndex.h"

static void migrateLegacyLayout(const std::string& folder){
	std::string legacyConfig = folder + "/" + CONFIG_FILE;
	if(!exists(legacyConfig)) return;
	std::string pairsDir = folder + "/" + PAIRS_DIR;
	if(!exists(pairsDir)){
		auto parameters = readConfig(legacyConfig);
		if(parameters[CFG_ENTRY] != "-1"){
			std::string output = getFullPath(cd, parameters[CFG_OUTPUT]);
			if(output != "-1") parameters[CFG_OUTPUT] = output;
			createDirectory(pairsDir);
			createDirectory(pairsDir + "/1");
			writeConfig(pairsDir + "/1/" + CONFIG_FILE, parameters);
			std::ofstream last(folder + "/" + LAST_PAIR_FILE);
			last << "1" << std::endl;
			last.close();
		}
	}
	auto dirs = getDirs(folder);
	for(int i = 1; i < dirs.size(); ++i){
		std::string name = getName(dirs[i]);
		if(name == PAIRS_DIR || name == PROFILES_DIR || name == LAST_PAIR_FILE || name == LOCK_FILE) continue;
		if(std::filesystem::is_directory(dirs[i])) removeDirectory(dirs[i]);
		else removeFile(dirs[i]);
	}
}

void removeBuildFolder(const std::string& currDir, bool silent){
	std::unique_ptr<FILE, int(*)(FILE*)> lock = lockFile(root + "/" + LOCK_FILE);
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

static std::string createEssentialsLocked();

std::string createEssentials(){
	std::unique_ptr<FILE, int(*)(FILE*)> lock = lockFile(root + "/" + LOCK_FILE);
	return createEssentialsLocked();
}

static std::string createEssentialsLocked(){
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
			// Имя папки берём до erase: после него it указывает на следующую запись
			std::string staleFolder = split(*it, "*")[1];
			it = projectList.erase(it);
			removeDirectory(root + "/" + staleFolder);
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
				return createEssentialsLocked();
			} 
			else
				isDir = true;
			break;
		}
	}
	if(!isDir){
		// size()+1 после удаления записей совпадает с индексом живого проекта,
		// поэтому берём максимальный существующий индекс + 1
		int maxIndex = 0;
		for(int i = 0; i < projectList.size(); ++i){
			auto s = split(projectList[i], "*");
			if(s.size() < 2) continue;
			try{
				maxIndex = std::max(maxIndex, std::stoi(s[1]));
			}
			catch(const std::exception&){}
		}
		index = std::to_string(maxIndex + 1);
		std::ofstream config(configPath,std::ios::app);
		config << cd << '*' << index << std::endl;
		config.close();
		createDirectory(root + "/" + index);
	}

	std::string folder = root + "/" + index;
	migrateLegacyLayout(folder);
	if(!exists(folder + "/" + PAIRS_DIR)) createDirectory(folder + "/" + PAIRS_DIR);
	if(!exists(folder + "/" + PROFILES_DIR)) createDirectory(folder + "/" + PROFILES_DIR);
	return folder;
}
