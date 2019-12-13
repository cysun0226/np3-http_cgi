#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

using namespace boost::asio;
using namespace boost::asio::ip;

io_service ioservice;
tcp::resolver resolv{ioservice};
tcp::socket tcp_socket{ioservice};
std::array<char, 4096> bytes;

bool receive_prompt = false;
std::vector<std::string> cmds;
int cmd_idx = 0;

void read_handler(const boost::system::error_code &ec,
  std::size_t bytes_transferred)
{
  if (!ec)
  {
    std::cout.write(bytes.data(), bytes_transferred);
    std::cout.flush();
    std::string recv_str(bytes.data());
    if (recv_str.find("% ") != std::string::npos){
        
        sleep(1);
        receive_prompt = true;
        std::string r = cmds[cmd_idx] + "\r\n";
        std::cout << cmds[cmd_idx] << std::endl;
        std::cout.flush();
        sleep(1);
        write(tcp_socket, buffer(r));
        tcp_socket.async_read_some(buffer(bytes), read_handler);
        cmd_idx++;
    }
    else{
        tcp_socket.async_read_some(buffer(bytes), read_handler);
    }

    // tcp_socket.async_read_some(buffer(bytes), read_handler);
  }
}

void connect_handler(const boost::system::error_code &ec)
{
  if (!ec)
  {
    tcp_socket.async_read_some(buffer(bytes), read_handler);
    // if (receive_prompt != true){
    //     sleep(0.01);
    // }
    // std::string r = "ls\r\n";
    // std::cout << r << std::endl;
    // receive_prompt = false;
    // write(tcp_socket, buffer(r));
    // tcp_socket.async_read_some(buffer(bytes), read_handler);
    // std::string r = "ls\r\n";
    // //   "GET / HTTP/1.1\r\nHost: theboostcpplibraries.com\r\n\r\n";
    // std::cout << r << std::endl;
    // write(tcp_socket, buffer(r));
    // tcp_socket.async_read_some(buffer(bytes), read_handler);
    // r = "who\r\n";
    // std::cout << r << std::endl;
    // write(tcp_socket, buffer(r));
    // tcp_socket.async_read_some(buffer(bytes), read_handler);
  }
}

void resolve_handler(const boost::system::error_code &ec,
  tcp::resolver::iterator it)
{
  if (!ec)
    tcp_socket.async_connect(*it, connect_handler);
}

int main(int argc, char* const argv[])
{
  if (argc < 2) {
    std::cerr << "Usage:" << argv[0] << " [ip] [port] [cmd_file]" << std::endl;
    return 1;
  }

  std::string cmd_line;
  std::ifstream input_file("./test_case/"+std::string(argv[3]));
  if(input_file.is_open()){
      while ( getline (input_file, cmd_line)){
      cmds.push_back(cmd_line);
    }
    input_file.close();
  }
  else{
      std::cout << "can't open " << argv[3] << std::endl;
  }
  
  tcp::resolver::query q{argv[1], argv[2]};
  resolv.async_resolve(q, resolve_handler);
  ioservice.run();
}