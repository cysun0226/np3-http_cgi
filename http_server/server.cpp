#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sstream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#include <string>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/signal_set.hpp>

#include <boost/process.hpp>


#define MAX_REQUEST_NUM 5

using namespace std;
using namespace boost::asio;

io_service global_io_service;
unsigned short port;
std::string server_addr;

typedef struct {
  std::string req_method;
  std::string req_uri;
  std::string query;
  std::string server_protocol;
  std::string http_host;
  std::string server_addr;
  std::string server_port;
  std::string remote_addr;
  std::string remote_port;
} Request;

/* The following environment variables are required to set
(a) REQUEST METHOD
(b) REQUEST URI
(c) QUERY STRING
(d) SERVER PROTOCOL
(e) HTTP HOST
(f) SERVER ADDR
(g) SERVER PORT
(h) REMOTE ADDR
(i) REMOTE PORT
*/

void parse_query(std::string query_str, Request* req){
  stringstream ss;
  ss.str(query_str);
  string str;
  string host_name[MAX_REQUEST_NUM];
  string host_port[MAX_REQUEST_NUM];
  string file_name[MAX_REQUEST_NUM];

  vector<string> tokens;
  boost::split( tokens, query_str, boost::is_any_of( "&" ), boost::token_compress_on);
  int host_num = 0;
  for( vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it ){
    if(it->at(0) == 'h'){
      host_num++;
      std::string hid = it->substr(0, it->find("="));
      host_name[atoi(hid.substr(1).c_str())] = it->substr(it->find("=")+1);
    }
    if(it->at(0) == 'p'){
      std::string pid = it->substr(0, it->find("="));
      host_port[atoi(pid.substr(1).c_str())] = it->substr(it->find("=")+1);
    }
    if(it->at(0) == 'f'){
      std::string fid = it->substr(0, it->find("="));
      file_name[atoi(fid.substr(1).c_str())] = it->substr(it->find("=")+1);
    }
  }

  for (size_t i = 0; i < host_num; i++){
    cout << host_name[i] << endl;
    cout << host_port[i] << endl;
    cout << file_name[i] << endl;
  }
  
}

void parse(std::string req_str, std::string server_ip){
    std::cout << req_str << std::endl;
    stringstream ss;
    Request requests[MAX_REQUEST_NUM];
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
    	return;
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
}

std::array<char, 4096> bytes;

void read_handler(const boost::system::error_code &ec,
  std::size_t bytes_transferred){
    std::cout << "read handle" << std::endl;
    if(!ec){
      std::string recv_str(bytes.data());
      std::cout << recv_str << std::endl;

       cout << "HTTP/1.1 200 OK" << endl;
          // cout << "Content-type: text/html\r\n\r\n<h1>Hello</h1>" << endl;

              std::string file = "hello.cgi";
                std::string arg = "";
                // std::string arg = " 8.8.8.8 -c 10";
                // execlp("ping", "ping", "8.8.8.8", "-c", "5",NULL);
                execlp("console.cgi", "console.cgi", NULL);
                // execlp(file.c_str(),file.c_str(),arg.c_str(), NULL); //works
        cerr << "exec error" << endl;

    }
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
            do_write(length);
            std::string req_str(std::begin(_data), std::end(_data));
            std::cout << "=== new request ===\n" << std::endl;
            // parse(req_str,);
          }
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
  boost::asio::signal_set _signal;
  boost::asio::io_context& _io_context;
  
  

 public:
  EchoServer(unsigned short port, boost::asio::io_context& io_context)
      : _acceptor(io_context, {ip::tcp::v4(), port}),
        _socket(io_context),
        _signal(io_context, SIGCHLD),
        _io_context(io_context)
  {
    signal_handler();
    do_accept();
  }

 private:
  

  void do_accept() {
    _acceptor.async_accept([this](boost::system::error_code ec, ip::tcp::socket client_socket) {
      // cout << _socket.remote_endpoint().address().to_string() << endl;
      // if (!ec) make_shared<EchoSession>(move(_socket))->start();
      if (!ec){

        // take client socket        
        _socket = std::move(client_socket);

        // prepare for fork
        _io_context.notify_fork(boost::asio::io_context::fork_prepare);

        if (fork() == 0){
          // child process
          _io_context.notify_fork(boost::asio::io_context::fork_child);

          // child don't need listener
          _acceptor.close();

          // unregister SIGCHLD
          _signal.cancel();

          int client_fd = _socket.native_handle();
          
          // _socket.async_read_some(buffer(bytes), read_handler);

          // execute the child process (cgi)
          // close(STDOUT_FILENO);
          // close(STDIN_FILENO);
          // close(STDERR_FILENO);

          dup2(client_fd, STDIN_FILENO);

          // if (dup(client_fd) != STDIN_FILENO || dup(client_fd) != STDOUT_FILENO || dup(client_fd) != STDERR_FILENO){
          //       std::cout << "can't dup client socket for stdin/out/err" << std::endl;
          //       exit(1);
          // }



          std::string req_token;
          std::string req_string = "";

          while(cin >> req_token){
            // cout << req_str << endl;
            req_string += req_token;
            req_string += " ";
            if(req_token == "Upgrade-Insecure-Requests:"){
              break;
            }
          }

          // stringstream tmp_ss;
          // tmp_ss << _socket.local_endpoint().address();

          parse(req_string, "");
          std::cout << "remote addr" << std::endl;
          std::cout << _socket.remote_endpoint().address() << std::endl;
          std::cout << _socket.remote_endpoint().port() << std::endl;


          std::string set_env_str;
          set_env_str = "REMOTE_ADDR"+_socket.remote_endpoint().address().to_string();
          setenv("REMOTE_ADDR", _socket.remote_endpoint().address().to_string().c_str(), 1);
          set_env_str = "REMOTE_PORT"+_socket.remote_endpoint().port();
          setenv("REMOTE_PORT", std::to_string(_socket.remote_endpoint().port()).c_str(), 1);
          // std::cout << "setenv st" << st << std::endl;

          dup2(client_fd, STDOUT_FILENO);
          cout << "HTTP/1.1 200 OK" << endl;
          
          // execlp("console.cgi", "console.cgi", NULL);
          execlp("printenv.cgi", "printenv.cgi", NULL);
          cerr << "exec error" << endl;

            // write(_socket, buffer("HTTP/1.1 200 OK\r\n"));
            // write(_socket, buffer("Content-type: text/html\r\n\r\n<h1>Hello</h1>\r\n"));
            
            // boost::process::ipstream pipe_stream;
            // boost::process::child c("ping 8.8.8.8 -c 10", boost::process::std_out > pipe_stream);
            // // boost::process::child c("./hello.cgi", boost::process::std_out > pipe_stream);

            //  std::string line;

          
          exit(0);

        }
        else{
          // parent or fork failed
          _io_context.notify_fork(boost::asio::io_context::fork_parent);

          // the socket is forward to the cgi, close the socket
          _socket.close();
          do_accept();
        }


      }
      else{
            std::cerr << "accept error: " << ec.message() << std::endl;
            do_accept();
      }
    });
  }

  void signal_handler() {
    _signal.async_wait(
      [this](boost::system::error_code, int /*signo*/){
        if (_acceptor.is_open()){
          // collect died child process
          int status = 0;
          while (waitpid(-1, &status, WNOHANG) > 0) {}

          signal_handler();
        }
      }
    );
  }
};

int main(int argc, char* const argv[]) {
  if (argc != 2) {
    cerr << "Usage:" << argv[0] << " [port]" << endl;
    return 1;
  }

  char default_path[] = "PATH=/bin:bin:.:/usr/bin/env:/home/cysun/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin";
  putenv(default_path);
  cout << "\n start server ..." << endl;

  try {
    port = atoi(argv[1]);
    boost::asio::io_context io_context;

    EchoServer server(port, io_context);
    io_context.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
