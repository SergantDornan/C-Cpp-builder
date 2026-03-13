#include "Mapping.h"

std::vector<int> getMap(
	const std::vector<std::string>& allHeaders,
	const std::vector<std::string>& allSource,
	std::vector<FileNode>& map) 
{
	map.clear();
	auto getRootDir = [](const std::string& s){
		for(int i = 0; i < s.size(); ++i){
			if(s[i] == '/'){
				return std::pair<std::string, std::string>({
					std::string(s.begin(), s.begin() + i),
					std::string(s.begin() + i + 1, s.end())
				});
			}
		}
		return std::pair<std::string,std::string>({"",s});
    };

	std::vector<std::string> files = allHeaders + allSource;
	if(files.size() == 0) {
		std::cerr << "==================== ERROR ====================" << std::endl;
		std::cerr << "Mapping.cpp: getMap(), no files found" << std::endl;
		std::cerr << std::endl;
		return std::vector<int>({});
	}
	merge_sort(files);
	for(int i = 0; i < files.size(); ++i){
		if(files[i][0] == '/') files[i] = std::string(files[i].begin() + 1, files[i].end());
	}

	std::vector<int> leaves;
	FileNode rootDir; 
	rootDir.name = "/";
	rootDir.parent = -1;
	map.push_back(rootDir);
	std::vector<int> last_vector;
	for(int i = 0; i < files.size(); ++i){
		FileNode newNode;
		newNode.name = files[i];
		newNode.parent = 0;
		newNode.file = false;
		map.push_back(newNode);
		map[0].children.push_back(i+1);
	}
	files.clear();
	last_vector.push_back(0);
	while(last_vector.size() > 0){
		auto oldV = last_vector;
		last_vector.clear();
		for(int i = 0; i < oldV.size(); ++i){
			std::vector<int> newChildVector;
			std::string currentRootDir = map[oldV[i]].name;
			int currentRootDirIndex = oldV[i];
			for(int j = 0; j < map[oldV[i]].children.size(); ++j){
				auto split = getRootDir(map[map[oldV[i]].children[j]].name);
				if(split.first == ""){
					newChildVector.push_back(map[oldV[i]].children[j]);
					map[map[oldV[i]].children[j]].file = true;
					leaves.push_back(map[oldV[i]].children[j]);
				}
				else{
					if(split.first != currentRootDir){
						map[map[oldV[i]].children[j]].name = split.first;
						newChildVector.push_back(map[oldV[i]].children[j]);
						FileNode newNode;
						newNode.name = split.second;
						newNode.parent = map[oldV[i]].children[j];
						map.push_back(newNode);
						map[map[oldV[i]].children[j]].children.push_back(map.size()-1);
						currentRootDir = split.first;
						currentRootDirIndex = map[oldV[i]].children[j];
						last_vector.push_back(map[oldV[i]].children[j]);
					}
					else{
						map[map[oldV[i]].children[j]].name = split.second;
						map[map[oldV[i]].children[j]].parent = currentRootDirIndex;
						map[currentRootDirIndex].children.push_back(map[oldV[i]].children[j]);
					}
				}
			}
			map[oldV[i]].children = newChildVector;
		}
	}
	//showTree(0, map[0], map);
	return leaves;
}

void showTree(int tabs,const FileNode& root, const std::vector<FileNode>& map){
	for(int i = 0; i < tabs; ++i) std::cout << "   ";
	std::cout << root.name << std::endl;
	for(int i = 0; i < root.children.size(); ++i)
		showTree(tabs+1, map[root.children[i]], map);
}

// pathDecoder возвращает список возможных файлов, подходящих под путь
// первый элемент - путь к файлу, второй - папка из которой его можно включить с помощью -I
std::pair<std::string,std::string> pathDecoder(
	const std::string& path,
	const std::vector<FileNode>& map,
	const std::vector<int>& leaves)
{
	std::string name = getName(path);
	std::vector<int> rootNodes;
	for(int i = 0 ; i < leaves.size(); ++i){
		if(map[leaves[i]].name == name) rootNodes.push_back(leaves[i]);
	}

	if(rootNodes.size() == 0){
		//std::cerr << "==================== ERROR ====================" << std::endl;
		//std::cerr << "Mapping.cpp: pathDecoder, cannot find \"" << name << "\" file in map" << std::endl;
		//std::cerr << std::endl;
		return std::pair<std::string,std::string>({"-1", "-1"});
	}

	auto splitPath = split(path, "/");
	std::vector<std::pair<std::string,std::string>> options;
	for(int i = 0; i < rootNodes.size(); ++i){
		std::vector<int> endNodes;
		if(map[rootNodes[i]].parent == -1){
			std::cout << "====================== VERY UNEXPECTED ERROR ======================" << std::endl;
			std::cout << "Mapping.cpp, pathDecoder" << std::endl;
			std::cout << "this should never happen" << std::endl;
			std::cout << "congrats" << std::endl;
			return std::pair<std::string,std::string>({"-1","-1"});
		}
		endNodes.push_back(map[rootNodes[i]].parent);
		for(int j = (splitPath.size() - 2); j >=0; --j){
			if(endNodes.size() == 0) break;
			auto oldV = endNodes;
			endNodes.clear();
			// std::cout << splitPath[j] << ":" << std::endl;
			// for(int a = 0; a < oldV.size(); ++a){
			// 	std::cout << map[oldV[a]].name << std::endl;
			// }
			// std::cout << "============================================\n" << std::endl;
			if(splitPath[j] == ".") endNodes = oldV;
			else if(splitPath[j] == ".."){
				for(int a = 0; a < oldV.size(); ++a){
					if(map[oldV[a]].parent == -1) continue;
					else{
						std::vector<int> children = map[oldV[a]].children;
						// std::cout << std::endl;
						// for(int b = 0; b < children.size(); ++b)
						// 	std::cout << map[children[b]].name << std::endl;
						auto it = children.begin();
						while(it != children.end()){
							if(map[*it].file) children.erase(it);
							else it++;
						}
						if(children.size() == 0) continue;
						else{
							for(int k = 0; k < children.size(); ++k) endNodes.push_back(children[k]);
						}
					}
				}
			}
			else{
				for(int a = 0; a < oldV.size(); ++a){
					if(map[oldV[a]].parent == -1) continue;
					else if(map[oldV[a]].name == splitPath[j]) 
						endNodes.push_back(map[oldV[a]].parent);
				}
			}
		}
		if(endNodes.size() != 0){
			std::string fullPath = "";
			int currentNode = rootNodes[i];
			while(map[currentNode].parent != -1){
				fullPath = ("/" + map[currentNode].name + fullPath);
				currentNode = map[currentNode].parent;
			}
			std::string folderPath = "";
			currentNode = endNodes[0];
			while(map[currentNode].parent != -1){
				folderPath = ("/" + map[currentNode].name + folderPath);
				currentNode = map[currentNode].parent;
			}
			if(folderPath == "") folderPath = "/";
			options.push_back({fullPath, folderPath});
		}
	}

	if(options.size() == 0){
		std::cerr << "=================== ERROR ===================" << std::endl;
		std::cerr << "No files found for this include path:" << std::endl;
		std::cerr << "\t" << path << std::endl;
		std::cerr << "This might happen if you got a lot of ../../../../ in #include line and empty folders" << std::endl;
		std::cerr << "Belder does not see empty folders (folders, that do not have files at the end)" << std::endl;
		char y = 'n';
		std::cerr << "Do you want to see map, contained inside belder? [y/n]: ";
		std::cin >> y;
		if(y == 'y') showTree(0, map[0], map);
		std::cerr << std::endl;
		return std::pair<std::string,std::string>({"-1", "-1"});
	}
	
	if(options.size() > 1){
		std::cerr << "=================== ERROR ===================" << std::endl;
		std::cerr << "Multiple files match this include path: " << std::endl;
		std::cerr << "\t" << path << std::endl;
		std::cerr << "Matching files:" << std::endl;
		for(int i = 0; i < options.size(); ++i)
			std::cerr << options[i].first << std::endl;
		std::cerr << std::endl;
		std::cerr << "You must add flags to exclude some files" << std::endl;
		std::cerr << std::endl;
		return std::pair<std::string,std::string>({"-1", "-1"});
	}
	return options[0];
}