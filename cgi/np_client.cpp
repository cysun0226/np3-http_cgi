#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <boost/algorithm/string/replace.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;

io_service ioservice;
tcp::resolver resolv{ioservice};
tcp::socket tcp_socket{ioservice};
std::array<char, 4096> bytes;

bool receive_prompt = false;
std::vector<std::string> cmds;
int cmd_idx = 0;
std::string session_id;

const std::string webpage_template = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>NP Project 3 Console</title>
    <link
      rel="stylesheet"
      href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css"
      integrity="sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO"
      crossorigin="anonymous"
    />
    <link
      href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
      rel="stylesheet"
    />
    <link
      rel="icon"
      type="image/png"
      href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
    />
    <style>
      * {
        font-family: 'Source Code Pro', monospace;
        font-size: 1rem !important;
      }
      body {
        background-color: #212529;
      }
      pre {
        color: #cccccc;
      }
      b {
        color: #ffffff;
      }
    </style>
  </head>
  <body>
    <table class="table table-dark table-bordered">
      <thead>
        <tr>
          <th scope="col">nplinux7.cs.nctu.edu.tw:3333</th>
          <th scope="col">nplinux8.cs.nctu.edu.tw:4444</th>
          <th scope="col">nplinux9.cs.nctu.edu.tw:5555</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td><pre id="s0" class="mb-0"></pre></td>
          <td><pre id="s1" class="mb-0"></pre></td>
          <td><pre id="s2" class="mb-0"></pre></td>
        </tr>
      </tbody>
    </table>
  </body>
</html>
)";

void output_command(std::string sid, std::string cmd){
    boost::replace_all(cmd, "\r", "");
    // boost::replace_all(cmd, "\n", "&NewLine");
    std::cout << "<script>document.getElementById('s" << 
    sid << "').innerHTML += '<b>" << cmd << "&NewLine;</b>';</script>" << std::endl;
    std::cout.flush();
}

void output_prompt(std::string sid){
    std::cout << "<script>document.getElementById('s" << 
    sid << "').innerHTML += '<b>" << "% " << "</b>';</script>" << std::endl;
    std::cout.flush();
}

void output_shell(std::string sid, std::string str){
    boost::replace_all(str, "\r", "");
    boost::replace_all(str, "\n", "&NewLine;");
    std::cout << "<script>document.getElementById('s" << sid << 
                 "').innerHTML += '" << str <<"';</script>" << std::endl;
    std::cout.flush();
}

void read_handler(const boost::system::error_code &ec,
  std::size_t bytes_transferred)
{
  if (!ec)
  {
    std::string recv_str(bytes.data());
    // std::cout << recv_str << std::endl;
    if (recv_str.find("% ") != std::string::npos){
        output_shell(session_id, recv_str.substr(0, recv_str.find("%")));
        // std::cout << recv_str.substr(0, recv_str.find("%")) << std::endl;
        usleep(100000);
        output_prompt(session_id);
    }
    else{
        output_shell(session_id, recv_str);
    }
    
    // std::cout.write(bytes.data(), bytes_transferred);
    
    bytes.fill(0);
    
    // send command
    if (recv_str.find("% ") != std::string::npos){ // find prompt
        // std::cout.write(bytes.data(), bytes_transferred);
        // std::cout.flush();
        
        usleep(100000);
        receive_prompt = true;
        
        std::string r = cmds[cmd_idx] + "\r\n";
        // std::cout << cmds[cmd_idx] << std::endl;
        output_command(session_id, cmds[cmd_idx]);
        std::cout.flush();

        usleep(100000);
        write(tcp_socket, buffer(r));
        cmd_idx++;
        tcp_socket.async_read_some(buffer(bytes), read_handler);
    }
    else{
        // std::cout.write(bytes.data(), bytes_transferred);
        // std::cout.flush();
        tcp_socket.async_read_some(buffer(bytes), read_handler);
    }
    
    // clear received content
    // bytes.fill(0);

    // tcp_socket.async_read_some(buffer(bytes), read_handler);
  }
}

void connect_handler(const boost::system::error_code &ec)
{
  if (!ec){
    // std::cout << "<script>document.getElementById('s1').innerHTML += '';</script>" << std::endl;
    // std::cout.flush();
    tcp_socket.async_read_some(buffer(bytes), read_handler);
  }
  else{
        std::cerr << "can't connect to np_server" << std::endl;
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
  if (argc < 4) {
    std::cerr << "Usage:" << argv[0] << " [ip] [port] [cmd_file] [session_id]" << std::endl;
    return 1;
  }

//   std::cout << "Content-type: text/html" << std::endl << std::endl;
//   std::cout << webpage_template << std::endl;

  std::string cmd_line;
  std::ifstream input_file("./test_case/"+std::string(argv[3]));
  if(input_file.is_open()){
      while ( getline (input_file, cmd_line)){
      cmds.push_back(cmd_line);
    }
    input_file.close();
  }
  else{
      std::cout << "<p> can't open " << argv[3] << "</p>" << std::endl;
      return 1;
  }
  session_id = argv[4];
  tcp::resolver::query q{argv[1], argv[2]};
  resolv.async_resolve(q, resolve_handler);
  ioservice.run();
}