#include "Compile.h"

std::mutex mtx;

// Структура dep файла:
// путь к описываемому файлу
// время изменения описываемого файла
// все файлы от которых файл зависит
// все зависимости (файлы которые зависят от текущего)

std::vector<std::string> compile(const std::string& wd,const std::vector<std::string>& parameters,
    const std::vector<std::string>& allHeaders, 
    const std::vector<std::string>& allSource,
    const bool changeSet, const bool log, const int linkType){

    std::vector<std::string> incDirs, toCompile;
    for(int i = 0; i < allHeaders.size();++i){
        std::string folder = getFolder(allHeaders[i]);
        if(find(incDirs, folder) == -1) incDirs.push_back(folder);
    }
    std::string bd = wd + "/" + reqFolders[1];
    std::string id = wd + "/" + reqFolders[0];
    auto HDdirs = getDirs(id + "/" + subFolders[0]);
    auto SDdirs = getDirs(bd + "/" + subFolders[0]);
    //Обновления списка зависимостей
    int code = 0;
    UpdateDependencies(HDdirs, SDdirs, bd, id, allHeaders, allSource);
    code |= updateFiles(toCompile, HDdirs, SDdirs); // создание списка toCompile
    if(code != 0) return std::vector<std::string>{};
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
            std::ref(incDirs), std::ref(bd),log,std::ref(parameters), linkType));
    for (auto& thread : threads) {
        if (thread.joinable())
            thread.join(); 
    }
    return toCompile;
}


void compileFile(const std::string& path, 
    const std::vector<std::string>& incDirs,const std::string& bd, const bool log,
    const std::vector<std::string>& parameters, const int linkType){

    std::vector<std::string> depfile;
    std::string line;
    std::ifstream in(path);
    while(std::getline(in,line)) depfile.push_back(line);
    in.close();
    if(!log){
        mtx.lock();
        std::cout << "Compiling " << getName(depfile[0]) << std::endl;
        mtx.unlock();
    }
    std::string compiler;
    int code = -1;
    std::string objFile = bd + "/" + subFolders[1] + "/" + converPathToName(depfile[0]) + ".o";
    std::string include = "";
    for(int i = 0; i < incDirs.size(); ++i) include += std::string("-I" + incDirs[i] + " ");
    std::vector<std::string> compilers = split(parameters[5]); 
    std::string ext = getExt(depfile[0]);
    if(ext == "c"){
        if(compilers[0] == "default") compiler = "gcc ";
        else compiler = (compilers[0] + " ");
    }
    else{
        if(compilers[1] == "default") compiler = "g++ ";
        else compiler = (compilers[1] + " ");
    }
    std::string cmd = "";

    // Компиляция сразу в объектник
    
    if(getExt(depfile[0]) == "cpp" || getExt(depfile[0]) == "c") cmd = compiler;
    else cmd = compiler + "-x assembler-with-cpp ";
    if(linkType == 2) cmd += "-fPIC ";
    for(int i = 7; i <= 10; ++i) // разные флаги + флаги конкретно компилятору
        if(parameters[i] != "-1") cmd += (parameters[i] + " "); 
    if(parameters[12] != "-1") cmd += (parameters[12] + " "); // general flags
    cmd += (include + depfile[0] + " -c -o " + objFile);
    if(log) {
        mtx.lock();
        std::cout << cmd << '\n' << std::endl;
        mtx.unlock();
    }
    code = system(cmd.c_str());

    // Обновление depFile
    std::ofstream out(path);
    out << depfile[0] << std::endl;
    if(code == 0) out << getChangeTime(depfile[0]) << std::endl;
    else out << "-1" << std::endl;
    out << depfile[2] << std::endl;
    out << depfile[3] << std::endl;
    out.close();
}
void oneThreadCompile(const std::vector<std::string>& toCompile, 
    const std::vector<std::string>& incDirs,const std::string& bd,const bool log,
    const std::vector<std::string>& parameters, const int linkType){

    for(int i = 0; i < toCompile.size(); ++i) 
        compileFile(toCompile[i],incDirs,bd,log,parameters,linkType);
}