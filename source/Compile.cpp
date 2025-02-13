#include "Compile.h"
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


    if(parameters[5] == "riscv"){
        if(!checkProgram("riscv64-linux-gnu-gcc")){
            std::cout << "========================= ERROR =========================" << std::endl;
            std::cout << "You trying to compile for riscv architecture," << std::endl;
            std::cout << "but no compiler for C language found" << std::endl;
            std::cout << "This builder works with riscv64-linux-gnu-gcc compiler" << std::endl;
            std::cout << "Install riscv64-linux-gnu-gcc to compile C" << std::endl;
        }
        if(!checkProgram("riscv64-linux-gnu-g++")){
            std::cout << "========================= ERROR =========================" << std::endl;
            std::cout << "You trying to compile for riscv architecture," << std::endl;
            std::cout << "but no compiler for C++ language found" << std::endl;
            std::cout << "This builder works with riscv64-linux-gnu-g++ compiler" << std::endl;
            std::cout << "Install riscv64-linux-gnu-g++ to compile C++" << std::endl;
        }
    }
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
        if(log){
            std::cout << "Set of files has changed => updating dependencies" << std::endl;
            std::cout << std::endl;
        }
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
            if(log)
                std::cout << "Changed file: " << getName(v[0]) << std::endl;
            std::ofstream out(HDdirs[i]);
            out << v[0] << std::endl;
            out << getChangeTime(v[0]) << std::endl;
            if(v.size() > 2)
                out << v[2];
            out.close();
            std::vector<std::string> recCheck;
            updateFile(toCompile,HDdirs[i], incDirs,bd, recCheck);
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
            if(log)
                std::cout << "Changed file: " << getName(v[0]) << std::endl;
            std::ofstream out(SDdirs[i]);
            out << v[0] << std::endl;
            out << getChangeTime(v[0]) << std::endl;
            if(v.size() > 2)
                out << v[2];
            out.close();
            std::vector<std::string> recCheck;
            updateFile(toCompile,SDdirs[i], incDirs,bd, recCheck);
        }
    }
    // if(log){
    //     std::cout << std::endl;
    //     for(int i = 0; i < toCompile.size(); ++i)
    //         std::cout << "Compiling " << getName(toCompile[i]) << std::endl;
    //     std::cout << std::endl;
    // }





    int m = (toCompile.size() / numThreads) + 1;
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
    const std::string& path,const std::vector<std::string>& incDirs,
    const std::string& bd, std::vector<std::string>& recCheck){


    
    recCheck.push_back(getName(path));
    std::ifstream file(path);
    if(!file.is_open())
        return;
    std::vector<std::string> v;
    std::string line;
    while(std::getline(file,line))
        v.push_back(line);
    file.close();
    if(getExt(path) != "h" && getExt(path) != "hpp" && find(toCompile, v[0]) == -1)
        toCompile.push_back(v[0]);
    if(v.size() > 2){
        auto depFiles = split(v[2]);
        for(int i = 0; i < depFiles.size(); ++i){
            if(find(recCheck, getName(depFiles[i])) == -1){
                updateFile(toCompile,depFiles[i],incDirs,bd, recCheck);
                recCheck.push_back(getName(depFiles[i]));
            }
        }
    }

}
void compileFile(const std::string& path, 
    const std::vector<std::string>& incDirs,const std::string& bd, const bool log,
    const std::vector<std::string>& parameters){

    std::string arch = parameters[5];
    std::string as, compiler, flags;
    if(arch == "x86"){
        if(getExt(path) == "cpp"){
            compiler = "g++ ";
            flags = (Cppstandart + " ");
        }
        else{
            compiler = "gcc ";
            flags = (Cstandart + " ");
        }
        as = "as ";
    }
    else if(arch == "riscv"){
        if(getExt(path) == "cpp"){
            compiler = "riscv64-linux-gnu-g++ ";
            flags = (Cppstandart + " ");
        }
        else{
            compiler = "riscv64-linux-gnu-gcc ";
            flags = (Cstandart + " ");
        }
        as = "riscv64-linux-gnu-as ";
    }
    else{
        std::cout << "================== WTF ==================" << std::endl;
        std::cout << "architecture is: " << arch << std::endl;
        std::cout << "how the fuck did it happen" << std::endl;
        std::cout << std::endl;
        return;
    }
    if(log)
        std::cout << "Compiling " << getName(path) << std::endl;
    flags += (opt + " ");
    flags += (debug + " ");
    flags += (otherFlags + " ");
    std::string name = getNameNoExt(path);
    std::string asmFile = bd + "/" + subFolders[1] + "/" + name + ".asm";
    std::string objFile = bd + "/" + subFolders[2] + "/" + name + ".o";
    std::string include = "";
    for(int i = 0; i < incDirs.size(); ++i)
        include += std::string("-I" + incDirs[i] + " ");
    std::string cmd = compiler + flags + include + path + " -S -o " + asmFile;
    system(cmd.c_str());
    cmd = as + asmFile + " -o " + objFile;
    system(cmd.c_str());
    if(arch != "x86"){
        if(getExt(path) == "cpp") compiler = "g++ ";
        else compiler = "gcc ";
        cmd = compiler + flags + include + path + " -S -o " + asmFile;
        system(cmd.c_str());
    }
}
void oneThreadCompile(const std::vector<std::string>& toCompile, 
    const std::vector<std::string>& incDirs,const std::string& bd,const bool log,
    const std::vector<std::string>& parameters){

    for(int i = 0; i < toCompile.size(); ++i)
        compileFile(toCompile[i],incDirs,bd,log,parameters);
}