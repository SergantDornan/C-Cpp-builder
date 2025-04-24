#include "StatusCheck.h"
void printStatus(const std::vector<std::string>& parameters){
	if(parameters[0] != "-1")
		std::cout << "Entry file: " << getName(parameters[0]) << std::endl;
	std::cout << "Output file: " << parameters[1] << std::endl;
	if(parameters[2] == "-1")
		std::cout << "Linking no libs" << std::endl;
	else
		std::cout << "Linking libs: " << parameters[2] << std::endl;
	if(parameters[3] != "-1")
		std::cout << "Force linking files: " << parameters[3] << std::endl;
	if(parameters[4] != "-1")
		std::cout << "Force unlinking files: " << parameters[4] << std::endl;
	if(parameters[5] != "default default default "){
		auto v = split(parameters[5]);
		std::cout << "C compiler:	" << v[0] << std::endl;
		std::cout << "CPP compiler:	" << v[1] << std::endl;
		std::cout << "Preprocessor:	" << v[2] << std::endl; 
	}
	if(parameters[6] != "-1"){
		auto v = split(parameters[6]);
		std::cout << "Additional directories: " << std::endl;
		for(int i = 0; i < v.size(); ++i)
			std::cout << "\t" << v[i] << std::endl;
	}
	if(parameters[7] != "-1")
		std::cout << "Standart: " << parameters[7] << std::endl;
	if(parameters[8] != "-1")
		std::cout << "Opt: " << parameters[8] << std::endl;
	if(parameters[9] != "-1")
		std::cout << "Debug: " << parameters[9] << std::endl;
	if(parameters[10] != "-1"){
		std::cout << "Compile flags: " << std::endl;
		std::cout << "\t" << parameters[10] << std::endl;
	}
	if(parameters[11] != "-1"){
		std::cout << "Link flags: " << std::endl;
		std::cout << "\t" << parameters[11] << std::endl;
	}
	if(parameters[12] != "-1"){
		std::cout << "Other Flags: " << std::endl;
		std::cout << "\t" << parameters[12] << std::endl;
	}
}