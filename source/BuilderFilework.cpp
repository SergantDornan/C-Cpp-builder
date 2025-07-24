#include "BuilderFilework.h"
// Следующая строка заполняется инсталлятором, не менять ее
const std::string root = getHomedir() + "/builder";
const std::string cd = cwd();
const bool pocket = (root == "./builder");
const std::string configFile = "config";
const std::vector<std::string> reqFolders = {"headers","source"}; // Если меньше двух имен - будет SegFault
const std::vector<std::string> subFolders = {"deps", "objects"}; // Если меньше двух имен - будет SegFault

std::string converPathToName(const std::string& path, const char ch){
    std::string result = path;
    if(result[0] == '/') result.erase(result.begin());
    for(int i = 0; i < result.size(); ++i){
        if(result[i] == '/') result[i] = ch;
    }
    return result;
}

void getAllheaders(std::vector<std::string>& headers,const std::string& path,
 const std::vector<std::string>& forceUnlink, const std::vector<std::string>& fUnIncludeDirs){
    auto dirs = getDirs(path);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(forceUnlink, dirs[i]) != -1) continue;
        if(((pocket && (dirs[i] == cd + "/builder")) || getName(dirs[i]) == ".git") &&
            std::filesystem::is_directory(dirs[i])) continue;
        if((getExt(dirs[i]) == "h" || getExt(dirs[i]) == "hpp") &&
            find(headers, dirs[i]) == -1) headers.push_back(dirs[i]);
        if(std::filesystem::is_directory(dirs[i]) && find(fUnIncludeDirs, dirs[i]) == -1)
            getAllheaders(headers, dirs[i],forceUnlink,fUnIncludeDirs);
    }
    merge_sort(headers);
}
void getAllsource(std::vector<std::string>& source, const std::string& path,
    const std::vector<std::string>& forceUnlink, const std::vector<std::string>& fUnIncludeDirs)
{
    auto dirs = getDirs(path);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(forceUnlink, dirs[i]) != -1) continue;
        if(((pocket && (dirs[i] == cd + "/builder")) || getName(dirs[i]) == ".git") &&
            std::filesystem::is_directory(dirs[i])) continue;
        std::string ext = getExt(dirs[i]);
        if((ext == "c" || ext == "cpp" || ext == "asm" || ext == "s" || ext == "S") &&
            find(source, dirs[i]) == -1) source.push_back(dirs[i]);
        if(std::filesystem::is_directory(dirs[i]) && find(fUnIncludeDirs, dirs[i]) == -1)
            getAllsource(source, dirs[i],forceUnlink,fUnIncludeDirs);
    }
    merge_sort(source);
}
void getAllLibs(std::vector<std::string>& libs, const std::string& path,
    const std::vector<std::string>& fUnlib, const std::vector<std::string>& fUnIncludeDirs)
{   
    auto dirs = getDirs(path);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(fUnlib, dirs[i]) != -1) continue;
        if(((pocket && (dirs[i] == cd + "/builder")) || getName(dirs[i]) == ".git") &&
            std::filesystem::is_directory(dirs[i])) continue;
        if(std::filesystem::is_directory(dirs[i]) && find(fUnIncludeDirs, dirs[i]) == -1) 
            getAllLibs(libs, dirs[i], fUnlib,fUnIncludeDirs);
        std::string longName = getName(dirs[i]);
        if(longName.size() < 4) continue;
        if(std::string(longName.begin(), longName.begin() + 3) != "lib") continue;
        if(getExt(longName) != "a" && getExt(longName) != "so") continue;
        if(!std::filesystem::is_directory(dirs[i]) && 
            find(libs, dirs[i]) == -1) libs.push_back(dirs[i]);
    }
}

void getIncludes(std::vector<std::string>& includes,
    const std::vector<std::string>& allHeaders,
    const std::vector<std::string>& allSource,
    const std::string& path, bool all){
    std::string l;
    std::ifstream input(path);
    if (input.is_open()){
        while (std::getline(input, l)){
            if(l.find("#include") != std::string::npos){
                std::string s;
                for(int j = 10; j < l.size() && l[j] != '>' && l[j] != '"'; ++j)
                    s+=l[j];
                std::string newline;
                bool b = false;
                for(int j = 0; j < allHeaders.size(); ++j){
                    if(s == getName(allHeaders[j])){
                        b = true;
                        newline = allHeaders[j];
                        break;
                    }
                }
                for(int j = 0; j < allSource.size(); ++j){
                    if(s == getName(allSource[j])){
                        b = true;
                        newline = allSource[j];
                        break;
                    }
                }
                if(b && find(includes, newline) == -1){
                    includes.push_back(newline);
                    if(all) getIncludes(includes,allHeaders,allSource,newline);
                }
            }
        }
    }
    input.close();
}
