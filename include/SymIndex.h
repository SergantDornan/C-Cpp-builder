#ifndef BELDER_SYMINDEX_H
#define BELDER_SYMINDEX_H

#include <cstdint>
#include <string>
#include <vector>

#define SYM_INDEX_FILE "symindex"

typedef struct {
	std::string path;
	uint64_t dev;
	uint64_t inode;
	uint64_t size;
	int64_t mtimeSec;
	int64_t mtimeNsec;
} IndexLib;

typedef struct {
	uint32_t lib;
	int32_t member;
} IndexProvider;

typedef struct {
	uint64_t hash;
	uint32_t provider;
	uint32_t reserved;
} IndexRecord;

typedef struct {
	std::vector<IndexLib> libs;
	std::vector<IndexProvider> providers;
	std::vector<IndexRecord> records;
} SymIndex;

uint64_t symbolHash(const std::string&);
bool statLib(const std::string&, IndexLib&);
bool readSymIndex(SymIndex&, const std::string&);
bool writeSymIndex(const SymIndex&, const std::string&);
bool updateSymIndex(SymIndex&, const std::string&, const std::vector<std::string>&);
void lookupSymbol(const SymIndex&, const std::string&, std::vector<IndexProvider>&);

#endif
