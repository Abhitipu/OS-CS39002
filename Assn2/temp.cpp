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
#include <termios.h>

#define BACK_SPACE 127
#define CTRL_R 18

using namespace std;

struct termios saved_attributes;

void reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

// This is for detecting ctrl + C and ctrl + Z signals
void signal_callback_handler(int signum) {
    cerr << "Caught signal " << signum << endl;
    // Terminate program
    reset_input_mode();
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


void set_input_mode (void)
{
    struct termios tattr;
    char *name;

    /* Make sure stdin is a terminal. */
    if (!isatty (STDIN_FILENO))
    {
        fprintf (stderr, "Not a terminal.\n");
        exit (EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr (STDIN_FILENO, &saved_attributes);
    atexit (reset_input_mode);

    // https://www.mkssoftware.com/docs/man5/struct_termios.5.asp
    /* Set the terminal modes. */
    tcgetattr (STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
    tattr.c_cc[VMIN] = 1;   // 1 byte
    tattr.c_cc[VTIME] = 0;  // timer is NULL
    tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

int main() {
    signal(SIGINT, signal_callback_handler);    // ctrl + C
    signal(SIGTSTP, signal_callback_handler);   // ctrl + Z

    set_input_mode ();

    cout << "Hello world!\n";
    string s;
    // cin >> s;
    cout << ">>>";
    int cnt = 10;
    while(cnt--)
    {
        char ch;
        cin.get(ch);
    
        if((int)ch == CTRL_R)
        {
            cout<<"Ctrl + r pressed\n";
            break;
        }
        s.push_back(ch);
        if((int)ch == BACK_SPACE)
        {
            cout << "\b \b";
            s += "\b \b";
        } else
            cout << ch;
    }
    cout << s << '\n';
    reset_input_mode();

    return 0;
}
