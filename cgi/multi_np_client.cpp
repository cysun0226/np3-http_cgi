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

#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;

io_service ioservice;

class ShellSession : public std::enable_shared_from_this<ShellSession> {
    private:
        tcp::resolver _resolv{ioservice};
        tcp::socket _socket{ioservice};
        tcp::resolver::query _query;
        enum { max_length = 4096 };
        std::array<char, 4096> bytes;
        std::vector<std::string> cmds;
        std::array<char, 4096> _cmd_buffer;
        int cmd_idx;
        std::string session_id;
        std::string host_ip;
        std::string host_port;
        std::string cmd_file;
        
    public:
        // constructor
        ShellSession(std::string host_ip, std::string host_port, std::string cmd_file, std::string sid)
            : _query{host_ip, host_port},
              cmd_file(cmd_file),
              session_id(sid),
              cmd_idx(0)
        {
            if (read_cmd_from_file()){
                do_resolve();
            }
        }
        // void start() { do_resolve(); }
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
        }

        void do_send_cmd(){
              _socket.async_send(
                buffer(cmds[cmd_idx]+"\r\n"),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
                        cmd_idx++;
                        do_read();
                    } 
                });
        }

        void do_read() {
            _socket.async_read_some(
                buffer(bytes, max_length),
                [this](boost::system::error_code ec, size_t length) {
                if (!ec){
                    std::string recv_str(bytes.data());

                    std::cout << recv_str;
                    std::cout.flush();
                    bytes.fill(0);
                    
                    // find prompt, send command
                    if (recv_str.find("% ") != std::string::npos){ // find prompt
                        usleep(100000);
                        
                        std::string r = cmds[cmd_idx] + "\r\n";
                        std::cout << cmds[cmd_idx] << std::endl;
                        std::cout.flush();

                        usleep(100000);
                        do_send_cmd();
                    }
                    else{
                        // std::cout.write(bytes.data(), bytes_transferred);
                        // std::cout.flush();
                        do_read();
                    }
                }
                else{
                    std::cerr << "(connection close)" << std::endl;
                }
            });
        }

        void do_connect(tcp::resolver::iterator it){
            _socket.async_connect(*it, [this](boost::system::error_code ec){
                if (!ec){
                    do_read();
                }
                else{
                    std::cerr << "can't connect to np server" << std::endl;
                }
            });
        }

         void do_resolve() {
            _resolv.async_resolve(_query, [this](boost::system::error_code ec, tcp::resolver::iterator it){
                if(!ec){
                    do_connect(it);
                }
                else{
                    std::cerr << "can't connect to np_server" << std::endl;
                }
            });
        }
};

int main(){
    ShellSession s1("127.0.0.1", "1240", "t1.txt", "1");
    ShellSession s2("127.0.0.1", "1241", "t1.txt", "1");
    ioservice.run();
}