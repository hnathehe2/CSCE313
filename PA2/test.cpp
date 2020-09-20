#include "trim.h"


void create_exec_string(vector<string>& input_vector, char** exec_string) { 
	int i;
    char* temp;
	for (int i=0; i<input_vector.size(); i++) {
        exec_string[i] = const_cast<char*>(input_vector[i].c_str());
    }
    return;
} 




int main() {
    while (true) {
        cout <<  "My shell$ ";
        string input_line;
        getline(cin,input_line);
        vector<string> input_vector = trim(input_line);

        char* store[100];
        create_exec_string(input_vector, store);
    }
    return 0;
} 

