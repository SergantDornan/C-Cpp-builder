#include "mainFuncs.h"
void uninstall(){
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
std::string CheckSameFiles(const std::vector<std::string>& allSource){
	for(int i = 0; i < allSource.size()-1; ++i){
		for(int j = i+1; j < allSource.size(); ++j){
			if(getNameNoExt(allSource[i]) == getNameNoExt(allSource[j]))
				return (allSource[i] + "\n" + allSource[j]);
		}
	}
	return "-1";
}