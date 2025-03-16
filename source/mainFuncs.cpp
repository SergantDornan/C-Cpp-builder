#include "mainFuncs.h"
std::string CheckSameFiles(const std::vector<std::string>& allSource){
	for(int i = 0; i < allSource.size()-1; ++i){
		for(int j = i+1; j < allSource.size(); ++j){
			if(getName(allSource[i]) == getName(allSource[j]))
				return (allSource[i] + "\n" + allSource[j]);
		}
	}
	return "-1";
}