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



#define MAX_REQUEST_NUM 5

using namespace std;
using namespace boost::asio;

io_service global_io_service;
unsigned short port;
std::string server_addr;

std::string parse(std::string req_str, std::string server_ip){
    boost::replace_all(req_str, "\\", "");
    std::cout << req_str << std::endl;
    stringstream ss;
    ss.str(req_str);
    std::string str;
    std::string req_method;
    std::string req_uri;
    std::string remote_addr;
    std::string remote_host;
    std::string query_str;
    std::string server_protocol;
    std::string http_host;

    // REQUEST METHOD
    ss >> req_method;
    cout << "req_method" << endl;
    cout << req_method << endl;

    // uri & QUERY STRING
    ss >> str;
    if (str == "/favicon.ico"){
      return str;
    }
    std::cout << str << std::endl;
    req_uri = str.substr(0, str.find("?"));
    cout << "req_uri" << endl;
    cout << req_uri << endl;
    query_str = str.substr(str.find("?")+1);
    cout << "query_str" << endl;
    cout << query_str << endl;
    // parse_query(query_str, requests);

    // SERVER PROTOCOL
    ss >> server_protocol;
    cout << "server_protocol" << endl;
    cout << server_protocol << endl;

    // host
    ss >> str; ss >> http_host;
    http_host = http_host.substr(0, http_host.find(":"));
    cout << "http_host" << endl;
    cout << http_host << endl;

    // server addr
    ip::tcp::endpoint ep(ip::tcp::v4(), port);
    cout << "server_addr" << endl;
    cout << server_addr << endl;
    cout << "server_port" << endl;
    cout << port << endl;

    // set env
    std::string set_env_str;
    setenv("REQUEST_METHOD", req_method.c_str(), 1);
    setenv("REQUEST_URI", req_uri.c_str(), 1);
    setenv("QUERY_STRING", query_str.c_str(), 1);
    setenv("HTTP_HOST", http_host.c_str(), 1);
    // setenv(// set_env_str = "SERVER ADDR="+serve.c_str(), 1)r
    setenv("SERVER_PORT", std::to_string(port).c_str(), 1);
    setenv("SERVER_PROTOCOL", server_protocol.c_str(), 1);

    return req_uri;
}

class EchoSession : public enable_shared_from_this<EchoSession> {
 private:
  enum { max_length = 1024 };
  ip::tcp::socket _socket;
  array<char, max_length> _data;

 public:
  EchoSession(ip::tcp::socket socket) : _socket(move(socket)) {}

  void start() { do_read(); }

 private:
  void do_read() {
    auto self(shared_from_this());
    _socket.async_read_some(
        buffer(_data, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
          if (!ec){
            std::string req_str(std::begin(_data), std::end(_data));
            std::cout << "=== new request ===\n" << std::endl;

            std::string uri_file = parse(req_str, "");
            std::cout << "remote addr" << std::endl;
            std::cout << _socket.remote_endpoint().address() << std::endl;
            std::cout << _socket.remote_endpoint().port() << std::endl;
            std::string target_cgi = "."+uri_file;
            std::cout << "execute" << target_cgi << std::endl;

            std::string set_env_str;
            set_env_str = "REMOTE_ADDR"+_socket.remote_endpoint().address().to_string();
            setenv("REMOTE_ADDR", _socket.remote_endpoint().address().to_string().c_str(), 1);
            set_env_str = "REMOTE_PORT"+_socket.remote_endpoint().port();
            setenv("REMOTE_PORT", std::to_string(_socket.remote_endpoint().port()).c_str(), 1);

            

            // reply to the client
            bool ok = true;
            // check if file exist
            if (!boost::filesystem::exists("."+uri_file) || uri_file == "/favicon.ico"){
                write(_socket, buffer("HTTP/1.1 404 Not Fount\r\n"));
                write(_socket, buffer("Content-type: text/html\r\n\r\n<h1>404 Not Found</h1>\r\n"));
                ok = false;
            }

            if(ok){
                write(_socket, buffer("HTTP/1.1 200 OK\r\n"));

            // fork to execute console.cgi
            global_io_service.notify_fork(boost::asio::io_context::fork_prepare);

            // child process (cgi)
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
          do_read();
        });
  }

  void do_write(size_t length) {
    auto self(shared_from_this());
    _socket.async_send(
        buffer(_data, length),
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
