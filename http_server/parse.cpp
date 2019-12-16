#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>

void set_env_var(std::vector<std::pair<std::string, std::string>> env_vars){
    for (size_t i = 0; i < env_vars.size(); i++){
        std::cout << env_vars[i].first.c_str() << " = " 
                  << env_vars[i].second.c_str() << std::endl;
        setenv(env_vars[i].first.c_str(), env_vars[i].second.c_str(), 1);
    }
}

std::string parse(std::string req_str){
    boost::replace_all(req_str, "\\", "");
    std::cout << req_str << std::endl;
    std::stringstream ss;
    ss.str(req_str);
    std::string str;
    std::string req_method;
    std::string req_uri;
    std::string remote_addr;
    std::string remote_host;
    std::string query_str;
    std::string server_protocol;
    std::string http_host;
    std::string server_addr;
    std::string server_port;

    while (ss >> str) {
        if (str == "GET"){
            req_method = "GET";
            // uri & QUERY STRING
            ss >> str;
            req_uri = str.substr(0, str.find("?"));
            query_str = (str.find("?") != std::string::npos)? str.substr(str.find("?")+1) : "";
            // SERVER PROTOCOL
            ss >> server_protocol;
        }
        if (str == "Host:"){
            ss >> http_host;
        }
    }
    
    std::vector<std::pair<std::string, std::string>> env_vars;
    env_vars.push_back(std::pair<std::string, std::string>("REQUEST_METHOD", req_method));
    env_vars.push_back(std::pair<std::string, std::string>("REQUEST_URI", req_uri));
    env_vars.push_back(std::pair<std::string, std::string>("QUERY_STRING" , query_str));
    env_vars.push_back(std::pair<std::string, std::string>("HTTP_HOST" , http_host));
    env_vars.push_back(std::pair<std::string, std::string>("SERVER_PORT" , server_port));
    env_vars.push_back(std::pair<std::string, std::string>("SERVER_ADDR" , server_addr));
    env_vars.push_back(std::pair<std::string, std::string>("SERVER_PROTOCOL" , server_protocol));
    // env_vars.push_back(std::pair("REMOTE_ADDR" , remote_addr));
    // env_vars.push_back(std::pair("REMOTE_PORT" , remote_port));
    set_env_var(env_vars);

    return req_uri;
}

int main(){
    std::string input = R"(
        GET /panel.cgi HTTP/1.1
        Host: localhost:1234
        User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:70.0) Gecko/20100101 Firefox/70.0
        Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
        Accept-Language: zh-TW,en-US;q=0.7,en;q=0.3
        Accept-Encoding: gzip, deflate
        DNT: 1
        Connection: keep-alive
        Cookie: pma_lang=zh_TW; pmaUser-1=%7B%22iv%22%3A%22PV2eivikV0pKZGuzCjHyNA%3D%3D%22%2C%22mac%22%3A%2202af860766e16e27f349e127726dfcd5f95d8829%22%2C%22payload%22%3A%22WN%5C%2FK%5C%2FtYVLjnht14tTz1efw%3D%3D%22%7D; phpMyAdmin=ac60216e31dc0ed77e048958fe7ea968
        Upgrade-Insecure-Requests: 1
    )";

    parse(input);

}