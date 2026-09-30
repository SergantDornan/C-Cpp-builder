#include "Compile.h"
#include "ConfigIndex.h"
#include "Process.h"

std::mutex mtx;

// Структура dep файла:
// путь к описываемому файлу
// время изменения описываемого файла
// все файлы от которых файл зависит
// все зависимости (файлы которые зависят от текущего)

std::vector<std::string> compile(const std::string& wd,const std::vector<std::string>& parameters,
    const bool changeSet, const bool log, const int linkType,
    const std::vector<FileNode>& map, const std::vector<int>& leaves,
    int numThreads, const std::vector<std::string>& pairSource){

    if(numThreads == -1) numThreads = std::thread::hardware_concurrency();
    std::vector<std::string> toCompile;
    std::string bd = wd + "/" + SOURCE_DIR;
    std::string id = wd + "/" + HEADERS_DIR;
    auto HDdirs = getDirs(id + "/" + DEPS_DIR);
    auto SDdirs = getDirs(bd + "/" + DEPS_DIR);
    //Обновления списка зависимостей
    int code = 0;
    UpdateDependencies(HDdirs, SDdirs, bd, id, map, leaves);
    code |= updateFiles(toCompile, HDdirs, SDdirs); // создание списка toCompile
    if(code != 0) return std::vector<std::string>{};
    std::vector<std::string> pairDepfiles;
    for(int i = 0; i < pairSource.size(); ++i)
        pairDepfiles.push_back(convertPathToName(pairSource[i]));
    auto it = toCompile.begin();
    while(it != toCompile.end()){
        if(find(pairDepfiles, getName(*it)) == -1){
            markDepfileDirty(*it);
            it = toCompile.erase(it);
        }
        else it++;
    }
    int m = (toCompile.size() / numThreads) + 1;
    //int m = (toCompile.size()) + 1;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> multiCompile;
    for(int i = 0; i < numThreads && i*m < toCompile.size(); ++i)
        multiCompile.push_back(std::vector<std::string>(toCompile.begin() + i*m,
         toCompile.begin() + (((i+1)*m < toCompile.size()) ? (i+1)*m : toCompile.size())));
    
    std::vector<int> results(multiCompile.size()); 
    
    for (int i = 0; i < multiCompile.size(); ++i)
        threads.push_back(std::thread(oneThreadCompile, 
            std::ref(multiCompile[i]),
            std::ref(bd),log, std::ref(parameters) , linkType, std::ref(results[i])));
    
    for (auto& thread : threads) {
        if (thread.joinable())
            thread.join(); 
    }

    for(int i = 0; i < results.size(); ++i){
        if(results[i] != 0) {
            toCompile.clear();
            toCompile.push_back("-1");
            break;
        }
    }
    return toCompile;
}


int compileFile(const std::string& path, 
    const std::string& bd, const bool log,
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
    std::string objFile = bd + "/" + OBJECTS_DIR + "/" + convertPathToName(depfile[0]) + ".o";
    std::vector<std::string> incDirs;
    if(depfile[4] != "-1") incDirs = split(depfile[4]);
    std::vector<std::string> compilers = split(parameters[CFG_COMPILERS]);
    std::string ext = getExt(depfile[0]);
    std::string standart = "-1";
    if(ext == "c"){
        if(compilers[0] == "default") compiler = "gcc";
        else compiler = compilers[0];
        standart = parameters[CFG_C_STANDARD];
    }
    else{
        if(compilers[1] == "default") compiler = "g++";
        else compiler = compilers[1];
        standart = parameters[CFG_CXX_STANDARD];
    }

    // Компиляция сразу в объектник - собираем argv без shell (см. Process.h)
    std::vector<std::string> argv;
    appendArgs(argv, compiler);
    if(!(ext == "cpp" || ext == "c")){
        argv.push_back("-x");
        argv.push_back("assembler-with-cpp");
    }
    if(linkType == 2) argv.push_back("-fPIC");
    for(int i = CFG_OPT; i <= CFG_COMPILE_FLAGS; ++i) // разные флаги + флаги конкретно компилятору
        if(parameters[i] != "-1") appendArgs(argv, parameters[i]);
    if(standart != "-1") appendArgs(argv, standart);
    if(parameters[CFG_GENERAL_FLAGS] != "-1") appendArgs(argv, parameters[CFG_GENERAL_FLAGS]); // general flags
    for(int i = 0; i < incDirs.size(); ++i) argv.push_back("-I" + incDirs[i]);
    argv.push_back(depfile[0]);
    argv.push_back("-c");
    argv.push_back("-o");
    argv.push_back(objFile);
    if(log) {
        mtx.lock();
        std::cout << joinArgs(argv) << '\n' << std::endl;
        mtx.unlock();
    }
    code = runProcess(argv);

    // Обновление depFile
    std::ofstream out(path);
    out << depfile[0] << std::endl;
    if(code == 0) out << getChangeTime(depfile[0]) << std::endl;
    else out << "-1" << std::endl;
    out << depfile[2] << std::endl;
    out << depfile[3] << std::endl;
    out << depfile[4] << std::endl;
    out.close();
    return code;
}
void oneThreadCompile(const std::vector<std::string>& toCompile,
    const std::string& bd,const bool log,
    const std::vector<std::string>& parameters, const int linkType, int& code){

    bool success = false;
    for(int i = 0; i < toCompile.size(); ++i) 
        success |= (compileFile(toCompile[i],bd,log,parameters,linkType) != 0);
    if(!success) code = 0;
    else code = 1; 

}