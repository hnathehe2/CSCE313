
#include "test_trim.h"

int main() {
    string alpha;
    getline(cin, alpha);
    vector<string> beta = trim_line(alpha);
    for (string i: beta)
        cout <<i << endl;
    return 0;
    
}