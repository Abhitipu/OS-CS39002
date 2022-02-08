#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <fstream>
#include <cassert>
#include <deque>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <dirent.h>
#include <time.h>

#define BACK_SPACE 127
#define CTRL_R 18
#define EXECVP_ERR 87

using namespace std;

void custom_exit(int exit_code = EXIT_SUCCESS);
void multiWatchHandler(vector<string>& tokens);

// non canonical input mode
// https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
// https://www.mkssoftware.com/docs/man5/struct_termios.5.asp
struct termios saved_attributes;

void reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode (void)
{
    struct termios tattr;
    char *name;

    /* Make sure stdin is a terminal. */
    if (!isatty (STDIN_FILENO))
    {
        fprintf (stderr, "Not a terminal.\n");
        custom_exit (EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr (STDIN_FILENO, &saved_attributes);
    // atexit (reset_input_mode);

    /* Set the terminal modes. */
    tcgetattr (STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
    tattr.c_cc[VMIN] = 1;   // 1 byte
    tattr.c_cc[VTIME] = 0;  // timer is NULL
    tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

// show current time
void showtime() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime (buffer,80,"Now it's %I:%M:%S%p.",timeinfo);
    // strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);
    cerr << buffer << '\n';
}

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
       printf("\nExit Command\n");
       return;
    } else if(signum == SIGTSTP)//ctrl+R ka bhi puch le
    {
        printf("\nStop Command\n");
        return;
    }
    custom_exit(signum);
}

// shell history:
const string bashHistoryFile = "bash_history.txt";
deque <string> bashHistory;
const int HISTORY_SIZE = 10'000;
const int DISP_HISTORY = 1'000;

// retrive data from bash_history file
void parse_history() {
    ifstream infile;
    infile.open(bashHistoryFile);
    if(infile.is_open()) {    
        string temp;
        while(getline(infile, temp))
            bashHistory.push_back(temp);
        
        assert((int)bashHistory.size() <= HISTORY_SIZE);

        infile.close();
    }
    return;
}

// update history after completion
void update_history() {
    ofstream fout;
    fout.open(bashHistoryFile, ofstream::trunc);
    if(fout.is_open())
    {
        for(auto cmd: bashHistory) {
            fout << cmd << '\n';
        }
        fout.close();
    }
    else{
        cerr<<"Error while opening\n";
    }
    return;
}

// add display history
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

// This returns the longest matching prefix (Tabs)
int match_substring(string& toMatch, string& filename) {
    int n = min(toMatch.length(), filename.length());
    int i;
    for(i = 0; i < n && toMatch[i] == filename[i]; i++);
    return i;
}

// This searches the directory for the given file
vector<string> search_directory(string toMatch) {
    vector<string> matches;
    DIR *dir;
    struct dirent *dp;
    string filename;
    dir = opendir(".");

    int best = 1;
    while ((dp=readdir(dir)) != NULL) {
        // printf("%s\n", dp->d_name);
        filename = dp->d_name;
        int cur = match_substring(toMatch, filename);
        if(cur == best) {
            matches.push_back(filename);
        } else if(cur > best) {
            best = cur;
            matches.clear();
            matches.push_back(filename);
        }
    }
    closedir(dir);
    return matches;
}

// The z algorithm used for string matching with linear complexity
vector<int> z_function(string s) {
    int n = (int) s.length();
    vector<int> z(n);
    for (int i = 1, l = 0, r = 0; i < n; ++i) {
        if (i <= r)
            z[i] = min (r - i + 1, z[i - l]);
        while (i + z[i] < n && s[z[i]] == s[i + z[i]])
            ++z[i];
        if (i + z[i] - 1 > r)
            l = i, r = i + z[i] - 1;
    }
    return z;
}

// Searching the bash history for the matching commands
vector<string> longest_substring_match(string toMatch) {
    set<string> matches;
    int best = 3;

    for(auto cmd: bashHistory) {
        // exact match
        if(cmd == toMatch) {
            matches.clear();
            matches.insert(cmd);
            break;
        }

        string temp = toMatch + "$" + cmd;
        int n = toMatch.length();
        int m = cmd.length();

        vector<int> z_arr = z_function(temp);

        assert((int)z_arr.size() == n + m + 1);
        int cur_best = 0;

        for(int i = n + 1; i < n + m + 1; i++)
            cur_best = max(cur_best, z_arr[i]);

        if(cur_best == best) {
            matches.insert(cmd);
        } else if(cur_best > best) {
            best = cur_best;
            matches.clear();
            matches.insert(cmd);
        }
    }
    return vector<string>{matches.begin(), matches.end()};
}

// We read a line in the shell using this function
void readline(vector<string> &tokens) {
    string temp = "";
    // More delimiters
    string symbols = "<>&|\",[]";
    
    while(true) {
        char c;
        cin.get(c);

        if(c == '\n'){
            cout << c;
            break;
        }
        // tabs
        else if(c == '\t') {
            vector<string> matches = search_directory(temp);

            int n = (int)matches.size(); 
            if(n > 0) {
                if(n == 1) {
                    // Single match
                    temp.clear();
                    tokens.push_back(matches[0]);
                    cout<<"\n>>>";
                    for(auto u : tokens)
                    {
                        cout<<u<<" ";
                    }
                } else {
                    // Multiple matches
                    cout << '\n';
                    for(int i = 0; i < n; i++) {
                        cout << i + 1 << ". " << matches[i] << "\n";
                    }
                    cout << '\n';
                    cout << "\n>>>";
                    string mychoice;
                    while(1) {
                        cout << "Enter your choice from 1 to " << n << ": ";
                        char c;
                        while(cin.get(c) && c != '\n') {
                            if((int)c == BACK_SPACE) {
                                // ignore backspaces
                            } else {
                                mychoice += c;
                                cout << c;
                            }
                        }
                        cout << '\n';
                        int choice;
                        try {
                            choice = stoi(mychoice);
                        } catch(exception& e) {
                            cout << "Invalid choice! " << e.what() << '\n';
                            continue;
                        }
                        // getchar();
                        if(choice < 1 || choice > n) {
                            cout << "Enter a valid choice\n";
                        } else {
                            temp.clear();
                            cout << ">>>";
                            for(auto u: tokens) {
                                cout << u << " ";
                            }
                            tokens.push_back(matches[choice - 1]);
                            cout << tokens.back() << ' ';
                            break;
                        }
                    }
                }
            } 
        } 
        else if(c == ' ') {
            // Space is out main delimiter
            if(!temp.empty()) {
                tokens.push_back(temp);
                temp.clear();
            }
            cout << c;
        } 
        else if(symbols.find(c) != string :: npos) {
            // The secondary delimiters
            if(!temp.empty()) {
                tokens.push_back(temp);
                temp.clear();
            }
            temp += c;
            tokens.push_back(temp);
            temp.clear();
            cout << c;
        } else if((int)c == CTRL_R) {
            // Search in bash history
            temp.clear();
            tokens.clear();
            cout << "\nEnter command for searching: ";
            string toSearch;
            while(1) {
                cin.get(c);
                if(c == '\n') {
                    break;
                } else if(c == BACK_SPACE) {
                    // ignore backspaces
                } else {
                    toSearch += c;
                    cout << c;
                }
            }
            cout << '\n';
            vector<string> matches = longest_substring_match(toSearch);
            
            if((int)matches.size() == 0) {
                // No matches
                cout << "No match for searched item in history\n";
                cout<< ">>>";
            } else {
                // Matches found
                int n = matches.size();
                int idx;
                if(n == 1) {
                    // Exactly one match
                    cout << "\n>>>" << matches[0] << " ";
                    idx = 0;
                    temp.clear();
                } else {
                    for(int i = 0; i < n; i++) {
                        cout << i + 1 << ". " << matches[i] << "\n";
                    }
                    cout << '\n';
                    while(1) {
                        cout << "Enter a choice from 1 to " << n << ": ";
                        string mychoice;
                        char c;
                        while(cin.get(c) && c != '\n') {
                            if(c == BACK_SPACE) {
                                // ignore
                            } else {
                                mychoice += c;
                                cout << c;
                            }
                        }
                        cout << '\n';
                        int choice;
                        try {
                            choice = stoi(mychoice);
                        } catch(exception& e) {
                            cout << "Invalid choice! " << e.what() << '\n';
                            continue;
                        }

                        if(choice < 1 || choice > n) {
                            cout << "Invalid choice!\n";
                        } else {
                            cout << "\n>>>";
                            temp.clear();
                            cout << matches[choice - 1] << ' ';
                            idx = choice - 1;
                            break;
                        }
                    }
                }

                string word;
                for(auto u: matches[idx]) {
                    if(u == ' ' && !word.empty()) {
                        tokens.push_back(word);
                        word.clear();
                    }
                    else
                        word += u;
                }
                if(!word.empty())
                    tokens.push_back(word);
            }

        } else if((int)c == 24) {
            // ctrl + X : Feature 1
            temp.clear();
            tokens.clear();
            cout << "\n>>>";
        } else {
            temp += c;
            cout << c;
        } 
    }

    if(!temp.empty())
        tokens.push_back(temp);
    
    return;
}

void splitCommands(vector<string> tokens) {
    vector<string> temp;
    if((int)tokens.size()>0 && tokens[0] == "multiWatch")
    {
        multiWatchHandler(tokens);
        return;
    }

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

// {"multiwatch" "[" "cmd1" "," "cmd2" "," ... "cmdN" "]"}
// Special function for handling multi watch
void multiWatchHandler(vector<string>& tokens) {
    // copy and paste the multiwatch handler here!\n
    vector<vector<string>> commands;
    int n = tokens.size();
    bool started = false;
    
    vector<string> temp;
    for(int i = 2; i < n - 1; i++) {
        string cur = tokens[i];
        if(cur == "\"") {
            if(started) {
                if(!temp.empty())
                    commands.push_back(temp);
            } else {
                temp.clear();
            }
            started = !started;
        } else if(cur != ",") {
            temp.push_back(cur);
        }
    }

    // map pid to command, map pid to fd (pipe [0])

    int cnt = 100;

    map <int, vector<string>> pid_to_cmd;
    map <int, int> pid_to_fd;
    
    for(auto cmd: commands) {
        int in_fd = 0, out_fd = 1;
        int pipefd[2];
            
        if(pipe(pipefd) == -1)
        {
            cerr << "Error in pipe!\n";
            break;
        }
        // send output to pipefd[1], 
        // later catch that from pipefd[0]
        // fork()
        int cid;
        if((cid = fork()) == 0){
            cout<<"Entered inside fork :  "<< cmd[0]<<"\n";
            childExecutes(cmd, in_fd, pipefd[1]);
            custom_exit(EXECVP_ERR); // this should be terminate or quick exit, we don't want to 
        }
        close(pipefd[1]);
        in_fd = pipefd[0]; // capture this in a map (pid => ) or vector
        pid_to_fd.insert({cid, in_fd});
        pid_to_cmd.insert({cid, cmd});
    }

    fd_set myfd, myfdcopy;
    FD_ZERO(&myfd);

    int max_fd = 0;
    for(auto key_value : pid_to_fd){
        FD_SET(key_value.second, &myfd);
        FD_SET(key_value.second, &myfdcopy);
        max_fd = max(max_fd, key_value.second);
    }

    while(cnt--) {
        if(pid_to_fd.empty())
        {
            cout<<"Successfully executed multiWatch\n";
            break;
        }

        // since select is a destructive call!
        myfd = myfdcopy;

        int select_status = select(max_fd + 1, &myfd, NULL, NULL, NULL);
        if(select_status < 0) {
            perror("Error in select!\n");
            custom_exit(-1);
        }

        vector <int> pids_to_remove;
        for(auto p : pid_to_fd) {
            int pid = p.first;
            int fd = p.second;

            if(FD_ISSET(fd, &myfd)) {
                char buf[1000];
                bzero(buf, sizeof(buf));
                int read_status = read(fd, buf, sizeof(buf)-1);
                if(read_status < 0) {
                    perror("Error in read!\n");
                    custom_exit(-1);
                }

                if(read_status == 0) {
                    FD_CLR(fd, &myfdcopy);
                    pids_to_remove.push_back(pid);
                    continue;
                }
                
                cout << '\n';
                for(auto u: pid_to_cmd[pid])
                    cout << u << " ";

                cout << ", ";
                showtime();
                for(int i = 0; i < 15; i++)
                    cout << "--";
                cout<<"\n";
                
                cout<<"\n"<<buf<<"\n";
                
                for(int i = 0; i < 15; i++)
                    cout << "--";
                cout << '\n';
            }
        }

        for(auto to_delete: pids_to_remove)
        {
            int fd = pid_to_fd[to_delete];
            if(kill(to_delete, SIGKILL) < 0)
            {
                perror("kill()");
            }
            close(pid_to_fd[to_delete]);
            pid_to_fd.erase(to_delete);
            pid_to_cmd.erase(to_delete);
        }
    }
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
        custom_exit(EXIT_FAILURE);
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
            if(WIFSTOPPED(status))
            {
                kill(fork_status, SIGCONT);
            }
        }
        else
        {
            printf("Not waiting\n");
        }
    }
}

// This is called after we fork a child
void childExecutes(vector<string> tokens, int in_fd, int out_fd) {
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
    for(int i = 0; i < n; i++)
    {
        args[i] = new char[(int)reqTokens[i].size()];
        strcpy(args[i], reqTokens[i].c_str());
    }
    args[n] = NULL;
    
    if(execvp(args[0], args) < 0) {
        cerr << "Error in execvp!\n";
        perror("Execvp()");
        custom_exit(EXECVP_ERR);
    }
    cout<<"This should never be printed\n";
    custom_exit(EXECVP_ERR);
    return;
}

// shell builtin cd function
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

// shell exit function
void exitFunction(vector<string> tokens) {
    // update_history();
    if((int)tokens.size() != 1) {
        cerr << "Error in exit function!\n";
        return;
    }
    custom_exit(0);
}

// shell inbuilt function help function 
void helpFunction(vector<string> tokens) {
    // help function
    if((int)tokens.size() != 1) {
        cerr << "Error in help function!\n";
        return;
    }
    cout<<"Help Manual\n";
    cout<<"1. history - shows last 1000 commands\n";
    cout<<"2. cd - change directory\n";
    cout<<"3. exit - exit the shell\n";
    cout<<"4. help - shows help manual\n";
    cout<<"5. ctrl-x - cancels the current typed command\n";
    return;
}

// shell builtin history function, prints last 1000 commands
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

// exit and based on exitcode call update_history and reset_input_mode
void custom_exit(int exit_code)
{
    if(exit_code != EXECVP_ERR)
    {
        update_history();
        reset_input_mode();
    }
    exit(exit_code);
}

// main function
int main() {
    // read inputs in a non canninical manner
    set_input_mode ();
    
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

    // stores the input as a vector of strings    
    vector<string> inp;
    while(1) {
        cout << ">>>";
        readline(inp);
        add_history(inp);
        splitCommands(inp);
        inp.clear();
    }
    return 0;
}
