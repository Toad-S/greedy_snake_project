#include <iostream>
#include <termios.h>
#include <unistd.h>

// Function to set terminal attributes for non-blocking input
void setNonBlockingMode() {
    struct termios ttystate;

    // Get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);

    // Turn off canonical mode (line buffering) and echoing
    ttystate.c_lflag &= ~(ICANON | ECHO);

    // Set the new attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

int main() {
    // Set terminal to non-blocking mode
    setNonBlockingMode();

    char key;

    std::cout << "Press 'q' to quit." << std::endl;

    while (true) {
        // Check if a key is pressed
        if (read(STDIN_FILENO, &key, 1) > 0) {
            // Print the pressed key
            std::cout << "Key pressed: " << key << std::endl;

            // Check if the pressed key is 'q' to exit
            if (key == 'q') {
                break;
            }
        }

        // You can do other processing here
        

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
