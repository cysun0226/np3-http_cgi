#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sstream>
#include <fstream>

#include <string>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <sys/types.h>
#include <windows.h>

#include <boost/algorithm/string/replace.hpp>
#include <thread>
#include <boost/thread.hpp>
#include <boost/asio/signal_set.hpp>

#define MAX_SESSION 5

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

io_service global_io_service;
io_service np_io_service;
unsigned short port;
std::string server_addr;
std::string query_str;

typedef struct {
  std::string host_name[MAX_SESSION];
  std::string host_port[MAX_SESSION];
  std::string file_name[MAX_SESSION];
  int host_num = 0;
} NpSession;

std::string change_escape(std::string cmd){
  boost::replace_all(cmd, "&", "&amp;");
  boost::replace_all(cmd, "\"", "&quot;");
  boost::replace_all(cmd, "\'", "&apos;");
  boost::replace_all(cmd, "<", "&lt;");
  boost::replace_all(cmd, ">", "&gt;");
  boost::replace_all(cmd, "\n", "&NewLine;");
  boost::replace_all(cmd, "\r", "");
  return cmd;
}

std::string output_command(std::string sid, std::string cmd){
    cmd = change_escape(cmd);
    // boost::replace_all(cmd, "\n", "&NewLine");
    std::stringstream ss;
    ss << "<script>document.getElementById('s" << 
    sid << "').innerHTML += '<b>" << cmd << "&NewLine;</b>';</script>" << std::endl;
    return ss.str();
}

std::string output_prompt(std::string sid){
    std::stringstream ss;
    ss << "<script>document.getElementById('s" << 
    sid << "').innerHTML += '<b>" << "% " << "</b>';</script>" << std::endl;
    return ss.str();
}

std::string output_shell(std::string sid, std::string str){
    str = change_escape(str);
    if(str == ""){
        return str;
    }
    std::stringstream ss;
    ss << "<script>document.getElementById('s" << sid << 
                 "').innerHTML += '" << str <<"';</script>" << std::endl;
    return ss.str();
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
)";



          
std::string generate_webpage(NpSession ns){
  std::vector<std::string>  host_cols;
  std::vector<std::string>  table_tds;

  const std::string footer_1 = "</tr>\n</thead>\n<tbody>\n<tr>";
  const std::string footer_2 = "</tr>\n</tbody>\n</table>\n</body>\n</html>";

  std::string page = webpage_template;

  for (size_t i = 0; i < ns.host_num; i++){
      host_cols.push_back("<th scope=\"col\">"+ns.host_name[i]+":"+ns.host_port[i]+"</th>");
      table_tds.push_back("<td><pre id=\"s"+std::to_string(i)+"\" class=\"mb-0\"></pre></td>");
  }

  for (size_t i = 0; i < ns.host_num; i++){
    std::cout << ns.host_num << std::endl;
    page += host_cols[i];
  }
  page += footer_1;
  for (size_t i = 0; i < ns.host_num; i++){
    page += table_tds[i];
  }
  page += footer_2;
  return page;
}

std::string panel_html = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>NP Project 3 Panel</title>
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
      href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png"
    />
    <style>
      * {
        font-family: 'Source Code Pro', monospace;
      }
    </style>
  </head>
  <body class="bg-secondary pt-5">
    <form action="console.cgi" method="GET">
      <table class="table mx-auto bg-light" style="width: inherit">
        <thead class="thead-dark">
          <tr>
            <th scope="col">#</th>
            <th scope="col">Host</th>
            <th scope="col">Port</th>
            <th scope="col">Input File</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <th scope="row" class="align-middle">Session 1</th>
            <td>
              <div class="input-group">
                <select name="h0" class="custom-select">
                  <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nctu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p0" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f0" class="custom-select">
                <option></option>
                <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option><option value="t6.txt">t6.txt</option><option value="t7.txt">t7.txt</option>
              </select>
            </td>
          </tr>
          <tr>
            <th scope="row" class="align-middle">Session 2</th>
            <td>
              <div class="input-group">
                <select name="h1" class="custom-select">
                  <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nctu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p1" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f1" class="custom-select">
                <option></option>
                <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option><option value="t6.txt">t6.txt</option><option value="t7.txt">t7.txt</option>
              </select>
            </td>
          </tr>
          <tr>
            <th scope="row" class="align-middle">Session 3</th>
            <td>
              <div class="input-group">
                <select name="h2" class="custom-select">
                  <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nctu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p2" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f2" class="custom-select">
                <option></option>
                <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option><option value="t6.txt">t6.txt</option><option value="t7.txt">t7.txt</option>
              </select>
            </td>
          </tr>
          <tr>
            <th scope="row" class="align-middle">Session 4</th>
            <td>
              <div class="input-group">
                <select name="h3" class="custom-select">
                  <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nctu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p3" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f3" class="custom-select">
                <option></option>
                <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option><option value="t6.txt">t6.txt</option><option value="t7.txt">t7.txt</option>
              </select>
            </td>
          </tr>
          <tr>
            <th scope="row" class="align-middle">Session 5</th>
            <td>
              <div class="input-group">
                <select name="h4" class="custom-select">
                  <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nctu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p4" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f4" class="custom-select">
                <option></option>
                <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option><option value="t6.txt">t6.txt</option><option value="t7.txt">t7.txt</option>
              </select>
            </td>
          </tr>
          <tr>
            <td colspan="3"></td>
            <td>
              <button type="submit" class="btn btn-info btn-block">Run</button>
            </td>
          </tr>
        </tbody>
      </table>
    </form>
  </body>
</html>
)";

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
    // setenv("REQUEST_METHOD", req_method.c_str(), 1);
    // setenv("REQUEST_URI", req_uri.c_str(), 1);
    // setenv("QUERY_STRING", query_str.c_str(), 1);
    // setenv("HTTP_HOST", http_host.c_str(), 1);
    // // setenv(// set_env_str = "SERVER ADDR="+serve.c_str(), 1)r
    // setenv("SERVER_PORT", std::to_string(port).c_str(), 1);
    // setenv("SERVER_PROTOCOL", server_protocol.c_str(), 1);

    return req_uri;
}

NpSession parse_query(std::string query_str){
  std::stringstream ss;
  ss.str(query_str);
  std::string str;
  NpSession ns;

  std::vector<std::string> tokens;
  boost::split( tokens, query_str, boost::is_any_of( "&" ), boost::token_compress_on);
  for( std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it ){
    if(it->at(0) == 'h'){
      std::string hid = it->substr(0, it->find("="));
      ns.host_name[atoi(hid.substr(1).c_str())] = it->substr(it->find("=")+1);
      if(it->substr(it->find("=")+1).size()>1){
        ns.host_num++;
      }
    }
    if(it->at(0) == 'p'){
      std::string pid = it->substr(0, it->find("="));
      ns.host_port[atoi(pid.substr(1).c_str())] = it->substr(it->find("=")+1);
    }
    if(it->at(0) == 'f'){
      std::string fid = it->substr(0, it->find("="));
      ns.file_name[atoi(fid.substr(1).c_str())] = it->substr(it->find("=")+1);
    }
  }

  return ns;
}


class EchoSession;

class ShellSession : public std::enable_shared_from_this<ShellSession> {
    private:
        tcp::resolver _resolv{global_io_service};
        tcp::socket _socket{global_io_service};
        tcp::resolver::query _query;
        std::vector<std::string> cmds;
        std::array<char, 4096> _cmd_buffer;
        int cmd_idx;
        std::string session_id;
        int s_id;
        std::string host_ip;
        std::string host_port;
        std::string cmd_file;
        enum { max_length = 4096 };
        std::array<char, 4096> bytes;
        tcp::socket* _web_socket;
        std::shared_ptr<EchoSession> es_ptr;
        
    public:
        // constructor
        ShellSession(std::string host_ip, std::string host_port, 
        std::string cmd_file, std::string sid, std::shared_ptr<EchoSession> es,
        tcp::socket* client_socket)
            : _query{host_ip, host_port},
              cmd_file(cmd_file),
              session_id(sid),
              cmd_idx(0),
              s_id(std::atoi(sid.c_str())),
              es_ptr(es),
              _web_socket(client_socket)
              
        {
            read_cmd_from_file();
        }
        void start() { do_resolve(); }
    private:
        bool read_cmd_from_file(){
            std::string cmd_line;
            std::ifstream input_file("./test_case/"+std::string(cmd_file));
            if(input_file.is_open()){
                while ( getline (input_file, cmd_line)){
                    cmds.push_back(cmd_line);
                }
                input_file.close();
                return true;
            }
            else{
                std::cout << "<p> can't open " << cmd_file << "</p>" << std::endl;
                return false;
            }
			return false;
        }

        void do_write(std::string data){
          // std::cout << data << std::endl;
          // write(_web_socket, buffer(data));
          
          _web_socket->async_send(
                buffer(data),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
                      // do_read();
                    }
                    else{
                      std::cout << "async send failed" << std::endl;
                    }
                });
        }

        void do_send_cmd(){
              _socket.async_send(
                buffer(cmds[cmd_idx]+"\r\n"),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
                        cmd_idx++;
                        if(cmd_idx != cmds.size()){
                          do_read();
                        }
                    } 
                });
        }

        void do_read() {
            _socket.async_read_some(
                buffer(bytes, max_length),
                [this](boost::system::error_code ec, size_t length) {
                if (!ec){
                    std::string recv_str(bytes.data());
                    bytes.fill(0);
                    // std::cout << recv_str;
                    // std::cout.flush();

                    if (recv_str.find("%") != std::string::npos){  
                        // std::cout << "<script>document.getElementById('s0').innerHTML += '';</script>" << std::endl;
                        // std::cout << recv_str.substr(0, recv_str.find("%")) << std::endl;
                        do_write(output_shell(std::to_string(s_id), recv_str.substr(0, recv_str.find("%"))));
                        do_write(output_prompt(std::to_string(s_id)));
                    }
                    else{
                        // if(bytes[0] != 0)
                        // std::cout << "# get data" << std::endl;
                        // std::cout << recv_str << std::endl;
                        do_write(output_shell(std::to_string(s_id), recv_str));
                    }
                    
                    // find prompt, send command
                    if (recv_str.find("%") != std::string::npos){ // find prompt
                        Sleep(100);
                        
                        // std::cout << "find %, cmds.size() = " << cmds.size() << std::endl;
                        // std::cout << "s" << session_id << " : " << cmds[cmd_idx] << std::endl;
                        std::string r = cmds[cmd_idx] + "\r\n";
                        do_write(output_command(std::to_string(s_id), cmds[cmd_idx]));

                        Sleep(100);
                        do_send_cmd();
                    }
                    else{
                        // std::cout.write(bytes.data(), bytes_transferred);
                        // std::cout.flush();
                        // output_shell(session_id, recv_str);
                        do_read();
                    }
                }
                else{
                    // output_command(std::to_string(s_id), "(connection close)");
                }
            });
        }

        void do_connect(tcp::resolver::iterator it){
          // std::cout << "cmds.size() = " << cmds.size() << std::endl;
            _socket.async_connect(*it, [this](boost::system::error_code ec){
                if (!ec){
                    // read_cmd_from_file();
                    // std::cout << "cmds.size() = " << cmds.size() << std::endl;
                    do_read();
                }
                else{
                    std::cerr << "do_connect failed: can't connect to np server" << std::endl;
                }
            });
        }

         void do_resolve() {
          //  std::cout << "cmds.size() = " << cmds.size() << std::endl;
            _resolv.async_resolve(_query, [this](boost::system::error_code ec, tcp::resolver::iterator it){
                if(!ec){
                    do_connect(it);
                }
                else{
                    std::cerr << "do_resolve failed: can't connect to np_server" << std::endl;
                }
            });
        }
};

class EchoSession : public enable_shared_from_this<EchoSession> {
 private:
  enum { max_length = 1024 };
  ip::tcp::socket _socket;
  array<char, max_length> _data;
  // boost::asio::signal_set _signal;

 public:
  EchoSession(ip::tcp::socket socket) : _socket(move(socket)) {}

  void start() { do_read(); }

  void do_write(std::string msg) {
    // std::cout << msg << std::endl;
    auto self(shared_from_this());
    _socket.async_send(
        buffer(msg),
        [this, self](boost::system::error_code ec, size_t /* length */) {
          if(ec){
            std::cerr << "async send failed" << std::endl;
          }
        });
  }

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
            // setenv("REMOTE_ADDR", _socket.remote_endpoint().address().to_string().c_str(), 1);
            set_env_str = "REMOTE_PORT"+_socket.remote_endpoint().port();
            // setenv("REMOTE_PORT", std::to_string(_socket.remote_endpoint().port()).c_str(), 1);

            

            // reply to the client
            bool ok = true;
            // check if file exist
            // if (!boost::filesystem::exists("."+uri_file)){
                // write(_socket, buffer("HTTP/1.1 404 Not Fount\r\n"));
                // write(_socket, buffer("Content-type: text/html\r\n\r\n<h1>404 Not Found</h1>\r\n"));
                // ok = false;
            // }

            if(uri_file == "/favicon.ico"){
              ok = false;
            }

            if(uri_file == "/panel.cgi"){
                write(_socket, buffer("HTTP/1.1 200 OK\r\n"));
				do_write("Content-type: text/html\r\n\r\n");
				// write(_socket, buffer("Content-type: text/html\r\n\r\n<h1>Hello</h1>\r\n"));
                do_write(panel_html);
                
            }

            if(uri_file == "/console.cgi"){
                write(_socket, buffer("HTTP/1.1 200 OK\r\n"));
				do_write("Content-type: text/html\r\n\r\n");
                NpSession ns = parse_query(query_str);
                std::cout << "generate_webpage" << std::endl;
                do_write(generate_webpage(ns));
                std::cout << "generate_webpage finish" << std::endl;

                // ShellSession s1("127.0.0.1", "1240", "t1.txt", "0");
                // s1.start();
                std::vector<ShellSession> shell_sessions;
                for (size_t i = 0; i < ns.host_num; i++){
                  shell_sessions.push_back(ShellSession(
                    ns.host_name[i], ns.host_port[i], 
                    ns.file_name[i], std::to_string(i),
                    shared_from_this(),
                    &_socket
                    ));
                }
                for (size_t i = 0; i < ns.host_num; i++){
                  shell_sessions[i].start();
                }
                global_io_service.run();
                

                // boost::thread shell_thread(boost::bind(&boost::asio::io_service::run, boost::ref(np_io_service)));

                // threads.create_thread(boost::bind(&boost::asio::io_service::run, &np_io_service));
                // threads.join_all();
                // shell_thread.join();
                // shell_thread.detach();
                
                
            }
          }
          
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

  // handle escape char in panel_html
  // panel_html = change_escape(panel_html);


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
