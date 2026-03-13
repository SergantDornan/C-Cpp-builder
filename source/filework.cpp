#include <filework.h>
std::string getFullPath(const std::string& cd_, const std::string& relpath_)
{
    std::string cd = cd_;
    std::string relpath = relpath_;
    if(cd.size() == 0 && relpath.size() == 0){
        std::cerr << "======================== ERROR ========================" << std::endl;
        std::cerr << "filework.cpp: getFullPath, cd.size() = 0 or relpath.size() = 0" << std::endl;
        std::cerr << std::endl;
        return "-1";
    }
    bool absolute = (relpath[0] == '/');
    if(cd[(cd.size()-1)] == '/') cd.erase(cd.end()-1);
    auto s = split(relpath, "/");
    for(int i = 0; i < s.size(); ++i){
        if(s[i] == ".") {
            absolute = false;
            continue;
        }
        if(s[i] == ".."){
            absolute = false;
            if(getFolder(cd) == ""){
                std::cerr << "======================== ERROR ========================" << std::endl;
                std::cerr << "filework.cpp: cannot convert relative path to full path" << std::endl;
                std::cerr << "Current directory: " << cd_ << std::endl;
                std::cerr << "relative path: " << relpath_ << std::endl;
                std::cerr << std::endl;
                return "-1";
            }
            cd = getFolder(cd);
        }
        else cd += ("/" + s[i]);
    }
    if(absolute) return relpath_;
    else return cd;
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
std::string formatTime(time_t timestamp) {
    std::tm *timeInfo = localtime(&timestamp);
    if (timeInfo == nullptr) {
        std::cerr << "===================== ERROR =====================" << std::endl;
        std::cerr << "filework.cpp: formatTime() - some error idk" << std::endl;
        std::cerr << std::endl;
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
        // std::cerr << "===================== ERROR =====================" << std::endl;
        // std::cerr << "filework.cpp: getChangeTime()" << std::endl;
        // std::cerr << "Error getting file information: " << filename << std::endl;
        // std::cerr << std::endl;
        return "0";
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
        std::cerr << "installer.cpp: rewriteLine: cannot open file: " << std::endl;
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
    std::string command = "which " + programName + " > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}