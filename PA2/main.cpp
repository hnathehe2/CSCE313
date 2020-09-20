#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h> 
#include <string.h>
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

void create_exec_string(string input_line, char** exec_string) { 
	int i;
    char* temp;
    temp = const_cast<char*>(input_line.c_str());
	for (i = 0; i < MAXLIST; i++) {
		exec_string[i] = strsep(&temp, " ");
		if (exec_string[i] == NULL)
			break;
		if (strlen(exec_string[i]) == 0)
			i--;
	}
    return;
} 

void run_one_line(char** exec_string) {
    //cout << exec_string[0] << endl;
    if (exec_string[0]=="cd") {
        //change to chdir() 
        return ; // delete this when done
    }
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
            // char* args[] = {(char*) input_line.c_str(), NULL};
            // cout << *args << endl;
            char* exec_string[MAXLIST];
            char* exec_pipe[MAXLIST];
            int exec_flag = execFlag(input_line.c_str());
            if (exec_flag==1) {
                //create exec_string
                create_exec_string(input_line, exec_string);
                run_one_line(exec_string);
            }
            else {
                //create exec_pipe
                cout << "work in progress" << endl;
            }
            //execvp(args[0],args);
        }
        else {
            waitpid(pid,0,0);
        }
    }
    return 0;
} 