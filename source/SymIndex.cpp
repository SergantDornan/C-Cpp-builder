#include "SymIndex.h"
#include "Linker.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>

uint64_t symbolHash(const std::string& name){
	uint64_t h = 1469598103934665603ULL;
	for(unsigned char c : name){
		h ^= c;
		h *= 1099511628211ULL;
	}
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdULL;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53ULL;
	h ^= h >> 33;
	return h;
}

bool statLib(const std::string& path, IndexLib& lib){
	struct stat st;
	if(stat(path.c_str(), &st) != 0) return false;
	lib.path = path;
	lib.dev = st.st_dev;
	lib.inode = st.st_ino;
	lib.size = st.st_size;
	lib.mtimeSec = st.st_mtim.tv_sec;
	lib.mtimeNsec = st.st_mtim.tv_nsec;
	return true;
}

static bool sameStamp(const IndexLib& a, const IndexLib& b){
	return a.dev == b.dev && a.inode == b.inode && a.size == b.size &&
		a.mtimeSec == b.mtimeSec && a.mtimeNsec == b.mtimeNsec;
}

template<typename T>
static void putValue(std::string& out, const T& value){
	out.append((const char*)&value, sizeof(T));
}

template<typename T>
static bool getValue(const std::string& in, size_t& pos, T& value){
	if(in.size() - pos < sizeof(T)) return false;
	memcpy(&value, in.data() + pos, sizeof(T));
	pos += sizeof(T);
	return true;
}

bool readSymIndex(SymIndex& index, const std::string& path){
	index.libs.clear();
	index.providers.clear();
	index.records.clear();
	long size = getFileSize(path);
	if(size <= 0) return false;
	std::string data(size, '\0');
	std::ifstream in(path, std::ios::binary);
	if(!in.is_open()) return false;
	in.read(&data[0], size);
	if(in.gcount() != size) return false;
	size_t pos = 0;
	uint64_t count = 0;
	bool ok = getValue(data, pos, count) && count <= data.size();
	for(uint64_t i = 0; ok && i < count; ++i){
		IndexLib lib;
		uint64_t len = 0;
		ok = getValue(data, pos, len) && len <= data.size() - pos;
		if(!ok) break;
		lib.path = data.substr(pos, len);
		pos += len;
		ok = getValue(data, pos, lib.dev) && getValue(data, pos, lib.inode) && getValue(data, pos, lib.size) &&
			getValue(data, pos, lib.mtimeSec) && getValue(data, pos, lib.mtimeNsec);
		index.libs.push_back(lib);
	}
	ok = ok && getValue(data, pos, count) && count <= (data.size() - pos) / sizeof(IndexProvider);
	for(uint64_t i = 0; ok && i < count; ++i){
		IndexProvider p;
		ok = getValue(data, pos, p.lib) && getValue(data, pos, p.member) && p.lib < index.libs.size();
		index.providers.push_back(p);
	}
	ok = ok && getValue(data, pos, count) && count == (data.size() - pos) / sizeof(IndexRecord) &&
		(data.size() - pos) % sizeof(IndexRecord) == 0;
	if(ok){
		index.records.resize(count);
		if(count > 0) memcpy(index.records.data(), data.data() + pos, count * sizeof(IndexRecord));
		for(size_t i = 0; ok && i < index.records.size(); ++i)
			ok = index.records[i].provider < index.providers.size() &&
				(i == 0 || index.records[i - 1].hash <= index.records[i].hash);
	}
	if(!ok){
		index.libs.clear();
		index.providers.clear();
		index.records.clear();
	}
	return ok;
}

bool writeSymIndex(const SymIndex& index, const std::string& path){
	std::string out;
	putValue(out, (uint64_t)index.libs.size());
	for(const IndexLib& lib : index.libs){
		putValue(out, (uint64_t)lib.path.size());
		out += lib.path;
		putValue(out, lib.dev);
		putValue(out, lib.inode);
		putValue(out, lib.size);
		putValue(out, lib.mtimeSec);
		putValue(out, lib.mtimeNsec);
	}
	putValue(out, (uint64_t)index.providers.size());
	for(const IndexProvider& p : index.providers){
		putValue(out, p.lib);
		putValue(out, p.member);
	}
	putValue(out, (uint64_t)index.records.size());
	out.append((const char*)index.records.data(), index.records.size() * sizeof(IndexRecord));
	std::string tmp = path + ".tmp";
	std::ofstream file(tmp, std::ios::binary);
	file.write(out.data(), out.size());
	file.close();
	if(!file){
		removeFile(tmp);
		return false;
	}
	return std::rename(tmp.c_str(), path.c_str()) == 0;
}

void swap(IndexRecord& a, IndexRecord& b){
	IndexRecord t = a;
	a = b;
	b = t;
}

static bool recordLess(const IndexRecord& a, const IndexRecord& b){
	return a.hash < b.hash || (a.hash == b.hash && a.provider < b.provider);
}

bool updateSymIndex(SymIndex& index, const std::string& wd, const std::vector<std::string>& libs){
	const std::string path = wd + "/" + SYM_INDEX_FILE;
	readSymIndex(index, path);

	std::unordered_map<std::string, uint32_t> oldByPath;
	std::vector<char> keep(index.libs.size(), 0);
	bool changed = false;
	for(uint32_t i = 0; i < index.libs.size(); ++i){
		IndexLib now;
		keep[i] = statLib(index.libs[i].path, now) && sameStamp(now, index.libs[i]) &&
			oldByPath.find(index.libs[i].path) == oldByPath.end();
		if(keep[i]) oldByPath[index.libs[i].path] = i;
		else changed = true;
	}
	std::vector<std::string> toAdd;
	for(const std::string& lib : libs)
		if(oldByPath.find(lib) == oldByPath.end() && std::find(toAdd.begin(), toAdd.end(), lib) == toAdd.end())
			toAdd.push_back(lib);
	if(!changed && toAdd.empty()) return true;

	SymIndex fresh;
	std::vector<int64_t> libMap(index.libs.size(), -1), providerMap(index.providers.size(), -1);
	for(uint32_t i = 0; i < index.libs.size(); ++i){
		if(!keep[i]) continue;
		libMap[i] = fresh.libs.size();
		fresh.libs.push_back(index.libs[i]);
	}
	for(size_t i = 0; i < index.providers.size(); ++i){
		if(libMap[index.providers[i].lib] < 0) continue;
		providerMap[i] = fresh.providers.size();
		fresh.providers.push_back({(uint32_t)libMap[index.providers[i].lib], index.providers[i].member});
	}
	for(const IndexRecord& r : index.records)
		if(providerMap[r.provider] >= 0)
			fresh.records.push_back({r.hash, (uint32_t)providerMap[r.provider], 0});

	for(const std::string& lib : toAdd){
		IndexLib stamp;
		if(!statLib(lib, stamp)) continue;
		binFile file = {lib};
		LibAnal(wd, file);
		uint32_t libId = fresh.libs.size();
		fresh.libs.push_back(stamp);
		uint32_t providerId = fresh.providers.size();
		fresh.providers.push_back({libId, -1});
		for(const std::string& def : file.defSyms)
			fresh.records.push_back({symbolHash(def), providerId, 0});
		for(size_t m = 0; m < file.members.size(); ++m){
			providerId = fresh.providers.size();
			fresh.providers.push_back({libId, (int32_t)m});
			for(const std::string& def : file.members[m].defSyms)
				fresh.records.push_back({symbolHash(def), providerId, 0});
		}
	}
	std::sort(fresh.records.begin(), fresh.records.end(), recordLess);
	fresh.records.erase(std::unique(fresh.records.begin(), fresh.records.end(),
		[](const IndexRecord& a, const IndexRecord& b){ return a.hash == b.hash && a.provider == b.provider; }),
		fresh.records.end());
	index = std::move(fresh);
	writeSymIndex(index, path);
	return true;
}

void lookupSymbol(const SymIndex& index, const std::string& name, std::vector<IndexProvider>& out){
	out.clear();
	uint64_t h = symbolHash(name);
	auto it = std::lower_bound(index.records.begin(), index.records.end(), h,
		[](const IndexRecord& r, uint64_t value){ return r.hash < value; });
	for(; it != index.records.end() && it->hash == h; ++it)
		out.push_back(index.providers[it->provider]);
}
