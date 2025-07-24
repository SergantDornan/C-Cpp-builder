#include "StatusCheck.h"
void printHelp(){
	std::cout << "Specify entry file right after belder if it is not main.c or main.cpp:" << std::endl;
	std::cout << "\t\tbelder entry.cpp (exmp)" << std::endl;
	std::cout << std::endl;
	std::cout << "\trun\ttype \"run\" in any place to run output file" << std::endl;
	std::cout << std::endl;
	std::cout << "\tuninstall\ttype \"uninstall\" right after \"belder\" to uninstall belder:" << std::endl;
	std::cout << "\t\tbelder uninstall" << std::endl;
	std::cout << std::endl;
	std::cout << "\treinstall\ttype \"reinstall\" right after \"belder\" to reinstall belder:" << std::endl;
	std::cout << "\t\tbelder reinstall (you must have source code folder)" << std::endl;
	std::cout << std::endl;
	std::cout << "\tstatus\ttype \"belder status\" to see all information about project (flags, force-link and force-unlink list etc)" << std::endl;
	std::cout << std::endl;
	std::cout << "\tclear, clean, mrproper\tremove build folder (with object files, dep files, configs)" << std::endl;
	std::cout << std::endl;
	std::cout << "FLAGS:" << std::endl;
	std::cout << "\t-log\toutput executed commands to console" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--rebuild, -reb\trecompile all files" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--relink, -rel\trelink output file" << std::endl;
	std::cout << std::endl;
	std::cout << "\t-o [filename]\tspecify output file name" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--no-link-force [file] [file] [file]\tspecify files that definitely will not be linked" << std::endl;
	std::cout << "\t\tbelder --no-link-force file1.cpp file2.cpp libMylib.a (exmp)" << std::endl;
	std::cout << "\t\tbelder --no-link-force Mylib (\"Mylib\" will be considered as a library,\n\tso the two files will not be linked: libMylib.a, libMylib.so)" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--link-force [file] [file] [file]\tspecify files that definitely will be linked" << std::endl;
	std::cout << "\t\tbelder --link-force file1.cpp file2.cpp libMylib.a (exmp)" << std::endl;
	std::cout << "\t\tbelder --link-force Mylib (\"Mylib\" will be considered as a library\n\tif belder finds both libMylib.a and libMylib.so then libMylib.so  will be linked)" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--default-link [file] [file] [file]\tlet belder decide whether to link files or not" << std::endl;
	std::cout << "\t\tbelder --default-link file1.cpp file2.cpp libMylib.a (exmp)" << std::endl;
	std::cout << "\t\tbelder --default-link Mylib (\"Mylib\" will be considered as a library,\n\tso the two files will be excluded from the force-link and force-unlink lists: libMylib.a, libMylib.so)" << std::endl;
	std::cout << std::endl;
	std::cout << "\t-l[short lib name]\tlink libraries with -l flag and short library name" << std::endl;
	std::cout << "\t\t\"belder -lMylib\" is equivalent to \"belder --link-force Mylib\"" << std::endl;
	std::cout << std::endl;
	std::cout << "\t-I[path]\tadditionally, specify the directory in which to search for headers, source files and libraries" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--no-include [path] [path] [path]\tspecify the directories in which NOT to search for headers, source files and libraries" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--defult-include [path] [path] [path]\tspecify the directories to exclude from --no-include and additional include list" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--compile-flags [many flags]\tCompilation will occur with these flags, but not linking" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--link-flags [many flags]\tLinking will occur with these flags, but not compilation" << std::endl;
	std::cout << "\t\tFlags passed to the belder without --compile-flags or --link-flags option will be used both during compilation and linking" << std::endl;
	std::cout << std::endl;
	std::cout << "\t--clear-flags, --clean-flags, --flags-clean, --flags-clear\tclears all flags" << std::endl;
	std::cout << std::endl;
}
void printStatus(const std::vector<std::string>& parameters){
	if(parameters[0] != "-1")
		std::cout << "Entry file: " << parameters[0] << std::endl;
	std::cout << "Output file: " << parameters[1] << std::endl;
	if(parameters[3] != "-1"){
		std::cout << "Force linking files: " << std::endl;
		std::cout << "\t" << parameters[3] << std::endl;
	}
	if(parameters[4] != "-1"){
		std::cout << "Force unlinking files: " << std::endl;
		std::cout << "\t" << parameters[4] << std::endl;
	}
	auto compilers = split(parameters[5]);
	std::cout << "C compiler:	" << ((compilers[0] == "default") ? "gcc" : compilers[0]) << std::endl;
	std::cout << "CPP compiler:	" << ((compilers[1] == "default") ? "g++" : compilers[1]) << std::endl;
	
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
	if(parameters[6] != "-1"){
		std::cout << "Additional directories: " << std::endl;
		std::cout << "\t" << parameters[6] << std::endl;
	}
	if(parameters[14] != "-1"){
		std::cout << "Force unlinking directories: " << std::endl;
		std::cout << "\t" << parameters[14] << std::endl;
	}
	if(parameters[2] != "-1"){
		std::cout << "Force linking libs: " << std::endl;
		std::cout << "\t" << parameters[2] << std::endl; 
	}
	if(parameters[13] != "-1"){
		std::cout << "Force Unlinking libs: " << std::endl;
		std::cout << "\t" << parameters[13] << std::endl;
	}
}