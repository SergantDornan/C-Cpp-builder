#include "Anal.h"
#include "Linker.h"
#include "SymIndex.h"
#include "Toolchain.h"
#include <map>
#include <set>
#include <unistd.h>

typedef struct {
	int kind;
	std::string path;
	std::vector<std::string> flags;
} AnalTarget;

enum { ANAL_CLEAR, ANAL_DIR_TARGET, ANAL_FILE, ANAL_COMPILER };

static bool analMode = false;

static std::string analDir(){
	return root + "/" + ANAL_DIR;
}

static std::string globalSymfile(const std::string& path){
	return analDir() + "/" + convertPathToName(path) + ".sym";
}

static bool isAnalFile(const std::string& path){
	return getLibType(path) != "" || getExt(path) == "o";
}

static std::string findInPath(const std::string& name){
	if(name.find('/') != std::string::npos) return "";
	const char* env = getenv("PATH");
	if(env == nullptr) return "";
	for(const std::string& dir : split(env, ":")){
		if(dir.empty()) continue;
		std::string candidate = dir + "/" + name;
		if(!std::filesystem::is_directory(candidate) && access(candidate.c_str(), X_OK) == 0) return candidate;
	}
	return "";
}

static void getAllObjects(std::vector<std::string>& objects, const std::string& path){
	auto dirs = getDirs(path);
	for(int i = 1; i < dirs.size(); ++i){
		try {
			if(((pocket && (dirs[i] == cd + "/builder")) || getName(dirs[i]) == ".git") &&
				std::filesystem::is_directory(dirs[i])) continue;
			if(std::filesystem::is_directory(dirs[i])){
				getAllObjects(objects, dirs[i]);
				continue;
			}
		} catch (const std::filesystem::filesystem_error& e) {
			continue;
		}
		if(getExt(dirs[i]) == "o") objects.push_back(dirs[i]);
	}
}

static bool symfileFresh(const std::string& symFile, const std::string& path){
	std::ifstream file(symFile);
	std::string name, time;
	if(!std::getline(file, name) || !std::getline(file, time)) return false;
	return name == path && time == getChangeTime(path);
}

std::string symfilePath(const std::string& wd, const std::string& path){
	std::string global = globalSymfile(path);
	if(analMode || exists(global)) return global;
	return wd + "/" + SYM_DIR + "/" + convertPathToName(path) + ".sym";
}

bool readFreshSymfile(binFile& newfile, const std::string& symFile){
	return symfileFresh(symFile, newfile.name) && readSymfile(newfile, symFile);
}

void saveSymfile(binFile& newfile, const std::string& symFile){
	std::string folder = getFolder(symFile);
	if(!exists(folder)) std::filesystem::create_directories(folder);
	std::string tmp = symFile + "." + std::to_string(getpid());
	createSymfile(newfile, tmp);
	std::error_code ec;
	std::filesystem::rename(tmp, symFile, ec);
	if(ec) removeFile(tmp);
}

void getLinkerLibs(std::vector<std::string>& libs, const std::vector<std::string>& dirs,
	const std::vector<std::string>& userArgs, const std::vector<std::string>& fUnLib)
{
	if(find(userArgs, "-nostdlib") != -1 || find(userArgs, "-nodefaultlibs") != -1) return;
	std::vector<std::string> all;
	for(const std::string& dir : dirs)
		if(std::filesystem::is_directory(dir)) getAllLibs(all, dir, fUnLib, {}, false);
	for(const std::string& path : all)
		if(exists(globalSymfile(path))) libs.push_back(path);
}

static void printAnalUsage(){
	std::cerr << "Usage:" << std::endl;
	std::cerr << "\tbelder anal <path> [<path> ...]" << std::endl;
	std::cerr << "\tbelder anal <compiler> [compiler flags]" << std::endl;
	std::cerr << "\tbelder anal clear" << std::endl;
	std::cerr << std::endl;
}

static int getAnalTargets(const std::vector<std::string>& args, std::vector<AnalTarget>& targets){
	for(int i = 1; i < args.size(); ++i){
		const std::string& arg = args[i];
		if(arg == "clear" || arg == "clean"){
			targets.push_back({ANAL_CLEAR, "", {}});
			continue;
		}
		if(!arg.empty() && arg[0] == '-'){
			if(targets.empty() || targets.back().kind != ANAL_COMPILER){
				std::cerr << "===================== ERROR =====================" << std::endl;
				std::cerr << "Flag " << arg << " is not after a compiler" << std::endl;
				std::cerr << "Flags are passed only to a compiler: belder anal gcc -m32" << std::endl;
				std::cerr << std::endl;
				return 1;
			}
			targets.back().flags.push_back(arg);
			continue;
		}
		std::string path = getFullPath(cd, arg);
		bool exist = (path != "-1" && exists(path));
		if(exist && std::filesystem::is_directory(path)) targets.push_back({ANAL_DIR_TARGET, path, {}});
		else if(exist && isAnalFile(path)) targets.push_back({ANAL_FILE, path, {}});
		else if(exist && access(path.c_str(), X_OK) == 0) targets.push_back({ANAL_COMPILER, path, {}});
		else if(!findInPath(arg).empty()) targets.push_back({ANAL_COMPILER, arg, {}});
		else{
			std::cerr << "===================== ERROR =====================" << std::endl;
			if(exist) std::cerr << arg << " is not a folder, a library, an object file or a compiler" << std::endl;
			else std::cerr << arg << " is not an existing path or a compiler from PATH" << std::endl;
			std::cerr << std::endl;
			return 1;
		}
	}
	if(targets.empty()){
		std::cerr << "===================== ERROR =====================" << std::endl;
		std::cerr << "Nothing to analyze" << std::endl;
		printAnalUsage();
		return 1;
	}
	return 0;
}

static int getCompilerFiles(const AnalTarget& target, std::vector<std::string>& files){
	std::string probe = analDir() + "/probe" + std::to_string(getpid()) + ".o";
	createFile(probe);
	std::vector<std::string> inputs, dirs;
	implicitLinkInputs(target.path, target.flags, false, probe, inputs, dirs);
	removeFile(probe);
	if(dirs.empty() && inputs.empty()){
		std::cerr << "===================== ERROR =====================" << std::endl;
		std::cerr << "Cannot get linker search folders from: " << target.path;
		for(const std::string& flag : target.flags) std::cerr << " " << flag;
		std::cerr << std::endl;
		std::cerr << "belder asks it with \"-###\", is it a C/C++ compiler?" << std::endl;
		std::cerr << std::endl;
		return 1;
	}
	for(const std::string& dir : dirs)
		if(std::filesystem::is_directory(dir)) getAllLibs(files, dir, {}, {}, false);
	for(const std::string& input : inputs)
		if(find(files, input) == -1) files.push_back(input);
	return 0;
}

static void analFiles(const std::vector<std::string>& files, int& analyzed, int& upToDate){
	std::map<std::pair<uint64_t, uint64_t>, binFile> parsed;
	for(const std::string& path : files){
		std::string symFile = symfilePath("", path);
		if(symfileFresh(symFile, path)){
			++upToDate;
			continue;
		}
		IndexLib stamp;
		bool known = statLib(path, stamp);
		auto same = parsed.find({stamp.dev, stamp.inode});
		if(known && same != parsed.end()){
			binFile copy = same->second;
			copy.name = path;
			saveSymfile(copy, symFile);
		}
		else{
			binFile newfile = {path};
			LibAnal("", newfile);
			if(known) parsed[{stamp.dev, stamp.inode}] = newfile;
		}
		++analyzed;
	}
}

int anal(const std::vector<std::string>& args){
	std::vector<AnalTarget> targets;
	if(getAnalTargets(args, targets) != 0) return 1;
	analMode = true;
	for(const AnalTarget& target : targets){
		if(target.kind == ANAL_CLEAR){
			removeDirectory(analDir());
			createDirectory(analDir());
			std::cout << "belder: " << analDir() << " has been cleared" << std::endl;
			continue;
		}
		if(!exists(analDir())) std::filesystem::create_directories(analDir());
		std::vector<std::string> files;
		if(target.kind == ANAL_DIR_TARGET){
			getAllLibs(files, target.path, {}, {});
			getAllObjects(files, target.path);
		}
		else if(target.kind == ANAL_FILE) files.push_back(target.path);
		else if(getCompilerFiles(target, files) != 0) return 1;
		int analyzed = 0, upToDate = 0;
		analFiles(files, analyzed, upToDate);
		std::cout << "belder anal: " << target.path;
		for(const std::string& flag : target.flags) std::cout << " " << flag;
		std::cout << std::endl;
		std::cout << "\tfiles: " << files.size() << ", analyzed: " << analyzed << ", up to date: " << upToDate << std::endl;
	}
	return 0;
}
