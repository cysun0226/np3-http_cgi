#include <iostream>
#include <thread>
#include <unistd.h>
#include <wait.h>

using namespace std;

void test_func()
{
  execlp("ls", "ls", (char*) NULL);
}

int main( int argc, char** argv )
{
  // execute thread
  thread mThread(test_func);

  // do somthing
  cout << "main thread" << endl;

  // wait the thread stop
  mThread.join();

  return 0;
}