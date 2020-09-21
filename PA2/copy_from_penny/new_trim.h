#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h> 
#include <string.h>
#include <vector>
using namespace std;

vector<string> trim_line(string alpha) {
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
    bool special = false;
    // cout << beta << endl;
    for (char i:beta) {
        if (special) {
            special=false;
            temp+=i;
        }
        if (in_quote && i!='\"') {
            temp += i;
            continue;
        }
        else if (in_quote && i=='\"') {
            in_quote = false;
            continue;
        }
        else if (in_quote && i=='\\') {
            special = true;
            temp += i;
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
            if (temp.length()>0)
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

vector<vector<string>> trim_pipe(string alpha) {
    vector<vector<string>> charlie;
    int pre_pos = 0;
    alpha += "   |";
    int pos = alpha.find("|", 0);
    while (pos < alpha.length()) {
        vector<string> temp = trim_line(alpha.substr(pre_pos, pos-pre_pos));
        charlie.push_back(temp);
        pre_pos = pos + 1;
        pos = alpha.find("|", pre_pos);
    }
    return charlie;
}

int search_vector(vector<string>& a, string b) {
    for (int i=0; i<a.size(); i++)
        if (a[i] == b) {
            return i;
        }
    return -1;
}