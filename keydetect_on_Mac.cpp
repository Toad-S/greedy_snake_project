#include <iostream>
#include <termios.h>
#include <unistd.h>
using namespace std;

void setNonBlockingMode();
// Function to set terminal attributes for non-blocking input


int main() {
    // Set terminal to non-blocking mode
    setNonBlockingMode();

    char key;

    cout << "Press 'q' to quit." << endl;

    while (true) {
        // Check if a key is pressed
        if (read(STDIN_FILENO, &key, 1) > 0) {
            // Print the pressed key
            cout << "Press 'q' to quit" << endl; 
            cout << "Key pressed: " << key << endl;

            // Check if the pressed key is 'q' to exit
            if (key == 'q') {
                break;
            }

            switch(key) {
                case 'w':
                    cout << "snake moves up" << endl; // snake moves up function
                    break;
                case 'a':
                    cout << "snake moves left" << endl; // snake moves left function
                    break;
                case 's':
                    cout << "snake moves down" << endl; // snake moves down function
                    break;
                case 'd':
                    cout << "snake moves right" << endl; // snake moves right function
                    break;
            }
        }
        

        // Add a delay to avoid high CPU usage
        usleep(10000); // Sleep for 10 milliseconds
    }

    // Reset terminal attributes
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

    return 0;
}


void setNonBlockingMode() {
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
    ttystate.c_lflag &= ~ICANON;
    ttystate.c_lflag &= ~ECHO;

    // tcsetattr stands for "Terminal Control Set Attributes"
    // Set the new attributes，把改好的 state set 回去
    // TCSANOW 代表要立刻執行 terminal set attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}