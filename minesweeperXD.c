#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
int pollingDelay = 8;
int trollingDelay = 16;

void askSize();
int size = 0;
int mines = 0;

void initGrid();
int* gridArr;
int counter = 0;

void assignNumbers();

void gameInputs();
char move = '0';
int cursorcol = 0;
int cursorrow = 0;

void inputEnter();
void inputFlag();

void moveUp();
void moveDown();
void moveRight();
void moveLeft();

void modifyCursorData(int);
int getCursorData();

void modifyClearVisit(int);
int getClearVisit();

int revealAdjacent();
int* visited;
int firstrec = 0;

void revealNumber();

void colorTileDark();

void gameDefeat();

void printTileData();
void printMines();
void printNumbers();
void printVisits();

int main()
{
    printf("Hello World!\n");
    /*#ifdef _WIN32
    Sleep(pollingDelay);
    #else
    usleep(pollingDelay*1000);
    #endif*/

    askSize();

    initGrid();

    gameInputs();

    printf("Game has shut down unexpectedly");
    return 0;
}

void askSize() {
    printf("give size(of one size): ");
    scanf("%d", &size);
    if (size > 20) size = 20;

    printf("how many mines: ");
    scanf("%d", &mines);
    if (size * size < mines) mines = size * size;
}

void initGrid() {
    int gridSize = size * size;
    printf("grid size: %d\n", gridSize);

    gridArr = calloc(gridSize, sizeof(int)); // Creating enough space for 'n' integers.
    if (gridArr == NULL) {
        printf("Unable to allocate memory\n");
    }

    visited = calloc(gridSize, sizeof(int)); // Creating enough space for 'n' integers.
    if (visited == NULL) {
        printf("Unable to allocate memory\n");
    }

    //intialize rng
    time_t t;
    srand((unsigned)time(&t));

    //create mines
    int random = 0;
    for (int i = 0; i < mines; ++i) {
        random = rand() % gridSize;
        if (gridArr[random] == 1) { //check if position is already a mine
            i--;
        }
        gridArr[random] = 1;
    }

    assignNumbers();

    printf("\033[2J"); //erase screen
    printf("\033[H"); //cursor to topleft

    //\033[text  \033[background
    printf("\033[30\033[48;5;255m");

    //prints the grid
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            printf("\033[%d;%dH[ ]", i + 1, j * 3 + 1);
        }
        printf("\n");
    }

    printf("\033[30;47m\033[%d;8H W \033[34mE\n\033[30m\033[7GA S D \033[31mF\033[30m", size + 4); //print instructions

    printf("\033[1;2H"); //cursor to first position
}

void assignNumbers() {
    int savecol = cursorcol;
    int saverow = cursorrow;

    cursorcol = 0;
    cursorrow = 0;

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (getCursorData() == 1) {
                int rowspace = 1;
                int colspace = 1;
                
                //for returning to mine
                int savec = cursorcol;
                int saver = cursorrow;

                //for if can't move right
                int cantmove = 0;

                //check for edges
                if (cursorrow < size - 1) {
                    rowspace++; //rows have space below
                }
                if (cursorrow != 0) {
                    moveUp();
                    rowspace++; //rows have space above
                }
                
                if (cursorcol < size - 1) {
                    colspace++; //cols have space on right
                }
                if (cursorcol != 0) {
                    moveLeft();
                    colspace++; //cols have space on left
                }

                //modify tiles around the mine
                for (int row = 0; row < rowspace; row++)
                {
                    for (int col = 0; col < colspace; col++)
                    {
                        if (getCursorData() == 0) {
                            modifyCursorData(5);
                        }
                        else if (getCursorData() >= 5) {
                            modifyCursorData(getCursorData() + 1);
                        }
                        if (cursorcol < size - 1) {
                            moveRight();
                        }
                        else cantmove = 1; //if cant move right have to make colspace negate one less
                    }
                    moveDown();
                    cursorcol = cursorcol - colspace + cantmove;
                    cantmove = 0;
                }

                //return cursor to current mine
                cursorcol = savec;
                cursorrow = saver;
            }
            moveRight();
        }
        moveDown();
        cursorcol = 0;
    }

    cursorcol = savecol;
    cursorrow = saverow;
    printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3);
}

void gameInputs() {
    while (1) {
        if (_kbhit()) {
            move = _getch();

            //let's implement up right down left as 1 2 3 4 for now
            switch (move) {
            case 'w': //up
                moveUp();
                break;
            case 'd': //right
                moveRight();
                break;
            case 's': //down
                moveDown();
                break;
            case 'a': //left
                moveLeft();
                break;
            case 'e':
                inputEnter();
                break;
            case 'f':
                inputFlag();
                break;
            case 'r':
                printMines(); //debugging
                break;
            case 't':
                printNumbers(); //debugging
                break;
            case 'y':
                printTileData(); //debugging
                break;
            case 'g':
                printf("\033[30m%d\033[1D", getCursorData());
                break;
            default:
                break;
            }
        }
#ifdef _WIN32
        Sleep(pollingDelay);
#else
        usleep(pollingDelay * 1000);
#endif
    }
}

void inputEnter() {
    switch (getCursorData())
    {
    case 0: //clear
        colorTileDark();
        revealAdjacent();
        break;
    case 1: //has mine
        gameDefeat();
        break;
    case 2: //clear and has flag
        printf("\033[31m \033[1D");
        break;
    case 3: //has mine and flag
        gameDefeat();
        break;
    case 4:
        break;
    default:
        revealNumber();
        break;
    }
}

void inputFlag() {
    switch (getCursorData())
    {
    case 0: //clear
        printf("\033[31mF\033[1D");
        modifyCursorData(2);
        break;
    case 1: //has mine
        printf("\033[31mF\033[1D");
        modifyCursorData(3);
        break;
    case 2: //clear and has flag
        printf("\033[31m \033[1D");
        modifyCursorData(0);
        break;
    case 3: //has mine and flag
        printf("\033[31m \033[1D");
        modifyCursorData(1);
        break;
    case 4:
        break;
    default:
        if (getCursorData() < 13) {
            printf("\033[31mF\033[1D");
            modifyCursorData(getCursorData() * 3);
        }
        else {
            printf("\033[31m \033[1D");
            modifyCursorData(getCursorData() / 3);
        }
        break;
    }
}

void moveUp() {
    if (cursorrow != 0) {
        printf("\033[1A");
        cursorrow--;
    }
}
void moveDown() {
    if (cursorrow < size - 1) {
        printf("\033[1B");
        cursorrow++;
    }
}
void moveRight() {
    if (cursorcol < size - 1) {
        printf("\033[3C");
        cursorcol++;
    }
}
void moveLeft() {
    if (cursorcol != 0) {
        printf("\033[3D");
        cursorcol--;
    }
}

void modifyCursorData(int data) {
    gridArr[cursorcol + size * cursorrow] = data;
}

int getCursorData() {
    int data = gridArr[cursorcol + size * cursorrow];

    return data;
}

void modifyClearVisit(int data) {
    visited[cursorcol + size * cursorrow] = data;
}

int getClearVisit() {
    int data = visited[cursorcol + size * cursorrow];

    return data;
}

int revealAdjacent() { //this needs to use recursion I think :D
    int gridSize = size * size;

    int savecol = cursorcol;
    int saverow = cursorrow;

    if (getCursorData() != 0 || getCursorData() == 5 || getClearVisit() != 0) {
        return;
    }
    modifyClearVisit(1); //mark current tile as visited
    colorTileDark();

    /*  this for loop switch structure should start the directional
    *   checks four times, always from a different position
    *   1234 -> 2341 -> 3412 -> 4123
    */ //FIXME reveals diagonal tiles that are trapped my mines, also sometimes doesn't reveal all proper tiles
    int recMovementStartPos = 0;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            recMovementStartPos = (i + j) % 4;

            switch (recMovementStartPos)
            {
            case 0:
                if (cursorrow > 0) {
                    moveUp();
                    revealAdjacent();
                }
                break;
            case 1:
                if (cursorcol < size) {
                    moveRight();
                    revealAdjacent();
                }
                break;
            case 2:

                if (cursorrow < size) {
                    moveDown();
                    revealAdjacent();
                }
                break;
            case 3:
                if (cursorcol > 0) {
                    moveLeft();
                    revealAdjacent();
                }
                break;
            default:
                break;
            }
        }
    }

    cursorcol = savecol;
    cursorrow = saverow;
    printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3);

    return 0;
}

void revealNumber() {
    if (getCursorData() >= 15) { //has mine nearby and is flagged
        modifyCursorData(getCursorData() / 3);
    }
    int realvalue = getCursorData() - 4;

    int colorvalue = 0;
    switch (realvalue)
    {
    case 1:
        colorvalue = 94;
        break;
    case 2:
        colorvalue = 32;
        break;
    case 3:
        colorvalue = 91;
        break;
    case 4:
        colorvalue = 34;
        break;
    case 5:
        colorvalue = 31;
        break;
    case 6:
        colorvalue = 36;
        break;
    case 7:
        colorvalue = 30;
        break;
    case 8:
        colorvalue = 37;
        break;
    default:
        break;
    }

    colorTileDark();

    //\033[text  \033[background
    printf("\033[30\033[48;5;250m");
    printf("\033[1D\033[37m[\033[1;%dm%d\033[22;37m]\033[2D\033[47m", colorvalue, realvalue);
    //                     ^        ^  ^           ^
    //                     [        c  n           ]                     
}

void colorTileDark() {
    //printf("\033[1D\033[30;100m[ ]\033[2D\033[47m");
    printf("\033[1D\033[48;5;250m[ ]\033[2D\033[47m");
}

void gameDefeat() {
    printMines();
    while (1) {
        printf("\033[30;47m\033[%d;16H HONESTLY QUITE INCREDIBLE ", size + 4);
#ifdef _WIN32
        Sleep(trollingDelay);
#else
        usleep(trollingDelay * 1000);
#endif
        printf("\033[30;40m\033[%d;16H HONESTLY QUITE INCREDIBLE ", size + 4);
#ifdef _WIN32
        Sleep(trollingDelay);
#else
        usleep(trollingDelay * 1000);
#endif
    }
}

void printTileData() {
    int savecol = cursorcol;
    int saverow = cursorrow;

    cursorcol = 0;
    cursorrow = 0;

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            printf("\033[%d;%dH[%d]", i + 1, j * 3 + 1, getCursorData());
            moveRight();
        }
        moveDown();
        cursorcol = 0;
    }

    cursorcol = savecol;
    cursorrow = saverow;
    printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3);
}

void printMines() {
    counter = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (gridArr[counter] == 1) {
                printf("\033[%d;%dH[x]", i + 1, j * 3 + 1);
            }
            counter++;
        }
        printf("\n");
    }
    printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3); //cursor to where it was
}

void printNumbers() {
    int savecol = cursorcol;
    int saverow = cursorrow;

    counter = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (gridArr[counter] >= 5) {
                printf("\033[%d;%dH[A]", i + 1, j * 3 + 1);
            }
            counter++;
        }
        printf("\n");
    }
    printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3); //cursor to where it was

    cursorcol = savecol;
    cursorrow = saverow;
    printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3);
}

void printVisits() {
    counter = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            printf("\033[%d;%dH[%d]", i + 1, j * 3 + 1, visited[counter]);
            counter++;
        }
        printf("\n");
    }
    printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3); //cursor to where it was
}
