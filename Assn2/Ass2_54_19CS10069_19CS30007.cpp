#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include <vector>
#include <string>
#include <algorithm>

#include <signal.h>

const int HISTORY_SIZE = 10'000;
const int DISP_HISTORY = 1'000;

using namespace std;

void signal_callback_handler(int signum) {
   cout << "Caught signal " << signum << endl;
   // Terminate program
   exit(signum);
}

void readline(vector<string> &tokens) {
    string temp = "";

    while(true) {
        char c;
        cin.get(c);

        int check = c;
        cout << check << " " << c << '\n';

        if(c == '\n')
            break;

        // tabs
        if(c == '\t') {
            cout << "Tabbed! Feature yet to be added!\n";
            // auto complete
            // if one choice : print

            // if more : list
            // 1. 
            // 2. 

            // User will select : print
        } else if(c == ' ') {
            if(!temp.empty()) {
                tokens.push_back(temp);
                temp = "";
            }
        } else if(c == '>' || c == '<' || c == '|' || c == '&') {
            if(!temp.empty()) {
                tokens.push_back(temp);
                temp = "";
            }
            temp += c;
            tokens.push_back(temp);
            temp = "";
        } else {
            temp += c;
        } 
    }

    if(!temp.empty())
        tokens.push_back(temp);

    for(auto word: tokens)
        cout << word << " ";
    cout << '\n';
    return;
}

void execute(vector<string> tokens) {
    int nArgs = (int)tokens.size();
    string temp;
    for(auto u: tokens)
        temp += u + " ";
    
    cout << temp << '\n';
    system(temp.c_str());
}

void splitCommands(vector<string> tokens) {
    vector<string> temp;
    for(auto token: tokens) {
        if(token == "|") {
            execute(temp);
            temp.clear();
        } else
            temp.push_back(token);
    }
    if(!temp.empty())
        execute(temp);
    return;
}


int main() {
    // Register signal callback handlers
    signal(SIGINT, signal_callback_handler);    // ctrl + C
    signal(SIGTSTP, signal_callback_handler);   // ctrl + Z

    vector<string> inp;

    while(1) {
        cout << ">>>";
        readline(inp);
        splitCommands(inp);
        inp.clear();
    }
    return 0;
}
