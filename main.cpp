#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <termios.h>
#include "get_keyboard_input.cpp"
//因為不知道要怎麼一起執行 main.cpp 和 get_keyboard_input.cpp 所以先 include .cpp file instead of .hpp file
using namespace std;
using namespace std::chrono;


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
    int starve_timer;
};

struct Map
{
    vector<vector<char> > map;
    int map_verlen; // Map vertical length
    int map_horlen; // Map horizontal length
    int refresh_frequency; // 控制 getkeyin() 的鍵盤偵測時間（因為鍵盤偵測時間 > usleep()時間，所以地圖更新速度受鍵盤偵測時間影響）
    int gametimer;
};

struct Food
{
    vector<vector<int> > normal_coordinate;
    vector<vector<int> > poison_coordinate;
    int timer;
};

void snakeMove(char&, Map&, Snake&, char&, int&);

vector<vector<char> > createMap(Map&);
// Intput argument 1: map_verlen, the gaming map's vertical length 
// Input argument 2: map_horlen, the gaming map's horizontal length
// map_verlen and map_horlen can be input by user before the game starts
// The function creates and return a map vector, which is constructed by a 2D vector

void createsnake(Map&, Snake&);

void recordboard(Map&, Snake&);
// The function prints score and rules

void printCurMap(Map&);

void createBarrier(Map&, vector<vector<int> >&);

void selectBarrier(Map&, vector<int>&, vector<vector<int> >&);

void generateFood(Map&, Food&, Snake&);

void clearFood(Map&, Food&);

void foodRot(Map&, Food&);

void speedUp(Map&, int);

int checkCollision(Map&, Snake&, char&, char&);

int gameOver(Map&, Snake&, int);


int main()
{
    srand(time(0));

    while (true)
    {
        // Make snake struct
        Snake snake;

        // Make map struct
        Map mapData;

        // Make food struct
        Food food;

        // Ask user to input map size
        // The minimal vertical and horizontal size of the map is 25
        do
        {
            string inputstr;
            cout << "Enter the vertical size of the map (minimal vertical size: 25): ";
            cin >> mapData.map_verlen;
            cout << "Enter the horizontal size of the map (minimal horizontal size: 25): ";
            cin >> mapData.map_horlen;
        } while ((mapData.map_verlen < 25) || (mapData.map_horlen < 25));

        // Set the record board
        snake.len = 3; // snake length = the score
        system("clear"); // Clear the screen before printing the map and the record board
        recordboard(mapData, snake); // Print the record board

        // Get the map vector
        mapData.map = createMap(mapData);

        // Create the snake
        createsnake(mapData, snake);

        // Create barrier
        vector<vector<int> > barriers;
        createBarrier(mapData, barriers);

        // Create foods
        generateFood(mapData, food, snake);

        // Print the map
        printCurMap(mapData);

        // Set timers
        steady_clock::time_point old_time_fifteen = steady_clock::now();
        steady_clock::time_point old_time_one = steady_clock::now();
        mapData.gametimer = 0;
        snake.starve_timer = 0;
        food.timer = 0;

        // Set map refresh frequency (snake speed)
        mapData.refresh_frequency = 100000; // 初始設定 100000微秒

        // Game over flag
        int gameover = 0;

        char pre_key = 'd'; // 預設如果沒按按鍵就先往右走

        while (gameover == 0)
        {
            // Get the current keyboard input
            char key = getkeyin(mapData.refresh_frequency);
            // getkeyin() has a input time limit.
            // When user doesn't input any value within 1/16 sec, it'll return '0'

            // Move the snake
            if (key == 'q') // Exit checking
            {
                exit(1);
            }
            else if (key == 'r') // Restart a new game
            {
                // Game restart
                gameover = 3;
            }
            else if ((key == '0') || (((key == 'w') || (key == 'a')) || ((key == 's') || (key == 'd')))) // input is w, a, s, d, 0
            {
                int state = checkCollision(mapData, snake, key, pre_key);

                if (state == 2)
                {
                    // Game over
                    gameover = 1;
                    break;
                }
                else
                {
                    snakeMove(key, mapData, snake, pre_key, state);

                    // Remember the w, a, s, d input as the previous input
                    // 此時就要先知道上次輸入是往哪邊走，才能決定蛇要繼續往哪個方向走
                    // 同時作為使用者操作 w, a, s, d 的輔助功能
                    // （輸入w或s後，再輸日a或d之前不能再有w或s的訊號傳進snakeMove()讓蛇動)
                    // （輸入a或d後，再輸日w或s之前不能再有a或d的訊號傳進snakeMove()讓蛇動）
                    if (key != '0')
                    {
                        pre_key = key;
                    }

                    // 食物控制項
                    if (state == 1) // 蛇吃到食物後要執行的
                    {
                        clearFood(mapData, food);
                        generateFood(mapData, food, snake); // 蛇吃到食物後要讓所有食物重生
                        snake.starve_timer = 0; // 紀錄蛇沒吃到食物的時間的計時器歸零
                    }
                    else // 蛇沒吃到食物要檢查食物有沒有過期
                    {
                        if ((food.timer > 3) && (food.timer <= 6))
                        {
                            foodRot(mapData, food); // 食物快要過期，更改食物外表
                        }
                        else if (food.timer > 6)
                        {
                            clearFood(mapData, food);
                            generateFood(mapData, food, snake); // 食物過期，重生食物
                        }
                    }

                    // 蛇飢餓度控制項
                    if (snake.starve_timer >= 15)
                    {
                        gameover = 2; // 蛇沒吃到食物 15 秒後就餓死
                        break;
                    }

                    // Clear the console
                    system("clear");
                    usleep(1000); // 1000微秒的閃爍頻率眼睛就看不出來
                    
                    // Print the recordboard first
                    recordboard(mapData, snake);

                    // Print current map
                    printCurMap(mapData);
                    
                    // 紀錄遊戲進行時間
                    steady_clock::time_point new_time = steady_clock::now();
                    double one_sec_gap = duration_cast<duration<double> >(new_time - old_time_one).count();
                    double fifteen_sec_gap = duration_cast<duration<double> >(new_time - old_time_fifteen).count();

                    if (one_sec_gap >= 1.0)
                    {
                        food.timer++;
                        snake.starve_timer++;
                        mapData.gametimer++;
                        old_time_one = new_time; // 每過一秒更新 old_time 一次（old_time是用來計算過了一秒了沒）
                    }

                    // 遊戲每進行 15 秒，蛇就要加速一次 -> 越玩難度越高
                    if (fifteen_sec_gap >= 15.0)
                    {
                        speedUp(mapData, 0); // 加速 mode 0，加速速率較吃到特殊食物慢十倍
                        old_time_fifteen = new_time;
                    }                
                }      
            }
                
        }


        // Reset terminal attributes (turn on line buffer and echo)
        struct termios ttystate;
        tcgetattr(STDIN_FILENO, &ttystate);
        ttystate.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

        // Game over
        system("clear"); // Clear the console
        if(gameOver(mapData, snake, gameover))
        {
            string restart;
            cout << "Would you like to start a new game? (Enter 'y' or 'Y' for Yes; 'n' or 'N' for No) ";
            cin >> restart;

            if ((restart != "Y") && (restart != "y"))
            {
                break;
            }

            cout << endl;
        }
    }

    return 0;
}

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
                int randomPlace0 = rand() % (mapData.map_horlen - 5) + 2;
                
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
                int randomPlace1 = rand() % (mapData.map_horlen - 5) + 2;
                
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

void snakeMove(char& key, Map& mapData, Snake& snake, char& pre_key, int& state)
{
    // The four corner of the map
    int top = MAP_HORBOND / 2;
    int bottom = mapData.map_verlen + (MAP_HORBOND / 2) - 1;
    int leftMost = MAP_VERBOND / 2;
    int rightMost = mapData.map_horlen + (MAP_VERBOND / 2) - 1;

    // Pop the snake tail
    // state = 0 代表蛇正常移動，也就是蛇頭往移動方向多一格，且去掉蛇尾巴，使蛇長度維持
    // state = 1 代表蛇變長一單位，也就是蛇頭往移動方向多一格，但不去尾
    // state = 0 1 2 以外的數 代表蛇撞到自己的身體
    switch (state)
    {
        case 0:
            {
                vector<int> tail = snake.body.back();
                mapData.map[tail[1]][tail[0]] = ' '; // tail[1] is the y coordinate of the snake tail; tail[0] is the x coordinate of the snake tail
                snake.body.pop_back();
            }
            break;
        case 1:
            snake.len++;
            break;
        default:
            {
                int popNum = snake.len - state;

                for (int i = 0; i <= popNum; i++) // 要做 popNum + 1 次，因為 pop 完 popNum 個之後還要扣除正常移動的「頭往前一格，去掉尾巴那一個」
                {
                    vector<int> tail = snake.body.back();
                    mapData.map[tail[1]][tail[0]] = ' '; // tail[1] is the y coordinate of the snake tail; tail[0] is the x coordinate of the snake tail
                    snake.body.pop_back();
                }

                snake.len -= popNum;
            }
            break;
    }

    // Modify the snake head before the move into a part of snake body
    vector<int> oldhead = snake.body.front();
    mapData.map[oldhead[1]][oldhead[0]] = 'x'; // oldhead[1] is the y coordinate of the old snake head; oldhead[0] is the x coordinate of the odl snake head

    // New head
    char head_appearence; // Snake head's appearence would change by direction

    // Keyboard input blocker（玩家操作輔助功能）
    // 使用者操作 w, a, s, d 的輔助功能
    // 輸入w或s後，再輸日a或d之前不能再有w或s的訊號傳進snakeMove()讓蛇動
    // 輸入a或d後，再輸日w或s之前不能再有a或d的訊號傳進snakeMove()讓蛇動
    // 因為在連續兩次再同列或同行的操作在貪食蛇的移動邏輯上是不被允許的
    if (
        (((pre_key == 'w') || (pre_key == 's')) && ((key == 'w') || (key == 's'))) ||
        (((pre_key == 'a') || (pre_key == 'd')) && ((key == 'a') || (key == 'd')))
       )
    {
        key = '0';
    }
    

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

void recordboard(Map& mapData, Snake& snake)
{
    cout << endl;
    cout << "- Press w, a, s, d to control the snake" << endl;
    cout << "- Press r to start a new game" << endl;
    cout << "- Press q to quit program" << endl;
    cout << endl;
    cout << "Score: " << snake.len << endl;
    cout << "Game processing time: " << mapData.gametimer << endl;
    cout << "Snake starving time: " << snake.starve_timer << endl;
    cout << "Map refreshing rate(unit: µs)(aka. snake moving speed): " << mapData.refresh_frequency << endl;
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

            coorH[0] = referencePoint[0] + 2;
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

            coorH[0] = referencePoint[0] + 1;
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

void generateFood(Map& mapData,Food& food, Snake& snake)
{
    // The corners of the map
    int top = MAP_HORBOND / 2;
    int leftMost = MAP_VERBOND / 2;

    int foodNum = mapData.map_horlen * mapData.map_verlen / 125; // area / 125 is the ratio of food number
    int foodCount = 0;

    while (foodCount < foodNum)
    {
        int randx = rand() % (mapData.map_horlen - 2);
        int randy = rand() % (mapData.map_verlen - 2);
        int foodx = leftMost + 1 + randx;
        int foody = top + 1 + randy;

        // 如果隨機生成的 food 座標在原本就存在的障礙物上(H)、之前產生的食物上(+, #)、蛇上(x, snake.y, snake.x)
        // 就要重新產生一組 food 座標
        if (
            (mapData.map[foody][foodx] == '#') || 
            (
             ((mapData.map[foody][foodx] == 'H') || (mapData.map[foody][foodx] == 'x')) || 
             (((foody == snake.y) && (foodx == snake.x)) || (mapData.map[foody][foodx] == '+'))
            )
           )
        {
            continue;
        }

        vector<int> foodCoor(2);
        foodCoor[0] = foodx;
        foodCoor[1] = foody;

        // 有 10% 的機率會產生長 "#" 的食物，吃了會讓蛇速度加快
        if (foodCount < (foodNum / 10 + 1))
        {
            mapData.map[foody][foodx] = '#';
            food.poison_coordinate.push_back(foodCoor);
        }
        else
        {
            mapData.map[foody][foodx] = '+';
            food.normal_coordinate.push_back(foodCoor);
        }

        foodCount++;
    }

    food.timer = 0; // 食物重生後要讓食物的存在時間計時器歸零（紀錄食物存在於地圖多久的計時器）
}

void clearFood(Map& mapData,Food& food)
{
    // 把普通食物在地圖上設為空白
    for(int i = 0; i < int(food.normal_coordinate.size()); i++)
    {
        mapData.map[food.normal_coordinate[i][1]][food.normal_coordinate[i][0]] = ' ';
    }

    // 把特殊食物在地圖上設為空白
    for(int i = 0; i < int(food.poison_coordinate.size()); i++)
    {
        mapData.map[food.poison_coordinate[i][1]][food.poison_coordinate[i][0]] = ' ';
    }

    // 把儲存普通食物的座標的 vector 清空
    food.normal_coordinate.clear();

    // 把儲存特殊食物的座標的 vector 清空
    food.poison_coordinate.clear();
}

void foodRot(Map& mapData, Food& food)
{
    for(int i = 0; i < int(food.normal_coordinate.size()); i++)
    {
        mapData.map[food.normal_coordinate[i][1]][food.normal_coordinate[i][0]] = '&';
    }
}

void speedUp(Map& mapData, int mode)
{
    if ((mapData.refresh_frequency >= 45000) && (mode == 0)) // mode 0 是每aaasaaaawddsaaawdwad 15 秒蛇自動加速一次的加速速率
    {
        mapData.refresh_frequency -= 1000;
    }
    else if ((mapData.refresh_frequency >= 50000) && (mode == 1)) // mode 1 是蛇吃到陷阱食物的加速速率
    {
        mapData.refresh_frequency -= 10000;
    }
}

int checkCollision(Map& mapData, Snake& snake, char& key, char& pre_key)
{
    // The four corner of the map
    int top = MAP_HORBOND / 2;
    int bottom = mapData.map_verlen + (MAP_HORBOND / 2) - 1;
    int leftMost = MAP_VERBOND / 2;
    int rightMost = mapData.map_horlen + (MAP_VERBOND / 2) - 1;

    int nextx = snake.x;
    int nexty = snake.y;

    // Keyboard input blocker（玩家操作輔助功能）
    // 使用者操作 w, a, s, d 的輔助功能
    // 輸入w或s後，再輸日a或d之前不能再有w或s的訊號傳進snakeMove()讓蛇動
    // 輸入a或d後，再輸日w或s之前不能再有a或d的訊號傳進snakeMove()讓蛇動
    // 因為在連續兩次再同列或同行的操作在貪食蛇的移動邏輯上是不被允許的
    if (
        (((pre_key == 'w') || (pre_key == 's')) && ((key == 'w') || (key == 's'))) ||
        (((pre_key == 'a') || (pre_key == 'd')) && ((key == 'a') || (key == 'd')))
       )
    {
        key = '0';
    }

    // decide the next position of the snake head after keyboard input
    switch(key)
    {
        case 'w':
            if (snake.y == top)
            {
                nexty = bottom;
            }
            else
            {
                nexty--;
            }
            break;
        case 's':
            if (snake.y == bottom)
            {
                nexty = top;
            }
            else
            {
                nexty++;
            }
            break;
        case 'a':
            if (snake.x == leftMost)
            {
                nextx = rightMost;
            }
            else
            {
                nextx--;
            }
            break;
        case 'd':
            if (snake.x == rightMost)
            {
                nextx = leftMost;
            }
            else
            {
                nextx++;
            }
            break;
        case '0':
            switch(pre_key)
            {
                case 'w':
                    if (snake.y == top)
                    {
                        nexty = bottom;
                    }
                    else
                    {
                        nexty--;
                    }
                    break;
                case 's':
                    if (snake.y == bottom)
                    {
                        nexty = top;
                    }
                    else
                    {
                        nexty++;
                    }
                    break;
                case 'a':
                    if (snake.x == leftMost)
                    {
                        nextx = rightMost;
                    }
                    else
                    {
                        nextx--;
                    }
                    break;
                case 'd':
                    if (snake.x == rightMost)
                    {
                        nextx = leftMost;
                    }
                    else
                    {
                        nextx++;
                    }
                    break;
            }
            break;
    }
    
    switch(mapData.map[nexty][nextx])
    {
        case ' ':
            return 0; // 蛇正常移動
            break;
        case '+': // + 和 & 都是普通型食物 -> snake grow
        case '&':
            snake.starve_timer = 0; // 蛇吃到東西之後就要重新計時飢餓時間（15秒不吃就餓死）

            // checkCollision() return 1 代表蛇變長一單位，當成一個參數傳入 snakeMove() 中
            // 因為蛇變長一單位相當於蛇移動的時候，頭往移動方向增長，然後不去尾，所以蛇變長可以共用 snakeMove()
            return 1; // 蛇變長
            break;
        case '#': // 吃到特殊食物 -> 蛇變長且加速
            speedUp(mapData, 1); // 吃到特殊陷阱食物，加速（mode 1）
            return 1; // 蛇變長
            break;
        case 'x': // 蛇撞到自己身體
            //檢查是撞到第幾節身體
            for (int i = 0; i < int(snake.body.size()); i++)
            {
                vector<int> nextPos(2);
                nextPos[0] = nextx;
                nextPos[1] = nexty;

                if (nextPos == snake.body[i])
                {
                    // nextPos 指到的那節，會在蛇頭再往前的時候變成下一節，所以實際上蛇撞到的是 i+1 節
                    return i + 1;
                }
            }
            break;
        case 'H': // The snake will hit the barrier -> game over
            return 2; // return 2 代表遊戲要結束
            break;
    }

    return 0; // default 狀況就設為蛇正常移動
} 

int gameOver(Map& mapData, Snake& snake, int deadmessage)
{
    cout << endl;

    // 印出 gameover
    ifstream instream;
    instream.open("gameover.txt");
    
    while (!instream.eof())
    {
        string str;
        getline(instream, str);
        cout << str << endl;
    }

    instream.close(); 

    cout << endl;
    cout << "Score: " << snake.len << endl;
    cout << "Game last: " << mapData.gametimer << " seconds" << endl;
    
    switch (deadmessage)
    {
        case 1:
            cout << "YOU BUMPED INTO THE WALL !" << endl;
            break;
        case 2:
            cout << "YOU STARVED TO DEAD !" << endl;
            break;
        case 3:
            cout << "Game restart" << endl;
            return 0; // return 0 代表不用詢問，直接開始下一局
            break;
    }

    return 1; // return 1 代表要詢問要不要繼續下一局
}
