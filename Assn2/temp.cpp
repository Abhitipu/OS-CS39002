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


using namespace std;

map<string, function<int(string)>> mp;

int cdfunc(string s) {
    cout << "Inside cd! " << s << '\n';
    return 1;
}

int helpfunc(string s) {
    cout << "Inside help! " << s << '\n';
    return 1;
}

int fib(int n) {
    if(n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

// non canonical
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
        exit (EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr (STDIN_FILENO, &saved_attributes);
    atexit (reset_input_mode);

    // https://www.mkssoftware.com/docs/man5/struct_termios.5.asp
    /* Set the terminal modes. */
    tcgetattr (STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON); /* Clear ICANON and ECHO. */
    tattr.c_cc[VMIN] = 1;   // 1 byte
    tattr.c_cc[VTIME] = 0;  // timer is NULL
    tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

int main() {
    set_input_mode ();

    cout << "Hello world!\n";
    string s;
    // cin >> s;
    while(1)
    {
        char ch;
        cin.get(ch);
        if((int)ch == 18)
        {
            cout<<"Ctrl + r pressed\n";
            break;
        }
        s.push_back(ch);
    }
    cout << s << '\n';
    return 0;
}