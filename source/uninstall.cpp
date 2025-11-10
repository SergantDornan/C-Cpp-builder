#include "uninstall.h"
void uninstall(){
    std::string cmd = "rm -rf " + root;
    system(cmd.c_str());
    removeAlias("belder", root + "/builder");
    std::cout << "belder has been removed from your computer" << std::endl;
}