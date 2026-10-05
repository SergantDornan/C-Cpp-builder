#include "Linker.h"
#include "Anal.h"
#include "ConfigIndex.h"
#include "SymIndex.h"
#include "Toolchain.h"
#include <algorithm>
#include <cxxabi.h>
#include <map>
#include <set>
#include <unordered_map>

typedef struct {
	binFile file;
	bool loaded;
	bool implicit;
	bool whole;
	size_t searchOrder;
	int rank;
	size_t exports;
	uint32_t arch;
	std::string group;
	std::unordered_map<std::string, int> firstMember;
} RunLib;

typedef struct {
	std::string owner;
	bool strong;
} Definition;

typedef struct {
	std::unordered_map<std::string, Definition> defs;
	std::set<std::string> undefined;
	std::set<std::pair<int, int>> pulledMembers;
	std::vector<int> chosen;
	std::vector<char> objectPulled;
	std::vector<int> objectOrder;
	std::string conflict;
} LinkState;

typedef struct {
	bool implicit;
	std::vector<int> libs;
	std::vector<int> members;
} Candidates;

typedef struct {
	std::string wd;
	uint32_t arch;
	bool idgaf;
	bool useLibs;
	const std::vector<binFile>* objects;
	std::unordered_map<std::string, std::vector<std::pair<int, bool>>> objectDefs;
	std::vector<RunLib> libs;
	std::unordered_map<std::string, int> libByPath;
	std::vector<std::string> roots;
	std::vector<std::string> searchDirs;
	SymIndex index;
	std::unordered_map<std::string, Candidates> candidates;
	std::set<std::pair<std::string, int>> banned;
	std::map<std::string, std::vector<std::string>> banReasons;
} Selector;

static std::string readableSymbol(const std::string& name){
	int status = 0;
	char* demangled = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
	if(status != 0 || demangled == nullptr) return name;
	std::string result = std::string(demangled) + " (" + name + ")";
	free(demangled);
	return result;
}

void printUnresolved(const std::vector<std::string>& unresolved){
	if(unresolved.empty()) return;
	std::cerr << "================== belder ==================" << std::endl;
	std::cerr << "belder did not find these symbols in any library:" << std::endl;
	for(const std::string& name : unresolved) std::cerr << '\t' << readableSymbol(name) << std::endl;
	std::cerr << "Add a folder with the library using -I or link it with --link-force / -l" << std::endl;
	std::cerr << std::endl;
}

static std::string libGroup(const std::string& path){
	std::string name = getName(path);
	size_t so = name.find(".so");
	size_t a = name.rfind(".a");
	size_t cut = (so != std::string::npos) ? so : ((a != std::string::npos && a + 2 == name.size()) ? a : name.size());
	return getFolder(path) + "/" + name.substr(0, cut);
}

static int libRank(const std::string& path, const std::vector<std::string>& roots){
	for(size_t i = 0; i < roots.size(); ++i)
		if(path.size() > roots[i].size() && path.compare(0, roots[i].size(), roots[i]) == 0 &&
			path[roots[i].size()] == '/') return i;
	return roots.size();
}

static RunLib& loadedLib(Selector& sel, int id){
	RunLib& lib = sel.libs[id];
	if(lib.loaded) return lib;
	lib.loaded = true;
	LibAnal(sel.wd, lib.file);
	lib.whole = lib.file.members.empty();
	lib.arch = lib.file.arch;
	for(size_t m = 0; m < lib.file.members.size() && lib.arch == 0; ++m) lib.arch = lib.file.members[m].arch;
	lib.exports = lib.file.defSyms.size();
	for(const binFile& member : lib.file.members) lib.exports += member.defSyms.size();
	for(const std::string& def : lib.file.defSyms) lib.firstMember.emplace(def, -1);
	for(size_t m = 0; m < lib.file.members.size(); ++m)
		for(const std::string& def : lib.file.members[m].defSyms)
			lib.firstMember.emplace(def, m);
	return lib;
}

static const Candidates& getCandidates(Selector& sel, const std::string& name){
	auto found = sel.candidates.find(name);
	if(found != sel.candidates.end()) return found->second;
	Candidates c{};
	if(isLinkerDefinedSymbol(name)) c.implicit = true;
	else{
		std::vector<IndexProvider> providers;
		lookupSymbol(sel.index, name, providers);
		for(const IndexProvider& p : providers){
			auto it = sel.libByPath.find(sel.index.libs[p.lib].path);
			if(it == sel.libByPath.end()) continue;
			RunLib& lib = loadedLib(sel, it->second);
			// библиотека другой архитектуры, чем объектник входа, не может быть кандидатом
			if(sel.arch != 0 && lib.arch != 0 && lib.arch != sel.arch) continue;
			auto member = lib.firstMember.find(name);
			if(member == lib.firstMember.end()) continue;
			if(lib.implicit){
				c.implicit = true;
				continue;
			}
			if(std::find(c.libs.begin(), c.libs.end(), it->second) != c.libs.end()) continue;
			c.libs.push_back(it->second);
			c.members.push_back(member->second);
		}
		for(size_t i = 0; i < c.libs.size();){
			bool shadowed = false;
			if(!sel.libs[c.libs[i]].whole)
				for(size_t j = 0; j < c.libs.size() && !shadowed; ++j)
					shadowed = sel.libs[c.libs[j]].whole && sel.libs[c.libs[j]].group == sel.libs[c.libs[i]].group;
			if(shadowed){
				c.libs.erase(c.libs.begin() + i);
				c.members.erase(c.members.begin() + i);
			}
			else ++i;
		}
	}
	return sel.candidates.emplace(name, c).first->second;
}

static std::string conflictMessage(const std::string& name, const std::string& first, const std::string& second){
	return "multiple definition of symbol: \n" + readableSymbol(name) + "\n\n" +
		"First definition in file: " + first + "\n" +
		"Second definition in file: " + second + "\n";
}

static bool addDefinitions(Selector& sel, LinkState& st, const binFile& file, const std::string& owner){
	for(size_t i = 0; i < file.defSyms.size(); ++i){
		const std::string& name = file.defSyms[i];
		bool strong = (i < file.defStrong.size() && file.defStrong[i] == 1);
		auto it = st.defs.find(name);
		if(it == st.defs.end()) st.defs.emplace(name, Definition{owner, strong});
		else if(strong && it->second.strong){
			if(!sel.idgaf){
				st.conflict = conflictMessage(name, it->second.owner, owner);
				return false;
			}
		}
		else if(strong) it->second = Definition{owner, true};
		st.undefined.erase(name);
	}
	for(size_t i = 0; i < file.callSyms.size(); ++i)
		if((i >= file.callStrong.size() || file.callStrong[i] != 0) && st.defs.find(file.callSyms[i]) == st.defs.end())
			st.undefined.insert(file.callSyms[i]);
	return true;
}

static bool pullObject(Selector& sel, LinkState& st, int object){
	if(st.objectPulled[object]) return true;
	st.objectPulled[object] = 1;
	st.objectOrder.push_back(object);
	const binFile& file = (*sel.objects)[object];
	return addDefinitions(sel, st, file, getName(file.name));
}

static int pullMember(Selector& sel, LinkState& st, int libId, int member){
	if(!st.pulledMembers.insert({libId, member}).second) return 2;
	binFile& lib = sel.libs[libId].file;
	if(!lib.members[member].parsed){
		parseArchiveMember(lib, member);
		saveSymfile(lib, symfilePath(sel.wd, lib.name));
	}
	const binFile& m = lib.members[member];
	return addDefinitions(sel, st, m, getName(lib.name) + "(" + m.name + ")") ? 1 : 0;
}

static bool closeUndefined(Selector& sel, LinkState& st){
	bool progress = true;
	while(progress){
		progress = false;
		std::vector<std::string> pending(st.undefined.begin(), st.undefined.end());
		for(const std::string& name : pending){
			if(st.undefined.find(name) == st.undefined.end()) continue;
			auto objects = sel.objectDefs.find(name);
			if(objects != sel.objectDefs.end()){
				std::vector<int> weak, strong;
				for(const auto& o : objects->second){
					if(st.objectPulled[o.first]) continue;
					(o.second ? strong : weak).push_back(o.first);
				}
				if(strong.size() > 1 && !sel.idgaf){
					st.conflict = conflictMessage(name, getName((*sel.objects)[strong[0]].name),
						getName((*sel.objects)[strong[1]].name));
					return false;
				}
				if(!strong.empty() || !weak.empty()){
					if(!pullObject(sel, st, strong.empty() ? weak[0] : strong[0])) return false;
					progress = true;
					continue;
				}
			}
			if(!sel.useLibs) continue;
			const Candidates& c = getCandidates(sel, name);
			size_t bestPos = st.chosen.size();
			int best = -1;
			for(size_t k = 0; k < c.libs.size(); ++k){
				size_t pos = std::find(st.chosen.begin(), st.chosen.end(), c.libs[k]) - st.chosen.begin();
				if(pos < bestPos){
					bestPos = pos;
					best = k;
				}
			}
			if(best < 0){
				if(c.implicit) st.undefined.erase(name);
				continue;
			}
			if(c.members[best] < 0){
				st.undefined.erase(name);
				continue;
			}
			int pulled = pullMember(sel, st, c.libs[best], c.members[best]);
			if(pulled == 0) return false;
			if(pulled == 2) st.undefined.erase(name);
			else progress = true;
		}
	}
	return true;
}

static void printConflict(const std::string& message){
	std::cerr << "=================== ERROR ===================" << std::endl;
	std::cerr << message << std::endl;
	std::cerr << "You can choose not to link files forcibly by using the flag: --no-link-force [filename]" << std::endl;
	std::cerr << "Or you can run builder with --idgaf flag to ignore this error" << std::endl;
	std::cerr << std::endl;
}

static void addLib(Selector& sel, const std::string& path, bool implicit,
	std::map<std::pair<uint64_t, uint64_t>, int>& identities)
{
	IndexLib stamp;
	if(!statLib(path, stamp) || std::filesystem::is_directory(path)) return;
	auto identity = std::make_pair(stamp.dev, stamp.inode);
	auto it = identities.find(identity);
	if(it != identities.end()){
		if(implicit) sel.libs[it->second].implicit = true;
		sel.libByPath.emplace(path, it->second);
		return;
	}
	RunLib lib;
	lib.file = {path};
	lib.loaded = false;
	lib.implicit = implicit;
	lib.whole = true;
	lib.rank = libRank(path, sel.roots);
	std::error_code ec;
	std::string dir = std::filesystem::weakly_canonical(getFolder(path), ec).string();
	lib.searchOrder = ec ? sel.searchDirs.size() :
		std::find(sel.searchDirs.begin(), sel.searchDirs.end(), dir) - sel.searchDirs.begin();
	lib.exports = 0;
	lib.arch = 0;
	lib.group = libGroup(path);
	identities[identity] = sel.libs.size();
	sel.libByPath.emplace(path, sel.libs.size());
	sel.libs.push_back(std::move(lib));
}

static bool libBefore(Selector& sel, const std::map<int, int>& score, int a, int b){
	int sa = score.at(a), sb = score.at(b);
	// 1. больше закрывает нужных символов
	if(sa != sb) return sa > sb;
	const RunLib& la = loadedLib(sel, a);
	const RunLib& lb = loadedLib(sel, b);
	// 2. проект, потом папки -I по порядку
	if(la.rank != lb.rank) return la.rank < lb.rank;
	// 3. ближе в порядке поиска линковщика (-L из "компилятор -###", там нужный multilib идет первым)
	if(la.searchOrder != lb.searchOrder) return la.searchOrder < lb.searchOrder;
	// 4. меньше экспортов
	if(la.exports != lb.exports) return la.exports < lb.exports;
	// 5. .so раньше .a
	if(la.whole != lb.whole) return la.whole;
	// 6. по имени
	return la.file.name < lb.file.name;
}

int findLinks(std::vector<std::string>& toLink, const std::vector<binFile>& filesInfo,
	const std::vector<std::string>& forceLinkLibs, const std::vector<std::string>& allLibs,
	const std::vector<std::string>& parameters, const std::string& wd, const bool idgaf,
	const int linkType, std::vector<std::string>& unresolved)
{
	Selector sel;
	sel.wd = wd;
	sel.idgaf = idgaf;
	sel.useLibs = (linkType != 1);
	sel.objects = &filesInfo;
	sel.arch = 0;
	for(size_t i = 0; i < filesInfo.size(); ++i)
		if(filesInfo[i].name == toLink[0]) sel.arch = filesInfo[i].arch;
	for(size_t i = 0; i < filesInfo.size(); ++i)
		for(size_t j = 0; j < filesInfo[i].defSyms.size(); ++j)
			sel.objectDefs[filesInfo[i].defSyms[j]].push_back({(int)i,
				j < filesInfo[i].defStrong.size() && filesInfo[i].defStrong[j] == 1});

	if(sel.useLibs){
		sel.roots = {cd};
		if(parameters[CFG_ADD_INCLUDE] != "-1")
			for(const std::string& dir : split(parameters[CFG_ADD_INCLUDE])) sel.roots.push_back(dir);
		std::vector<std::string> userArgs, implicitInputs;
		linkUserArgs(parameters, userArgs);
		implicitLinkInputs(linkCompiler(parameters), userArgs, linkType == 2, toLink[0], implicitInputs, sel.searchDirs);
		std::map<std::pair<uint64_t, uint64_t>, int> identities;
		for(const std::string& path : implicitInputs) addLib(sel, path, true, identities);
		for(const std::string& path : forceLinkLibs) addLib(sel, path, false, identities);
		for(const std::string& path : allLibs) addLib(sel, path, false, identities);
		std::vector<std::string> linkerLibs, fUnLib;
		if(parameters[CFG_FORCE_UNLINK_LIBS] != "-1") fUnLib = split(parameters[CFG_FORCE_UNLINK_LIBS]);
		getLinkerLibs(linkerLibs, sel.searchDirs, userArgs, fUnLib);
		for(const std::string& path : linkerLibs) addLib(sel, path, false, identities);
		std::vector<std::string> paths;
		for(const RunLib& lib : sel.libs) paths.push_back(lib.file.name);
		updateSymIndex(sel.index, wd, paths);
	}

	LinkState st;
	st.objectPulled.assign(filesInfo.size(), 0);
	bool ok = true;
	for(size_t i = 0; i < toLink.size() && ok; ++i)
		for(size_t j = 0; j < filesInfo.size() && ok; ++j)
			if(filesInfo[j].name == toLink[i]) ok = pullObject(sel, st, j);
	for(const std::string& path : forceLinkLibs){
		auto it = sel.libByPath.find(path);
		if(it != sel.libByPath.end() &&
			std::find(st.chosen.begin(), st.chosen.end(), it->second) == st.chosen.end())
			st.chosen.push_back(it->second);
	}
	if(ok) ok = closeUndefined(sel, st);
	if(!ok){
		printConflict(st.conflict);
		return 1;
	}

	std::vector<std::string> conflicted;
	while(sel.useLibs){
		std::map<std::string, std::vector<int>> open;
		std::vector<std::string> pending(st.undefined.begin(), st.undefined.end());
		for(const std::string& name : pending){
			const Candidates& c = getCandidates(sel, name);
			if(c.implicit){
				st.undefined.erase(name);
				continue;
			}
			std::vector<int> libs;
			bool anyBanned = false;
			for(int lib : c.libs){
				if(sel.banned.count({name, lib})) anyBanned = true;
				else libs.push_back(lib);
			}
			if(libs.empty()){
				st.undefined.erase(name);
				(anyBanned ? conflicted : unresolved).push_back(name);
				continue;
			}
			open[name] = libs;
		}
		if(open.empty()) break;

		std::vector<int> order;
		for(const auto& entry : open)
			if(entry.second.size() == 1){
				order.push_back(entry.second[0]);
				break;
			}
		if(order.empty()){
			std::map<int, int> score;
			for(const auto& entry : open)
				for(int lib : entry.second) score[lib]++;
			for(const auto& s : score) order.push_back(s.first);
			std::sort(order.begin(), order.end(), [&](int a, int b){ return libBefore(sel, score, a, b); });
		}

		for(int lib : order){
			LinkState trial = st;
			trial.chosen.push_back(lib);
			if(closeUndefined(sel, trial)){
				st = std::move(trial);
				break;
			}
			for(const auto& entry : open)
				if(std::find(entry.second.begin(), entry.second.end(), lib) != entry.second.end()){
					sel.banned.insert({entry.first, lib});
					sel.banReasons[entry.first].push_back(trial.conflict);
				}
		}
	}

	if(!conflicted.empty() && !idgaf){
		for(const std::string& name : conflicted){
			std::string message = "All libraries that define symbol:\n" + readableSymbol(name) +
				"\nconflict with files that are already linked:\n";
			for(const std::string& reason : sel.banReasons[name]) message += "\n" + reason;
			printConflict(message);
		}
		return 1;
	}
	toLink.clear();
	for(int object : st.objectOrder) toLink.push_back(filesInfo[object].name);
	for(int lib : st.chosen) toLink.push_back(sel.libs[lib].file.name);
	return 0;
}
