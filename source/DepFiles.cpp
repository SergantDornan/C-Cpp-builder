#include "DepFiles.h"

// Структура dep файла:
// путь к описываемому файлу
// время изменения описываемого файла
// все файлы от которых файл зависит (пути к настоящим файлам)
// файлы которые зависят от текущего (пути к деп файлам)

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
                updateFile(toCompile,depFiles[i], recCheck); // идем вниз по дереву зависимостей
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
    const std::vector<std::string>& allHeaders, 
    const std::vector<std::string>& allSource){

    std::vector<std::string> dirs = std::vector<std::string>(HDdirs.begin() + 1, HDdirs.end()) 
        + std::vector<std::string>(SDdirs.begin() + 1, SDdirs.end());
    std::vector<std::string> changedFiles;
    // file name: <файлы от которых зависит, файлы которые зависят от>
    std::map<std::string, std::pair<std::vector<std::string>, std::vector<std::string>>> data;
    for(int i = 0; i < dirs.size(); ++i){
        std::fstream file(dirs[i]);
        std::vector<std::string> v;
        std::string line;
        while(std::getline(file,line)) v.push_back(line);
        file.close();
        if(getChangeTime(v[0]) != v[1]) changedFiles.push_back(v[0]);
        std::vector<std::string> inc1, inc2;
        if(v[2] != "-1") inc1 = split(v[2]);
        if(v[3] != "-1") inc2 = split(v[3]);
        data[v[0]] = std::pair<std::vector<std::string>, std::vector<std::string>>{inc1, inc2};
    }

    std::vector<std::string> addChangedFiles;
    for(int i = 0; i < changedFiles.size(); ++i){
        std::vector<std::string> includes;
        getIncludes(includes, allHeaders, allSource, changedFiles[i]);
        std::vector<std::pair<std::string,bool>> changes = getChanges(data[changedFiles[i]].first, includes);
        if(changes.size() > 0){
            data[changedFiles[i]].first = includes;
            for(int j = 0; j < changes.size(); ++j){
                if(find(addChangedFiles, changes[j].first) == -1) addChangedFiles.push_back(changes[j].first);
                std::string pathToDepfile;
                if(getExt(changedFiles[i]) == "h" || getExt(changedFiles[i]) == "hpp")
                    pathToDepfile = id + "/" + subFolders[0] + "/" + converPathToName(changedFiles[i]);
                else
                    pathToDepfile = bd + "/" + subFolders[0] + "/" + converPathToName(changedFiles[i]);
                if(changes[j].second) data[changes[j].first].second.push_back(pathToDepfile);
                else data[changes[j].first].second.erase(
                    std::find(data[changes[j].first].second.begin(),
                    data[changes[j].first].second.end(), pathToDepfile));
            }
        }
    }
    changedFiles += addChangedFiles;

    for(int i = 0; i < changedFiles.size(); ++i){
        std::string pathToDepfile;
        if(getExt(changedFiles[i]) == "h" || getExt(changedFiles[i]) == "hpp")
            pathToDepfile = id + "/" + subFolders[0] + "/" + converPathToName(changedFiles[i]);
        else
            pathToDepfile = bd + "/" + subFolders[0] + "/" + converPathToName(changedFiles[i]);
        std::string time;
        std::ifstream file(pathToDepfile);
        std::getline(file, time);
        std::getline(file, time);
        file.close();
        std::ofstream out(pathToDepfile);
        out << changedFiles[i] << std::endl;
        out << time << std::endl;
        if(data[changedFiles[i]].first.size() == 0) out << "-1" << std::endl;
        else{
            for(int j = 0; j < data[changedFiles[i]].first.size(); ++j)
                out << data[changedFiles[i]].first[j] << ' ';
            out << std::endl;
        }
        if(data[changedFiles[i]].second.size() == 0) out << "-1" << std::endl;
        else{
            for(int j = 0; j < data[changedFiles[i]].second.size(); ++j)
                out << data[changedFiles[i]].second[j] << ' ';
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
	std::string bd = wd + "/" + reqFolders[1];
	auto dirs = getDirs(bd + "/" + subFolders[0]);
	//Проход по dep файлам:

    // Удаление лишних деп файлов и объектников у сурс файлов
    std::string cmd = "rm";
	for(int i = 1; i < dirs.size(); ++i){
		std::ifstream file(dirs[i]);
		std::string line;
		std::getline(file, line);
		file.close();
        if(find(allSource, line) == -1){
			changeSet = true;
			std::string objFile = wd + "/" + reqFolders[1] + "/" + subFolders[1] + "/" + getName(dirs[i]) + ".o";
			cmd += (" " + dirs[i] + " " + objFile);
		}
	}

    // Удаление лишних деп файлов у хедеров
	dirs = getDirs(wd + "/" + reqFolders[0] + "/" + subFolders[0]);
	for(int i = 1; i < dirs.size(); ++i){
		std::ifstream file(dirs[i]);
		std::string line;
		std::getline(file, line);
		file.close();
        if(find(allHeaders, line) == -1){
			changeSet = true;
			cmd += (" " + dirs[i]);
		}
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
            newfile << "-1" << std::endl;
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