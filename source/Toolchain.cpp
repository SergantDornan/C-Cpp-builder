#include "Toolchain.h"
#include "ConfigIndex.h"
#include "Process.h"
#include "algs.h"
#include "filework.h"
#include <algorithm>
#include <cstring>
#include <set>
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

extern char** environ;

static int runProcessCapture(const std::vector<std::string>& argv, std::string& output){
	output.clear();
	if(argv.empty()) return -1;
	std::vector<char*> cargv;
	for(const std::string& a : argv) cargv.push_back(const_cast<char*>(a.c_str()));
	cargv.push_back(nullptr);
	int fds[2];
	if(pipe2(fds, O_CLOEXEC) != 0) return -1;
	posix_spawn_file_actions_t actions;
	if(posix_spawn_file_actions_init(&actions) != 0){
		close(fds[0]);
		close(fds[1]);
		return -1;
	}
	posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&actions, fds[1], STDERR_FILENO);
	pid_t pid;
	int rc = posix_spawnp(&pid, cargv[0], &actions, nullptr, cargv.data(), environ);
	posix_spawn_file_actions_destroy(&actions);
	close(fds[1]);
	if(rc != 0){
		close(fds[0]);
		return -1;
	}
	char buf[4096];
	ssize_t n;
	while((n = read(fds[0], buf, sizeof(buf))) > 0) output.append(buf, n);
	close(fds[0]);
	int status = 0;
	if(waitpid(pid, &status, 0) < 0) return -1;
	return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string linkCompiler(const std::vector<std::string>& parameters){
	std::vector<std::string> compilers = split(parameters[CFG_COMPILERS]);
	if(getExt(parameters[CFG_ENTRY]) == "cpp")
		return (compilers.size() < 2 || compilers[1] == "default") ? "g++" : compilers[1];
	return (compilers.empty() || compilers[0] == "default") ? "gcc" : compilers[0];
}

void linkUserArgs(const std::vector<std::string>& parameters, std::vector<std::string>& argv){
	for(int i = CFG_LINK_FLAGS; i <= CFG_GENERAL_FLAGS; ++i)
		if(parameters[i] != "-1") appendArgs(argv, parameters[i]);
}

static std::vector<std::string> splitBy(const std::string& s, char delimiter){
	std::vector<std::string> parts;
	std::string cur;
	for(char c : s){
		if(c == delimiter){
			parts.push_back(cur);
			cur.clear();
		}
		else cur += c;
	}
	parts.push_back(cur);
	return parts;
}

static std::vector<std::string> splitCommandLine(const std::string& line){
	std::vector<std::string> tokens;
	std::string cur;
	bool quoted = false, any = false;
	for(size_t i = 0; i < line.size(); ++i){
		char c = line[i];
		if(c == '\\' && i + 1 < line.size()){
			cur += line[++i];
			any = true;
		}
		else if(c == '"'){
			quoted = !quoted;
			any = true;
		}
		else if(!quoted && (c == ' ' || c == '\t')){
			if(any) tokens.push_back(cur);
			cur.clear();
			any = false;
		}
		else{
			cur += c;
			any = true;
		}
	}
	if(any) tokens.push_back(cur);
	return tokens;
}

static std::string normalPath(const std::string& path){
	return std::filesystem::path(path).lexically_normal().string();
}

static bool isElfOrArchive(const std::string& path){
	char magic[8] = {0};
	std::ifstream file(path, std::ios::binary);
	if(!file.is_open()) return false;
	file.read(magic, 8);
	if(file.gcount() >= 4 && memcmp(magic, "\x7f" "ELF", 4) == 0) return true;
	return file.gcount() == 8 && (memcmp(magic, "!<arch>\n", 8) == 0 || memcmp(magic, "!<thin>\n", 8) == 0);
}

static std::string findLibrary(const std::string& name, const std::vector<std::string>& dirs){
	for(const std::string& dir : dirs){
		if(!name.empty() && name[0] == ':'){
			if(exists(dir + "/" + name.substr(1))) return normalPath(dir + "/" + name.substr(1));
			continue;
		}
		if(exists(dir + "/lib" + name + ".so")) return normalPath(dir + "/lib" + name + ".so");
		if(exists(dir + "/lib" + name + ".a")) return normalPath(dir + "/lib" + name + ".a");
	}
	return "";
}

static void addInput(const std::string& path, const std::vector<std::string>& dirs,
	std::vector<std::string>& inputs, int depth)
{
	if(path.empty() || depth > 8 || std::find(inputs.begin(), inputs.end(), path) != inputs.end()) return;
	if(!exists(path) || std::filesystem::is_directory(path)) return;
	if(isElfOrArchive(path)){
		inputs.push_back(path);
		return;
	}
	if(getFileSize(path) > (1 << 20)) return;
	std::ifstream in(path);
	std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	size_t comment;
	while((comment = text.find("/*")) != std::string::npos){
		size_t end = text.find("*/", comment + 2);
		text.erase(comment, (end == std::string::npos) ? std::string::npos : end + 2 - comment);
	}
	for(char& c : text)
		if(c == '(' || c == ')' || c == ',' || c == '\n' || c == '\t') c = ' ';
	for(const std::string& token : splitBy(text, ' ')){
		if(token.empty()) continue;
		std::string t = (token[0] == '=') ? token.substr(1) : token;
		if(t[0] == '/') addInput(normalPath(t), dirs, inputs, depth + 1);
		else if(t.size() > 2 && t.compare(0, 2, "-l") == 0) addInput(findLibrary(t.substr(2), dirs), dirs, inputs, depth + 1);
		else if(t.find(".so") != std::string::npos || getExt(t) == "a" || getExt(t) == "o"){
			std::vector<std::string> searchDirs = {getFolder(path)};
			searchDirs.insert(searchDirs.end(), dirs.begin(), dirs.end());
			for(const std::string& dir : searchDirs)
				if(exists(dir + "/" + t)){
					addInput(normalPath(dir + "/" + t), dirs, inputs, depth + 1);
					break;
				}
		}
	}
}

void implicitLinkInputs(const std::string& compiler, const std::vector<std::string>& userArgs,
	bool shared, const std::string& probe, std::vector<std::string>& inputs, std::vector<std::string>& searchDirs)
{
	inputs.clear();
	searchDirs.clear();
	std::vector<std::string> argv = {compiler, "-###"};
	if(shared) argv.push_back("-shared");
	argv.insert(argv.end(), userArgs.begin(), userArgs.end());
	argv.push_back(probe);
	argv.push_back("-o");
	argv.push_back("/dev/null");
	std::string output;
	runProcessCapture(argv, output);

	std::vector<std::string> tokens;
	for(const std::string& line : splitBy(output, '\n')){
		if(line.compare(0, 20, "COLLECT_GCC_OPTIONS=") == 0) continue;
		std::vector<std::string> t = splitCommandLine(line);
		if(std::find(t.begin(), t.end(), probe) != t.end()) tokens = t;
	}
	if(tokens.empty()) return;

	static const std::set<std::string> withArgument = {"-plugin", "-o", "-m", "-z", "-soname", "-rpath",
		"-rpath-link", "-T", "-e", "-u", "-y", "-Y", "-a", "-A", "-b", "-f", "-F", "-G", "-h", "-R",
		"--sysroot", "-plugin-opt", "--dependency-file", "-dynamic-linker"};
	std::vector<std::string> dirs, names, files;
	for(size_t i = 1; i < tokens.size(); ++i){
		const std::string& t = tokens[i];
		if(t == "-dynamic-linker" && i + 1 < tokens.size()) files.push_back(tokens[i + 1]);
		if(withArgument.count(t)){
			++i;
			continue;
		}
		if(t == "-L" && i + 1 < tokens.size()) dirs.push_back(normalPath(tokens[++i]));
		else if(t.compare(0, 2, "-L") == 0 && t.size() > 2) dirs.push_back(normalPath(t.substr(2)));
		else if(t == "-l" && i + 1 < tokens.size()) names.push_back(tokens[++i]);
		else if(t.compare(0, 2, "-l") == 0 && t.size() > 2) names.push_back(t.substr(2));
		else if(!t.empty() && t[0] != '-' && t != probe && t[0] == '/') files.push_back(normalPath(t));
	}
	for(const std::string& dir : dirs){
		std::error_code ec;
		std::string canonical = std::filesystem::weakly_canonical(dir, ec).string();
		if(!ec && std::find(searchDirs.begin(), searchDirs.end(), canonical) == searchDirs.end())
			searchDirs.push_back(canonical);
	}
	for(const std::string& file : files) addInput(file, dirs, inputs, 0);
	for(const std::string& name : names) addInput(findLibrary(name, dirs), dirs, inputs, 0);
}

void standardLibDirs(const std::string& compiler, std::vector<std::string>& dirs){
	dirs.clear();
	std::string output;
	runProcessCapture({compiler, "-print-search-dirs"}, output);
	std::vector<std::string> raw = {"/lib", "/usr/lib", "/lib64", "/usr/lib64"};
	for(const std::string& line : splitBy(output, '\n')){
		if(line.compare(0, 11, "libraries: ") != 0) continue;
		std::string list = line.substr(11);
		if(!list.empty() && list[0] == '=') list.erase(0, 1);
		for(const std::string& dir : splitBy(list, ':')) if(!dir.empty()) raw.push_back(dir);
	}
	for(const std::string& dir : raw){
		std::error_code ec;
		std::string canonical = std::filesystem::weakly_canonical(dir, ec).string();
		if(ec) canonical = normalPath(dir);
		while(canonical.size() > 1 && canonical.back() == '/') canonical.pop_back();
		if(std::find(dirs.begin(), dirs.end(), canonical) == dirs.end()) dirs.push_back(canonical);
	}
}

bool isLinkerDefinedSymbol(const std::string& name){
	static const std::set<std::string> names = {"_GLOBAL_OFFSET_TABLE_", "_DYNAMIC", "__ehdr_start",
		"__executable_start", "_end", "end", "_edata", "edata", "__bss_start", "_etext", "etext", "__etext",
		"__init_array_start", "__init_array_end", "__fini_array_start", "__fini_array_end",
		"__preinit_array_start", "__preinit_array_end", "__dso_handle", "__TMC_END__",
		"_PROCEDURE_LINKAGE_TABLE_", "__GNU_EH_FRAME_HDR", "__rela_iplt_start", "__rela_iplt_end"};
	if(names.count(name)) return true;
	return name.compare(0, 8, "__start_") == 0 || name.compare(0, 7, "__stop_") == 0;
}

bool isStandardLibDir(const std::string& dir, const std::vector<std::string>& parameters){
	static std::string compiler;
	static std::vector<std::string> dirs;
	std::string current = linkCompiler(parameters);
	if(compiler != current){
		compiler = current;
		standardLibDirs(compiler, dirs);
	}
	std::error_code ec;
	std::string canonical = std::filesystem::weakly_canonical(dir, ec).string();
	return !ec && std::find(dirs.begin(), dirs.end(), canonical) != dirs.end();
}
