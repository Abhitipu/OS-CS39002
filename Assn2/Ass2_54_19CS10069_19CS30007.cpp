#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <map>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

const int HISTORY_SIZE = 10'000;
const int DISP_HISTORY = 1'000;

using namespace std;

// Useful for the inbult shell functions
map<string, function<void(vector<string>)>> builtInFunctions;

// function declaration for better program flow
void childExecutes(vector<string> tokens, int in_fd, int out_fd);
void execute(vector<string> tokens, int in_fd=0, int out_fd=1);

// This is for detecting ctrl + C and ctrl + Z signals
void signal_callback_handler(int signum) {
   cerr << "Caught signal " << signum << endl;
   // Terminate program
   exit(signum);
}

// We read a line in the shell using this function
void readline(vector<string> &tokens) {
    string temp = "";

    while(true) {
        char c;
        cin.get(c);

        if(c == '\n')
            break;

        // tabs
        if(c == '\t') {
            cerr << "Tabbed! Feature yet to be added!\n";
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

    // for(auto word: tokens)
    //     cout << word << " ";
    // cout << '\n';
    return;
}

// Here we split on a pipe and then manage the input and output file descriptors
void splitCommands(vector<string> tokens) {
    vector<string> temp;

    int in_fd = 0, out_fd = 1;
    int pipefd[2];
    for(auto token: tokens) {
        if(token == "|") {
            if(pipe(pipefd) == -1)
            {
                cerr << "Error in pipe!\n";
                break;
            }
            // send output to pipefd[1], 
            // later catch that from pipefd[0]
            
            execute(temp, in_fd, pipefd[1]);
            close(pipefd[1]);
            in_fd = pipefd[0];
            temp.clear();
        } else
            temp.push_back(token);
    }
    
    if(!temp.empty())
        execute(temp, in_fd, out_fd);
    return;
}

// A wrapper where we distinguish between built in functions and those for which binaries are present
void execute(vector<string> tokens, int in_fd, int out_fd) {
    int nArgs = (int)tokens.size();
    
    if(nArgs < 1)
        return;

    if(builtInFunctions.count(tokens[0])) {
        builtInFunctions[tokens[0]](tokens);
        return;
    }

    int fork_status = fork();
    if(fork_status < 0) {
        cerr << "Error in fork!\n";
        exit(EXIT_FAILURE);
    }

    if(fork_status == 0) {
        childExecutes(tokens, in_fd, out_fd);
    } else {
        if(tokens.back() != "&")
        // parent waits for the child
            wait(NULL);     
    }
}

// This is called after we fork a child
void childExecutes(vector<string> tokens, int in_fd, int out_fd) {
    cerr << "Inside child now!\n";

    cerr << "Just a check on tokens sent\n";

    for(auto u: tokens)
        cerr << u << " ";
    cerr << '\n';

    if(in_fd!=0)
    {
        dup2(in_fd, 0);
        close(in_fd);
    }

    if(out_fd!=1)
    {
        dup2(out_fd, 1);
        close(out_fd);
    }

    int old_in_fd, old_out_fd;
    old_in_fd = old_out_fd = -1;

    int n = (int)tokens.size();
    int lenOfMainCommand = n;

    vector<string> reqTokens;
    bool ignoreNext = false;

    for(int i = 0; i < n; i++) {
        string token = tokens[i];
        cerr << "At " << i << " " << tokens[i] << '\n';
        // ./a.out < input.txt > output.txt < redier.txt -n
        if(token == ">") {
            ignoreNext = true;
            if(lenOfMainCommand == n)
                lenOfMainCommand = i;
            cerr << "Hereeee\n";
            if(i+1 < n){
                int new_out_fd = open(tokens[i+1].c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666);
                cerr << "Opened!\n";
                if(new_out_fd < 0) {
                    cerr << "Error in opening file for writing!\n";
                } else {
                    cerr << "Started dup! " << tokens[i+1] << " \n";
                    old_out_fd = dup(STDOUT_FILENO);
                    dup2(new_out_fd, STDOUT_FILENO);
                    cerr << "Done dup!\n";
                }
            }
        } else if(token == "<") {
            ignoreNext = true;
            if(i+1 < n){
                int new_in_fd = open(tokens[i+1].c_str(), O_RDONLY);
                if(new_in_fd < 0) {
                    cerr << "Error in open file for reading\n";
                } else {
                    old_in_fd = dup(STDIN_FILENO);
                    dup2(new_in_fd, STDIN_FILENO);
                }
            }
        } else {
            if(ignoreNext == false)
                reqTokens.push_back(token);
            ignoreNext = false;
        }
    }

    n = (int)reqTokens.size();
    char **args = new char* [n + 1];
    /*
    cat temp.cpp > temp.txt
    after redirecting
    cat temp.cpp
    */
    for(int i = 0; i < n; i++)
    {
        args[i] = new char[(int)reqTokens[i].size()];
        strcpy(args[i], reqTokens[i].c_str());
    }
    args[n] = NULL;
    
    if(execvp(args[0], args) < 0) {
        cerr << "Error in execvp!\n";
    }

    if(old_out_fd != -1)
        dup2(old_out_fd, STDOUT_FILENO);
    
    if(old_in_fd != -1)
        dup2(old_in_fd, STDIN_FILENO);
    
    return;
}

void cdFunction(vector<string> tokens) {
    // chdir function!
    if((int)tokens.size() != 2) {
        cerr << "Error in chdir function!\n";
        return;
    }
    if(chdir(tokens[1].c_str())!=0)
    {
        cerr << "Error: directory not found\n";
        return;
    }
} 

void exitFunction(vector<string> tokens) {
    // exit from shell
    if((int)tokens.size() != 1) {
        cerr << "Error in exit function!\n";
        return;
    }
    exit(0);
}

void helpFunction(vector<string> tokens) {
    // help function
    if((int)tokens.size() != 1) {
        cerr << "Error in help function!\n";
        return;
    }
    cerr<<"Help Manual\n";
    return;
}

void historyFunction(vector<string> tokens) {
    // history function
    if((int)tokens.size() != 1) {
        cerr << "Error in history function!\n";
        return;
    }
    return;
}

int main() {
    // Register signal callback handlers
    signal(SIGINT, signal_callback_handler);    // ctrl + C
    signal(SIGTSTP, signal_callback_handler);   // ctrl + Z

    // associating the builtin functions with the suitable commands
    builtInFunctions["cd"] = cdFunction;
    builtInFunctions["help"] = helpFunction;
    builtInFunctions["exit"] = exitFunction;
    builtInFunctions["history"] = historyFunction;

    vector<string> inp;

    while(1) {
        cout << ">>>";
        readline(inp);
        splitCommands(inp);
        inp.clear();
        fflush(stdin);
        fflush(stdout);
    }
    return 0;
}
