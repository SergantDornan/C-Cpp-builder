#include "essentials.h"
#include "DepFiles.h"
#include "Compile.h"
#include "Linker.h"
#include "Flags.h"
#include "uninstall.h"
#include "StatusCheck.h"
#include "Mapping.h"
#include "ConfigIndex.h"

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
// --clean-options, --clear-options - удалить все флаги, очиситить все force-link листы, AddInc листы и прочее 

// Структура project config:
// main input 0
// outputname 1
// libs linking 2
// force link list 3
// force unlink list 4
// compilers 5
// additional -I list 6
// C++ standart 7
// optimization 8
// debug 9
// Flags to compiler 10
// Flags to linker 11
// generalFlags 12
// force unlink libs 13
// force unlink dirs 14
// C standart 15

int main(int argc, char* argv[]){
	if(pocket && !exists(root))
		createDirectory(root);

	std::vector<std::string> args;
	for(int i = 1; i < argc; ++i) args.push_back(std::string(argv[i]));

	int numThreads = -1;
	// Переделываем текущую директорию + ищем количество потоков:
	auto it = args.begin();
	while(it != args.end()){
		if(*(it) == "-C"){
			if((it+1) == args.end() || isFlag(*(it+1))){
				std::cerr << "No directory after -C flag" << std::endl;
				std::cerr << std::endl;
				return 1;
			}
			if(!exists(*(it+1))){
				std::cerr << *(it+1) << " does not exist" << std::endl;
				std::cerr << std::endl;
				return 1;
			}
			std::string newcd = getFullPath(cd,*(it+1));
			if(newcd[newcd.size() - 1] == '/') cd = std::string(newcd.begin(), newcd.end()-1);
			else cd = newcd;
			args.erase(it+1);
			args.erase(it);
		}
		else if(*(it) == "-T"){
			if((it+1) == args.end() || isFlag(*(it+1))){
				std::cerr << "No number after -T flag" << std::endl;
				std::cerr << std::endl;
				return 1;
			}
			try{
				numThreads = std::stoi(*(it + 1));
			}
			catch(const std::exception&){
				std::cerr << "Invalid number after -T flag: " << *(it + 1) << std::endl;
				std::cerr << std::endl;
				return 1;
			}
			if(numThreads <= 0){
				std::cerr << "Number of threads after -T flag must be positive" << std::endl;
				std::cerr << std::endl;
				return 1;
			}
			args.erase(it+1);
			args.erase(it);
		}
		else it++;
	} 
	// --------------------------------

	bool clear = (find(args, "clean") != -1 || 
					find(args, "clear") != -1 ||
					find(args, "mrproper") != -1 ||
					find(args, "silent_clear") != -1);

	if(exists(root) && clear){
		if(pocket){
			removeDirectory(root);
			std::cout << root << " has been removed" << std::endl;
		}
		else removeBuildFolder(cd, (find(args, "silent_clear") != -1));
		return 0;
	}
	if(cd.find(' ') != std::string::npos || 
		cd.find('(') != std::string::npos ||
		cd.find(')') != std::string::npos){
		std::cerr << "================== ERROR ==================" << std::endl;
		std::cerr << "Current directory has forbidden characters in it: " << std::endl;
		std::cerr << cd << std::endl;
		std::cerr << "Shell will not understeand you while compiling" << std::endl;
		return 1;
	}

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
			std::cerr << "===================== ERROR =====================" << std::endl;
			std::cerr << "Cannot find folder with source code, cannot reinstall" << std::endl;
			std::cerr << std::endl;
			std::cerr << std::endl;
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
		if(pocket && (SourceCodeFolder != cd)){
			removeFile("pocketbuilder");
			cmd = "cp " + SourceCodeFolder + "/pocketbuilder " + cd;
			system(cmd.c_str());
		}
		return 0;
	}
	bool log = (find(args, "-log") != -1);
	bool rebuild = ((find(args, "-reb") != -1) || (find(args, "--rebuild") != -1));
	bool run = (find(args, "run") != -1);
	bool idgaf = (find(args, "--idgaf") != -1);
	bool relink = (find(args, "--relink") != -1 || find(args, "-rel") != -1);
	std::string wd = createEssentials(rebuild);
	std::string projectConfig = wd + "/" + CONFIG_FILE;
	std::string prInName, prOutName;
	std::ifstream f(projectConfig);
	std::getline(f, prInName);
	std::getline(f, prOutName);
	f.close();
	bool parameters_recompile = false, parameters_relink = false;
	std::vector<std::string> parameters = getParameters(args, projectConfig, cd, prInName,
		parameters_recompile, parameters_relink);
	rebuild |= parameters_recompile;
	relink |= parameters_relink;
	if(rebuild) clearAllDepFiles(wd);
	rebuildForSharedLib(prOutName, parameters[CFG_OUTPUT], wd);
	//if(prOutName != parameters[CFG_OUTPUT] || prInName != parameters[CFG_ENTRY]) relink = true;
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

	// Имя выходного файла подставляется в shell-команды компиляции/линковки
	// (system()). Метасимволы в нём -> инъекция команд. Пока сборка идёт через
	// system() со склейкой строк, отсекаем такие имена. Разрешены обычные
	// для пути символы; запрещены shell-метасимволы и пробелы.
	const std::string forbiddenChars = " \t\n\r;&|<>()$`\"'\\*?!{}[]~#";
	if(parameters[CFG_OUTPUT].find_first_of(forbiddenChars) != std::string::npos){
		std::cerr << "================== ERROR ==================" << std::endl;
		std::cerr << "Output file name contains forbidden characters: " << std::endl;
		std::cerr << parameters[CFG_OUTPUT] << std::endl;
		std::cerr << "Shell metacharacters and spaces are not allowed in the output name" << std::endl;
		std::cerr << std::endl;
		return 1;
	}

	int linkType = 0;
	if(getName(parameters[CFG_OUTPUT]).size() > 5){
		std::string name = getName(parameters[CFG_OUTPUT]);
		std::string prefix(name.begin(), name.begin() + 3);
		if(prefix == "lib"){
			std::string ext = getExt(name); 
			if(ext == "a") linkType = 1;
			else if(ext == "so") linkType = 2;
		}
	}
	if(parameters[CFG_ENTRY] == "-1") return 1;
	std::vector<std::string> allHeaders, allSource, allLibs;
	std::vector<std::string> fUnIncludeDirs, fUnLib, forceUnlink;
	if(parameters[CFG_FORCE_UNLINK] != "-1") forceUnlink = split(parameters[CFG_FORCE_UNLINK]);
	if(parameters[CFG_FORCE_UNLINK_LIBS] != "-1") fUnLib = split(parameters[CFG_FORCE_UNLINK_LIBS]);
	if(linkType == 1 || linkType == 2) fUnLib.push_back(parameters[CFG_OUTPUT]);
	if(parameters[CFG_FORCE_UNLINK_DIRS] != "-1") fUnIncludeDirs = split(parameters[CFG_FORCE_UNLINK_DIRS]);
	getAllheaders(allHeaders,cd,forceUnlink,fUnIncludeDirs);
	getAllsource(allSource,cd,forceUnlink,fUnIncludeDirs); 
	getAllLibs(allLibs,cd,fUnLib,fUnIncludeDirs); 
	if(parameters[CFG_ADD_INCLUDE] != "-1"){ // additional -I list
		auto AddInc = split(parameters[CFG_ADD_INCLUDE]);
		for(int i = 0; i < AddInc.size(); ++i){
			if(!std::filesystem::is_directory(AddInc[i]) || !exists(AddInc[i])){
            	std::cerr << "========================== ERROR ==========================" << std::endl;
            	std::cerr << "Additional directory: " << AddInc[i] << std::endl;
            	std::cerr << "does not exists" << std::endl;
            	std::cerr << "if it does write full path to this folder" << std::endl;
            	std::cerr << std::endl;
            	return 1;
        	} 
			getAllheaders(allHeaders, AddInc[i], forceUnlink,fUnIncludeDirs);
			getAllsource(allSource, AddInc[i], forceUnlink,fUnIncludeDirs);
			getAllLibs(allLibs,AddInc[i],fUnLib,fUnIncludeDirs);
		}
	}
	
	std::vector<FileNode> map;
	std::vector<int> leaves = getMap(allHeaders,allSource,map);
 	std::vector<std::string> includes, dummy;
	getIncludes(includes,dummy,map,leaves,parameters[CFG_ENTRY],true);
	bool changeSet = createDepfiles(wd, allHeaders, allSource, log);
	std::vector<std::string> toCompile = compile(wd,parameters,changeSet,log,linkType,map,leaves,numThreads);
	updateSymfiles(wd, allLibs);
	std::string linkmsg = link(wd, parameters, includes, toCompile, 
		log, linkType, relink, idgaf, allLibs);
	
	if(linkmsg == "success" && exists(parameters[CFG_OUTPUT]))
		std::cout << "============================ SUCCESS ============================\n" << std::endl;
    else if(linkmsg == "nothing to link" && exists(parameters[CFG_OUTPUT]))
    	std::cout << "belder: nothing to link" << std::endl;
    else if(linkmsg == "compilation error")
    	std::cout << "belder: compilation error" << std::endl;
    if(run && exists(parameters[CFG_OUTPUT]) && linkmsg != "compilation error"){
		if(linkType == 0){
			std::string cmd = parameters[CFG_OUTPUT];
			system(cmd.c_str());
		}
		else{
			std::cerr << "================= ERROR =================" << std::endl;
			std::cerr << "Cannot run a library" << std::endl;
			std::cerr << std::endl;
		}
	}
	if(linkmsg == "nothing to link") return 0;
	else if(linkmsg == "compilation error") return 2;
    else return 0;
}
