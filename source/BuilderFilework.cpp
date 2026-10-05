#include "BuilderFilework.h"
// Следующая строка заполняется инсталлятором, не менять ее
const std::string root = getHomedir() + "/builder";
std::string cd = cwd();
const bool pocket = (root == "./builder");

std::string convertPathToName(const std::string& path, const char ch){
    std::string result;
    for(int i = (!path.empty() && path[0] == '/') ? 1 : 0; i < path.size(); ++i){
        if(path[i] == '/') result += ch;
        else if(path[i] == ch || path[i] == '%') result += std::string("%") + path[i];
        else result += path[i];
    }
    return result;
}

bool isSourceFile(const std::string& path){
    std::string ext = getExt(path);
    return (ext == "c" || ext == "cpp" || ext == "asm" || ext == "s" || ext == "S");
}

void getAllheaders(std::vector<std::string>& headers,const std::string& path,
 const std::vector<std::string>& forceUnlink, const std::vector<std::string>& fUnIncludeDirs){
    auto dirs = getDirs(path);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(forceUnlink, dirs[i]) != -1) continue;
        try {
            if(((pocket && (dirs[i] == cd + "/builder")) || getName(dirs[i]) == ".git") &&
                std::filesystem::is_directory(dirs[i])) continue;
        } catch (const std::filesystem::filesystem_error& e) {
            continue;
        }
        if((getExt(dirs[i]) == "h" || getExt(dirs[i]) == "hpp") &&
            find(headers, dirs[i]) == -1) headers.push_back(dirs[i]);
        try {
            if(std::filesystem::is_directory(dirs[i]) && find(fUnIncludeDirs, dirs[i]) == -1)
                getAllheaders(headers, dirs[i],forceUnlink,fUnIncludeDirs);
        } catch (const std::filesystem::filesystem_error& e) {
            continue;
        }
    }
    merge_sort(headers);
}
void getAllsource(std::vector<std::string>& source, const std::string& path,
    const std::vector<std::string>& forceUnlink, const std::vector<std::string>& fUnIncludeDirs)
{
    auto dirs = getDirs(path);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(forceUnlink, dirs[i]) != -1) continue;
        try {
            if(((pocket && (dirs[i] == cd + "/builder")) || getName(dirs[i]) == ".git") &&
                std::filesystem::is_directory(dirs[i])) continue;
        } catch (const std::filesystem::filesystem_error& e) {
            continue;
        }
        if(isSourceFile(dirs[i]) && find(source, dirs[i]) == -1) source.push_back(dirs[i]);
        try {
            if(std::filesystem::is_directory(dirs[i]) && find(fUnIncludeDirs, dirs[i]) == -1)
                getAllsource(source, dirs[i],forceUnlink,fUnIncludeDirs);
        } catch (const std::filesystem::filesystem_error& e) {
            continue;
        }
    }
    merge_sort(source);
}
void getAllLibs(std::vector<std::string>& libs, const std::string& path,
    const std::vector<std::string>& fUnlib, const std::vector<std::string>& fUnIncludeDirs, bool recursive)
{   
    auto dirs = getDirs(path);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(fUnlib, dirs[i]) != -1) continue;
        
        try {
            if(((pocket && (dirs[i] == cd + "/builder")) || getName(dirs[i]) == ".git") &&
                std::filesystem::is_directory(dirs[i])) continue;
            if(recursive && std::filesystem::is_directory(dirs[i]) && find(fUnIncludeDirs, dirs[i]) == -1) 
                getAllLibs(libs, dirs[i], fUnlib,fUnIncludeDirs, recursive);
        } catch (const std::filesystem::filesystem_error& e) {
            continue;
        }
        
        if(getLibType(dirs[i]) == "") continue;

        try {
            if(!std::filesystem::is_directory(dirs[i]) &&
                find(libs, dirs[i]) == -1) libs.push_back(dirs[i]);
        } catch (const std::filesystem::filesystem_error& e) {
            continue;
        }
    }
}

void getIncludes(std::vector<std::string>& includes,
    std::vector<std::string>& Ilist,
    const std::vector<FileNode>& map,
    const std::vector<int>& leaves,
    const std::string& path, bool all)
{
    std::string l;
    std::ifstream input(path);
    if (input.is_open()){
        while (std::getline(input, l)){
            if(l.find("#include") != std::string::npos)
            {
                size_t incPos = l.find("#include");
                size_t startQuote = l.find_first_of("\"<", incPos + 8); 
                if (startQuote == std::string::npos) continue;
                char closeSym = (l[startQuote] == '"') ? '"' : '>';
                size_t endQuote = l.find(closeSym, startQuote + 1);
                if (endQuote == std::string::npos) continue;
                std::string s = l.substr(startQuote + 1, endQuote - startQuote - 1);
                std::pair<std::string,std::string> pair = pathDecoder(s,map,leaves);
                if(pair.first != "-1" && find(includes, pair.first) == -1){
                    includes.push_back(pair.first);
                    if(find(Ilist, pair.second) == -1) Ilist.push_back(pair.second);
                    if(all) getIncludes(includes,Ilist,map,leaves,pair.first,all);
                }
            }
        }
    }
    input.close();
}

void clearAllDepFiles(const std::string& wd){
    std::vector<std::vector<std::string>> dirs;
    dirs.push_back(getDirs(wd + "/" + HEADERS_DIR + "/" + DEPS_DIR));
    dirs.push_back(getDirs(wd + "/" + SOURCE_DIR + "/" + DEPS_DIR));
    dirs.push_back(getDirs(wd + "/" + SOURCE_DIR + "/" + OBJECTS_DIR));
    dirs.push_back(getDirs(wd + "/" + SYM_DIR));
    std::vector<std::string> files_to_remove;
    for(int i = 0; i < dirs.size(); ++i){
        for(int j = 1; j < dirs[i].size(); ++j)
            files_to_remove.push_back(dirs[i][j]);
    }
    removeFiles(files_to_remove);
}
