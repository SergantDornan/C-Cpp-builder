#include "essentials.h"
#include "DepFiles.h"
#include "Compile.h"
#include "Linker.h"
#include "mainFuncs.h"
#include "Flags.h"
// -log (Выводить все действия)
// --rebuild -reb (Удалить папку проекта, потом восстановить)
// -o
// run
// uninstall
// --no-link-force - force unlink file
// --link-force - force link file
// --default-link - default link file
// --no-link-lib
// status - show config
// -I<path> + include folder
// /// <externLinkFlags> ///
// // <externCompileFlags> //


// Структура project config:
// main input
// outputname
// libs linking
// force link list
// force unlink list
// compilers
// additional -I list


const std::vector<std::string> possibleFlags = {"--rebuild", "-reb", 
	"-o", "--no-link-force", "--link-force", "--default-link",
	"--no-link-lib", "--CC", "--CXX"};
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
	bool getFlags = false;
	for(int i = 0; i < args.size(); ++i){
		if(args[i] == "//" || args[i] == "///")
			getFlags = !getFlags;
		else if(isFlag(args[i]) && find(possibleFlags, args[i]) == -1 &&
			args[i][1] != 'I' && args[i][1] != 'l' && !getFlags)
			otherFlags += (args[i] + " ");
	}
	for(int i = 0; i < args.size(); ++i){
		if(args[i] == "//" && !getFlags)
			getFlags = true;
		else if(args[i] == "//" && getFlags){
			getFlags = false;
			break;
		}
		else if(getFlags && args[i] != "//")
			externCompileFlags += (args[i] + " ");
	}
	for(int i = 0; i < args.size(); ++i){
		if(args[i] == "///" && !getFlags)
			getFlags = true;
		else if(args[i] == "///" && getFlags){
			getFlags = false;
			break;
		}
		else if(getFlags && args[i] != "///")
			externLinkFlags += (args[i] + " ");
	}
	//bool log = (find(args, "-log") != -1);
	bool log = true;
	bool rebuild = ((find(args, "-reb") != -1) || (find(args, "--rebuild") != -1));
	bool run = (find(args, "run") != -1);
	std::string wd = createEssentials(rebuild);
	std::string projectConfig = wd + "/" + configFile;
	std::vector<std::string> parameters = getParameters(args, projectConfig,cd);
	bool relink = false;
	if(find(args, "--no-link-force") != -1 || find(args, "--link-force") != -1 ||
		find(args, "--default-link") != -1){
		relink = true;
		std::vector<std::string> fLink, fUnlink;
		if(parameters[3] != "-1")
			fLink = split(parameters[3]);
		if(parameters[4] != "-1")
			fUnlink = split(parameters[4]);
		std::vector<std::string> newfLink, newfUnlink;
		std::vector<std::string> defLink;
		for(int i = 0; i < args.size(); ++i){
			if(args[i] == "--no-link-force"){
				if((i + 1) >= args.size() || ((i + 1) <  args.size() &&
					isFlag(args[i+1]))){
					std::cout << "==================== ERROR ====================" << std::endl;
					std::cout << "no file name after --no-link-force flag" << std::endl;
					return -1;
				}
				newfUnlink.push_back(args[i+1]);
			}
			if(args[i] == "--link-force"){
				if((i + 1) >= args.size() || ((i + 1) <  args.size() &&
					isFlag(args[i+1]))){
					std::cout << "==================== ERROR ====================" << std::endl;
					std::cout << "no file name after --link-force flag" << std::endl;
					return -1;
				}
				newfLink.push_back(args[i+1]);
			}
			if(args[i] == "--default-link"){
				if((i + 1) >= args.size() || ((i + 1) <  args.size() &&
					isFlag(args[i+1]))){
					std::cout << "==================== ERROR ====================" << std::endl;
					std::cout << "no file name after --default-link flag" << std::endl;
					return -1;
				}
				defLink.push_back(args[i+1]);
			}
		}
		auto it1 = fLink.begin();
		while(it1 != fLink.end()){
			if(find(newfUnlink, *it1) != -1)
				fLink.erase(it1);
			else
				it1++;
		} 
		auto it2 = fUnlink.begin();
		while(it2 != fUnlink.end()){
			if(find(newfLink, *it2) != -1)
				fUnlink.erase(it2);
			else
				it2++;
		}
		for(int i = 0; i < newfLink.size(); ++i)
			fLink.push_back(newfLink[i]);
		for(int i = 0; i < newfUnlink.size(); ++i)
			fUnlink.push_back(newfUnlink[i]);
		it1 = fLink.begin();
		while(it1 != fLink.end()){
			if(find(defLink, *it1) != -1)
				fLink.erase(it1);
			else
				it1++;
		}
		it2 = fUnlink.begin();
		while(it2 != fUnlink.end()){
			if(find(defLink, *it2) != -1)
				fUnlink.erase(it2);
			else
				it2++;
		}
		if(fLink.size() == 0)
			parameters[3] = "-1";
		else{
			std::string s;
			for(int i = 0; i < fLink.size(); ++i)
				s += (fLink[i] + " ");
			parameters[3] = s;
		}
		if(fUnlink.size() == 0)
			parameters[4] = "-1";
		else{
			std::string s;
			for(int i = 0; i < fUnlink.size(); ++i)
				s += (fUnlink[i] + " ");
			parameters[4] = s;
		}
	}
	for(int i = 0; i < args.size(); ++i){
		std::vector<std::string> exLibs, newlibs;
		bool rewrite = false;
		if(args[i].size() > 2 && isFlag(args[i]) && args[i][1] == 'l' && args[i] != "-log")
			newlibs.push_back(std::string(args[i].begin() + 2, args[i].end()));
		if(parameters[2] != "-1")
			exLibs = split(parameters[2]);
		for(int i = 0; i < newlibs.size(); ++i){
			if(find(exLibs, newlibs[i]) == -1){
				rewrite = true;
				exLibs.push_back(newlibs[i]);
			}
		}
		for(int i = 0; i < args.size(); ++i){
			if(args[i] == "--no-link-lib"){
				if((i + 1) >= args.size() || ((i + 1) < args.size() && isFlag(args[i+1]))){
					std::cout << "===================== ERROR =====================" << std::endl;
					std::cout << "No lib name after --no-link-lib flag" << std::endl;
					return -1;
				}
				rewrite = true;
				auto it = exLibs.begin();
				while(it != exLibs.end()){
					if(*it == args[i+1])
						exLibs.erase(it);
					else
						it++;
				}
			}
		}
		if(rewrite){
			if(exLibs.size() == 0)
				parameters[2] = "-1";
			else{
				std::string s = "";
				for(int i = 0; i < exLibs.size(); ++i)
					s += (exLibs[i] + " ");
				parameters[2] = s;
			}
		}
	}
	if(args.size() != 0 && args[0] == "status"){
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
		if(parameters[5] != "default default")
			std::cout << "Compilers for C and C++ : " << parameters[5] << std::endl;
		if(parameters[6] != "-1")
			std::cout << "Additional directories: " << parameters[6] << std::endl;
		return 0;
	}
	std::ofstream out(projectConfig);
	for(int i = 0; i < parameters.size(); ++i)
		out << parameters[i] << std::endl;
	out.close();
	if(args.size() != 0 && args[0] == "config"){
		if(log)
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
    else if(linkmsg == "nothing to link" && exists(parameters[1]))
    	std::cout << "belder: nothing to link" << std::endl;
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