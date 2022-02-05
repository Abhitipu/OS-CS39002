#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <time.h>

using namespace std;

time_t rawtime;
struct tm * timeinfo;
char buffer [80];

void showtime() {
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime (buffer,80,"Now it's %I:%M:%S%p.",timeinfo);
    // strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);
    cerr << buffer << '\n';
}
    
  
int printFileAndShowFiles(char *filename){
    char buf[100];
    bzero(buf, sizeof(buf));
    int fd = open(filename, O_RDONLY);
    cout<<"Filename : "<<filename<<", ";
    showtime();
    cout<<"\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
    while(read(fd, buf, sizeof(buf)-1))
    {
        cout<<buf;
        bzero(buf, sizeof(buf));
    }
    cout<<"\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    close(fd);
    return 0;
}

int main()
{
    char buf[1000];
    int fd = inotify_init();

    cout<< fd<< "\n";

    int watch_desc;

    watch_desc = inotify_add_watch(fd, ".", IN_MODIFY);
    // What is this lol?
   
    cout<< watch_desc<<"\n";
    while(1){
        // https://man7.org/linux/man-pages/man7/inotify.7.html
        bzero(buf, sizeof(buf));
        int i = 0;
        size_t bufsiz = sizeof(struct inotify_event) + 1000 + 1; // 1000  max file name size
        struct inotify_event* event = (struct inotify_event*)malloc(bufsiz);
        int n = read(fd, event, bufsiz);
        
        cout<< event->len<<"\n";
        cout<< event->name<<" modified\n";
        if(strcmp(event->name, "t1.txt") == 0)
        {
            printFileAndShowFiles(event->name);
        }

    }
    return 0;
}