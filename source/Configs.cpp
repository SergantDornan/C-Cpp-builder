#include "Configs.h"
#include "Flags.h"
#include "ConfigIndex.h"
#include <algorithm>

static constexpr const char* profileFolders[] = {
	HEADERS_DIR "/" DEPS_DIR,
	SOURCE_DIR "/" DEPS_DIR,
	SOURCE_DIR "/" OBJECTS_DIR,
	SYM_DIR
};

static std::vector<std::string> numberedDirs(const std::string& dir){
	std::vector<std::string> result;
	if(!std::filesystem::is_directory(dir)) return result;
	auto dirs = getDirs(dir);
	for(int i = 1; i < dirs.size(); ++i){
		std::string name = getName(dirs[i]);
		if(name.empty() || name.size() > 9 || name.find_first_not_of("0123456789") != std::string::npos)
			continue;
		if(std::filesystem::is_directory(dirs[i])) result.push_back(dirs[i]);
	}
	std::sort(result.begin(), result.end(), [](const std::string& a, const std::string& b){
		return std::stoi(getName(a)) < std::stoi(getName(b));
	});
	return result;
}

static std::string createNumberedDir(const std::string& dir){
	if(!exists(dir)) createDirectory(dir);
	int maxIndex = 0;
	auto dirs = numberedDirs(dir);
	for(int i = 0; i < dirs.size(); ++i)
		maxIndex = std::max(maxIndex, std::stoi(getName(dirs[i])));
	std::string result = dir + "/" + std::to_string(maxIndex + 1);
	createDirectory(result);
	return result;
}

static std::string lastPairDir(const std::string& projectDir){
	std::ifstream in(projectDir + "/" + LAST_PAIR_FILE);
	std::string name;
	std::getline(in, name);
	in.close();
	std::string dir = projectDir + "/" + PAIRS_DIR + "/" + name;
	if(!name.empty() && name.find('/') == std::string::npos && exists(dir + "/" + CONFIG_FILE))
		return dir;
	std::string newest;
	std::filesystem::file_time_type newestTime;
	auto pairs = numberedDirs(projectDir + "/" + PAIRS_DIR);
	for(int i = 0; i < pairs.size(); ++i){
		std::error_code ec;
		auto time = std::filesystem::last_write_time(pairs[i] + "/" + CONFIG_FILE, ec);
		if(ec) continue;
		if(newest.empty() || time > newestTime){
			newest = pairs[i];
			newestTime = time;
		}
	}
	return newest;
}

int selectPair(std::vector<std::string>& args, const std::string& cd,
	const std::string& projectDir, std::string& pairDir)
{
	std::vector<std::string> base = defaultConfig();
	std::string last = lastPairDir(projectDir);
	if(last != "") base = readConfig(last + "/" + CONFIG_FILE);
	std::vector<std::string> search = defaultConfig();
	std::vector<std::string> searchArgs = args;
	if(getAddDirs(searchArgs, cd, search) != 0) return 1;
	search[CFG_ENTRY] = base[CFG_ENTRY];
	if(findEntryFile(args, cd, search) != 0) return 1;
	base[CFG_ENTRY] = search[CFG_ENTRY];
	getNameAfterFlag(args, "-o", base[CFG_OUTPUT]);
	std::string output = getFullPath(cd, base[CFG_OUTPUT]);
	if(output == "-1") return 1;
	std::string outputExt = getExt(output);
	if(std::filesystem::is_directory(output) || isSourceFile(output) || outputExt == "h" || outputExt == "hpp"){
		std::cerr << "================== ERROR ==================" << std::endl;
		std::cerr << "Output file cannot be a directory, a source file or a header:" << std::endl;
		std::cerr << output << std::endl;
		std::cerr << std::endl;
		return 1;
	}

	std::string pairsDir = projectDir + "/" + PAIRS_DIR;
	pairDir = "";
	auto pairs = numberedDirs(pairsDir);
	for(int i = 0; i < pairs.size(); ++i){
		auto parameters = readConfig(pairs[i] + "/" + CONFIG_FILE);
		if(parameters[CFG_ENTRY] == base[CFG_ENTRY] && parameters[CFG_OUTPUT] == output){
			pairDir = pairs[i];
			break;
		}
	}
	if(pairDir == ""){
		pairDir = createNumberedDir(pairsDir);
		auto parameters = defaultConfig();
		parameters[CFG_ENTRY] = base[CFG_ENTRY];
		parameters[CFG_OUTPUT] = output;
		writeConfig(pairDir + "/" + CONFIG_FILE, parameters);
	}
	std::ofstream out(projectDir + "/" + LAST_PAIR_FILE);
	out << getName(pairDir) << std::endl;
	out.close();
	return 0;
}

static std::string joinTokens(const std::vector<std::string>& tokens){
	if(tokens.size() == 0) return "-1";
	std::string result = tokens[0];
	for(int i = 1; i < tokens.size(); ++i) result += (" " + tokens[i]);
	return result;
}

static std::string normalizedTokens(const std::string& field){
	if(field == "-1") return field;
	return joinTokens(split(field));
}

static std::string sortedTokens(std::vector<std::string> tokens){
	std::sort(tokens.begin(), tokens.end());
	return joinTokens(tokens);
}

static std::vector<std::string> fieldTokens(const std::string& field){
	if(field == "-1") return {};
	return split(field);
}

std::string profileKey(const std::vector<std::string>& parameters){
	std::vector<std::string> profileUnlink;
	auto forceUnlink = fieldTokens(parameters[CFG_FORCE_UNLINK]);
	for(int i = 0; i < forceUnlink.size(); ++i)
		if(!isSourceFile(forceUnlink[i])) profileUnlink.push_back(forceUnlink[i]);

	std::vector<std::string> key = {
		normalizedTokens(parameters[CFG_COMPILERS]),
		normalizedTokens(parameters[CFG_CXX_STANDARD]),
		normalizedTokens(parameters[CFG_C_STANDARD]),
		normalizedTokens(parameters[CFG_OPT]),
		normalizedTokens(parameters[CFG_DEBUG]),
		normalizedTokens(parameters[CFG_COMPILE_FLAGS]),
		normalizedTokens(parameters[CFG_GENERAL_FLAGS]),
		(getLibType(parameters[CFG_OUTPUT]) == "so") ? "pic" : "nopic",
		sortedTokens(fieldTokens(parameters[CFG_ADD_INCLUDE])),
		sortedTokens(fieldTokens(parameters[CFG_FORCE_UNLINK_DIRS])),
		sortedTokens(profileUnlink)
	};
	std::string result;
	for(int i = 0; i < key.size(); ++i) result += (key[i] + "\n");
	return result;
}

static bool isProfileNumber(const std::string& s){
	return !s.empty() && s.size() <= 9 && s.find_first_not_of("0123456789") == std::string::npos;
}

void removeUnusedProfiles(const std::string& projectDir){
	std::vector<std::string> used;
	auto pairs = numberedDirs(projectDir + "/" + PAIRS_DIR);
	for(int i = 0; i < pairs.size(); ++i)
		used.push_back(readConfig(pairs[i] + "/" + CONFIG_FILE)[CFG_PROFILE]);

	std::vector<std::string> unused;
	auto profiles = numberedDirs(projectDir + "/" + PROFILES_DIR);
	for(int i = 0; i < profiles.size(); ++i)
		if(find(used, getName(profiles[i])) == -1) unused.push_back(profiles[i]);
	removeDirectories(unused);
}

static void prepareProfileTree(const std::string& folder){
	bool broken = false;
	for(const char* sub : profileFolders)
		broken |= !std::filesystem::is_directory(folder + "/" + sub);
	if(!broken) return;
	std::vector<std::string> dirs_to_remove = {
		folder + "/" + HEADERS_DIR, folder + "/" + SOURCE_DIR, folder + "/" + SYM_DIR
	};
	removeDirectories(dirs_to_remove);
	for(const char* sub : profileFolders)
		std::filesystem::create_directories(folder + "/" + sub);
}

std::string selectProfile(const std::string& projectDir, const std::string& pairDir,
	std::vector<std::string>& parameters)
{
	std::string profilesDir = projectDir + "/" + PROFILES_DIR;
	std::string number = parameters[CFG_PROFILE];
	if(!isProfileNumber(number) || !std::filesystem::is_directory(profilesDir + "/" + number)){
		number = "";
		std::string key = profileKey(parameters);
		int maxIndex = 0;
		auto profiles = numberedDirs(profilesDir);
		for(int i = 0; i < profiles.size(); ++i)
			maxIndex = std::max(maxIndex, std::stoi(getName(profiles[i])));
		auto pairs = numberedDirs(projectDir + "/" + PAIRS_DIR);
		for(int i = 0; i < pairs.size(); ++i){
			if(pairs[i] == pairDir) continue;
			auto other = readConfig(pairs[i] + "/" + CONFIG_FILE);
			if(!isProfileNumber(other[CFG_PROFILE])) continue;
			maxIndex = std::max(maxIndex, std::stoi(other[CFG_PROFILE]));
			if(number.empty() && profileKey(other) == key &&
				std::filesystem::is_directory(profilesDir + "/" + other[CFG_PROFILE]))
				number = other[CFG_PROFILE];
		}
		if(number.empty()) number = std::to_string(maxIndex + 1);
		parameters[CFG_PROFILE] = number;
		writeConfig(pairDir + "/" + CONFIG_FILE, parameters);
	}
	std::string folder = profilesDir + "/" + number;
	prepareProfileTree(folder);
	return folder;
}

void printConfigs(const std::string& projectDir, const std::string& currentPair){
	auto pairs = numberedDirs(projectDir + "/" + PAIRS_DIR);
	if(pairs.size() < 2) return;
	std::cout << std::endl;
	std::cout << "Configs in this project:" << std::endl;
	for(int i = 0; i < pairs.size(); ++i){
		auto parameters = readConfig(pairs[i] + "/" + CONFIG_FILE);
		std::cout << ((pairs[i] == currentPair) ? "  * " : "    ");
		std::cout << parameters[CFG_ENTRY] << " -> " << parameters[CFG_OUTPUT] << std::endl;
	}
}
