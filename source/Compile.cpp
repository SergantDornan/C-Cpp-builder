#include "Compile.h"

// Структура dep файла:
// путь к описываемому файлу
// время изменения описываемого файла
// все зависимости (файлы которые зависят от текущего)

bool checkProgram(const std::string& programName) {
    std::string command = "which " + programName + " > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}
void getIncludes(std::vector<std::string>& includes,
	const std::vector<std::string>& allHeaders,
	const std::vector<std::string>& allSource,
	const std::string& path, bool all){
    std::string l;
    std::ifstream input(path);
    if (input.is_open()){
  		while (std::getline(input, l)){
        	if(l.find("#include") != std::string::npos){
        		std::string s;
        		
        		for(int j = 10; j < l.size() && l[j] != '>' && l[j] != '"'; ++j)
        			s+=l[j];
        		std::string newline;
        		bool b = false;
        		for(int j = 0; j < allHeaders.size(); ++j){
        			if(s == getName(allHeaders[j])){
        				b = true;
        				newline = allHeaders[j];
        				break;
        			}
        		}
        		for(int j = 0; j < allSource.size(); ++j){
        			if(s == getName(allSource[j])){
        				b = true;
        				newline = allSource[j];
        				break;
        			}
        		}
        		if(b && find(includes, newline) == -1){
        			includes.push_back(newline);
                    if(all) getIncludes(includes,allHeaders,allSource,newline);
        		}
    		}
        }
    }
    input.close();
}
std::vector<std::string> compile(const std::string& wd,const std::vector<std::string>& parameters,
    const std::vector<std::string>& allHeaders, 
    const std::vector<std::string>& allSource,
    const bool changeSet, const bool log){

    std::vector<std::string> incDirs, toCompile;
    for(int i = 0; i < allHeaders.size();++i){
        std::string folder = getFolder(allHeaders[i]);
        if(find(incDirs, folder) == -1)
            incDirs.push_back(folder);
    }
    std::string bd = wd + "/" + reqFolders[1];
    std::string id = wd + "/" + reqFolders[0];
    auto HDdirs = getDirs(id + "/" + subFolders[0]);
    auto SDdirs = getDirs(bd + "/" + subFolders[0]);
    //Обновления списка зависимостей
    if(changeSet){
        for(int i = 1; i < HDdirs.size(); ++i){
            std::fstream file(HDdirs[i]);
            if(!file.is_open()){
                std::cout << std::endl;
                std::cout << "======================== ERROR ========================" << std::endl;
                std::cout << "====== Compile.cpp: compile() ======" << std::endl;
                std::cout << "Cannot open file: " << HDdirs[i] << std::endl;
                std::cout << "Try launching builder with --rebuild / -reb flag" << std::endl;       
                std::cout << std::endl;
                return std::vector<std::string>{};
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
                return std::vector<std::string>{};
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
        UpdateDependencies(bd,id,allHeaders,allSource);
    }
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
            return std::vector<std::string>{};
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
            return std::vector<std::string>{};
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

    int m = (toCompile.size() / numThreads) + 1;
    //int m = (toCompile.size()) + 1;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> multiCompile;
    for(int i = 0; i < numThreads && i*m < toCompile.size(); ++i)
        multiCompile.push_back(std::vector<std::string>(toCompile.begin() + i*m,
         toCompile.begin() + (((i+1)*m < toCompile.size()) ? (i+1)*m : toCompile.size())));
    for (int i = 0; i < multiCompile.size(); ++i)
        threads.push_back(std::thread(oneThreadCompile, 
            std::ref(multiCompile[i]),
            std::ref(incDirs), std::ref(bd),log,std::ref(parameters)));
    for (auto& thread : threads) {
        if (thread.joinable())
            thread.join(); 
    }
    return toCompile;
}
void UpdateDependencies(const std::string& bd, const std::string& id,
    const std::vector<std::string>& allHeaders, 
    const std::vector<std::string>& allSource){

    // Проход по сурс файлам
    for(int i = 0; i < allSource.size(); ++i){
        std::vector<std::string> includes;
        getIncludes(includes, allHeaders, allSource, allSource[i]);
        for(int j = 0; j < includes.size(); ++j){
            std::string path;
            if(getExt(includes[j]) == "h" || getExt(includes[j]) == "hpp")
                path = id + "/" + subFolders[0] + "/" + getName(includes[j]);
            else
                path = bd + "/" + subFolders[0] + "/" + getName(includes[j]);
            std::string s = bd + "/" + subFolders[0] + "/" + getName(allSource[i]) + " ";
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
                path = id + "/" + subFolders[0] + "/" + getName(includes[j]);
            else
                path = bd + "/" + subFolders[0] + "/" + getName(includes[j]);
            std::string s = id + "/" + subFolders[0] + "/" + getName(allHeaders[i]) + " ";
            appendToFile(path,s);
        }
    }
}
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
void compileFile(const std::string& path, 
    const std::vector<std::string>& incDirs,const std::string& bd, const bool log,
    const std::vector<std::string>& parameters){

    if(!log) std::cout << "Compiling " << getName(path) << std::endl;
    std::vector<std::string> depfile;
    std::string line;
    std::ifstream in(path);
    while(std::getline(in,line)) depfile.push_back(line);
    std::string compiler;
    int code = -1;
    std::string name = getName(path);
    //std::string asmFile = bd + "/" + subFolders[1] + "/" + name + ".asm";
    std::string objFile = bd + "/" + subFolders[1] + "/" + name + ".o";
    std::string include = "";
    for(int i = 0; i < incDirs.size(); ++i) include += std::string("-I" + incDirs[i] + " ");
    std::vector<std::string> compilers = split(parameters[5]); 
    std::string ext = getExt(path);
    if(ext == "cpp"){
        if(compilers[1] == "default") compiler = "g++ ";
        else compiler = (compilers[1] + " ");
    }
    else{
        if(compilers[0] == "default") compiler = "gcc ";
        else compiler = (compilers[0] + " ");
    }
    std::string cmd = "";
    // =============== КОМПИЛЯЦИЯ В АССЕМБЛЕР : УСТАРЕЛО ===============
    // // Компиляция в ассемблер для последующего парсинга
    // 
    // if(ext != "cpp" && ext != "c"){ // ассемблерный файл
    //     if(compilers[2] == "default")
    //         cmd = "cpp ";
    //     else
    //         cmd = (compilers[2] + " ");
    //     if(exists(asmFile)){
    //         std::string rmcmd = "rm " + asmFile;
    //         system(rmcmd.c_str());
    //     }
    //     cmd += ("-P " + depfile[0] + " >> " + asmFile);
    //     if(log){
    //         std::cout << cmd << std::endl;
    //         std::cout << std::endl;
    //     }
    //     code = system(cmd.c_str());
    // }
    // else{
    //     cmd = compiler;
    //     for(int i = 7; i <= 10; ++i){
    //         if(parameters[i] != "-1")
    //             cmd += (parameters[i] + " "); 
    //     } // флаги компилятору
    //     if(parameters[12] != "-1") // general flags
    //         cmd += (parameters[12] + " ");
    //     cmd += (include + depfile[0] + " -S -o " + asmFile);
    //     if(log){
    //         std::cout << cmd << std::endl;
    //         std::cout << std::endl;
    //     }
    //     code = system(cmd.c_str());
    // }
    // =========================================================================


    // Компиляция сразу в объектник
    //if(code == 0){
    if(getExt(depfile[0]) == "cpp" || getExt(depfile[0]) == "c") cmd = compiler;
    else cmd = compiler + "-x assembler-with-cpp ";
    for(int i = 7; i <= 10; ++i) // разные флаги + флаги конкретно компилятору
        if(parameters[i] != "-1") cmd += (parameters[i] + " "); 
    if(parameters[12] != "-1") cmd += (parameters[12] + " "); // general flags
    cmd += (include + depfile[0] + " -c -o " + objFile);
    if(log) std::cout << cmd << '\n' << std::endl;
    code = system(cmd.c_str());
    //}      

    // Обновление depFile
    std::ofstream out(path);
    out << depfile[0] << std::endl;
    if(code == 0) out << getChangeTime(depfile[0]) << std::endl;
    else out << "-1" << std::endl;
    if(depfile.size() > 2) out << depfile[2];
    out.close();
}
void oneThreadCompile(const std::vector<std::string>& toCompile, 
    const std::vector<std::string>& incDirs,const std::string& bd,const bool log,
    const std::vector<std::string>& parameters){

    for(int i = 0; i < toCompile.size(); ++i)
        compileFile(toCompile[i],incDirs,bd,log,parameters);
}