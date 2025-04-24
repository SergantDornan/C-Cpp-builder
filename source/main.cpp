#include "essentials.h"
#include "DepFiles.h"
#include "Compile.h"
#include "Linker.h"
#include "mainFuncs.h"
#include "Flags.h"
#include "uninstall.h"
#include "StatusCheck.h"
// -log (Выводить все действия)
// --rebuild -reb (Удалить папку проекта, потом восстановить)
// --relink -rel (Просто перелинковать)
// -o
// run
// uninstall
// --no-link-force - force unlink file
// --link-force - force link file
// --default-link - default link file
// --no-link-lib
// status - show config
// -I<path> + include folder
// --link-flags
// --compile-flags
// --clear-flags

// Структура project config:
// main input
// outputname
// libs linking
// force link list
// force unlink list
// compilers
// additional -I list
// C standart
// optimization
// debug
// Flags to compiler
// Flags to linker
// generalFlags

int main(int argc, char* argv[]){
	if(cd.find(' ') != std::string::npos || 
		cd.find('(') != std::string::npos ||
		cd.find(')') != std::string::npos){
		std::cout << "================== ERROR ==================" << std::endl;
		std::cout << "Current directory has forbidden characters in it: " << std::endl;
		std::cout << cd << std::endl;
		std::cout << "Shell will not understeand you while compiling" << std::endl;
		return 0;
	}
	std::vector<std::string> args;
	for(int i = 1; i < argc; ++i)
		args.push_back(std::string(argv[i]));
	if(args.size() != 0 && args[0] == "uninstall"){
		uninstall();
		return 0;
	}


	bool log = (find(args, "-log") != -1);
	bool rebuild = ((find(args, "-reb") != -1) || (find(args, "--rebuild") != -1));
	bool run = (find(args, "run") != -1);

	bool relink = (find(args, "--no-link-force") != -1 || find(args, "--link-force") != -1 ||
		find(args, "--default-link") != -1 || find(args, "--relink") != -1 || find(args, "-rel") != -1);
	std::string wd = createEssentials(rebuild);
	std::string projectConfig = wd + "/" + configFile;
	std::vector<std::string> parameters = getParameters(args, projectConfig,cd);
	
	

	if(args.size() != 0 && args[0] == "status"){
		printStatus(parameters);
		return 0;
	}

	std::ofstream out(projectConfig);
	for(int i = 0; i < parameters.size(); ++i)
		out << parameters[i] << std::endl;
	out.close();
	if(args.size() != 0 && args[0] == "config"){
		std::cout << "Config updated" << std::endl;
		return 0;
	}

	std::vector<std::string> allHeaders, allSource;
	getAllheaders(allHeaders,cd,parameters[4]);
	getAllsource(allSource,cd,parameters[4]);
	if(parameters[6] != "-1"){
		auto AddInc = split(parameters[6]);
		for(int i = 0; i < AddInc.size(); ++i){
			if(!std::filesystem::is_directory(AddInc[i]) || !exists(AddInc[i])){
            	std::cout << "========================== ERROR ==========================" << std::endl;
            	std::cout << "Additional directory: " << AddInc[i] << std::endl;
            	std::cout << "does not exists" << std::endl;
            	std::cout << "if it does write full path to this folder" << std::endl;
            	return -1;
        	} 
			getAllheaders(allHeaders, AddInc[i],parameters[4]);
			getAllsource(allSource, AddInc[i],parameters[4]);
		}
	}



	std::string sameFiles = CheckSameFiles(allSource);
	if(sameFiles != "-1"){
		std::cout << "===================== ERROR =====================" << std::endl;
		std::cout << "Found two files with same name, cannot build:" << std::endl;
		std::cout << sameFiles << std::endl;
		return -1;
	}


	std::vector<std::string> includes;
	getIncludes(includes,allHeaders,allSource,parameters[0],true);
	int linkType = 0;
	if(getName(parameters[1]).size() > 5){
		std::string name = getName(parameters[1]);
		std::string prefix(name.begin(), name.begin() + 3);
		if(prefix == "lib" && getExt(name) == "a")
			linkType = 1;
	}

	
	bool changeSet = createDepfiles(wd, allHeaders, allSource, log);
	std::vector<std::string> toCompile = compile(wd,parameters,allHeaders,allSource,changeSet, log);
	std::string linkmsg = link(wd, parameters, includes, toCompile, log, linkType, relink);
	

	if(linkmsg == "succes" && exists(parameters[1]))
		std::cout << "============================ SUCCES ============================" << std::endl;
    else if(linkmsg == "nothing to link" && exists(parameters[1])){
    	std::cout << "belder: nothing to link" << std::endl;
    	std::cout << "================================================================" << std::endl;
    }
    if(run && exists(parameters[1])){
		//if(run && !onefile){
		if(linkType == 0){
			std::string cmd = parameters[1];
			system(cmd.c_str());
		}
		else{
			std::cout << "================= ERROR =================" << std::endl;
			std::cout << "Cannot run a library" << std::endl;
		}
	}
	if(linkmsg == "nothing to link")
    	return 10;
    else
    	return 0;
}