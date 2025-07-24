#include "essentials.h"
#include "DepFiles.h"
#include "Compile.h"
#include "Linker.h"
#include "Flags.h"
#include "uninstall.h"
#include "StatusCheck.h"

// ИЗМЕНЕНИЯ В РИДМИ:
// УКАЗАТЬ ПРО БИБЛИОТЕКИ, ЧТО ОНИ ТОЖЕ ПАРСЯТСЯ, ЧТО У НИХ ТЕПЕРЬ ТЕ ЖЕ ПРАВИЛА
// И ФЛАГИ, ЧТО И У ФАЙЛОВ (force-link, force-unlink, def-link и тд), ТАКЖЕ СОХРАНЕНО
// ПРОСТО -laboba, оно попадает в категорию force-link
// ЕЩЕ ПРО ОПЦИЮ ХЕЛП НАПИСАТЬ
// Переделать вывод status
// Вывод логов с мутехами (компиляция + линковка)


// Следующая строка заполняется инсталлятором, не менять ее
const std::string SourceCodeFolder;
// -log (Выводить все действия)
// --rebuild -reb (Удалить папку проекта, потом восстановить)
// --relink -rel (Просто перелинковать)
// -o
// run
// uninstall
// --no-link-force - force unlink file (lib)
// --link-force - force link file (lib)
// --default-link - default link file (lib)
// status - show config
// -I<path> + include folder
// --link-flags
// --compile-flags
// --clear-flags
// clean, clear, mrproper - удалить папку с build
// --no-include чтобы отменить -I флаг или не включать подпапку

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
// force unlink libs
// force unlink dirs

int main(int argc, char* argv[]){

	if(pocket && !exists(root)){
		std::string cmd = "mkdir " + root;
		system(cmd.c_str());
	}
	if(exists(root) && 
		argc >= 2 && (std::string(argv[1]) == "clean" || std::string(argv[1]) == "clear" ||
			std::string(argv[1]) == "mrproper"))
	{
		if(pocket){
			std::string cmd = "rm -rf " + root;
			system(cmd.c_str());
			std::cout << root << " has been removed" << std::endl;
		}
		else removeBuildFolder();
		return 0;
	}
	if(cd.find(' ') != std::string::npos || 
		cd.find('(') != std::string::npos ||
		cd.find(')') != std::string::npos){
		std::cout << "================== ERROR ==================" << std::endl;
		std::cout << "Current directory has forbidden characters in it: " << std::endl;
		std::cout << cd << std::endl;
		std::cout << "Shell will not understeand you while compiling" << std::endl;
		return 1;
	}
	std::vector<std::string> args;
	for(int i = 1; i < argc; ++i) args.push_back(std::string(argv[i]));
	if(args.size() != 0 && args[0] == "help"){
		printHelp();
		return 0;
	}
	if(args.size() != 0 && args[0] == "uninstall"){
		uninstall();
		return 0;
	}
	if(args.size() != 0 && args[0] == "reinstall"){
		if(!exists(SourceCodeFolder)){
			std::cout << "===================== ERROR =====================" << std::endl;
			std::cout << "Cannot find folder with source code, cannot reinstall" << std::endl;
			std::cout << std::endl;
			std::cout << std::endl;
			return 1;
		}
		std::string cmd;
		int checkCompileCode = 0;
		cmd = "make -C " + SourceCodeFolder;
		if(args.size() > 2 && args[1] == "-j")
			cmd += (" " + args[1] + " " + args[2]);
		else
			cmd += " -j 8";
		checkCompileCode |= system(cmd.c_str());
		if(checkCompileCode != 0) return 1;
		if(!pocket) uninstall();
		if(!pocket) cmd = "make install -C " + SourceCodeFolder;
		else cmd = "make pocket -C " + SourceCodeFolder;
		system(cmd.c_str());
		if(pocket){
			cmd = "rm pocketbuilder";
			system(cmd.c_str());
			cmd = "cp " + SourceCodeFolder + "/pocketbuilder " + cd;
			system(cmd.c_str());
		}
		return 0;
	}

	bool log = (find(args, "-log") != -1);
	bool rebuild = ((find(args, "-reb") != -1) || (find(args, "--rebuild") != -1));
	bool run = (find(args, "run") != -1);
	bool idgaf = (find(args, "--idgaf") != -1);
	bool relink = (find(args, "--no-link-force") != -1 || find(args, "--link-force") != -1 ||
		find(args, "--default-link") != -1 || find(args, "--relink") != -1 || find(args, "-rel") != -1);
	std::string wd = createEssentials(rebuild);
	std::string projectConfig = wd + "/" + configFile;
	std::vector<std::string> parameters = getParameters(args, projectConfig, cd);
	
	std::ofstream out(projectConfig);
	for(int i = 0; i < parameters.size(); ++i) out << parameters[i] << std::endl;
	out.close();

	if(args.size() != 0 && args[0] == "status"){
		printStatus(parameters);
		return 0;
	}

	if(args.size() != 0 && args[0] == "config"){
		std::cout << "Config updated" << std::endl;
		return 0;
	}
	if(parameters[0] == "-1") return 1;
	std::vector<std::string> allHeaders, allSource, allLibs;
	std::vector<std::string> fUnIncludeDirs, fUnLib, forceUnlink;
	if(parameters[4] != "-1") forceUnlink = split(parameters[4]);
	if(parameters[13] != "-1") fUnLib = split(parameters[13]);
	if(parameters[14] != "-1") fUnIncludeDirs = split(parameters[14]);
	getAllheaders(allHeaders,cd,forceUnlink,fUnIncludeDirs);
	getAllsource(allSource,cd,forceUnlink,fUnIncludeDirs); 
	getAllLibs(allLibs,cd,fUnLib,fUnIncludeDirs); 
	if(parameters[6] != "-1"){ // additional -I list
		auto AddInc = split(parameters[6]);
		for(int i = 0; i < AddInc.size(); ++i){
			if(!std::filesystem::is_directory(AddInc[i]) || !exists(AddInc[i])){
            	std::cout << "========================== ERROR ==========================" << std::endl;
            	std::cout << "Additional directory: " << AddInc[i] << std::endl;
            	std::cout << "does not exists" << std::endl;
            	std::cout << "if it does write full path to this folder" << std::endl;
            	return 1;
        	} 
			getAllheaders(allHeaders, AddInc[i], forceUnlink,fUnIncludeDirs);
			getAllsource(allSource, AddInc[i], forceUnlink,fUnIncludeDirs);
			getAllLibs(allLibs, AddInc[i],fUnLib,fUnIncludeDirs);
		}
	}
 	std::vector<std::string> includes;
	getIncludes(includes,allHeaders,allSource,parameters[0],true);

	int linkType = 0;
	if(getName(parameters[1]).size() > 5){
		std::string name = getName(parameters[1]);
		std::string prefix(name.begin(), name.begin() + 3);
		if(prefix == "lib"){
			std::string ext = getExt(name); 
			if(ext == "a") linkType = 1;
			else if(ext == "so") linkType = 2;
		}
	}
	bool changeSet = createDepfiles(wd, allHeaders, allSource, log);
	std::vector<std::string> toCompile = compile(wd,parameters,allHeaders,allSource,changeSet,log);
	std::string linkmsg = link(wd, parameters, includes, toCompile, 
		log, linkType, relink, idgaf, allLibs);
	
	if(linkmsg == "success" && exists(parameters[1]))
		std::cout << "============================ SUCCESS ============================" << std::endl;
    else if(linkmsg == "nothing to link" && exists(parameters[1]))
    	std::cout << "belder: nothing to link" << std::endl;
    if(run && exists(parameters[1])){
		if(linkType == 0){
			std::string cmd = parameters[1];
			system(cmd.c_str());
		}
		else{
			std::cout << "================= ERROR =================" << std::endl;
			std::cout << "Cannot run a library" << std::endl;
		}
	}
	if(linkmsg == "nothing to link") return 10;
    else return 0;
}
