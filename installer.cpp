#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> 
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <filesystem>
#include <fstream>
#include <thread>
const size_t numThreads = std::thread::hardware_concurrency();
std::string getHomedir(){
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        return homeDir;
    } else {
        std::cout << "======================== ERROR ========================" << std::endl;
        std::cout << "==== some error in installer.cpp: getHomedir() ====" << std::endl;
        std::cout << std::endl;      
        return "";
    }
}
void uninstall(){
    std::string root = getHomedir() + "/builder";
    std::string cmd = "rm -rf " + root;
    system(cmd.c_str());
    std::string alias = "alias belder='" + root + "/builder'";
    std::string bash = getHomedir() + "/" + ".bashrc";
    std::string line;
    std::vector<std::string> v;
    std::ifstream file(bash);
    while(std::getline(file,line)){
        if(line != alias)
            v.push_back(line);
    }
    file.close();
    std::ofstream newfile(bash);
    for(int i = 0; i < v.size(); ++i)
        newfile << v[i] << std::endl;
    newfile.close();
    std::cout << "Builder has been removed from your computer" << std::endl;
}
std::vector<std::string> getDirs(const std::string &path) {
	std::vector<std::string> dirs;
	if(!std::filesystem::is_directory(path)){
		std::cout << "======================= ERROR =======================" << std::endl;
		std::cout << "=============== installer.cpp: getDirs ===============" << std::endl;
		std::cout << "path leads to a file, not directory" << std::endl;
		std::cout << path << std::endl;
		std::cout << "=====================================================" << std::endl;
		return dirs;
	}
  std::string back = path;
  while (back.back() != '/')
    {
      back.pop_back();
    }
  back.pop_back();
  dirs = {back};
  for (const std::filesystem::directory_entry &dir : std::filesystem::directory_iterator(path))
    {
      dirs.push_back(dir.path().string());
    }
  return dirs;
}
bool checkProgram(const std::string& programName) {
    std::string command = "which " + programName + " > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}
bool exists(const std::string& path){
	return std::filesystem::exists(path);
}
const std::string root = getHomedir() + "/builder";
int main(int argc, char* argv[]) {
    if(argc >= 2 && std::string(argv[1]) == "reinstall")
        uninstall();
    bool doMake = true;
    if(argc > 1 && std::string(argv[1]) == "--no-make")
        doMake = false;
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
    std::ifstream makefile("./Makefile");
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
        std::ofstream out("./Makefile");
        for(int i = 0; i < lines.size(); ++i)
            out << lines[i] << std::endl;
        out.close();
    }
    std::string bash = getHomedir() + "/" + ".bashrc";
    std::string alias = "alias belder='" + root + "/builder'";
    std::ofstream bshrc(bash, std::ios::app);
    bshrc << alias << std::endl;
    bshrc.close();
    system(alias.c_str());
    if(doMake){
        cmd = "make -j " + std::to_string(numThreads);
        system(cmd.c_str());
    }
    if(exists(root + "/builder"))
        std::cout << "================= Builder has been installed =================" << std::endl;
    return 0;
}