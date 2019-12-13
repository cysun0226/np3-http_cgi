#include <iostream>
#include <csignal>
#include <unistd.h>
#include <algorithm>
#include <boost/algorithm/string/replace.hpp>

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

void output_shell(std::string sid, std::string str){
    boost::replace_all(str, "\n", "&NewLine;");
    std::cout << "<script>document.getElementById('s" << sid << 
                 "').innerHTML += '" << str <<"';</script>" << std::endl;
}

void output_cmd(std::string sid, std::string cmd){
    boost::replace_all(cmd, "\n", "&NewLine");
    std::cout << "<script>document.getElementById('s" << 
    sid << "').innerHTML += '<b>" << cmd << ";</b>';</script>" << std::endl;
}


int main() {
    std::cout << "Content-type: text/html" << std::endl << std::endl;
    std::cout << webpage_template << std::endl;

    
    output_shell("0", "% ");
    output_cmd("0", "ls\n");
    output_shell("0", "bin\n");
    output_shell("0", "test.html\n");
    output_shell("0", "% ");
    output_cmd("0", "exit\n");
    
    output_shell("1", "% ");
    output_shell("2", "% ");
    sleep(1);
    output_cmd("1", "ls\n");
    sleep(1);
    output_shell("1", "bin\n");
    output_cmd("2", "ls\n");
    sleep(1);
    output_shell("1", "test.html\n");
    sleep(1);
    output_shell("1", "% ");
    output_shell("2", "bin\n");
    sleep(1);
    output_cmd("1", "exit\n");
    output_cmd("2", "exit\n");

    return 0;
}