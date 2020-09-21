#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h> 
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
using namespace std;

int main() {
    cout << "true" << endl;
    char* const argv[] = {"-al", nullptr};
    execvp("ls", argv);
    cout << "troll" << endl;
    char* const argv1[] = {"-l", nullptr};
    execvp("ls", argv1);
    return 0;
}