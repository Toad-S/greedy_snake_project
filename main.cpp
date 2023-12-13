#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <deque>
#include <cstdlib>
#include "get_keyboard_input.cpp"
//因為不知道要怎麼一起執行 main.cpp 和 get_keyboard_input.cpp 所以先 include .cpp file instead of .hpp file
using namespace std;


int map_verlen; // Map vertical length
int map_horlen; // Map horizontal length
const int MAP_HORBOND = 2; // The size of the top and bottom boundary (1 + 1)
const int MAP_VERBOND = 4; // The size of the left and right boundary (2 + 2)

struct Snake
{
    int len; // The snake's  length
    int x; // The x coordinate of the snake
    int y; // The y coordinate of the snake
};

void snakeMove(char&, vector<vector<char> >&, Snake&, int&, int&, char&);

vector<vector<char> > createMap();
// Intput argument 1: map_verlen, the gaming map's vertical length 
// Input argument 2: map_horlen, the gaming map's horizontal length
// map_verlen and map_horlen can be input by user before the game starts
// The function creates and return a map vector, which is constructed by a 2D vector

void createsnake(vector<vector<char> >&, Snake&);

void recordboard(int&);
// Input argument 1: score
// The function prints score and rules

void printCurMap(vector<vector<char> >&);


int main()
{
    // Make snake struct
    Snake snake;

    // Ask user to input map size
    // The minimal vertical and horizontal size of the map is 25
    do
    {
        cout << "Enter the vertical size of the map (minimal vertical size: 25): ";
        cin >> map_verlen;
        cout << "Enter the horizontal size of the map (minimal horizontal size: 25): ";
        cin >> map_horlen;
    } while ((map_verlen < 25) || (map_horlen < 25));

    // Set the record board
    snake.len = 1;
    int score = snake.len;

    system("clear"); // Clear the screen before printing the map and the record board
    recordboard(score); // Print the record board

    // Get the map vector
    vector<vector<char> > map = createMap();

    // Print the map
    printCurMap(map);

    // Create the snake
    createsnake(map, snake);

    char pre_key = 'd'; // 預設如果沒按按鍵就先往右走

    while (true)
    {
        // Get the current keyboard input
        char key = getkeyin();
        // getkeyin() has a input time limit.
        // When user doesn't input any value within 1/16 sec, it'll return '0'
        
        // Remember the w, a, s, d input as the previous input
        // 當 keyboard input 不是 w, a, s, d, q 時，會進入 snakeMove() 不讓其他輸入影響蛇的移動速度
        // 此時就要先知道上次輸入是往哪邊走，才能決定蛇要繼續往哪個方向走
        if (((key == 'w') || (key == 'a')) || ((key == 's') || (key == 'd')))
        {
            pre_key = key;
        }

        // Clear the console
        system("clear");
        usleep(1000); // 1000微秒的閃爍頻率眼睛就看不出來了
        
        // Print the recordboard first
        recordboard(score);

        // Move the snake
        if (key == 'q') // Exit checking
        {
            exit(1);
        }
        else if (key == 'r') // Restart a new game
        {
            break;
        }
        else // input is w, a, s, d
        {
            snakeMove(key, map, snake, map_verlen, map_horlen, pre_key);
            printCurMap(map);
        }
            
        
    }

    return 0;
}

//testing function
void createsnake(vector<vector<char> >& map, Snake& snake)
{
    // Choose the random size
    
    //The four corner of the map
    int top = MAP_HORBOND / 2;
    int leftMost = MAP_VERBOND / 2;

    // Create x to imitate the snake
    snake.x = leftMost;
    snake.y = top;
    map[snake.y][snake.x] = 'x'; // Set x at the top leftMost at first
}

//testing function
void snakeMove(char& key, vector<vector<char> >& map, Snake& snake, int& map_verlen, int& map_horlen, char& pre_key)
{
    //The four corner of the map
    int top = MAP_HORBOND / 2;
    int bottom = map_verlen + (MAP_HORBOND / 2) - 1;
    int leftMost = MAP_VERBOND / 2;
    int rightMost = map_horlen + (MAP_VERBOND / 2) - 1;

    switch(key)
        {
            case 'w':
                // When pressing valid control button, make the snake's old place = ' '
                map[snake.y][snake.x] = ' ';

                // Snake move
                if (snake.y == top) // Snake penetrate through the top boundary
                {
                    snake.y = bottom;
                    map[snake.y][snake.x] = 'x';
                }
                else
                {
                    snake.y--;
                    map[snake.y][snake.x] = 'x';
                }

                break;
            case 's':
                // When pressing valid control button, make the snake's old place = ' '
                map[snake.y][snake.x] = ' ';

                // Snake move
                if (snake.y == bottom) // Snake penetrate through the bottom boundary
                {
                    snake.y = top;
                    map[snake.y][snake.x] = 'x';
                }
                else
                {
                    snake.y++;
                    map[snake.y][snake.x] = 'x';
                }

                break;
            case 'a':
                // When pressing valid control button, make the snake's old place = ' '
                map[snake.y][snake.x] = ' ';

                // Snake move
                if (snake.x == leftMost) // Snake penetrate through the left boundary
                {
                    snake.x = rightMost;
                    map[snake.y][snake.x] = 'x';
                }
                else
                {
                    snake.x--;
                    map[snake.y][snake.x] = 'x';
                }

                break;
            case 'd':
                // When pressing valid control button, make the snake's old place = ' '
                map[snake.y][snake.x] = ' ';

                // Snake move
                if (snake.x == rightMost) // Snake penetrate through the right boundary
                {
                    snake.x = leftMost;
                    map[snake.y][snake.x] = 'x';
                }
                else
                {
                    snake.x++;
                    map[snake.y][snake.x] = 'x';
                }

                break;
            case '0':
                // User doesn't input, so snake moves with the same direction
                // same direction -> previous input direction
                switch(pre_key)
                {
                    case 'w':
                        // When pressing valid control button, make the snake's old place = ' '
                        map[snake.y][snake.x] = ' ';

                        // Snake move
                        if (snake.y == top) // Snake penetrate through the top boundary
                        {
                            snake.y = bottom;
                            map[snake.y][snake.x] = 'x';
                        }
                        else
                        {
                            snake.y--;
                            map[snake.y][snake.x] = 'x';
                        }

                        break;
                    case 's':
                        // When pressing valid control button, make the snake's old place = ' '
                        map[snake.y][snake.x] = ' ';

                        // Snake move
                        if (snake.y == bottom) // Snake penetrate through the bottom boundary
                        {
                            snake.y = top;
                            map[snake.y][snake.x] = 'x';
                        }
                        else
                        {
                            snake.y++;
                            map[snake.y][snake.x] = 'x';
                        }

                        break;
                    case 'a':
                        // When pressing valid control button, make the snake's old place = ' '
                        map[snake.y][snake.x] = ' ';

                        // Snake move
                        if (snake.x == leftMost) // Snake penetrate through the left boundary
                        {
                            snake.x = rightMost;
                            map[snake.y][snake.x] = 'x';
                        }
                        else
                        {
                            snake.x--;
                            map[snake.y][snake.x] = 'x';
                        }

                        break;
                    case 'd':
                        // When pressing valid control button, make the snake's old place = ' '
                        map[snake.y][snake.x] = ' ';

                        // Snake move
                        if (snake.x == rightMost) // Snake penetrate through the right boundary
                        {
                            snake.x = leftMost;
                            map[snake.y][snake.x] = 'x';
                        }
                        else
                        {
                            snake.x++;
                            map[snake.y][snake.x] = 'x';
                        }

                        break;
                }

                break;
        }
}

vector<vector<char> > createMap()
{
    vector<vector<char> > map;

    // Create top boundary;
    vector<char> horbond(map_horlen + MAP_VERBOND);

    for (int i = 0; i < map_horlen + MAP_VERBOND; i++)
    {
        horbond[i] = '=';
    }
    
    map.push_back(horbond);

    // Create middle part
    vector<char> row(map_horlen + MAP_VERBOND);

    row[0] = '|';
    row[1] = '|';
    row[map_horlen + MAP_VERBOND - 1] = '|';
    row[map_horlen + MAP_VERBOND - 2] = '|';

    for (int i = 2; i < map_horlen + MAP_VERBOND - 2; i++)
    {
        row[i] = ' ';
    }

    for (int i = 0; i < map_verlen; i++)
    {
        map.push_back(row);
    }

    // Create bottom boundary;
    map.push_back(horbond);

    return map;
}

void recordboard(int& score)
{
    cout << endl;
    cout << "score: " << score << endl;
    cout << "- press w, a, s, d to control the snake" << endl;
    cout << "- press r to start a new game" << endl;
    cout << "- press q to quit program" << endl;
    cout << endl;
}

void printCurMap(vector<vector<char> >& map)
{
    for (int i = 0; i < map_verlen + MAP_HORBOND; i++)
    {
        for (int j = 0; j < map_horlen + MAP_VERBOND; j++)
        {
            cout << map[i][j];
        }
        cout << endl;
    }
}