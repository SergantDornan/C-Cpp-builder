#include "DepFiles.h"
bool createDepfiles(const std::string& wd,
	const std::vector<std::string>& allHeaders, 
	const std::vector<std::string>& allSource,
	const bool log)
{
	bool changeSet = false;
	std::string bd = wd + "/" + reqFolders[1];
	auto dirs = getDirs(bd + "/" + subFolders[0]);
	//Проход по dep файлам
	for(int i = 1; i < dirs.size(); ++i){
		std::ifstream file(dirs[i]);
		if(!file.is_open()){
			std::cout << std::endl;
			std::cout << "=================== ERROR ===================" << std::endl;
			std::cout << "DepFiles.cpp: createDepfiles()" << std::endl;
			std::cout << "Cannot open file: " << dirs[i] << std::endl;
			std::cout << "Try launching builder with --rebuild / -reb flag" << std::endl;
			std::cout << std::endl;
			return false;
		}
		std::string line;
		std::getline(file, line);
		if(!exists(line)){
			changeSet = true;
			std::string cmd = "rm " + dirs[i];
			system(cmd.c_str());	
		}
		file.close();
	}

	dirs = getDirs(wd + "/" + reqFolders[0] + "/" + subFolders[0]);
	for(int i = 1; i < dirs.size(); ++i){
		std::ifstream file(dirs[i]);
		if(!file.is_open()){
			std::cout << std::endl;
			std::cout << "=================== ERROR ===================" << std::endl;
			std::cout << "DepFiles.cpp: createDepfiles()" << std::endl;
			std::cout << "Cannot open file: " << dirs[i] << std::endl;
			std::cout << "Try launching builder with --rebuild / -reb flag" << std::endl;
			std::cout << std::endl;
			return false;
		}
		std::string line;
		std::getline(file, line);
		if(!exists(line)){
			changeSet = true;
			std::string cmd = "rm " + dirs[i];
			system(cmd.c_str());
		}
		file.close();
	}

	if(log)
		std::cout << std::endl;
	// Проход по хедерам и сурс файлам
	for(int i = 0; i < allHeaders.size(); ++i){
		std::string file = wd + "/" + reqFolders[0] + "/" + subFolders[0] + "/" + getName(allHeaders[i]);
		if(!exists(file)){
			changeSet = true;
			std::string cmd = "touch " + file;
			system(cmd.c_str());
			std::ofstream newfile(file);
			newfile << allHeaders[i] << std::endl;
			newfile << "-1" << std::endl;
			newfile.close();
		}
	}

	for(int i = 0; i < allSource.size(); ++i){
		std::string file = bd + "/" + subFolders[0] + "/" + getName(allSource[i]);
		if(!exists(file)){
			changeSet = true;
			std::string cmd = "touch " + file;
			system(cmd.c_str());
			std::ofstream newfile(file);
			newfile << allSource[i] << std::endl;
			newfile << "-1" << std::endl;
			newfile.close();
		}
	}
	return changeSet;
}