#include<iostream>
#include <unistd.h>
using namespace std;
/*
void getDiff(struct timer tv, timer tv2) {
    long long ans;
    
    return;
} 
*/
void solve()
{
    timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    clock_t s =clock();
    usleep(1'700'000);    // 0.5 s
    clock_t e =clock();
    clock_gettime(CLOCK_REALTIME, &end);
    // 1s 200 ns
    // 0s 400 ns
    cout<< ((end.tv_sec- start.tv_sec)*((int)1e9) + (end.tv_nsec- start.tv_nsec))/(1000000.0)<<"\n";
    cout<<double(e-s) * 1000 /CLOCKS_PER_SEC<<"\n";
}
signed main()
{
    // OJ
  
    solve();
    return 0;
}