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

#define MAX_REQUEST_NUM 5

using namespace std;
using namespace boost::asio;

io_service global_io_service;

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
    cout << *it << endl;
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

void parse(std::string req_str){
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

    // uri & QUERY STRING
    ss >> str;
    if (str == "/favicon.ico"){
    	return;
    }
    std::cout << str << std::endl;
    req_uri = str.substr(0, str.find("?"));
    cout << req_uri << endl;
    query_str = str.substr(str.find("?")+1);
    parse_query(query_str, requests);

    // SERVER PROTOCOL
    ss >> server_protocol;
    // host
    ss >> str; ss >> http_host;
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
            parse(req_str);
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

  try {
    unsigned short port = atoi(argv[1]);
    EchoServer server(port);
    global_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
