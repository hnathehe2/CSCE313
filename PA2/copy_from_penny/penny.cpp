#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include "trim.h"

using namespace std;
char** vec_to_char_array(vector<string> &c) {
    char** result = new char* [c.size()+1]; // 1 for NULL
    result[c.size()] = NULL;

    for(int i = 0; i < c.size(); ++i) {
        // cout << c[i] << endl;
        result[i] = (char*) c[i].c_str();
    }
    
    return result;
}



vector<string> split(string c, char delimiter) { 
  c.push_back(delimiter);
  vector<string> store;
  string temp;
  for(int i = 0; i < c.size(); ++i) {
    if (c[i] != delimiter) {
      temp.push_back(c[i]);
    }  
    else {
      temp = trim(temp);
      store.push_back(temp);     
      temp.clear();
    }   
  }  
  return store;
} 

int main() {
    dup2(0,3);
    dup2(1,4);
    
    vector<int> back_grounds; //list of back_grounds
    while (true) {
        //check out background output before print out Shell -avoid zoombie processes
        for(int i = 0; i < back_grounds.size(); ++i) {
            if (waitpid(back_grounds[i], 0, WNOHANG) < 0) {
                ;
            }
            else {
                back_grounds.erase(back_grounds.begin() + i);
                i--;
            }
            
        }
    
        cout << "Shell$ ";
        string input_line;
        dup2(3,0);
        dup2(4,1);
        getline (cin, input_line);
        input_line = trim(input_line);
        if (input_line.find("exit") == 0){
            cerr << "Bye!! End of shell" << endl;
            break;
        }
        //cd
        
        if(input_line.find("cd") == 0) {
            string dir_cd = split(input_line, ' ')[1];
            dir_cd = trim(dir_cd);
            chdir (dir_cd.c_str());
            continue;
        }

        vector<string> pparts = split(input_line,'|');

        for (int i = 0; i < pparts.size(); ++i) {
            string input_pipe_line = pparts[i];
            
            int fds[2];
            pipe(fds);
            int pid = fork();

            //background process
            bool back_ground = false;

            //to get risk of the spaces before and after the command
            input_pipe_line = trim(input_pipe_line);
            if (input_pipe_line[input_pipe_line.size() - 1] == '&') {
                cout << "Background process init" << endl;
                back_ground = true;
                input_pipe_line = input_pipe_line.substr(0, input_pipe_line.size()-2);
            }

            if (pid == 0) {      
                int pos = input_pipe_line.find('>');
                if (pos >= 0){
                    string command = input_pipe_line.substr(0, pos);  // command = ls >
                    string file_name = input_pipe_line.substr(pos + 1); // file_name = a.txt
                    file_name = trim(file_name);
                    command = trim(command);
                    input_pipe_line = command;
                    int fd = open(file_name.c_str(), O_WRONLY | O_CREAT);
                    dup2(fd,1);
                    close(fd);
                }   

                pos = input_pipe_line.find('<');
                if (pos >= 0){
                    string command = input_pipe_line.substr(0, pos); 
                    string file_name = input_pipe_line.substr(pos + 1);
                    file_name = trim(file_name);
                    command = trim(command);
                    input_pipe_line = command;
                    int fd = open(file_name.c_str(), O_RDONLY);
                    dup2(fd,1);
                    close(fd);
                }   

                if (i < pparts.size() - 1) {
                    dup2(fds[1], 1);
                }

                vector<string> c = split(input_pipe_line,' ');
                char** args = vec_to_char_array(c);
                
                
                execvp(args[0], args); //execute command in this level
            }
            else {
                if (!back_ground) { 
                    waitpid(pid,0,0);
                    dup2(fds[0], 0);
                    close(fds[1]);
                }
                else {
                    back_grounds.push_back(pid);
                }        
            }
        } 
    } 
    return 0;  
}

 