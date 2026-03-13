#ifndef MAPPING
#define MAPPING

#include "algs.h"
#include "filework.h"


typedef struct {
	std::string name;
	int parent;
	std::vector<int> children;
	bool file;
} FileNode;

std::vector<int> getMap(const std::vector<std::string>&,
	const std::vector<std::string>&,std::vector<FileNode>&);

void showTree(int,const FileNode& ,const std::vector<FileNode>&);


// pathDecoder возвращает список возможных файлов, подходящих под путь
// первый элемент - путь к файлу, второй - папка из которой его можно включить с помощью -I
std::pair<std::string,std::string> pathDecoder(
	const std::string&,
	const std::vector<FileNode>&,
	const std::vector<int>&); 

#endif