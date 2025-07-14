#include <algs.h>

extern std::vector<std::string> split(std::string s, std::string ch, std::string except){
    std::vector<std::string> res;
    std::string curr;
    bool push = true;
    for(int i = 0; i < s.size(); ++i){
        if(ch.find(s[i]) != std::string::npos && i != (s.size() - 1) && push){
            if(i != 0)
                res.push_back(curr);
            curr.clear();
            if(except.find(s[i]) != std::string::npos){
                curr += s[i];
            }
            push = false;
        }
        else if(ch.find(s[i]) != std::string::npos && i != (s.size() - 1) && !push && except.find(s[i]) != std::string::npos){
            curr += s[i];
        }
        else if(i == (s.size() - 1)){
        //  std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" << std::endl;
            if(ch.find(s[i]) == std::string::npos)
                curr+=s[i];
            res.push_back(curr);
        }
        else if(ch.find(s[i]) == std::string::npos){
            push = true;
            curr += s[i];
        }
    }
    return res;
}

extern std::string strip(std::string s, std::string ch){
    std::string res;
    for(int i = 0; i < s.size(); ++i){
        if(ch.find(s[i]) == std::string::npos)
            res+=s[i];
    }
    return res;
}


int find(const std::string& s,const char s0){
    for(int i = 0; i < s.size(); ++i){
        if(s[i] == s0)
            return i;
    }
    return -1;
}
int find(const std::string& s,const std::string& s0){
    if(s.size() < s0.size())
        return -1;
    for(int i = 0; i < s.size() - s0.size() + 1; ++i){
        std::string tmp(s.begin() + i, s.begin() + i + s0.size());
        if(tmp == s0)
            return i;
    }
    return -1;
}
int find(const std::vector<std::string>& v, const char* s){
	for(int i = 0; i < v.size(); ++i){
		if(v[i] == s)
			return i;
	}
	return -1;
}