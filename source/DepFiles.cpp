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
    std::string cmd = "rm";
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
		if(find(allSource, line) == -1){
			changeSet = true;
			std::string objFile = wd + "/" + reqFolders[1] + "/" + subFolders[1] + "/" + getName(dirs[i]) + ".o";
			cmd += (" " + dirs[i] + " " + objFile);
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
		if(find(allHeaders, line) == -1){
			changeSet = true;
			cmd = (" " + dirs[i]);
		}
		file.close();
	}
    if(cmd != "rm") system(cmd.c_str());

	// Проход по хедерам и сурс файлам
	for(int i = 0; i < allHeaders.size(); ++i){
		std::string file = wd + "/" + reqFolders[0] + "/" + subFolders[0] + "/" + converPathToName(allHeaders[i]);
		if(!exists(file)){
			changeSet = true;
			cmd = "touch " + file;
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
			cmd = "touch " + file;
			system(cmd.c_str());
			std::ofstream newfile(file);
			newfile << allSource[i] << std::endl;
			newfile << "-1" << std::endl;
			newfile.close();
		}
	}
	return changeSet;
}


void rebuildForSharedLib(const std::string& n1, const std::string& n2,
    const std::string& wd){

    auto isSharedLib = [](const std::string& s0){
        std::string s = getName(s0);
        if(s.size() < 4) return false;
        if(std::string(s.begin(), s.begin() + 3) != "lib") return false;
        if(getExt(s) != "so") return false;
        return true;
    };

    if((!isSharedLib(n1) && isSharedLib(n2)) || 
        (isSharedLib(n1) && !isSharedLib(n2)))
    {
        std::string dir = wd + "/" + reqFolders[1] + "/" + subFolders[0];
        std::string cmd = "rm -rf";
        auto dirs = getDirs(dir);
        for(int i = 1; i < dirs.size(); ++i)
            cmd += (" " + dirs[i]);
        system(cmd.c_str());
    }
}

// Структура файла с символами:
// Само имя: converPathToName(path) + ".sym"
// путь к файлу на который ссылается (объектник или библиотека)
// дата изменения файла на который ссылается
// Число callSyms
// Число defSyms
// callSyms
// defSyms

void updateSymfiles(const std::string& wd, const std::vector<std::string>& allLibs){

    auto isLib = [](const std::string& s0){
        std::string s = getName(s0);
        if(s.size() < 4) return false;
        if(std::string(s.begin(), s.begin() + 3) != "lib") return false;
        if(getExt(s) != "a" && getExt(s) != "so") return false;
        return true;
    };

    auto allObj = getDirs(wd + "/" + reqFolders[1] + "/" + subFolders[1]);
    auto dirs = getDirs(wd + "/" + reqFolders[2]);
    std::string cmd = "rm";
    for(int i = 1; i < dirs.size(); ++i){
        std::ifstream file(dirs[i]);
        std::string path, changeTime;
        std::getline(file, path);
        std::getline(file, changeTime);
        file.close();
        bool del = false;
        if(isLib(path)) del |= (find(allLibs, path) == -1);
        else del |= (find(allObj, path) == -1);
        if(!del) del |= (getChangeTime(path) != changeTime);
        if(del) cmd += (" " + dirs[i]);
    }
    if(cmd != "rm") system(cmd.c_str());
}