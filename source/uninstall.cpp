#include "uninstall.h"
void uninstall(){
    removeDirectory(root);
    removeAlias("belder", root + "/builder");
    removeAlias("sudo_belder","sudo " + root + "/builder");
    std::cout << "belder has been removed from your computer" << std::endl;
}