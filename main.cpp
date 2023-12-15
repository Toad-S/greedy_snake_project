#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <deque>
#include <cstdlib>
#include <ctime>
#include "get_keyboard_input.cpp"
//因為不知道要怎麼一起執行 main.cpp 和 get_keyboard_input.cpp 所以先 include .cpp file instead of .hpp file
using namespace std;


const int MAP_HORBOND = 2; // The size of the top and bottom boundary (1 + 1)
const int MAP_VERBOND = 4; // The size of the left and right boundary (2 + 2)

struct Snake
{
    int len; // The snake's  length
    int x; // The x coordinate of the snake head
    int y; // The y coordinate of the snake head
    // Every int vector in the deque is a coordinate of one part of the snake body
    // The first element in the deque is the head of the snake
    // The last element in the deque is the tail of the snake
    deque<vector<int> > body;
};

struct Map
{
    vector<vector<char> > map;
    int map_verlen; // Map vertical length
    int map_horlen; // Map horizontal length
};

void snakeMove(char&, Map&, Snake&, char&);

vector<vector<char> > createMap(Map&);
// Intput argument 1: map_verlen, the gaming map's vertical length 
// Input argument 2: map_horlen, the gaming map's horizontal length
// map_verlen and map_horlen can be input by user before the game starts
// The function creates and return a map vector, which is constructed by a 2D vector

void createsnake(Map&, Snake&);

void recordboard(int&);
// Input argument 1: score
// The function prints score and rules

void printCurMap(Map&);

void createBarrier(Map&, vector<vector<int> >&);

void selectBarrier(Map&, vector<int>&, vector<vector<int> >&);


int main()
{
    srand(time(0));

    // Make snake struct
    Snake snake;

    // Make map struct
    Map mapData;

    // Ask user to input map size
    // The minimal vertical and horizontal size of the map is 25
    do
    {
        cout << "Enter the vertical size of the map (minimal vertical size: 25): ";
        cin >> mapData.map_verlen;
        cout << "Enter the horizontal size of the map (minimal horizontal size: 25): ";
        cin >> mapData.map_horlen;
    } while ((mapData.map_verlen < 25) || (mapData.map_horlen < 25));

    // Set the record board
    snake.len = 3;
    int score = snake.len;

    system("clear"); // Clear the screen before printing the map and the record board
    recordboard(score); // Print the record board

    // Get the map vector
    mapData.map = createMap(mapData);

    // Create the snake
    createsnake(mapData, snake);

    // Create barrier
    vector<vector<int> > barriers;
    createBarrier(mapData, barriers);

    // Print the map
    printCurMap(mapData);

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
        usleep(1000); // 1000微秒的閃爍頻率眼睛就看不出來
        
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
            snakeMove(key, mapData, snake, pre_key);
            printCurMap(mapData);
        }
            
        
    }

    return 0;
}

//testing function
void createsnake(Map& mapData, Snake& snake)
{
    //The four corner of the map
    int top = MAP_HORBOND / 2;
    int bottom = mapData.map_verlen + (MAP_HORBOND / 2) - 1;
    int leftMost = MAP_VERBOND / 2;

    // Choose the random place where the snake would be generated
    // The random place would only be on four boundaries
    //randomSide = 0 -> on the top boundary
    //randomSide = 1 -> on the bottom boundary
    //randomSide = 2 -> on the left boundary
    int randomSide = rand() % 3;

    vector<int> coordinate(2);

    switch (randomSide)
    {
        case 0:
            {
                int randomPlace0 = rand() % (mapData.map_horlen - 5);
                
                snake.y = top;
                snake.x = leftMost + randomPlace0;

                // Push the x, y to the vector (head)
                coordinate[0] = snake.x;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);

                // Push the x, y to the vector
                coordinate[0] = snake.x - 1;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);

                // Push the x, y to the vector (tail)
                coordinate[0] = snake.x - 2;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);
            }

            break;
        case 1:
            {
                int randomPlace1 = rand() % (mapData.map_horlen - 5);
                
                snake.y = bottom;
                snake.x = leftMost + randomPlace1;

                // Push the x, y to the vector (head)
                coordinate[0] = snake.x;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);

                // Push the x, y to the vector
                coordinate[0] = snake.x - 1;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);

                // Push the x, y to the vector (tail)
                coordinate[0] = snake.x - 2;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);
            }

            break;
        case 2:
            {
                int randomPlace2 = rand() % (mapData.map_verlen - 2);

                snake.y = top + randomPlace2;
                snake.x = leftMost + 2;

                // Push the x, y to the vector (head)
                coordinate[0] = snake.x;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);

                // Push the x, y to the vector
                coordinate[0] = snake.x - 1;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);

                // Push the x, y to the vector (tail)
                coordinate[0] = snake.x - 2;
                coordinate[1] = snake.y;
                // Push the coordinate to the deque
                snake.body.push_back(coordinate);
            }

            break;
    }

    // Set the snake on the map
    // snake.body[0] 存了 snake head 的 vector 座標，snake.body[0][0] 是 x 座標 snake.body[0][1] 是 y 座標
    // snake.body[1] 存了 snake body 的 vector 座標，snake.body[0][0] 是 x 座標 snake.body[0][1] 是 y 座標
    // snake.body[2] 存了 snake tail 的 vector 座標，snake.body[0][0] 是 x 座標 snake.body[0][1] 是 y 座標
    mapData.map[snake.body[0][1]][snake.body[0][0]] = '>';
    mapData.map[snake.body[1][1]][snake.body[1][0]] = 'x';
    mapData.map[snake.body[2][1]][snake.body[2][0]] = 'x';
}

//testing function
void snakeMove(char& key, Map& mapData, Snake& snake, char& pre_key)
{
    // The four corner of the map
    int top = MAP_HORBOND / 2;
    int bottom = mapData.map_verlen + (MAP_HORBOND / 2) - 1;
    int leftMost = MAP_VERBOND / 2;
    int rightMost = mapData.map_horlen + (MAP_VERBOND / 2) - 1;

    // Pop the snake tail
    vector<int> tail = snake.body.back();
    mapData.map[tail[1]][tail[0]] = ' '; // tail[1] is the y coordinate of the snake tail; tail[0] is the x coordinate of the snake tail
    snake.body.pop_back();

    // Modify the snake head before the move into a part of snake body
    vector<int> oldhead = snake.body.front();
    mapData.map[oldhead[1]][oldhead[0]] = 'x'; // oldhead[1] is the y coordinate of the old snake head; oldhead[0] is the x coordinate of the odl snake head

    // New head
    char head_appearence; // Snake head's appearence would change by direction

    // Decide the snake head's next position, and set snake.x and snake.y to the position
    // Decide the snake head's new appearence
    switch(key)
        {
            case 'w':
                // Snake move
                if (snake.y == top) // Snake penetrate through the top boundary
                {
                    snake.y = bottom;
                }
                else
                {
                    snake.y--;
                }

                head_appearence = '^';
                break;
            case 's':
                // Snake move
                if (snake.y == bottom) // Snake penetrate through the bottom boundary
                {
                    snake.y = top;
                }
                else
                {
                    snake.y++;
                }

                head_appearence = 'v';
                break;
            case 'a':
                // Snake move
                if (snake.x == leftMost) // Snake penetrate through the left boundary
                {
                    snake.x = rightMost;
                }
                else
                {
                    snake.x--;
                }

                head_appearence = '<';
                break;
            case 'd':
                // Snake move
                if (snake.x == rightMost) // Snake penetrate through the right boundary
                {
                    snake.x = leftMost;
                }
                else
                {
                    snake.x++;
                }

                head_appearence = '>';
                break;
            case '0':
                // User doesn't input, so snake moves with the same direction
                // same direction -> previous input direction
                switch(pre_key)
                {
                    case 'w':
                        // Snake move
                        if (snake.y == top) // Snake penetrate through the top boundary
                        {
                            snake.y = bottom;
                        }
                        else
                        {
                            snake.y--;
                        }

                        head_appearence = '^';
                        break;
                    case 's':
                        // Snake move
                        if (snake.y == bottom) // Snake penetrate through the bottom boundary
                        {
                            snake.y = top;
                        }
                        else
                        {
                            snake.y++;
                        }

                        head_appearence = 'v';
                        break;
                    case 'a':
                        // Snake move
                        if (snake.x == leftMost) // Snake penetrate through the left boundary
                        {
                            snake.x = rightMost;
                        }
                        else
                        {
                            snake.x--;
                        }

                        head_appearence = '<';
                        break;
                    case 'd':
                        // Snake move
                        if (snake.x == rightMost) // Snake penetrate through the right boundary
                        {
                            snake.x = leftMost;
                        }
                        else
                        {
                            snake.x++;
                        }

                        head_appearence = '>';
                        break;
                }
                break;
        }

    // Adding new head
    vector<int> newhead(2);
    newhead[0] = snake.x;
    newhead[1] = snake.y;
    mapData.map[newhead[1]][newhead[0]] = head_appearence;
    snake.body.push_front(newhead);
}

vector<vector<char> > createMap(Map& mapData)
{
    vector<vector<char> > map;

    // Create top boundary;
    vector<char> horbond(mapData.map_horlen + MAP_VERBOND);

    for (int i = 0; i < mapData.map_horlen + MAP_VERBOND; i++)
    {
        horbond[i] = '=';
    }
    
    map.push_back(horbond);

    // Create middle part
    vector<char> row(mapData.map_horlen + MAP_VERBOND);

    row[0] = '|';
    row[1] = '|';
    row[mapData.map_horlen + MAP_VERBOND - 1] = '|';
    row[mapData.map_horlen + MAP_VERBOND - 2] = '|';

    for (int i = 2; i < mapData.map_horlen + MAP_VERBOND - 2; i++)
    {
        row[i] = ' ';
    }

    for (int i = 0; i < mapData.map_verlen; i++)
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

void printCurMap(Map& mapData)
{
    for (int i = 0; i < mapData.map_verlen + MAP_HORBOND; i++)
    {
        for (int j = 0; j < mapData.map_horlen + MAP_VERBOND; j++)
        {
            cout << mapData.map[i][j];
        }
        cout << endl;
    }
}

void createBarrier(Map& mapData, vector<vector<int> >& barriers)
{
    // The corners of the map
    int top = MAP_HORBOND / 2;
    int leftMost = MAP_VERBOND / 2;

    int barrierNum = mapData.map_horlen * mapData.map_verlen * 2 / 625; // 2/625 是障礙物數量對地圖面積的比例
    int horRegionNum = (mapData.map_horlen - 10) / 4;
    int verRegionNum = (mapData.map_verlen - 5) / 4;

    for (int i = 0; i < barrierNum; i++)
    {
        int randx = rand() % horRegionNum;
        int randy = rand() % verRegionNum;
        vector<int> referencePoint(2);

        referencePoint[0] = leftMost + 6 + randx * 4;
        referencePoint[1] = top + 1 + randy * 4;
        
        selectBarrier(mapData, referencePoint, barriers);
    }
}

void selectBarrier(Map& mapData, vector<int>& referencePoint, vector<vector<int> >& barriers)
{
    int barrierType = rand() % 8;
    vector<int> coorH(2);

    switch (barrierType)
    {
        case 0:
            /*
            Type 0: 
            H
            HHHH
               H
            */

            mapData.map[referencePoint[1]][referencePoint[0]] = 'H';
            barriers.push_back(referencePoint);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;
        case 1:
            /*
            Type 1:
            H H
            HHHH
             H H
             HH
            */
            
            mapData.map[referencePoint[1]][referencePoint[0]] = 'H';
            barriers.push_back(referencePoint);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;

        case 2:
            /*
            Type 2:
            HH
             HHH
               H
            */
            
            mapData.map[referencePoint[1]][referencePoint[0]] = 'H';
            barriers.push_back(referencePoint);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;
        case 3:
            /*
            Type 3:
             HH
             HH
             HH
             HH
            */
            
            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;
        case 4:
            /*
             HH
            HHHH
            H  H
             HHH
            */
            
            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;
        case 5:
            /*
            HHHH
               H
               H
            */
            
            mapData.map[referencePoint[1]][referencePoint[0]] = 'H';
            barriers.push_back(referencePoint);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;
        case 6:
            /*
            
             HHH
            H H
            HHHH
            */

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);


            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;
        case 7:
            /*
               H
            HHHH
            HHHH
               H
            */

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1];
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 1;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0];
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 1;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 2;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 2;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);

            coorH[0] = referencePoint[0] + 3;
            coorH[1] = referencePoint[1] + 3;
            mapData.map[coorH[1]][coorH[0]] = 'H';
            barriers.push_back(coorH);
            break;
        
    }

}