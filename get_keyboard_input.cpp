#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include "get_keyboard_input.hpp"
using namespace std;

void setNonBlockingMode();
// Function to set terminal attributes for non-blocking input

char getkeyin(int&);
// Function that gets a keyboard input immediately
// and return the character that it gets from the keyboard input


// use for debuging
// int main()
// {
//     while (true)
//     {
//         char key = '0';
//         while (key != 'q')
//         {
//             int refresh_frequency = 100000;
//             key = getkeyin(refresh_frequency);
            
//             // if (key == 'q')
//             //     break;
            
//             system("clear");
//             usleep(10000);
//             cout << "keypressed: " << key << endl;
//         }

//         string str;
//         cout << "input something" << endl;
//         cin >> str;
//         cout << str << endl;
//     }

//     return 0;
// }

char getkeyin(int& refresh_frequency)
{
    // Set terminal to non-blocking mode
    setNonBlockingMode();

    // Settint input time limit
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval timeout;
    timeout.tv_sec = 0;  // input 時間限制（單位：秒）
    timeout.tv_usec = refresh_frequency; // input 時間限制（單位：微秒）

    // Check input time
    int result = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &timeout);

    char key = '\0';

    if (result == -1)
    {
        perror("select");
    }
    else if (result == 0)
    {
        return '0';
    }
    else
    {
        // Get keyboard input
        read(STDIN_FILENO, &key, 1);
    }

    // Read one byte from the standard input from the keyboard and store the value in variable key
    // read(STDIN_FILENO, &key, 1);

    // Reset terminal attributes
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

    return key;
}


void setNonBlockingMode()
{
    // 建立一個 termios object 叫做 ttystate
    struct termios ttystate;

    // tcgetattr stands for "Terminal Control Get Attributes"
    // Get the terminal state，get state 出來改
    // STDIN_FILENO 參數代表 tcgetattr() 會把 standard input 的參數傳進 ttystate 這個 struct 中
    tcgetattr(STDIN_FILENO, &ttystate);

    // Turn off canonical mode (line buffering) and echoing
    // c_lflag 是一由一個個 bit 組成的，c_lflag 裡面的每個 bit 都表示一個控制項
    // ICANON 是一個在 c_lflag 中的其中一個 bit，控制 canonical mode 的 on/off -> off 就會只累積一個輸入，而不會累積一行
    // ECHO 是一個在 c_lflag 中的其中一個 bit，控制 terminal input 的 echo 的 on/off -> off 就會不在 terminal 中顯示重複輸入
    ttystate.c_lflag &= ~(ICANON | ECHO);

    // tcsetattr stands for "Terminal Control Set Attributes"
    // Set the new attributes，把改好的 state set 回去
    // TCSANOW 代表要立刻執行 terminal set attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}