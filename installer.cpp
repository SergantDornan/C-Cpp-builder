#include "uninstall.h"
#include "BuilderFilework.h"
#include "alias.h"
#include <thread>
const size_t numThreads = std::thread::hardware_concurrency();
bool checkProgram(const std::string& programName) {
    std::string command = "which " + programName + " > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}
int main(int argc, char* argv[]) {
    if(argc >= 2 && std::string(argv[1]) == "reinstall")
        uninstall();
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
    if(exists(root)){
    	std::cout << "====================== ERROR ======================" << std::endl;
    	std::cout << "Folder: " << root << " already exists" << std::endl;
        std::cout << "Cannot install builder" << std::endl;
    	std::cout << "Remove " << root << " before installation" << std::endl;
        return -1;
    }
    std::string cmd = "mkdir " + root;
    system(cmd.c_str());
    std::ifstream makefile(cd + "/Makefile");
    std::vector<std::string> lines;
    std::string line;
    std::getline(makefile,line);
    lines.push_back(line);
    if(line != std::string("OUTPUT=" + root + "/builder")){
        while(std::getline(makefile, line))
            lines.push_back(line);
        lines[0] = "OUTPUT=" + root + "/builder";
    }
    makefile.close();
    if(lines.size() > 1){
        std::ofstream out(cd + "/Makefile");
        for(int i = 0; i < lines.size(); ++i)
            out << lines[i] << std::endl;
        out.close();
    }
    addAlias("belder", root + "/builder");
    cmd = "make -C " + cd + " -j " + std::to_string(numThreads);
    system(cmd.c_str());
    if(exists(root + "/builder"))
        std::cout << "================= Builder has been installed =================" << std::endl;
    return 0;
}