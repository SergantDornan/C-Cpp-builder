#include "uninstall.h"
#include "BuilderFilework.h"
#include "alias.h"
#include <thread>

void rewriteLine(const std::string& path, 
    const std::string& oldLine, const std::string& newLine)
{
    std::vector<std::string> lines;
    std::string line;
    std::ifstream input(path);
    if(!input.is_open()){
        std::cout << "================= ERROR =================" << std::endl;
        std::cout << "installer.cpp: rewriteLine: cannot open file: " << std::endl;
        std::cout << path << std::endl;
        std::cout << "=========================================" << std::endl;
        return;
    }
    while(std::getline(input, line))
        lines.push_back(line);
    input.close();
    for(int i = 0; i < lines.size(); ++i){
        if(lines[i] == oldLine){
            lines[i] = newLine;
            break;
        }
    }
    std::ofstream out(path);
    for(int i = 0; i < lines.size(); ++i)
        out << lines[i] << std::endl;
    out.close();
}

std::string standartOutput;
const std::string pocketOutput = "OUTPUT=./pocketbuilder";
const size_t numThreads = std::thread::hardware_concurrency();
bool checkProgram(const std::string& programName) {
    std::string command = "which " + programName + " > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}
int main(int argc, char* argv[]) {
    std::string standartOutput = "OUTPUT=" + root + "/builder";
    bool pocket = false;
    if(argc >= 2 && std::string(argv[1]) == "pocket")
        pocket = true;
    if(pocket && exists("./pocketbuilder")){
        std::string cmd = "rm ./pocketbuilder";
        system(cmd.c_str());
    }
    if(!checkProgram("gcc")){
        std::cout << "=============== ERROR ===============" << std::endl;
        std::cout << "No gcc found" << std::endl;
    	std::cout << "Install gcc to compile C" << std::endl;
        std::cout << std::endl;
    }
    if(!checkProgram("g++")){
        std::cout << "=============== ERROR ===============" << std::endl;
        std::cout << "No g++ found" << std::endl;
        std::cout << "Install g++ to compile C++" << std::endl;
        std::cout << std::endl;
    }
    if(!checkProgram("make")){
        std::cout << "=============== FATAL ERROR ===============" << std::endl;
        std::cout << "No make found" << std::endl;
        std::cout << "You need make to install builder" << std::endl;
        std::cout << "This is fatal error, install make before installation" << std::endl;
        return -1;
    }
    std::string cmd;
    std::string output;
    if(pocket) output = pocketOutput;
    else output = standartOutput;
    rewriteLine(cd + "/source/main.cpp",
        "const std::string SourceCodeFolder;",
        std::string("const std::string SourceCodeFolder = \"" + cd + "\";"));
    rewriteLine(cd + "/Makefile","OUTPUT=",output);
    if(pocket)
        rewriteLine(cd + "/source/BuilderFilework.cpp",
            "const std::string root = getHomedir() + \"/builder\";",
            "const std::string root = \"./builder\";");
    if(!pocket) addAlias("belder", root + "/builder");
    cmd = "make -C " + cd + " -j " + std::to_string(numThreads);
    int code = system(cmd.c_str());
    if(exists(root) && !pocket && code == 0){
        std::cout << "====================== ERROR ======================" << std::endl;
        std::cout << "Folder: " << root << " already exists" << std::endl;
        std::cout << "Cannot install builder" << std::endl;
        std::cout << "Remove " << root << " before installation" << std::endl;
        return -1;
    }
    if(!pocket && code == 0){
        cmd = "mkdir " + root;
        system(cmd.c_str());
    }
    if(code == 0){
        cmd = "make link -C " + cd + " -j " + std::to_string(numThreads);
        system(cmd.c_str());
    }
    rewriteLine(cd + "/source/main.cpp",
        std::string("const std::string SourceCodeFolder = \"" + cd + "\";"),
        "const std::string SourceCodeFolder;");
    rewriteLine(cd + "/Makefile",output,"OUTPUT=");
    if(pocket)
        rewriteLine(cd + "/source/BuilderFilework.cpp",
            "const std::string root = \"./builder\";",
            "const std::string root = getHomedir() + \"/builder\";");
    if(code == 0 && (exists(root + "/builder") || exists("./pocketbuilder")))
        std::cout << "================= Builder has been installed =================" << std::endl;
    return 0;
}