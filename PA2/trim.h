#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h> 
#include <string.h>
#include <vector>
using namespace std;

vector<string> trim(string alpha) {
    bool in_quote = false;
    bool space_between = true;
    string beta;
    for (char i:alpha) {
        if (in_quote) {
            beta+=i;
            if (i=='\"') {
                in_quote = false;
            }
        }
        else {
            if (i==' ' || i == '\t') {
                if (!space_between) {
                    beta+=' ';
                    space_between = true;
                }
                continue;
            }
            else {
                beta+=i;
                space_between = false;
                if (i=='\"') {
                    in_quote = true;
                } 
            }
        }
    }
    vector<string> charlie;
    string temp="";
    in_quote = false;
    beta+=" ";
    // cout << beta << endl;
    for (char i:beta) {
        if (in_quote && i!='\"') {
            temp += i;
            continue;
        }
        else if (in_quote && i=='\"') {
            in_quote = false;
            continue;
        }
        else if (i!=' ' && i!='\"') {
            temp += i;
            continue;
        }
        else if (i=='\"') {
            in_quote = true;
            continue;
        }
        else if (i==' ') {
            charlie.push_back(temp);
            temp = "";
        }
    }

    //testing
    // cout << "-------------------" << endl;
    // for (string i: charlie)
    //     cout << "\"" << i << "\"" << endl;
    // cout << "-------------------" << endl;

    return charlie;
}