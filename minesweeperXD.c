#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define _getch() getchar()

int _kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	oldt.c_lflag &= ~ECHO; //movement will write over tiles without this
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if (ch != EOF)
	{
		ungetc(ch, stdin);

		oldt.c_lflag |= ECHO; //enable echo
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		return 1;
	}

	return 0;
}
#endif

#ifdef _WIN32
int pollingDelay = 8;
int trollingDelay = 16;
#else
int pollingDelay = 8000;
int trollingDelay = 16000;
#define Sleep(int) usleep(int)
#endif

void askSize();
int size = 0;
int mines = 0;

void initGrid();
int* gridArr;
int* visited;

void firstReveal();
int firstreveal = 0;

void assignNumbers();

void gameInputs();
char action = '0';
int cursorcol = 0;
int cursorrow = 0;
time_t startTime = 0;
time_t currentTime = 0;

void updateTimer();

void inputEnter();
void inputFlag();
int flagcounter = 0;
void updateMineIndicator();

void moveUp();
void moveDown();
void moveRight();
void moveLeft();

void storeAndResetCursorLocation();
void restoreCursorLocation();
int savecol = 0;
int saverow = 0;
void modifyCursorData(int);
int getCursorData();

void modifyClearVisit(int);
int getClearVisit();

void revealAdjacent();

void revealNumber();

void colorTileDark();

void gameDefeat();
void gameVictory();

void checkForVictory();

void restartGame();
int gameCleared = 0;
void resetValues();

void printTileData();
void printMines();
void printNumbers();
void printVisits();

int main()
{
#ifdef _WIN32
	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE), hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode;
	GetConsoleMode(hOutput, &dwMode);
	dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOutput, dwMode)) {
		printf("SetConsoleMode failed. Press any key to exit.");
		while (!_kbhit()) {
		}
		return 0;
	}
#endif

	//intialize rng
	time_t t;
	srand((unsigned)time(&t));

	while (1)
	{
		askSize();

		initGrid();

		gameInputs();

		resetValues();
	}

	return 0;
}

void askSize() {
	printf("give size(of one size): ");
	scanf("%d", &size);
	if (size > 24) size = 24;

	printf("how many mines: ");
	scanf("%d", &mines);
	if (size * size < mines + 9) mines = size * size - 9; //force room for starting tile reveal
}

void initGrid() {
	int gridSize = size * size;

	gridArr = calloc(gridSize, sizeof(int));
	if (gridArr == NULL) {
		printf("Unable to allocate memory\n");
		while (1) {}
	}

	visited = calloc(gridSize, sizeof(int));
	if (visited == NULL) {
		printf("Unable to allocate memory\n");
		while (1) {}
	}

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

	printf("\033[30;47m\033[%d;6H   W \033[34mE   \n\033[30m\033[6G A S D \033[31mF \033[30m", size + 4); //print instructions

	updateMineIndicator();

	printf("\033[1;2H"); //cursor to first position
}

void firstReveal() {
	int gridSize = size * size;

	//create mines
	int random = 0;
	for (int i = 0; i < mines; ++i) {
		random = rand() % gridSize;

		//these are for checking surrounding tiles
		int a = random - (cursorcol + size * (cursorrow - 1)); //top row
		int b = random - (cursorcol + size * cursorrow); //mid row
		int c = random - (cursorcol + size * (cursorrow + 1)); //bot row

		if (gridArr[random] == 1 || (a >= -1 && a <= 1) || (b >= -1 && b <= 1) || (c >= -1 && c <= 1)) { //check if position is already a mine or is near current position
			i--;
		}
		else gridArr[random] = 1;
	}

	assignNumbers();
}

void assignNumbers() {
	storeAndResetCursorLocation();

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

	restoreCursorLocation();
}

void gameInputs() {
	startTime = time(NULL); //game start time

	while (gameCleared == 0) {
		if (_kbhit()) {
			action = _getch();

			switch (action) {
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
				if (firstreveal == 0) {
					firstReveal();
					firstreveal = 1;
				}
				inputEnter();
				break;
			case 'f':
				if (firstreveal == 0) {
					break;
				}
				inputFlag();
				break;
			case 'r':
				//printMines(); //debugging
				break;
			case 't':
				//printNumbers(); //debugging
				break;
			case 'y':
				//printTileData(); //debugging
				break;
			case 'g':
				//printf("\033[30m%d\033[1D", getCursorData()); //debugging, print data at cursor location
				break;
			default:
				break;
			}
		}
		Sleep(pollingDelay);
		updateTimer();
	}
}

void updateTimer() {
	storeAndResetCursorLocation();

	currentTime = time(NULL);
	printf("\033[30;47m\033[%d;18H TIME: %ld ", size + 2, currentTime - startTime); //print time

	restoreCursorLocation();
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
		colorTileDark();
		modifyCursorData(41);
		flagcounter--;
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

	updateMineIndicator();
	checkForVictory();
}

void inputFlag() {
	printf("\033[48;5;255m\033[31m"); //color
	switch (getCursorData())
	{
	case 0: //clear
		printf("F\033[1D");
		modifyCursorData(2);
		flagcounter++;
		break;
	case 1: //mine
		printf("F\033[1D");
		modifyCursorData(3);
		flagcounter++;
		break;
	case 2: //clear and flag
		printf(" \033[1D");
		modifyCursorData(0);
		flagcounter--;
		break;
	case 3: //mine and flag
		printf(" \033[1D");
		modifyCursorData(1);
		flagcounter--;
		break;
	case 4:
		break;
	default:
		if (getCursorData() < 40) { //is not a cleared tiled
			if (getCursorData() < 13) { //number
				printf("\033[31mF\033[1D");
				modifyCursorData(getCursorData() * 3);
				flagcounter++;
			}
			else {
				printf("\033[31m \033[1D"); //number and flag
				modifyCursorData(getCursorData() / 3);
				flagcounter--;
			}
		}
		break;
	}

	updateMineIndicator();
}

void updateMineIndicator() {
	storeAndResetCursorLocation();
	printf("\033[30;47m\033[%d;6H MINES: %d ", size + 2, mines - flagcounter); //print mines - flagged tiles
	restoreCursorLocation();
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

void storeAndResetCursorLocation() {
	savecol = cursorcol;
	saverow = cursorrow;

	cursorcol = 0;
	cursorrow = 0;
}

void restoreCursorLocation() {
	cursorcol = savecol;
	cursorrow = saverow;
	printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3);
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

void revealAdjacent() {
	int savecol = cursorcol;
	int saverow = cursorrow;

	if (getCursorData() >= 5) { //is a number
		revealNumber();
	}
	if (getCursorData() != 0 || getClearVisit() != 0) {
		return;
	}
	modifyClearVisit(1); //mark current tile as visited
	modifyCursorData(41); //mark tile for victory check
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
}

void revealNumber() {
	if (getCursorData() < 5 || getCursorData() > 40) { //tile is not a number or has already been cleared
		return;
	}

	if (getCursorData() >= 15) { //has mine nearby and is flagged
		modifyCursorData(getCursorData() / 3);
		flagcounter--;
		updateMineIndicator();
	}
	int realvalue = getCursorData() - 4;

	modifyCursorData(getCursorData() * 10); //mark tile for victory check

	int colorvalue = 0;
	switch (realvalue)
	{
	case 1:
		colorvalue = 27; //bright blue
		break;
	case 2:
		colorvalue = 28; //green
		break;
	case 3:
		colorvalue = 196; //bright red
		break;
	case 4:
		colorvalue = 17; //dark blue
		break;
	case 5:
		colorvalue = 88; //dark red
		break;
	case 6:
		colorvalue = 30; //dark cyan
		break;
	case 7:
		colorvalue = 16; //black
		break;
	case 8:
		colorvalue = 244; //grey
		break;
	default:
		break;
	}

	colorTileDark();

	//\033[text  \033[background
	//printf("\033[38;5;{ID}m\033[48;5;{ID}m");
	printf("\033[1D\033[37m[\033[1m\033[38;5;%dm%d\033[22;37m]\033[2D\033[47m", colorvalue, realvalue);
	//                     ^                  ^  ^           ^
	//                     [                  c  n           ]
}

void colorTileDark() {
	//printf("\033[1D\033[30;100m[ ]\033[2D\033[47m");
	printf("\033[1D\033[48;5;252m   \033[2D\033[47m");
}

void gameDefeat() {
	printMines();

	//color death tile
	printf("\033[1D\033[38;5;16m\033[48;5;196m[x]\033[2D\033[47m");

	int waitcounter = 0;
	while (gameCleared == 0) {
		printf("\033[30;47m\033[%d;16H HONESTLY QUITE INCREDIBLE ", size + 4);
		Sleep(trollingDelay);

		printf("\033[37;40m\033[%d;16H HONESTLY QUITE INCREDIBLE ", size + 4);
		Sleep(trollingDelay);

		if (waitcounter < 20) waitcounter++; //so it doesn't quit out almost instantly
		if (_kbhit()) {
			action = _getch();

			switch (action) {
			default:
				restartGame();
				break;
			}
		}
	}
}

void gameVictory() {
	int waitcounter = 0;
	while (gameCleared == 0) {
		printf("\033[30;47m\033[%d;16H SHEEEEEEEEEEEEEEEEEEEEESH ", size + 4);
		Sleep(trollingDelay);

		printf("\033[37;40m\033[%d;16H SHEEEEEEEEEEEEEEEEEEEEESH ", size + 4);
		Sleep(trollingDelay);

		if (waitcounter < 20) waitcounter++; //so it doesn't quit out almost instantly
		if (_kbhit()) {
			action = _getch();

			switch (action) {
			default:
				restartGame();
				break;
			}
		}
	}
}

void checkForVictory() {
	storeAndResetCursorLocation();

	int winningtiles = 0;
	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			//if cursordata in marked tile range
			if (getCursorData() >= 40) {
				winningtiles++;
			}
			moveRight();
		}
		moveDown();
		cursorcol = 0;
	}

	//printf("\033[30;47m\033[4;%dHwinningtiles: %d, area - mines: %d ", (size + 2) * 3, winningtiles, size * size - mines); //debugging

	restoreCursorLocation();

	//area minus mines should be the amount of clear tiles
	if (winningtiles == size * size - mines) {
		gameVictory();
	}
}

void restartGame() {
	gameCleared = 1; //mark game as cleared to break out of gameInputs()
}

void resetValues() {
	gameCleared = 0;
	flagcounter = 0;
	firstreveal = 0;

	//free up memory before re-init
	free(gridArr);
	free(visited);

	printf("\033[22;0;0m"); //reset styles
	printf("\033[2J"); //erase screen
	printf("\033[H"); //cursor to topleft
}

void printTileData() {
	storeAndResetCursorLocation();

	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			printf("\033[30m\033[%d;%dH[%d]", i + 1, j * 3 + 1, getCursorData());
			moveRight();
		}
		moveDown();
		cursorcol = 0;
	}

	restoreCursorLocation();
}

void printMines() {
	storeAndResetCursorLocation();

	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			if (getCursorData() == 1 || getCursorData() == 3) {
				printf("\033[30m\033[%d;%dH[x]", i + 1, j * 3 + 1);
			}
			moveRight();
		}
		moveDown();
		cursorcol = 0;
	}

	restoreCursorLocation();
}

void printNumbers() {
	storeAndResetCursorLocation();

	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			if (getCursorData() > 4) {
				revealNumber();
				//printf("\033[30m\033[%d;%dH[%d]", i + 1, j * 3 + 1, getCursorData()-4); //curiously enough this works fine while revealNumber() does some weird shit
			}
			moveRight();
		}
		moveDown();
		cursorcol = 0;
	}

	restoreCursorLocation();
}

void printVisits() {
	int counter = 0;
	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			printf("\033[%d;%dH[%d]", i + 1, j * 3 + 1, visited[counter]);
			counter++;
		}
		printf("\n");
	}
	printf("\033[%d;%dH", 1 + cursorrow, 2 + cursorcol * 3); //cursor to where it was
}
