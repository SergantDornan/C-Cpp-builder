#ifndef filework_h
#define filework_h
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <iomanip>
#include <sstream>
std::vector<std::string> getDirs(const std::string&); 
// Выдает список файлов и папок, аналогично ls, только путь полный относительно папки, 
// откуда вызывается функция

extern void clear(std::string&);
bool exists(const std::string&);
long getFileSize(const std::string&);
std::string cwd();
std::string getExt(const std::string&);
std::string getName(const std::string&);
std::string getFolder(const std::string&);
std::string getExt(const std::string&);
std::string getName(const std::string&);
std::string getFolder(const std::string&);
std::string getHomedir();
void appendToFile(const std::string&, const std::string&);
std::string formatTime(time_t timestamp);
std::string getChangeTime(const std::string&);
std::string getNameNoExt(const std::string&);
void findFile(std::vector<std::string>&,
	const std::string&,const std::string&,
	const std::vector<std::string>&,
	const std::vector<std::string>&);
std::string getFullPath(std::string, std::string);
void rewriteLine(const std::string&,const std::string&,const std::string&);
bool checkProgram(const std::string&);
#endif