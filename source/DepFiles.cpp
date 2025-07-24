#include "DepFiles.h"

// Структура dep файла:
// путь к описываемому файлу
// время изменения описываемого файла
// все зависимости (файлы которые зависят от текущего)

void updateFile(std::vector<std::string>& toCompile,
    const std::string& path,std::vector<std::string>& recCheck){

    recCheck.push_back(getName(path));
    std::ifstream file(path);
    if(!file.is_open())
        return;
    std::vector<std::string> v;
    std::string line;
    while(std::getline(file,line))
        v.push_back(line);
    file.close();
    if(getExt(path) != "h" && getExt(path) != "hpp" && find(toCompile, path) == -1)
        toCompile.push_back(path);
    if(v.size() > 2){
        auto depFiles = split(v[2]); // Берем все файлы, которые зависят от текущего
        for(int i = 0; i < depFiles.size(); ++i){
            if(find(recCheck, getName(depFiles[i])) == -1) // Чтобы не было бесконечной рекурсии
                updateFile(toCompile,depFiles[i], recCheck); // идем вниз по дереву зависимостей
        }
    }
}

int updateFiles(std::vector<std::string>& toCompile, 
	const std::vector<std::string>& HDdirs, const std::vector<std::string>& SDdirs)
{
	// Проход по depFiles хедеров, смотрим что надо пересобрать
    for(int i = 1; i < HDdirs.size(); ++i){
        std::fstream file(HDdirs[i]);
        if(!file.is_open()){
            std::cout << std::endl;
            std::cout << "======================== ERROR ========================" << std::endl;
            std::cout << "====== Compile.cpp: compile() ======" << std::endl;
            std::cout << "Cannot open file: " << HDdirs[i] << std::endl;
            std::cout << "Try launching builder with --rebuild / -reb flag" << std::endl;       
            std::cout << std::endl;
            return 1;
        }
        std::vector<std::string> v;
        std::string line;
        while(std::getline(file,line))
            v.push_back(line);
        file.close();
        if(v[1] != getChangeTime(v[0])){
            std::ofstream out(HDdirs[i]);
            out << v[0] << std::endl;
            out << getChangeTime(v[0]) << std::endl;
            if(v.size() > 2) out << v[2];
            out.close();
            std::vector<std::string> recCheck;
            updateFile(toCompile,HDdirs[i],recCheck);
        }
    }
    // Такой же проход по сурсам
    for(int i = 1; i < SDdirs.size(); ++i){
        std::fstream file(SDdirs[i]);
        if(!file.is_open()){
            std::cout << std::endl;
            std::cout << "======================== ERROR ========================" << std::endl;
            std::cout << "====== Compile.cpp: compile() ======" << std::endl;
            std::cout << "Cannot open file: " << SDdirs[i] << std::endl;
            std::cout << "Try launching builder with --rebuild / -reb flag" << std::endl;       
            std::cout << std::endl;
            return 1;
        }
        std::vector<std::string> v;
        std::string line;
        while(std::getline(file,line))
            v.push_back(line);
        file.close();
        if(v[1] != getChangeTime(v[0])){
            std::vector<std::string> recCheck;
            updateFile(toCompile,SDdirs[i], recCheck);
        }
    }
    return 0;
}

int UpdateDependencies(const std::vector<std::string>& HDdirs,
	const std::vector<std::string>& SDdirs,
	const std::string& bd, const std::string& id,
    const std::vector<std::string>& allHeaders, 
    const std::vector<std::string>& allSource){

	for(int i = 1; i < HDdirs.size(); ++i){
        std::fstream file(HDdirs[i]);
		if(!file.is_open()){
            std::cout << std::endl;
	        std::cout << "======================== ERROR ========================" << std::endl;
            std::cout << "====== Compile.cpp: compile() ======" << std::endl;
            std::cout << "Cannot open file: " << HDdirs[i] << std::endl;
            std::cout << "Try launching builder with --rebuild / -reb flag" << std::endl;       
            std::cout << std::endl;
            return 1;
        }
        std::vector<std::string> v;
        std::string line;
        while(std::getline(file,line))
            v.push_back(line);
        file.close();
        if(v.size() > 2){
            std::ofstream out(HDdirs[i]);
            out << v[0] << std::endl;
            out << v[1] << std::endl;
            out.close();
        }
    }
    for(int i = 1; i < SDdirs.size(); ++i){
        std::fstream file(SDdirs[i]);
        if(!file.is_open()){
            std::cout << std::endl;
            std::cout << "======================== ERROR ========================" << std::endl;
            std::cout << "====== Compile.cpp: compile() ======" << std::endl;
            std::cout << "Cannot open file: " << SDdirs[i] << std::endl;
            std::cout << "Try launching builder with --rebuild / -reb flag" << std::endl;       
            std::cout << std::endl;
            return 1;
        }
        std::vector<std::string> v;
		std::string line;
        while(std::getline(file,line))
            v.push_back(line);
        file.close();
        if(v.size() > 2){
            std::ofstream out(SDdirs[i]);
            out << v[0] << std::endl;
            out << v[1] << std::endl;
            out.close();
        }
    }

    // Проход по сурс файлам
    for(int i = 0; i < allSource.size(); ++i){
        std::vector<std::string> includes;
        getIncludes(includes, allHeaders, allSource, allSource[i]);
        for(int j = 0; j < includes.size(); ++j){
            std::string path;
            if(getExt(includes[j]) == "h" || getExt(includes[j]) == "hpp")
                path = id + "/" + subFolders[0] + "/" + converPathToName(includes[j]);
            else
                path = bd + "/" + subFolders[0] + "/" + converPathToName(includes[j]);
            std::string s = bd + "/" + subFolders[0] + "/" + converPathToName(allSource[i]) + " ";
            appendToFile(path,s);
        }
    }
    // Проход по хедерам
    for(int i = 0; i < allHeaders.size(); ++i){
        std::vector<std::string> includes;
        getIncludes(includes, allHeaders, allSource, allHeaders[i]);
        for(int j = 0; j < includes.size(); ++j){
            std::string path;
            if(getExt(includes[j]) == "h" || getExt(includes[j]) == "hpp")
                path = id + "/" + subFolders[0] + "/" + converPathToName(includes[j]);
            else
                path = bd + "/" + subFolders[0] + "/" + converPathToName(includes[j]);
            std::string s = id + "/" + subFolders[0] + "/" + converPathToName(allHeaders[i]) + " ";
            appendToFile(path,s);
        }
    }
    return 0;
}


bool createDepfiles(const std::string& wd,
	const std::vector<std::string>& allHeaders, 
	const std::vector<std::string>& allSource,
	const bool log)
{
	bool changeSet = false;
	std::string bd = wd + "/" + reqFolders[1];
	auto dirs = getDirs(bd + "/" + subFolders[0]);
	//Проход по dep файлам:

    // Удаление лишних деп файлов и объектников у сурс файлов
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
			std::string objFile = wd + "/" + reqFolders[1] + "/" + subFolders[1] + "/" + getName(dirs[i]) + ".o";
			std::string cmd = "rm " + dirs[i] + " " + objFile;
			system(cmd.c_str());
		}
		file.close();
	}


    // Удаление лишних деп файлов у хедеров
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
		std::string file = wd + "/" + reqFolders[0] + "/" + subFolders[0] + "/" + converPathToName(allHeaders[i]);
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
		std::string file = bd + "/" + subFolders[0] + "/" + converPathToName(allSource[i]);
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



