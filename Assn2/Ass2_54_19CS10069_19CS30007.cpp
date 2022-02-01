#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <map>
#include <fstream>
#include <cassert>
#include <deque>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

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
    if(signum == SIGINT)
    {
       printf("\nlol want to exit\n");
       return;
    } else if(signum == SIGTSTP)//ctrl+R ka bhi puch le
    {
        printf("\nlol want to stop\n");
        return;
    }
    exit(signum);
}

// shell history:
const string bashHistoryFile = "bash_history.txt";
deque <string> bashHistory;
const int HISTORY_SIZE = 10'000;
const int DISP_HISTORY = 1'000;

void parse_history() {
    // accha thik hai
    // redirect kar loon? ok fopen wala c++ fin<< jaisa rehta hai
    ifstream infile;
    infile.open(bashHistoryFile);
    if(infile.fail()) {
        cerr << "Failed to open bash history file!\n";
        return;
    }

    string temp;
    while(getline(infile, temp))
        bashHistory.push_back(temp);
    
    assert((int)bashHistory.size() <= HISTORY_SIZE);

    infile.close();
    return;
}

void update_history() {
    ofstream fout;
    fout.open(bashHistoryFile, ofstream::trunc);
    if(fout.is_open())
    {
        for(auto cmd: bashHistory) {
            fout << cmd << '\n';
        }
        // thik hai
    }
    else{
        cerr<<"Error while opening\n";
    }
    fout.close();
    return;
}

// ab display history
void add_history(const vector<string> &tokens) {
    string cmd = "";
    int n = (int) tokens.size();
    for(int i = 0; i < n; ++i)
    {
        cmd = cmd + (tokens[i]);
        if(i < n-1)
        {
            cmd.push_back(' ');
        }
    }
    bashHistory.push_back(cmd);
    
    // pop the first element if size limit is exceeded
    if((int)bashHistory.size() > HISTORY_SIZE)
        bashHistory.pop_front();
    return;
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
    
    // Haan file mein
    return;
    // shell builtin.. so builtin function mein hoga?
    // Naa child mein hi.. read write bhi karna padega
    // Maybe append bhi
    // so har ek command ke baad change karna padega
    
    // Implementation details sahi hai
    // Pehle ek file se read karenge
    // Uske baad ek vector mein rakhenge
    // Then vector mein hi sab operation karenge
    // At the end waapas file mein daalke exit kardenge
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
        if(tokens.back() != "&") {
            // parent waits for the child
            int status;
            do{
                int wpid = waitpid(fork_status,&status,WUNTRACED);
            }while(!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));
        }
        else
        {
            printf("Not waiting\n");
        }
    }
}

// This is called after we fork a child
void childExecutes(vector<string> tokens, int in_fd, int out_fd) {
    // cerr << "Inside child now!\n";

    // cerr << "Just a check on tokens sent\n";

    // for(auto u: tokens)
    //     cerr << u << " ";
    // cerr << '\n';

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

    int n = (int)tokens.size();
    int lenOfMainCommand = n;

    vector<string> reqTokens;
    bool ignoreNext = false;

    for(int i = 0; i < n; i++) {
        string token = tokens[i];
        // cerr << "At " << i << " " << tokens[i] << '\n';
        if(token == ">") {
            ignoreNext = true;
            if(lenOfMainCommand == n)
                lenOfMainCommand = i;
            if(i+1 < n){
                int new_out_fd = open(tokens[i+1].c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666);
                if(new_out_fd < 0) {
                    cerr << "Error in opening file for writing!\n";
                } else {
                    dup2(new_out_fd, STDOUT_FILENO);
                }
            }
        } else if(token == "<") {
            ignoreNext = true;
            if(i+1 < n){
                int new_in_fd = open(tokens[i+1].c_str(), O_RDONLY);
                if(new_in_fd < 0) {
                    cerr << "Error in open file for reading\n";
                } else {
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
    exit(0);
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
    
    // ye tu karde? thik parse and update kar deta hu
    // write back to the file here
    update_history();
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
    int end = min((int)bashHistory.size(), 1000);
    int start = max(0, end - 1000);
    for(int i = start; i< end; ++i)
    {
        cout << i << ". " << bashHistory[i] << "\n";
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

    // parse history here for the first time
    parse_history();
    vector<string> inp;

    while(1) {
        cout << ">>>";
        readline(inp);
        add_history(inp);
        splitCommands(inp);
        inp.clear();
        // fflush(stdin);
        // fflush(stdout);
    }
    return 0;
}
