#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>

void set_env_var(std::vector<std::pair<std::string, std::string>> env_vars){
    for (size_t i = 0; i < env_vars.size(); i++){
        setenv(env_vars[i].first.c_str(), env_vars[i].second.c_str(), 1);
    }
}

std::string parse(std::string req_str, std::string server_ip){
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
            query_str = str.substr(str.find("?")+1);
            // SERVER PROTOCOL
            ss >> server_protocol;
        }
        if (str == "Host:"){
            ss >> http_host;
        }   
    
    // // server addr
    // ip::tcp::endpoint ep(ip::tcp::v4(), port);
    // cout << "server_addr" << endl;
    // cout << server_addr << endl;
    // cout << "server_port" << endl;
    // cout << port << endl;
    std::vector<std::pair<std::string, std::string>> env_vars;
    env_vars.push_back(std::pair("REQUEST_METHOD", req_method));
    env_vars.push_back(std::pair("REQUEST_URI", req_uri));
    env_vars.push_back(std::pair("QUERY_STRING" , query_str));
    env_vars.push_back(std::pair("HTTP_HOST" , http_host));
    env_vars.push_back(std::pair("SERVER_PORT" , server_port));
    env_vars.push_back(std::pair("SERVER_ADDR" , server_addr));
    env_vars.push_back(std::pair("SERVER_PROTOCOL" , server_protocol));
    // env_vars.push_back(std::pair("REMOTE_ADDR" , remote_addr));
    // env_vars.push_back(std::pair("REMOTE_PORT" , remote_port));
    set_env_var(env_vars);

    return req_uri;
}

int main(){

}