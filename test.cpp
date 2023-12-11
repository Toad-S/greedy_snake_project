#include <iostream>
#include <unistd.h>
using namespace std;

int main()
{
    cout << "hello world" << endl;
    usleep(3000000);
    system("clear");
    return 0;
}