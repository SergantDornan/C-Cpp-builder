#include "BuilderFilework.h"
// Следующая строка заполняется инсталлятором, не менять ее
const std::string root = getHomedir() + "/builder";
const std::string cd = cwd();
const bool pocket = (root == "./builder");
const std::string configFile = "config";
const std::vector<std::string> reqFolders = {"headers","source"}; // Если меньше двух имен - будет SegFault
const std::vector<std::string> subFolders = {"deps", "objects"}; // Если меньше двух имен - будет SegFault

void getAllheaders(std::vector<std::string>& headers,const std::string& path,
 const std::string& forceUnlink){
    auto dirs = getDirs(path);
    auto v = split(forceUnlink);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(v, getName(dirs[i])) != -1) continue;
        if(pocket && (dirs[i] == cd + "/builder") &&
            std::filesystem::is_directory(dirs[i])) continue;
        if((getExt(dirs[i]) == "h" || getExt(dirs[i]) == "hpp") 
            && find(headers, dirs[i]) == -1)
            headers.push_back(dirs[i]);
        if(std::filesystem::is_directory(dirs[i]))
            getAllheaders(headers, dirs[i],forceUnlink);
    }
    merge_sort(headers);
}
void getAllsource(std::vector<std::string>& source, const std::string& path,
    const std::string& forceUnlink)
{
    auto dirs = getDirs(path);
    auto v = split(forceUnlink);
    for(int i = 1; i < dirs.size(); ++i){
        if(find(v, getName(dirs[i])) != -1) continue;
        if(pocket && (dirs[i] == cd + "/builder") &&
            std::filesystem::is_directory(dirs[i])) continue;
        std::string ext = getExt(dirs[i]);
        if(find(source, dirs[i]) == -1 && 
            (ext == "c" || ext == "cpp" || ext == "asm" || ext == "s" || ext == "S"))
                source.push_back(dirs[i]);
        if(std::filesystem::is_directory(dirs[i]))
            getAllsource(source, dirs[i],forceUnlink);
    }
    merge_sort(source);
}
void getAllLibs(std::vector<std::string>& libs, const std::string& path,
    const std::string& fUnLibs)
{
    auto dirs = getDirs(path);
    auto v = split(fUnLibs);
    for(int i = 1; i < dirs.size(); ++i){
        std::string longName = getName(dirs[i]);
        if(longName.size() < 4) continue;
        if(std::string(longName.begin(), longName.begin() + 3) != "lib") continue;
        if(getExt(longName) != "a" && getExt(longName) != "so") continue;
        std::string shortName = getNameNoExt(dirs[i]);
        shortName = std::string(shortName.begin() + 3, shortName.end());
        if(find(v, shortName) != -1) continue;
        if(pocket && (dirs[i] == cd + "/builder") &&
            std::filesystem::is_directory(dirs[i])) continue;
        if(std::filesystem::is_directory(dirs[i])) getAllLibs(libs, dirs[i], fUnLibs);
        if(!std::filesystem::is_directory(dirs[i]) && 
            find(libs, dirs[i]) == -1) libs.push_back(dirs[i]);
    }
}