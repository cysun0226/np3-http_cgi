#include <iostream>
#include <csignal>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <string>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <csignal>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <sys/types.h>
#include <sys/wait.h>

#define MAX_SESSION 5


// ==================================================================================
// np client
// ==================================================================================
using namespace boost::asio;
using namespace boost::asio::ip;

io_service ioservice;
tcp::resolver resolv{ioservice};
tcp::socket tcp_socket{ioservice};
// tcp::resolver resolv;
// tcp::socket tcp_socket;

std::array<char, 4096> bytes;

bool receive_prompt = false;
std::vector<std::string> cmds;
int cmd_idx = 0;
std::string session_id;
int session_idx;

std::string host_name[MAX_SESSION];
std::string host_port[MAX_SESSION];
std::string file_name[MAX_SESSION];
int host_num = 0;

// void handler(
//     const boost::system::error_code& error,
//     int signal_number)
// {
//   if (!error)
//   {
//     std::cout << "get SIGINT" << std::endl;
//     tcp_socket.cancel();
//     tcp_socket.close();
//     ioservice.stop();
//   }
// }


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
    std::cout << recv_str << std::endl;
    // left
    // if (recv_str.find("left. ***") != std::string::npos){
    //     output_shell(session_id, recv_str);
    //     tcp_socket.shutdown(socket_base::shutdown_type::shutdown_both);
    //     tcp_socket.close();
    // }

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
  if (!ec){
      tcp_socket.async_connect(*it, connect_handler);
  }
  else{
      std::cout << "can't resolve" << std::endl;
  }
    

}

int np_client(std::string ip, std::string port, std::string cmd_file, std::string sid){
    std::cout << "np_client" <<  sid << std::endl;
   std::string cmd_line;
  std::ifstream input_file("./test_case/"+cmd_file);
  if(input_file.is_open()){
      while ( getline (input_file, cmd_line)){
      cmds.push_back(cmd_line);
    }
    input_file.close();
  }
  else{
      std::cout << "<p> can't open " << cmd_file << "</p>" << std::endl;
      return 1;
  }
  session_id = sid;
//   boost::asio::signal_set signals(ioservice, SIGINT, SIGTERM);
//   signals.async_wait(handler);
  

  tcp::resolver::query q{ip, port};
  resolv.async_resolve(q, resolve_handler);
  ioservice.run();
}


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

void parse_query(std::string query_str){
  std::stringstream ss;
  ss.str(query_str);
  std::string str;

  std::vector<std::string> tokens;
  boost::split( tokens, query_str, boost::is_any_of( "&" ), boost::token_compress_on);
  for( std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it ){
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
}


int main(int argc, char** argv) {
    std::cout << "Content-type: text/html" << std::endl << std::endl;
    char* ptr = getenv("QUERY_STRING");
    std::cout << "<p>" << ptr << "</p>" << std::endl;
    std::string query_str(ptr);
    parse_query(query_str);

    // for (size_t i = 0; i < host_num; i++){
    //   std::cout << host_name[i] << std::endl;
    //   std::cout << host_port[i] << std::endl;
    //   std::cout << file_name[i] << std::endl;
    // }

    // exit(0);

    char default_path[] = "PATH=/bin:bin:.:/usr/bin/env:/home/cysun/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin";
    putenv(default_path);

    std::cout << "Content-type: text/html" << std::endl << std::endl;
    std::cout << webpage_template << std::endl;

    std::vector<pid_t> session_pid;

    for (size_t i = 0; i < 3; i++){
        // np_client("127.0.0.1", "124"+std::to_string(i), "t1.txt", std::to_string(i));
        pid_t pid = fork();
        if(pid == 0){ // child
            // np_client("127.0.0.1", "124"+std::to_string(i), "t1.txt", std::to_string(i));
            
            execlp("np_client", "np_client", host_name[i].c_str(), host_port[i].c_str(), file_name[i].c_str(), std::to_string(i).c_str(), NULL);
            std::cout << "can't execute" << std::endl;
            exit(0);
        }
        session_pid.push_back(pid);
    }

    int status;
    wait(&status);
    wait(&status);
    wait(&status);
    // waitpid(session_pid[0], &status, 0);
    // waitpid(session_pid[1], &status, 0);
    // waitpid(session_pid[2], &status, 0);

    
    
    
    // np_client("127.0.0.1", "1237", "t1.txt", "1");

    return 0;
}