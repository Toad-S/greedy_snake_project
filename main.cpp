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
    int starve_timer; // 紀錄蛇連續沒吃到食物的秒數，蛇一吃到食物，starve_timer就會歸零重新開始計算，若連續大於 15 秒沒吃東西，蛇就會死掉
};

struct Map
{
    // 以二維 vector 的形式存 map，map 包括整張地圖、地圖邊界、每個當下的蛇、每個當下的食物、障礙物
    // map會隨著遊戲進行而改變，所有遊戲的動態變動（計分板以外）都會由 map 顯示
    vector<vector<char> > map;
    int map_verlen; // Map vertical length
    int map_horlen; // Map horizontal length

    // refresh_frequency 控制接收鍵盤按鍵輸入的函式 getkeyin() 等待鍵盤輸入的時間
    // main function 中的第二層 while loop 每跑一圈就會印出 map 一次（視為地圖動態更新）
    // 而第二層 while loop 中有 getkeyin() 和 usleep() 兩個 function 會造成 while loop 的延遲
    // 其中 usleep() 是為了不造成電腦過大的負擔，所以每一圈 loop 都要讓程式停一下
    // 而 getkeyin() 則是接收鍵盤輸入的 function，其運作方式會先等待鍵盤輸入一段時間，若使用者在那段時間內有任何鍵盤輸入，就會接收鍵盤輸入，若使用者在那段時間內沒有鍵盤輸入，函式則會回傳 0(type: char)
    // 由於等待鍵盤輸入的時間至少是 usleep() 的 45 倍以上，所以控制地圖更新速度主要是控制 getkeyin() 中等待鍵盤輸入的時間
    // 因此將此變數命名為 refresh_frequency
    int refresh_frequency;
    int gametimer; // 紀錄每一局遊戲的持續時間
};

struct Food
{
    vector<vector<int> > normal_coordinate; // 存所有普通食物的座標
    vector<vector<int> > poison_coordinate; // 存所有陷阱食物的座標

    // 紀錄食物的存在時間，普通食物出現三秒後會改變自己的外觀，提示玩家食物快要消失了，但陷阱食物不會改變外觀
    // 所有食物出現六秒後會重生在不同位置
    int timer;
};

void snakeMove(char&, Map&, Snake&, char&, int&);
// Input argument 1: key, 是來自 getkeyin() 的 return 值，且在 main function 中被篩選過，只有 'w', 'a', 's', 'd', '0' 會進入 snakeMove 中（其中，當使用者沒輸入時，getkeyin()會 return '0'）
// Input argument 2: mapData, 調用 Map structure 中的所有資料
// Input argument 3: snake, 調用 Snake structure 中的所有資料
// Input argument 4: pre_key, pre_key 為上一個使用者輸入，用來阻擋不合理的移動和判斷使用者沒輸入(key == '0')時，蛇要繼續往哪個方向走
// Input argument 5: state, 來自 checkCollision() 的 return 值，表示蛇在移動中遇到的三種狀況：
// Input argument 5: state == 0 代表蛇正常移動，也就是蛇頭往移動方向多一格，且去掉蛇尾巴，使蛇長度維持
// Input argument 5: state == 1 代表蛇變長一單位，也就是蛇頭往移動方向多一格，但不去尾
// Input argument 5: state == 2 代表蛇撞到障礙物。但因為撞到障礙物就會死亡，所以 state == 2 的狀況會在 main function 中被過濾掉，不會進入 snakeMove() 中，因此 snakeMove() 沒有 state == 2 的判斷
// Input argument 5: state == 0 1 2 以外的數 代表蛇撞到自己的身體
// Function 1: 當 state == 0 時，蛇會正常按照鍵盤輸入(key == 'w', 'a', 's', 'd')方向轉向移動，若鍵盤沒有輸入(key == '0')，則會維持原本的移動方向
// Function 2: 當蛇移動到邊界時則會穿牆
// Function 3: 當 state == 1 時，蛇會變長一單位，並且讓 snake.len++
// Function 4: 當 state == n(n 是0 1 2 以外的數)，代表蛇撞到自己的第 state 節身體，此時會先去除蛇的撞到的那節以後的身體，再讓蛇正常移動

vector<vector<char> > createMap(Map&);
// Intput argument 1: mapData, 調用 Map 中的 map_horlen 和 map_verlen 
// Intput argument 1: map_verlen and map_horlen can be input by user before the game starts
// Return 1: createMap() 回傳一個按照使用者輸入的地圖大小建立好的二維 vector，當作整張地圖
// Function 1: 二維 vector 中指包含了地圖的邊界，其餘中心部分則都是由空白組成的(key spaces)
// Function 1: 而回傳的二維空地圖 vector 則會由 main function 中的二維 vector 接收，並作為往後遊戲進行時的地圖，任何操作的結果都會顯示在地圖上

void createsnake(Map&, Snake&);
// Input argument 1: mapData, 調用 Map 中的 map_verlen，和 const MAP_HORBOND 和 const MAP_VERBOND 定出蛇隨機生成的範圍
// Input argument 2: snake, 調用 Snake 中的 body(type: deque<vector<int> >) 。在隨機生成蛇之後，要把蛇的每一節身體座標放入 body 中
// Function 1: 確立蛇隨機生成的範圍（地圖的上邊界、下邊界、左邊界，由於蛇生在右邊界，一出生就會穿牆，並從左邊界出現，所以不特別讓蛇隨機出生在右邊界）
// Function 2: 在隨機生成範圍中隨機生成蛇的頭的座標，並由頭座標決定每節身體座標
// Function 3: 將每節身體和頭座標都放入 body 中，並於地圖對應座標上印出蛇的頭和身體

void recordboard(Map&, Snake&);
// Input argument 1: mapData, 調用 Map 中的 gametimer 和 refresh_frequency
// Input argument 2: snake, 調用 Snake 中的 len 和 starve_timer
// Function 1: record board 永遠會被印在地圖上方，且包含以下資訊：
// 1. 遊戲規則
// 2. 目前分數（蛇長度）
// 3. 遊戲進行時間（單位：秒）
// 4. 蛇的飢餓時間（單位：秒）
// 5. 螢幕刷新速度（意義是螢幕刷新週期）（單位：微秒/次），刷新週期越短代表蛇會動得越快

void printCurMap(Map&);
// Input argument 1: mapData, 調用 Map 中的 map (type: vector<vector<char> >)、map_horlen、map_horlen
// Function 1: 以 Map 中的 map (type: vector<vector<char> >)、map_horlen、map_verlen 結合 const MAP_HORBOND 和 const MAP_VERBOND 印出地圖

void createBarrier(Map&, vector<vector<int> >&);
// Input argument 1: mapData, 調用 Map 中的 map_horlen、map_verlen
// Input argument 2: vector<vector<int> > barriers 儲存所有障礙物座標的 vector
// Function 1: 在地圖中匡限一塊能生成障礙物的區塊，並將該區塊切分成一快快相鄰的 4x4 小區塊，每個小區塊都是障礙物能隨機生成的位置
// Function 2: 隨機在小區塊中生成 n 塊障礙物(n 是按照障礙物和地圖面積的比例決定)
// Function 2: 生成障礙物時，會另外隨機決定障礙物外型（將從預先設定好的八種障礙物排列中挑出一種）作為該障礙物外型（由 selectBarrier() 負責）
// Function 2: 將小區塊的最左上座標存入 vector<int> referencePoint 中，並將 referencePoint 傳入 selectBarrier() 中，決定障礙物外型

void selectBarrier(Map&, vector<int>&, vector<vector<int> >&);
// Input argument 1: mapData, 調用 Map 中的 map，將障礙物印在地圖上
// Input argument 2: referencePoint, 來自 createBarrier()，傳遞 4x4 小區塊中的左上角座標，作為隨機生成障礙物的座標依據
// Input argument 3: barriers, 儲存障礙物座標的二維 vector
// Function 1: 根據 createBarrier() 傳來的 referencePoint，建立選定小區塊中的隨機障礙物外型
// Function 2: 依照隨機外型和 referencePoint 決定障礙物座標，並印在地圖上及存入 vector<vector<int> >barriers 中

void generateFood(Map&, Food&, Snake&);
// Input argument 1: mapData, 調用 Map 中的 map、map_horlen、map_verlen
// Input argument 2: food, 調用 Food 中的 normal_coordinate、poison_coordinate、timer
// Input argument 3: snake, 調用 Snake 中的 x, y
// Function 1: 以照地圖面積(map_horlen * map_verlen) 決定要生成的食物數量 n 個
// Function 2: 在地圖內隨機生成 n 個食物
// Function 2: 並確認隨機生成的食物座標不在障礙物上或是蛇的目前位置上，否則重新生成隨機食物座標，直到確認食物座標不會和其他物件重疊為止
// Function 2: 在 n 個食物中有 10% 的食物會是陷阱食物
// Function 3: 將食物座標按照食物類別分別存入 vector<vector<int> > normal_coordinate 和 vector<vector<int> > poison_coordinate
// Function 4: 生成完食物後，將計算食物存在地圖時間的 timer 歸零

void clearFood(Map&, Food&);
// Input argument 1: mapData, 調用 Map 中的 map
// Input argument 2: food, 調用 Food 中的 normal_coordinate、poison_coordinate
// Function 1: 在地圖上將所有食物置換成空白(keyboard space)
// Function 2: 將 normal_coordinate、poison_coordinate 兩 vector 清空

void foodRot(Map&, Food&);
// Input argument 1: mapData, 調用 Map 中的 map
// Input argument 2: food, 調用 Food 中的 normal_coordinate
// Function 1: 將普通食物的外型從 '+' 改成 '&' 以提示玩家食物快要過期了

void speedUp(Map&, int);
// Input argument 1: mapData, 調用 Map 中的 refresh_frequency
// Input argument 2: mode, 可為 0 或 1（為了增加程式本身的可擴充性，不排除可增加其他模式，所以用 int 而非 bool）
// Function 1: mode == 0: 遊戲每進行 15 秒，就讓蛇的速度變快一點（refresh_frequency -= 1000 將地圖重置週期降低 1000 微秒）（較無明顯感覺）
// Function 2: mode == 1: 一旦蛇吃到陷阱食物，蛇的移動速度就會大幅增加（refresh_frequency -= 10000 將地圖重置週期降低  10000 微秒）（有感）
// Function 3: 不管是 mode == 0 或 1，只要 refresh_frequency 為 45000，就不會再降低（refresh_frequency 低於 45000 微秒會造成螢幕閃爍）

int checkCollision(Map&, Snake&, char&, char&);
// Input argument 1: mapData, 調用 Map 中的 map、map_horlen、map_verlen、body
// Input argument 2: snake, 調用 Snake 中的 x、y
// Input argument 3: key, 鍵盤當下的輸入（key 只可能 ==  '0', 'w', 'a', 's', 'd'）
// Input argument 4: pre_key, 鍵盤前一次輸入
// Function 1: 按照鍵盤輸入算出蛇頭的下一個位置
// Function 1: 如果前一次鍵盤輸入和當下鍵盤輸入在同一方向軸上（例如前一次輸入'w'，當下輸's'或'w'），就將 key 從原本的方向改成 '0'（視同沒有改變移動方向）
// Function 1: 蛇到邊界會穿牆
// Function 2: 判斷蛇頭的下一個位置的座標對應到地圖上是什麼字元
// Function 2: 如果是 ' '，代表蛇頭沒有撞到東西，就 return state = 0（代表正常移動）
// Function 2: 如果是 '+' 或 '&'，都代表蛇吃到普通食物，就 return state = 1（代表蛇吃到東西並正常移動）
// Function 2: 如果是 '#'，代表蛇吃到陷阱食物，就先呼叫 speed(mapData, 1) 並 return state = 1
// Function 2: 如果是 'x'，代表蛇撞到自己的身體，則計算蛇撞到第幾節身體，並 return state = 撞到第幾節身體（撞到第五節身體就 return 5）
// Function 2: 如果是 'H'，代表蛇撞到障礙物，則 return 2（代表蛇撞到障礙物）

int gameOver(Map&, Snake&, int);
// Input argument 1: mapData, 調用 Map 中的 gametimer
// Input argument 2: snake, 調用 Snake 中的 len
// Input argument 3: deadmessage, 代表遊戲結束原因（死亡原因）
// Function 1: 從 gameover.txt 中印出 Game over 字樣
// Function 2: 由 len 印出玩家該局得分
// Function 3: 由 gametimer 印出玩家該局遊玩時間
// Function 4: 由 deadmessage 印出遊戲結束原因
// Function 4: deadmessage == 1 代表玩家因撞到牆而使遊戲結束
// Function 4: deadmessage == 2 代表玩家因餓死(15秒內沒吃到食物)而使遊戲結束
// Function 4: deadmessage == 3 代表玩家按了 'r' 使遊戲重新開始新的一局


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
    if ((mapData.refresh_frequency >= 45000) && (mode == 0)) // mode 0 是每 15 秒蛇自動加速一次的加速速率
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
