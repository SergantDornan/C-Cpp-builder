#include <filework.h>
#include "Process.h"
#include <cctype>
#include <fcntl.h>
#include <sys/file.h>
std::string getFullPath(const std::string& cd_, const std::string& relpath_)
{
    std::string cd = cd_;
    std::string relpath = relpath_;
    // Дальше безусловно обращаемся к relpath[0] и cd[cd.size()-1],
    // поэтому пустым не должен быть ни один из аргументов.
    if(cd.size() == 0 || relpath.size() == 0){
        std::cerr << "======================== ERROR ========================" << std::endl;
        std::cerr << "filework.cpp: getFullPath, cd.size() = 0 or relpath.size() = 0" << std::endl;
        std::cerr << std::endl;
        return "-1";
    }
    if(relpath[0] == '/') cd = "";
    else if(cd[(cd.size()-1)] == '/') cd.erase(cd.end()-1);
    auto s = split(relpath, "/");
    for(int i = 0; i < s.size(); ++i){
        if(s[i] == "" || s[i] == ".") continue;
        if(s[i] == "..") cd = getFolder(cd);
        else cd += ("/" + s[i]);
    }
    if(cd.empty()) return "/";
    return cd;
}

void findFile(std::vector<std::string>& result,
    const std::string& name0, const std::string& dir,
    const std::vector<std::string>& AddInc,
    const std::vector<std::string>& fUnInc){
    
    auto find = [](const std::vector<std::string>& v, const std::string& s){
        for(int i = 0; i < v.size(); ++i)
            if(v[i] == s) return i;
        return -1;
    };
    std::string name = getFullPath(dir, name0);
    // Нашли по полному пути:
    if(exists(name)){
        std::string path;
        if(getFolder(name) == "") path = (dir + "/" + name);
        else path = name;
        if(find(result, path) == -1) result.push_back(path);
        return;
    }
    // Не нашли по полному пути => было только имя, ищем по нему
    // Проход по dir

    auto dirs = getDirs(dir);
    for(int i = 1; i < dirs.size(); ++i){
        if(dirs[i].find(name0) != std::string::npos && 
            (dirs[i].find(name0) == (dirs[i].size() - name0.size())) &&
            find(result, dirs[i]) == -1) result.push_back(dirs[i]);
        if(std::filesystem::is_directory(dirs[i]) && 
            find(fUnInc, dirs[i]) == -1) findFile(result, name0, dirs[i], {}, fUnInc);
    }
    // Проход по всем AddInc
    for(int i = 0; i < AddInc.size(); ++i){
        auto addDirs = getDirs(AddInc[i]);
        for(int j = 1; j < addDirs.size(); ++j){
            if(addDirs[j].find(name0) != std::string::npos &&
                (addDirs[j].find(name0) == (addDirs[j].size() - name0.size())) &&
                find(result, addDirs[j]) == -1) result.push_back(addDirs[j]);
            if(std::filesystem::is_directory(addDirs[j]) &&
                find(fUnInc, addDirs[j]) == -1) findFile(result, name0, addDirs[j], {}, fUnInc);
        }
    }
}

long getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ifstream::ate | std::ifstream::binary);
    if(!file.is_open())
        return -1;
    return file.tellg();
}

std::string cwd(){
    char cwd0[PATH_MAX];
    if (getcwd(cwd0, sizeof(cwd0)) != nullptr) {
        return cwd0;
    } 
    else {
        std::cerr << "==================== ERROR ====================" << std::endl;
        std::cerr << "====== some error in filework.cpp : std::string cwd() ======";
        std::cerr << std::endl;
        return "";
    }
}
bool exists(const std::string& path){
    return std::filesystem::exists(path);
}
extern void clear(std::string& path){
	std::ofstream out(path);
	if(!out.is_open()){
		std::string s = "filework.h : clear : Cannot open file " + path;
		std::cout << s << std::endl;
	}
	else{
		out << "";
	}
	out.close();
}
std::vector<std::string> getDirs(const std::string &path) {
	std::vector<std::string> dirs;
    if(!exists(path)){
        std::cerr << "======================= ERROR =======================" << std::endl;
        std::cerr << "filework.cpp: getDirs" << std::endl;
        std::cerr << "path does not exists" << std::endl;
        std::cerr << path << std::endl;
        std::cerr << std::endl;
        return dirs;
    }
	if(!std::filesystem::is_directory(path)){
		std::cerr << "======================= ERROR =======================" << std::endl;
		std::cerr << "filework.cpp: getDirs" << std::endl;
		std::cerr << "path leads to a file, not directory" << std::endl;
		std::cerr << path << std::endl;
        std::cerr << std::endl;
		return dirs;
	}
  std::string back = path;
  while (back.back() != '/')
    {
      back.pop_back();
    }
  back.pop_back();
  dirs = {back};
  for (const std::filesystem::directory_entry &dir : std::filesystem::directory_iterator(path))
    {
      dirs.push_back(dir.path().string());
    }
  return dirs;
}

std::string getHomedir(){
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        return homeDir;
    } else {
        std::cerr << "======================== ERROR ========================" << std::endl;
        std::cerr << "filework.cpp: getHomedir(): cannot get env var" << std::endl;
        std::cerr << std::endl;      
        return "";
    }
}

void appendToFile(const std::string& path, const std::string& s){
    std::ofstream out(path, std::ios::app);
    if(out.is_open()){
        out << s;
        out.close();
    }
    else{
        std::cerr << "============================ ERROR ============================" << std::endl;
        std::cerr << "filework.cpp: appendToFile()" << std::endl;
        std::cerr << "Cannot open file: " << path << std::endl;
        std::cerr << std::endl;
    }
}
std::string getChangeTime(const std::string& path){
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) != 0) return "0";
    return std::to_string(fileInfo.st_mtim.tv_sec) + "." + std::to_string(fileInfo.st_mtim.tv_nsec);
}
std::unique_ptr<FILE, int(*)(FILE*)> lockFile(const std::string& path){
    FILE* file = fopen(path.c_str(), "ae");
    if(file) flock(fileno(file), LOCK_EX);
    return std::unique_ptr<FILE, int(*)(FILE*)>(file, fclose);
}
bool fixFutureTime(const std::string& path){
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) != 0) return false;
    if (fileInfo.st_mtime <= time(nullptr)) return false;
    if (utimensat(AT_FDCWD, path.c_str(), nullptr, 0) != 0) {
        std::cerr << "belder: warning: file " << path << " has modification time in the future and cannot be reset" << std::endl;
        return false;
    }
    std::cerr << "belder: warning: file " << path << " had modification time in the future, reset to current time" << std::endl;
    return true;
}
std::string getExt(const std::string& file){
    int index = -1;
    for(int i = file.size()-1; i>=0; --i){
        if(file[i] == '.' && i != 0){
            index = i;
            break;
        }
    }
    if(index != -1)
        return std::string(file.begin() + index + 1,file.end());
    return "";
}
std::string getLibType(const std::string& path){
    std::string s = getName(path);
    if(s.size() < 4 || std::string(s.begin(), s.begin() + 3) != "lib") return "";
    std::vector<std::string> parts = split(s, "."); // "libfoo.so.1.2.3" -> [libfoo, so, 1, 2, 3]
    if(parts.size() < 2) return "";
    if(parts.back() == "a") return "a"; // статическую версионируют крайне редко
    // разделяемая: есть компонент "so", а всё после него - числовая версия
    for(size_t i = 1; i < parts.size(); ++i){
        if(parts[i] != "so") continue;
        bool versionOk = true;
        for(size_t j = i + 1; j < parts.size() && versionOk; ++j){
            if(parts[j].empty()){ versionOk = false; break; }
            for(char c : parts[j])
                if(!std::isdigit(static_cast<unsigned char>(c))){ versionOk = false; break; }
        }
        if(versionOk) return "so";
    }
    return "";
}
std::string getNameNoExt(const std::string& path){
    std::string long_name = path;
    for(int i = path.size() - 1; i >= 0; --i){
        if(path[i] == '/'){
            long_name = std::string(path.begin() + i + 1, path.end()); 
            break;
        }
    }
    int index = -1;
    for(int i = long_name.size()-1; i>=0; --i){
        if(long_name[i] == '.' && i != 0){
            index = i;
            break;
        }
    }
    if(index != -1)
        return std::string(long_name.begin(),long_name.begin() + index);
    else
        return long_name;
}
std::string getName(const std::string& path){
    for(int i = path.size() - 1; i >= 0; --i){
        if(path[i] == '/')
            return std::string(path.begin() + i + 1, path.end()); 
    }
    return path;
}
std::string getFolder(const std::string& path){
    for(int i = path.size() - 1; i >= 0; --i){
        if(path[i] == '/')
            return std::string(path.begin(), path.begin() + i);
    }
    return "";
}

void rewriteLine(const std::string& path, 
    const std::string& oldLine, const std::string& newLine)
{
    std::vector<std::string> lines;
    std::string line;
    std::ifstream input(path);
    if(!input.is_open()){
        std::cerr << "================= ERROR =================" << std::endl;
        std::cerr << "filework.cpp: rewriteLine: cannot open file: " << std::endl;
        std::cerr << path << std::endl;
        std::cerr << std::endl;
        return;
    }
    while(std::getline(input, line))
        lines.push_back(line);
    input.close();
    for(int i = 0; i < lines.size(); ++i){
        if(lines[i] == oldLine){
            lines[i] = newLine;
            break;
        }
    }
    std::ofstream out(path);
    for(int i = 0; i < lines.size(); ++i)
        out << lines[i] << std::endl;
    out.close();
}
bool checkProgram(const std::string& programName) {
    // Запускаем "which <programName>" без shell, вывод - в /dev/null.
    return runProcessQuiet({"which", programName}) == 0;
}

void removeDirectory(const std::string& path) {
    std::filesystem::remove_all(path);
}
void removeDirectories(const std::vector<std::string>& paths){
    if(paths.size() == 0) return;
    for(int i = 0; i < paths.size(); ++i)
        std::filesystem::remove_all(paths[i]);
}
void removeFile(const std::string& path) {
    std::filesystem::remove(path);
}
void removeFiles(const std::vector<std::string>& paths){
    if(paths.size() == 0) return;
    for(int i = 0; i < paths.size(); ++i)
        std::filesystem::remove(paths[i]);
}
void createDirectory(const std::string& path) {
    std::filesystem::create_directories(path);
}
void createDirectories(const std::vector<std::string>& paths){
    if(paths.size() == 0) return;
    for(int i = 0; i < paths.size(); ++i)
        std::filesystem::create_directories(paths[i]);
}
void createFile(const std::string& path) {
    std::ofstream file(path);
    file.close();
}