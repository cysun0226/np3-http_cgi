clang++ server.cpp -o server -I /usr/local/include -L /usr/local/lib -std=c++11 -Wall -pedantic -pthread -lboost_system -lboost_filesystem
clang++ ../cgi/np_client.cpp -o np_client -I /usr/local/include -L /usr/local/lib -std=c++11 -Wall -pedantic -pthread -lboost_system -lboost_filesystem
