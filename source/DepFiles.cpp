#include "DepFiles.h"

// Структура dep файла:
// путь к описываемому файлу
// время изменения описываемого файла
// все файлы от которых файл зависит (пути к настоящим файлам)
// файлы которые зависят от текущего (пути к деп файлам)
// Список папок, которые надо указать для компиляции с флагом -I

std::vector<std::pair<std::string,bool>> getChanges(const std::vector<std::string>& oldV, 
    const std::vector<std::string>& newV){

    if(oldV == newV) return std::vector<std::pair<std::string,bool>>{};
    std::vector<std::pair<std::string,bool>> result;
    for(int i = 0; i < newV.size(); ++i){
        if(find(oldV, newV[i]) == -1) result.push_back(std::pair<std::string,bool>{newV[i],true}); 
    }
    for(int i = 0; i < oldV.size(); ++i){
        if(find(newV, oldV[i]) == -1) result.push_back(std::pair<std::string,bool>{oldV[i],false});
    }
    return result;
}

void updateFile(std::vector<std::string>& toCompile,
    const std::string& path,std::vector<std::string>& recCheck){

    recCheck.push_back(getName(path));
    std::ifstream file(path);
    if(!file.is_open()) return;
    std::vector<std::string> v;
    std::string line;
    while(std::getline(file,line)) v.push_back(line);
    file.close();
    if(getExt(path) != "h" && getExt(path) != "hpp" && find(toCompile, path) == -1)
        toCompile.push_back(path);
    if(v[3] != "-1"){
        auto depFiles = split(v[3]); // Берем все файлы, которые зависят от текущего
        for(int i = 0; i < depFiles.size(); ++i){
            if(find(recCheck, getName(depFiles[i])) == -1) // Чтобы не было бесконечной рекурсии
                updateFile(toCompile,depFiles[i],recCheck); // идем вниз по дереву зависимостей
        }
    }
}

int updateFiles(std::vector<std::string>& toCompile, 
	const std::vector<std::string>& HDdirs, const std::vector<std::string>& SDdirs)
{
    std::vector<std::string> dirs = std::vector<std::string>(HDdirs.begin() + 1, HDdirs.end()) 
        + std::vector<std::string>(SDdirs.begin() + 1, SDdirs.end());

    for(int i = 0; i < dirs.size(); ++i){
        std::fstream file(dirs[i]);
        std::vector<std::string> v;
        std::string line;
        while(std::getline(file,line)) v.push_back(line);
        file.close();
        if(v[1] != getChangeTime(v[0])){
            if(getExt(v[0]) == "h" || getExt(v[0]) == "hpp"){
                std::ofstream out(dirs[i]);
                out << v[0] << std::endl;
                out << getChangeTime(v[0]) << std::endl;
                out << v[2] << std::endl;
                out << v[3] << std::endl;
                out << v[4] << std::endl;
                out.close();
            }
            std::vector<std::string> recCheck;
            updateFile(toCompile,dirs[i],recCheck);
        }
    }
    return 0;
}

void UpdateDependencies(const std::vector<std::string>& HDdirs,
	const std::vector<std::string>& SDdirs,
	const std::string& bd, const std::string& id,
    const std::vector<FileNode>& map,
    const std::vector<int>& leaves){


    std::vector<std::string> dirs = std::vector<std::string>(HDdirs.begin() + 1, HDdirs.end()) 
        + std::vector<std::string>(SDdirs.begin() + 1, SDdirs.end());
    std::vector<std::string> changedFiles;

    // file name: <файлы от которых зависит, файлы которые зависят от, -I список>
    std::map<std::string, std::vector<std::vector<std::string>>> data;
    for(int i = 0; i < dirs.size(); ++i){
        std::fstream file(dirs[i]);
        std::vector<std::string> v;
        std::string line;
        while(std::getline(file,line)) v.push_back(line);
        file.close();
        if(getChangeTime(v[0]) != v[1]) changedFiles.push_back(v[0]);
        std::vector<std::string> inc1, inc2, inc3;
        if(v[2] != "-1") inc1 = split(v[2]);
        if(v[3] != "-1") inc2 = split(v[3]);
        if(v[4] != "-1") inc3 = split(v[4]);
        data[v[0]] = std::vector<std::vector<std::string>>{inc1, inc2, inc3};
    }
    
    std::vector<std::string> addChangedFiles;
    for(int i = 0; i < changedFiles.size(); ++i){
        std::vector<std::string> includes, Ilist;
        getIncludes(includes, Ilist, map, leaves, changedFiles[i]);
        std::vector<std::pair<std::string,bool>> changes = getChanges(data[changedFiles[i]][0], includes);
        if(changes.size() > 0){
            data[changedFiles[i]][0] = includes;
            data[changedFiles[i]][2] = Ilist;
            for(int j = 0; j < changes.size(); ++j){
                if(find(addChangedFiles, changes[j].first) == -1) addChangedFiles.push_back(changes[j].first);
                std::string pathToDepfile;
                if(getExt(changedFiles[i]) == "h" || getExt(changedFiles[i]) == "hpp")
                    pathToDepfile = id + "/" + DEPS_DIR + "/" + convertPathToName(changedFiles[i]);
                else
                    pathToDepfile = bd + "/" + DEPS_DIR + "/" + convertPathToName(changedFiles[i]);
                if(changes[j].second) data[changes[j].first][1].push_back(pathToDepfile);
                else data[changes[j].first][1].erase(
                    std::find(data[changes[j].first][1].begin(),
                    data[changes[j].first][1].end(), pathToDepfile));
            }
        }
    }
    changedFiles += addChangedFiles;

    for(int i = 0; i < changedFiles.size(); ++i){
        std::string pathToDepfile;
        if(getExt(changedFiles[i]) == "h" || getExt(changedFiles[i]) == "hpp")
            pathToDepfile = id + "/" + DEPS_DIR + "/" + convertPathToName(changedFiles[i]);
        else
            pathToDepfile = bd + "/" + DEPS_DIR + "/" + convertPathToName(changedFiles[i]);
        std::string time;
        std::ifstream file(pathToDepfile);
        std::getline(file, time);
        std::getline(file, time);
        file.close();
        std::ofstream out(pathToDepfile);
        out << changedFiles[i] << std::endl;
        out << time << std::endl;
        if(data[changedFiles[i]][0].size() == 0) out << "-1" << std::endl;
        else{
            for(int j = 0; j < data[changedFiles[i]][0].size(); ++j)
                out << data[changedFiles[i]][0][j] << ' ';
            out << std::endl;
        }
        if(data[changedFiles[i]][1].size() == 0) out << "-1" << std::endl;
        else{
            for(int j = 0; j < data[changedFiles[i]][1].size(); ++j)
                out << data[changedFiles[i]][1][j] << ' ';
            out << std::endl;
        }
        if(data[changedFiles[i]][2].size() == 0) out << "-1" << std::endl;
        else{
            for(int j = 0; j < data[changedFiles[i]][2].size(); ++j)
                out << data[changedFiles[i]][2][j] << ' ';
            out << std::endl;
        }
    }
}


bool createDepfiles(const std::string& wd,
	const std::vector<std::string>& allHeaders, 
	const std::vector<std::string>& allSource,
	const bool log)
{
	bool changeSet = false;
	std::string bd = wd + "/" + SOURCE_DIR;
	auto dirs = getDirs(bd + "/" + DEPS_DIR + "/");
	//Проход по dep файлам:

    // Удаление лишних деп файлов и объектников и sym у сурс файлов
    std::vector<std::string> files_to_remove;
	for(int i = 1; i < dirs.size(); ++i){
		std::ifstream file(dirs[i]);
		std::string line;
		std::getline(file, line);
		file.close();
        if(find(allSource, line) == -1){
			changeSet = true;
			std::string objFile = wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR + "/" + getName(dirs[i]) + ".o";
			std::string symFile = wd + "/" + SYM_DIR + "/" + getName(dirs[i]) + ".sym";
            files_to_remove.push_back(dirs[i]);
            files_to_remove.push_back(objFile);
            files_to_remove.push_back(symFile);
		}
	}

    // Удаление лишних деп файлов у хедеров
	dirs = getDirs(wd + "/" + HEADERS_DIR + "/" + DEPS_DIR + "/");
	for(int i = 1; i < dirs.size(); ++i){
		std::ifstream file(dirs[i]);
		std::string line;
		std::getline(file, line);
		file.close();
        if(find(allHeaders, line) == -1){
			changeSet = true;
            files_to_remove.push_back(dirs[i]);
		}
	}
    removeFiles(files_to_remove);

	// Проход по хедерам и сурс файлам
	for(int i = 0; i < allHeaders.size(); ++i){
		std::string file = wd + "/" + HEADERS_DIR + "/" + DEPS_DIR + "/" + convertPathToName(allHeaders[i]);
		if(!exists(file)){
			changeSet = true;
			std::ofstream newfile(file);
			newfile << allHeaders[i] << std::endl;
			newfile << "-1" << std::endl;
            newfile << "-1" << std::endl;
            newfile << "-1" << std::endl;
			newfile << "-1" << std::endl;
            newfile.close();
		}
	}

	for(int i = 0; i < allSource.size(); ++i){
		std::string file = bd + "/" + DEPS_DIR + "/" + convertPathToName(allSource[i]);
		if(!exists(file)){
			changeSet = true;
			std::ofstream newfile(file);
			newfile << allSource[i] << std::endl;
			newfile << "-1" << std::endl;
            newfile << "-1" << std::endl;
            newfile << "-1" << std::endl;
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
        std::string dir = wd + "/" + SOURCE_DIR + "/" + DEPS_DIR;
        std::vector<std::string> files_to_remove;
        auto dirs = getDirs(dir);
        for(int i = 1; i < dirs.size(); ++i)
            files_to_remove.push_back(dirs[i]);
        dirs = getDirs(wd + "/" + SYM_DIR);
        for(int i = 1; i < dirs.size(); ++i)
            files_to_remove.push_back(dirs[i]);
        dirs = getDirs(wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR);
        for(int i = 1; i < dirs.size(); ++i)
            files_to_remove.push_back(dirs[i]);
        removeFiles(files_to_remove);
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

    auto allObj = getDirs(wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR);
    auto dirs = getDirs(wd + "/" + SYM_DIR);
    std::vector<std::string> files_to_remove;
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
        if(del) files_to_remove.push_back(dirs[i]);
    }
    removeFiles(files_to_remove);
}