#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sstream>

#include <string>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <thread>



#define MAX_REQUEST_NUM 5

using namespace std;
using namespace boost::asio;

io_service global_io_service;
unsigned short port;
std::string server_addr;

void exec_cgi(int client_fd, std::string target_cgi){
    dup2(client_fd, STDOUT_FILENO);
    execlp(target_cgi.c_str(), target_cgi.c_str(), NULL);
    cerr << "exec error" << endl;
    // exit(0);
}

void set_env_var(std::vector<std::pair<std::string, std::string>> env_vars){
    for (size_t i = 0; i < env_vars.size(); i++){
        std::cout << env_vars[i].first.c_str() << " = " 
                  << env_vars[i].second.c_str() << std::endl;
        setenv(env_vars[i].first.c_str(), env_vars[i].second.c_str(), 1);
    }
}

std::string parse(std::string req_str, std::string remote_addr, std::string remote_port){
    boost::replace_all(req_str, "\\", "");
    // std::cout << req_str << std::endl;
    std::stringstream ss;
    ss.str(req_str);
    std::string str;
    std::string req_method;
    std::string req_uri;
    std::string remote_host;
    std::string query_str;
    std::string server_protocol;
    std::string http_host;

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
    env_vars.push_back(std::pair<std::string, std::string>("SERVER_PORT" , std::to_string(port)));
    env_vars.push_back(std::pair<std::string, std::string>("SERVER_ADDR" , server_addr));
    env_vars.push_back(std::pair<std::string, std::string>("SERVER_PROTOCOL" , server_protocol));
    env_vars.push_back(std::pair<std::string, std::string>("REMOTE_ADDR" , remote_addr));
    env_vars.push_back(std::pair<std::string, std::string>("REMOTE_PORT" , remote_port));
    set_env_var(env_vars);

    return req_uri;
}

class EchoSession : public enable_shared_from_this<EchoSession> {
 private:
  enum { max_length = 1024 };
  ip::tcp::socket _socket;
  array<char, max_length> _data;

 public:
  EchoSession(ip::tcp::socket socket) : _socket(move(socket)) {}

  void start() { 
    do_read(); 
    server_addr = _socket.local_endpoint().address().to_string();
  }

 private:
  void do_read() {
    auto self(shared_from_this());
    _socket.async_read_some(
        buffer(_data, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
          if (!ec){
            std::string req_str(std::begin(_data), std::end(_data));
            std::cout << "\n\n=== new request ===\n" << std::endl;

            std::string uri_file = parse(
              req_str, 
              _socket.remote_endpoint().address().to_string(), 
              std::to_string(_socket.remote_endpoint().port())
            );
            std::string target_cgi = "."+uri_file;
            std::cout << "execute" << target_cgi << std::endl;

            // reply to the client
            bool ok = true;
            // check if file exist
            if (!boost::filesystem::exists("."+uri_file)){
                do_write("HTTP/1.1 404 Not Fount\r\n");
                // write(_socket, buffer("HTTP/1.1 404 Not Fount\r\n"));
                do_write("Content-type: text/html\r\n\r\n<h1>404 Not Found</h1>\r\n");
                // write(_socket, buffer("Content-type: text/html\r\n\r\n<h1>404 Not Found</h1>\r\n"));
                ok = false;
            }

            if(uri_file == "/favicon.ico" || uri_file == "" || uri_file == "/"){
              std::cout << "null uri file" << std::endl;
              do_write("HTTP/1.1 200 OK\r\n");
              // write(_socket, buffer("HTTP/1.1 200 OK\r\n"));
              do_write("Content-type: text/html\r\n\r\n");
              ok = false;
            }

            if(ok){
              do_write("HTTP/1.1 200 OK\r\n");
                // write(_socket, buffer());

            // fork to execute console.cgi
            global_io_service.notify_fork(boost::asio::io_context::fork_prepare);

            // child process (cgi)
            // int client_fd = _socket.native_handle();
            // thread cgiThread(exec_cgi, client_fd, target_cgi);
            // cgiThread.detach();

            if (fork() == 0){
                global_io_service.notify_fork(boost::asio::io_context::fork_child);
                int client_fd = _socket.native_handle();
                dup2(client_fd, STDOUT_FILENO);
                execlp(target_cgi.c_str(), target_cgi.c_str(), NULL);
                cerr << "exec error" << endl;
                exit(0);
            }
            else{
              global_io_service.notify_fork(boost::asio::io_context::fork_parent);

              // the socket is forward to the cgi, close the socket
              _socket.close();
            }
            }

            
          }
          // do_read();
        });
  }

  void do_write(std::string msg) {
    auto self(shared_from_this());
    _socket.async_send(
        buffer(msg),
        [this, self](boost::system::error_code ec, size_t /* length */) {
          // if (!ec) do_read();
        });
  }
};

class EchoServer {
 private:
  ip::tcp::acceptor _acceptor;
  ip::tcp::socket _socket;

 public:
  EchoServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service) {
    do_accept();
  }

 private:
  void do_accept() {
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
      if (!ec) make_shared<EchoSession>(move(_socket))->start();
      do_accept();
    });
  }
};

int main(int argc, char* const argv[]) {
  if (argc != 2) {
    cerr << "Usage:" << argv[0] << " [port]" << endl;
    return 1;
  }

  cout << "\n start server ..." << endl;

  try {
    port = atoi(argv[1]);
    EchoServer server(port);
    global_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
