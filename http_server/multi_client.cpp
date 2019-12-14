#include <boost/asio.hpp>
#include <iostream>

using namespace boost::asio::ip;

class ClientSocket{
private:
    boost::asio::io_service io_service_;
    tcp::socket socket;

public:

    ClientSocket():
        socket(io_service_)
    {
        
    }

    void connect(std::string host, std::string port){
        boost::asio::ip::tcp::resolver resolver(io_service_);
        boost::asio::ip::tcp::resolver::query query( boost::asio::ip::tcp::v4(), host.c_str(), port.c_str() );
        boost::asio::ip::tcp::resolver::iterator iterator;

        std::cout << "resolving " << host << "\n";
        iterator = resolver.resolve(query);

        std::cout << "connecting to " << host << "\n";
        boost::asio::connect(socket.lowest_layer(), iterator);

        std::cout << "connected \n";
    }
};

int main(int argc, char** argv) {
    ClientSocket s, s2;

    try{
        s.connect("echo.websocket.org","https");
        std::cout << "---------\n";
        s2.connect("stackoverflow.com","https");

    }catch(std::exception& e){
        std::cout << "Error: " << e.what();
        return 1;
    }

    return 0;
}