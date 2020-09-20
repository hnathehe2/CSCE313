#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h> 
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;
#include "trim.h"

#define MAXCOM 1000 // max number of letters to be supported 
#define MAXLIST 100

int execFlag(string input_line) {
    int option = 0;
    for (char i: input_line) {
        if (i=='|') {
            return 2;
        }
    }
    // return 0 do nothing
    // return 1 do exec one line (normal case)
    // return 2 do pipelining
    // return 3 do exec one line with redirection
    return 1;
    return 0;
}

char** vec_to_char_array(vector<string>& input_vector) {    
    int pos = -1;
    for (int i=0; i< input_vector.size(); i++) {
        if (input_vector[i]==">") {
            pos = i;
        }
    }
    if (pos > 0) {
        string filename = input_vector[pos+1];
        input_vector.erase (input_vector.begin() + pos + 1);
        input_vector.erase (input_vector.begin() + pos);
        int fd = open(filename.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
        dup2(fd, 1);
        close(fd);
    }

    pos = -1;
    for (int i=0; i< input_vector.size(); i++) {
        if (input_vector[i]=="<") {
            pos = i;
        }
    }
    if (pos > 0) {
        string filename = input_vector[pos+1];
        input_vector.erase (input_vector.begin() + pos + 1);
        input_vector.erase (input_vector.begin() + pos);
        int fd = open(filename.c_str(), O_RDONLY | O_CREAT, S_IWUSR | S_IRUSR);
        dup2(fd, 0);
        close(fd);
    }

    
    
    char** result = new char* [input_vector.size()+1];
    for (int i=0; i<input_vector.size(); i++) {
        result[i] = (char*) input_vector[i].c_str();
    }
    result[input_vector.size()] = NULL;
    return result;
} 

void run_one_line(char** exec_string) {
    //cout << exec_string[0] << endl;
    // if (exec_string[0]=="cd") {
    //     //change to chdir() 
    //     return ; // delete this when done
    // }
    execvp(exec_string[0],exec_string);
    return;
}


int main() {
    while (true) {
        cout <<  "My shell$ ";
        string input_line;
        getline(cin,input_line);
        if (input_line == string("exit")) {
            cout << "ending .... bye" << endl;
            break;
        }

        int pid = fork();
        if (pid == 0) {
            char* exec_string[MAXLIST];
            int exec_flag = execFlag(input_line.c_str());
            
            if (exec_flag==1) {
                vector<string> input_vector = trim_line(input_line);
                char** exec_string = vec_to_char_array(input_vector);
                run_one_line(exec_string);
            }
            else {
                vector<vector<string>> input_vector_vector = trim_pipe(input_line);
            }
        }
        else {
            waitpid(pid,0,0);
        }
    }
    return 0;
} 