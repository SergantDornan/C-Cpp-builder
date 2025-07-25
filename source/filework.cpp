#include <filework.h>
std::string getFullPath(std::string cd, std::string relpath){
    if(relpath[relpath.size()-1] == '.') relpath += '/';
    if(cd[(cd.size()-1)] == '/') cd = std::string(cd.begin(), cd.end()-1);
    if(relpath.size() >= 2 && relpath[0] == '.' && relpath[1] == '/')
        return (cd + std::string(relpath.begin() + 1, relpath.end()));
    bool b = false;
    while(relpath.size() >= 3 && relpath[0] == '.' && relpath[1] == '.' && relpath[2] == '/'){
        b = true;
        if(getFolder(cd) == ""){
            std::cout << "======================== ERROR ========================" << std::endl;
            std::cout << "filework.cpp: cannot convert relative path to full path" << std::endl;
            std::cout << "Current directory: " << cd << std::endl;
            std::cout << "relative path: " << relpath << std::endl;
            std::cout << std::endl;
            return "-1";
        }
        cd = getFolder(cd);
        relpath = std::string(relpath.begin() + 3, relpath.end());
    }
    if(b) return (cd + "/" + relpath);
    else return relpath;
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
        if(dirs[i].find(name) != std::string::npos && 
            (dirs[i].find(name) == (dirs[i].size() - name.size())) &&
            find(result, dirs[i]) == -1) result.push_back(dirs[i]);
        if(std::filesystem::is_directory(dirs[i]) && 
            find(fUnInc, dirs[i]) == -1) findFile(result, name, dirs[i], {}, fUnInc);
    }
    // Проход по всем AddInc
    for(int i = 0; i < AddInc.size(); ++i){
        auto addDirs = getDirs(AddInc[i]);
        for(int j = 1; j < addDirs.size(); ++j){
            if(addDirs[j].find(name) != std::string::npos &&
                (addDirs[j].find(name) == (addDirs[j].size() - name.size())) &&
                find(result, addDirs[j]) == -1) result.push_back(addDirs[j]);
            if(std::filesystem::is_directory(addDirs[j]) &&
                find(fUnInc, addDirs[j]) == -1) findFile(result, name, addDirs[j], {}, fUnInc);
        }
    }
}

long getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ifstream::ate | std::ifstream::binary);
    return file.tellg();
}

std::string cwd(){
    char cwd0[PATH_MAX];
    if (getcwd(cwd0, sizeof(cwd0)) != nullptr) {
        return cwd0;
    } 
    else {
        std::cout << "==================== ERROR ====================" << std::endl;
        std::cout << "====== some error in filework.cpp : std::string cwd() ======";
        std::cout << std::endl;
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
	if(!std::filesystem::is_directory(path)){
		std::cout << "======================= ERROR =======================" << std::endl;
		std::cout << "=============== filework.cpp: getDirs ===============" << std::endl;
		std::cout << "path leads to a file, not directory" << std::endl;
		std::cout << path << std::endl;
		std::cout << "=====================================================" << std::endl;
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
        std::cout << "======================== ERROR ========================" << std::endl;
        std::cout << "==== some error in filework.cpp: getHomedir() ====" << std::endl;
        std::cout << std::endl;      
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
        std::cout << "============================ ERROR ============================" << std::endl;
        std::cout << "======= filework.cpp: appendToFile() ======" << std::endl;
        std::cout << "Cannot open file: " << path << std::endl;
        std::cout << std::endl;
    }
}
std::string formatTime(time_t timestamp) {
    std::tm *timeInfo = localtime(&timestamp);
    if (timeInfo == nullptr) {
        std::cout << "===================== ERROR =====================" << std::endl;
        std::cout << "====== filework.cpp: formatTime() - some error idn ======" << std::endl;
        std::cout << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << std::put_time(timeInfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
std::string getChangeTime(const std::string& path){
    const char *filename = path.c_str();
    struct stat fileInfo;
    if (stat(filename, &fileInfo) != 0) {
        std::cout << "===================== ERROR =====================" << std::endl;
        std::cout << "====== filework.cpp: getChangeTime() ======" << std::endl;
        std::cout << "Error getting file information: " << filename << std::endl;
        std::cout << std::endl;
        return "";
    }
    time_t modificationTime = fileInfo.st_mtime;
    return formatTime(modificationTime);
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
    return "";
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
        std::cout << "================= ERROR =================" << std::endl;
        std::cout << "installer.cpp: rewriteLine: cannot open file: " << std::endl;
        std::cout << path << std::endl;
        std::cout << "=========================================" << std::endl;
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
    std::string command = "which " + programName + " > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}