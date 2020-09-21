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
#include "new_trim.h"

using namespace std;

char** vec_to_char_array(vector<string>& input_vector) {    
    char** result = new char* [input_vector.size()+1];
    for (int i=0; i<input_vector.size(); i++) {
        result[i] = (char*) input_vector[i].c_str();
    }
    result[input_vector.size()] = NULL;
    return result;
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
        vector<string> input_vector = trim_line(input_line);
        
        for (string i: input_vector) {
            if (i == "exit"){
            cerr << "Bye!! End of shell" << endl;
            break;
            }
        }

        for (int i=0; i<input_vector.size(); i++) {
            if(input_vector[i]=="cd") {
                string dir_cd = input_vector[i+1];
                chdir (dir_cd.c_str());
                continue;
            }
        }
        //cout << input_line << endl;
        vector<vector<string>> pparts = trim_pipe(input_line);
        //cout << pparts.size() << endl;
        for (int i = 0; i < pparts.size(); ++i) {
            vector<string> input_pipe_line = pparts[i];
            
            int fds[2];
            pipe(fds);
            int pid = fork();

            //background process
            bool back_ground = false;

            //to get risk of the spaces before and after the command
            
            if (input_pipe_line[input_pipe_line.size() - 1] == "&") {
                cout << "Background process init" << endl;
                back_ground = true;
                input_pipe_line.pop_back();
            }

            if (pid == 0) {      
                int pos = search_vector(input_pipe_line,">");
                if (pos >= 0){
                    string file_name = input_pipe_line[pos + 1];
                    cout << "output name " << file_name << endl;
                    input_pipe_line.erase (input_pipe_line.begin() + pos + 1);
                    input_pipe_line.erase (input_pipe_line.begin() + pos);
                    int fd = open(file_name.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
                    dup2(fd,1);
                    close(fd);
                }   

                pos = search_vector(input_pipe_line,"<");
                if (pos >= 0){
                    string file_name = input_pipe_line[pos + 1];
                    cout << "input name " << file_name << endl;
                    input_pipe_line.erase (input_pipe_line.begin() + pos + 1);
                    input_pipe_line.erase (input_pipe_line.begin() + pos);
                    int fd = open(file_name.c_str(), O_RDONLY | O_CREAT, S_IWUSR | S_IRUSR);
                    dup2(fd,0);
                    close(fd);
                }   

                if (i < pparts.size() - 1) {
                    dup2(fds[1], 1);
                }

                char** args = vec_to_char_array(input_pipe_line);
                // for (string i:input_pipe_line)
                //    cout << i << " ";
                // cout << endl;
                
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

 