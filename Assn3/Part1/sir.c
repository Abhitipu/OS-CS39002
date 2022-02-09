#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>


int main()
{
    char *name = "Matrix";
    
    int shm_fd;
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    int r, c;
    scanf("%d", &r);

    ftruncate(shm_fd, 4096);
    
    int *matrix = mmap(0, r * sizeof(double), PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);

    for(int i = 0; i < r; i++) {
        matrix[i] = i;        
    }

    int fork_status = fork();

    if(fork_status == 0) {
        matrix[0] = 1;
    } else {
        wait(NULL);
        matrix[r-1] = 0;
    }

    for(int i = 0; i < r; i++) {
        printf("%d ",matrix[i]);
    }
    printf("\n");

    shm_unlink(name);

    return 0;
}