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
using namespace std;


string& ltrim(string& inputline, const string& chars = "\t\n\v\f\r ")
{
    inputline.erase(0, inputline.find_first_not_of(chars));
    return inputline;
}
 
string& rtrim(string& inputline, const string& chars = "\t\n\v\f\r ")
{
    inputline.erase(inputline.find_last_not_of(chars) + 1);
    return inputline;
}
 
string& trim(string& inputline, const string& chars = "\t\n\v\f\r ")
{
    return ltrim(rtrim(inputline, chars), chars);
}