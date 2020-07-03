#include "stdafx.h"
#include<iostream>
#include<windows.h>
#include<string>
#include<cstdlib>
#include<ctime>
#include<fstream>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

using namespace std;

const int WIDTH = 12;
const int HEIGHT = 24;

const int GAME_WIDTH_SCALE = WIDTH - 2; // We do not count borders
const int GAME_HEIGHT_SCALE = HEIGHT - 4; // nor invisible space

const int BLOCK_WIDTH = 5;
const int BLOCK_HEIGHT = 5;
const int BLOCK_CENTRE = 2;
const int BLOCK_COUNT = 9;

const char BORDER = '#';
const char BLOCK = '@';
const char EMPTY = ' ';
const char UPPER_BORDER = '*';

const int RESET_X = WIDTH / 2;
const int RESET_Y = 2; // Safety because blockMap centre is (2,2)
const int USER_INPUT_CHANCES = 6; // User input chances before gravity
const int TIME_DELAY = 360; // Time delay before gravity // 360/6 = 60

const int GRAPHICAL_OFFSET = 30;
const int BORDER_SLIM = 1;
const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = GRAPHICAL_OFFSET*GAME_HEIGHT_SCALE + BORDER_SLIM;

int globalUpdate = 31; // DO NOT SET BELOW 2!!!
bool globalEnableHackVision = false;
long long highScore = 0;

void muteManager(bool mute, sf::Music &theme);

// Game engine related functions
string toString(int number)
{
	string str = "";
	bool firstEntry = true;

	while (number != 0 || firstEntry)
	{
		firstEntry = false;
		str = (char)('0' + number % 10) + str;
		number /= 10;
	}

	return str;
}

void mapMaker(char map[HEIGHT][WIDTH+1])
{
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			if (j == 0 || i == HEIGHT - 1 || j == WIDTH - 1)
			{
				map[i][j] = BORDER;
			}
			else
			{
				map[i][j] = EMPTY;
			}

			if (i <= 2 && j != 0 && j != WIDTH - 1) map[i][j] = UPPER_BORDER;
		}
	}
}

void consoleMapDrawer(char map[HEIGHT][WIDTH+1])
{
	system("CLS");
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			cout << map[i][j];
		}
		cout << endl;
	}
}

void shapeMaker(char blockMap[BLOCK_HEIGHT][BLOCK_WIDTH+1], char type)
{

	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			blockMap[i][j] = EMPTY;
		}
	}

	switch (type)
	{

		case 'L':
		{
			blockMap[2][1] = BLOCK;
			blockMap[2][2] = BLOCK;
			blockMap[2][3] = BLOCK;
			blockMap[3][1] = BLOCK;
			break;
		}

		case 'T':
		{
			blockMap[2][1] = BLOCK;
			blockMap[2][2] = BLOCK;
			blockMap[2][3] = BLOCK;
			blockMap[3][2] = BLOCK;
			break;
		}

		case 'X':
		{
			blockMap[1][2] = BLOCK;
			blockMap[3][2] = BLOCK;
			blockMap[2][2] = BLOCK;
			blockMap[2][3] = BLOCK;
			blockMap[2][1] = BLOCK;
			break;
		}

		case 'D':
		{
			blockMap[2][2] = BLOCK;
			break;
		}

		case 'B':
		{
			blockMap[2][2] = BLOCK;
			blockMap[3][2] = BLOCK;
			blockMap[2][3] = BLOCK;
			blockMap[3][3] = BLOCK;
			break;
		}

		case 'P': // Backwards L
		{
			blockMap[2][1] = BLOCK;
			blockMap[2][2] = BLOCK;
			blockMap[2][3] = BLOCK;
			blockMap[3][3] = BLOCK;
			break;
		}

		case 'Z': // Backwards staircase
		{
			blockMap[2][1] = BLOCK;
			blockMap[2][2] = BLOCK;
			blockMap[3][2] = BLOCK;
			blockMap[3][3] = BLOCK;
			break;
		}

		case 'S': // Staircase
		{
			blockMap[2][2] = BLOCK;
			blockMap[2][3] = BLOCK;
			blockMap[3][1] = BLOCK;
			blockMap[3][2] = BLOCK;
			break;
		}

		case 'I': // Line
		{
			blockMap[2][1] = BLOCK;
			blockMap[2][2] = BLOCK;
			blockMap[2][3] = BLOCK;
			blockMap[2][4] = BLOCK;
			break;
		}

	}
}

void overwriteMap(char map[HEIGHT][WIDTH+1], char blockMap[BLOCK_HEIGHT][BLOCK_WIDTH+1], int x, int y, int centre)
{
	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			if (blockMap[i][j] != EMPTY)
			{
				map[y + i - centre][x + j - centre] = BLOCK; // x,y is of centre so we need to subsrtract centre offset
			}
		}
	}
	for (int i = 0; i <= 2; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			if (j != 0 && j != WIDTH - 1)map[i][j] = UPPER_BORDER; // Fix borders
		}
	}
}

void removeShape(char map[HEIGHT][WIDTH+1], char blockMap[BLOCK_HEIGHT][BLOCK_WIDTH+1], int x, int y, int centre)
{
	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			if (blockMap[i][j] == BLOCK) map[i + y - centre][j + x - centre] = EMPTY; // Remove old shape
		}
	}
}

void applyGravity(char map[HEIGHT][WIDTH+1], char blockMap[BLOCK_HEIGHT][BLOCK_WIDTH+1], int &x, int &y, int centre, bool &isBound)
{
	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			if (blockMap[i][j] == BLOCK) // if it is a block we check if we have blocks below it
			{
				isBound = (i == BLOCK_HEIGHT - 1 || blockMap[i + 1][j] == EMPTY) &&
					(map[y - centre + i + 1][x - centre + j] != EMPTY && map[y - centre + i + 1][x - centre + j] != UPPER_BORDER);
			}
		}
	}

	if (!isBound)
	{
		removeShape(map, blockMap, x, y, centre);
		y++;
		overwriteMap(map, blockMap, x, y, centre); // Make new one after gravity
	}
}

void copyBlockArray(char arr1[BLOCK_HEIGHT][BLOCK_WIDTH+1], char arr2[BLOCK_HEIGHT][BLOCK_WIDTH+1])
{
	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			arr1[i][j] = arr2[i][j];
		}
	}
}

void rotationManager(char blockMapRotateOverwrite[BLOCK_HEIGHT][BLOCK_WIDTH+1], int &rotationCCW, char blockType)
{
	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			blockMapRotateOverwrite[i][j] = EMPTY;
		}
	}

	rotationCCW = (rotationCCW + 1) % 4;

	switch (blockType)
	{
		case 'L':
		{
			if (rotationCCW == 0)
			{
				shapeMaker(blockMapRotateOverwrite, 'L');
			}
			if (rotationCCW == 1)
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[3][2] = BLOCK;
				blockMapRotateOverwrite[3][3] = BLOCK;


			}
			if (rotationCCW == 2)
			{
				blockMapRotateOverwrite[1][3] = BLOCK;
				blockMapRotateOverwrite[2][1] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[2][3] = BLOCK;
			}
			if (rotationCCW == 3)
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[3][2] = BLOCK;
				blockMapRotateOverwrite[1][1] = BLOCK;
			}

			break;
		}

		case 'T':
		{
			if (rotationCCW == 0)
			{
				shapeMaker(blockMapRotateOverwrite, 'T');
			}
			if (rotationCCW == 1)
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[3][2] = BLOCK;
				blockMapRotateOverwrite[2][3] = BLOCK;

			}
			if (rotationCCW == 2)
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][1] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[2][3] = BLOCK;
			}
			if (rotationCCW == 3)
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[3][2] = BLOCK;
				blockMapRotateOverwrite[2][1] = BLOCK;
			}
			break;
		}
		case 'X':
		{
			shapeMaker(blockMapRotateOverwrite, 'X');
			break;
		}
		case 'D':
		{
			shapeMaker(blockMapRotateOverwrite, 'D');
			break;
		}
		case 'B':
		{
			shapeMaker(blockMapRotateOverwrite, 'B');
			break;
		}
		case 'P': // Backwards L
		{
			if (rotationCCW == 0)
			{
				shapeMaker(blockMapRotateOverwrite, 'P');
			}
			if (rotationCCW == 1)
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[3][2] = BLOCK;
				blockMapRotateOverwrite[1][3] = BLOCK;


			}
			if (rotationCCW == 2)
			{
				blockMapRotateOverwrite[1][1] = BLOCK;
				blockMapRotateOverwrite[2][1] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[2][3] = BLOCK;
			}
			if (rotationCCW == 3)
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[3][2] = BLOCK;
				blockMapRotateOverwrite[3][1] = BLOCK;
			}

			break;
		}
		case 'Z': // Backwards staircase
		{
			if (rotationCCW % 2 == 0)
			{
				shapeMaker(blockMapRotateOverwrite, 'Z');
			}
			else
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[2][1] = BLOCK;
				blockMapRotateOverwrite[3][1] = BLOCK;
			}
			break;
		}
		case 'S': // Staircase
		{
			if (rotationCCW % 2 == 0)
			{
				shapeMaker(blockMapRotateOverwrite, 'S');
			}
			else
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[2][3] = BLOCK;
				blockMapRotateOverwrite[3][3] = BLOCK;
			}
			break;
		}

		case 'I': // Line
		{
			if (rotationCCW % 2 == 0)
			{
				shapeMaker(blockMapRotateOverwrite, 'I');
			}
			else
			{
				blockMapRotateOverwrite[1][2] = BLOCK;
				blockMapRotateOverwrite[2][2] = BLOCK;
				blockMapRotateOverwrite[3][2] = BLOCK;
				blockMapRotateOverwrite[4][2] = BLOCK;
			}
			break;
		}
		default:
			cout << "ERROR IN ROTATION MANAGER"<<endl;
	}
}

bool canRotate(char map[HEIGHT][WIDTH+1], char currentBlock[BLOCK_HEIGHT][BLOCK_WIDTH+1], char rotatedBlock[BLOCK_HEIGHT][BLOCK_WIDTH+1], int x, int y)
{
	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			if (rotatedBlock[i][j] != EMPTY)
			{
				char tempRot = map[y - BLOCK_CENTRE + i][x - BLOCK_CENTRE + j]; // tempRot is map points where block will land after rotation
				char tempCur = currentBlock[i][j];

				if (tempRot != EMPTY&&tempRot != UPPER_BORDER&&tempCur == EMPTY) return false; // cause tempCur is already drawn in map should be ignored
			}
		}
	}

	return true;
}

void userInput(char map[HEIGHT][WIDTH+1], char currentBlock[BLOCK_HEIGHT][BLOCK_WIDTH+1], int &x, int &y, int centre, bool &userGravity, int &rotationCCW, char blockType, bool &isBound, sf::Clock &deltaRotationTime, bool &hasRotated, bool &restarter, bool &isPaused, sf::Clock &deltaHardDropTime, sf::Clock &deltaMuteTime, bool &mute, sf::Music &theme, sf::Clock &deltaHacks, bool &hardCore, sf::Clock &deltaHardCore, bool &load, bool &toSave, sf::Clock deltaSave)
{

	if (!isBound) // Cannot move or rotate if figure is about to be bound, if not checked there will be rare cases of floating figures
	{
		if (GetAsyncKeyState(VK_UP) && deltaRotationTime.getElapsedTime().asMilliseconds()>200)
		{
			deltaRotationTime.restart(); // if not timed some crazy rotation speeds can be achieved
			char rotated[BLOCK_HEIGHT][BLOCK_WIDTH+1];
			rotationManager(rotated, rotationCCW, blockType);

			if (canRotate(map, currentBlock, rotated, x, y))
			{
				removeShape(map, currentBlock, x, y, BLOCK_CENTRE);
				overwriteMap(map, rotated, x, y, BLOCK_CENTRE);
				copyBlockArray(currentBlock, rotated);
				hasRotated = true;

			}
			else rotationCCW--; // Rotation manager increments rotation no matter what
		}

		if (GetAsyncKeyState(VK_RIGHT) && !GetAsyncKeyState(VK_LEFT))
		{
			int mostRight;
			bool hasChanged;
			bool canMove = true;

			for (int i = 0; i < BLOCK_HEIGHT; i++)
			{
				hasChanged = false;

				for (int j = 0; j < BLOCK_WIDTH; j++)
				{
					if (currentBlock[i][j] == BLOCK)
					{
						mostRight = j;
						hasChanged = true;
					}

				}

				if (hasChanged)
				{
					if (map[y - centre + i][x - centre + (mostRight + 1)] != EMPTY && map[y - centre + i][x - centre + (mostRight + 1)] != UPPER_BORDER)//if we have block on the right we cannot move right
					{
						canMove = false;
						break;
					}
				}
			}

			if (canMove)
			{
				removeShape(map, currentBlock, x, y, BLOCK_CENTRE);
				x++;
				overwriteMap(map, currentBlock, x, y, BLOCK_CENTRE);
			}
		}// Right check

		if (!GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_LEFT))
		{
			int mostLeft;
			bool hasChanged;
			bool canMove = true;

			for (int i = 0; i < BLOCK_HEIGHT; i++)
			{
				hasChanged = false;
				for (int j = 0; j < BLOCK_WIDTH; j++)
				{
					if (currentBlock[i][j] == BLOCK)
					{
						mostLeft = j;
						hasChanged = true;
						break;// To stop at first left block
					}

				}

				if (hasChanged)
				{
					if (map[y - centre + i][x - centre + (mostLeft - 1)] != EMPTY && map[y - centre + i][x - centre + (mostLeft - 1)] != UPPER_BORDER)
					{
						canMove = false;
						break;
					}
				}
			}

			if (canMove)
			{
				removeShape(map, currentBlock, x, y, BLOCK_CENTRE);
				x--;
				overwriteMap(map, currentBlock, x, y, BLOCK_CENTRE);
			}
		}// Left check

		bool isHardDrop = false;

		if (GetAsyncKeyState(VK_SPACE) && deltaHardDropTime.getElapsedTime().asMilliseconds()>500) // Gotta check it after others because it changes isBound which may cause floating figures if not handled correctly
		{
			isHardDrop = true;
			while (!isBound)
			{
				applyGravity(map, currentBlock, x, y, BLOCK_CENTRE, isBound);
			}
			deltaHardDropTime.restart();
		}

		if (GetAsyncKeyState(VK_DOWN) && !isHardDrop && !isBound) // No need to speed up things if we are hardDropping the figure
		{
			userGravity = true;
		}
	}

	if (GetAsyncKeyState(VK_ESCAPE))
	{
		Sleep(200);
		isPaused = true;
	}

	if (GetAsyncKeyState('R'))
	{
		restarter = true;
	}

	if (GetAsyncKeyState('M') && deltaMuteTime.getElapsedTime().asMilliseconds()>500)
	{
		mute = !mute;
		deltaMuteTime.restart();
		muteManager(mute, theme);
	}

	if (GetAsyncKeyState('T') && deltaHacks.getElapsedTime().asMilliseconds()>500)
	{
		globalEnableHackVision = globalEnableHackVision? false:true;
		deltaHacks.restart();
	}

	if (GetAsyncKeyState('H') && deltaHardCore.getElapsedTime().asMilliseconds()>500)
	{
		hardCore = hardCore ? false : true;
		deltaHardCore.restart();
	}

	if (GetAsyncKeyState('S') && deltaSave.getElapsedTime().asSeconds()>3 )
	{
		toSave = true;
		deltaSave.restart();
	}

	if (GetAsyncKeyState('L'))
	{
		load = true;
	}
}

bool isFullLine(char map[HEIGHT][WIDTH+1], int y) // Should check for multilines aswel
{
	for (int i = 1; i < WIDTH - 1; i++)
	{
		if (map[y][i] != BLOCK)
		{
			return false;
		}

	}
	return true;
}

void removeLineAndMoveDown(char map[HEIGHT][WIDTH+1], int y)
{
	for (int i = y; i >= 4; i--)
	{
		for (int j = 1; j < WIDTH - 1; j++)
		{
			map[i][j] = map[i - 1][j];
		}
	}
	for (int j = 1; j < WIDTH - 1; j++)
	{
		map[3][j] = EMPTY;
	}
}

char randomBlockTypeReturn()
{
	srand(time(0));
	int random = 1 + rand() % (BLOCK_COUNT - 2); 

	switch (random)
	{
	case 1:
		return 'L';
	case 2:
		return 'T';
	case 3:
		return 'B';
	case 4:
		return 'P';
	case 5:
		return 'Z';
	case 6:
		return 'S';
	case 7:
		return 'I';
	}
}

bool gameOver(char map[HEIGHT][WIDTH+1], char currentBlock[BLOCK_HEIGHT][BLOCK_WIDTH+1], int x, int y)
{
	for (int i = 0; i < BLOCK_HEIGHT; i++)
	{
		for (int j = 0; j < BLOCK_WIDTH; j++)
		{
			if (currentBlock[i][j] != EMPTY&&map[i - BLOCK_CENTRE + y][j - BLOCK_CENTRE + x] == UPPER_BORDER) // if we have figure out of visible space its game over
			{
				return true;
			}
		}
	}

	return false;
}

void respawnRandomBlock(char map[HEIGHT][WIDTH+1], int &x, int &y, char &blockType, bool &isBound, int &rotationCCW, char currentBlock[BLOCK_HEIGHT][BLOCK_WIDTH+1])
{
	rotationCCW = 0;
	x = RESET_X;
	y = RESET_Y;
	blockType = randomBlockTypeReturn();
	shapeMaker(currentBlock, blockType);
	isBound = false;
}

//SFML functions
void drawStaticInterface(sf::RenderWindow &window, sf::RectangleShape vertical, sf::RectangleShape horizontal, sf::Text textScore, sf::Text &numberScore, sf::Text textLine, sf::Text &numberLine, sf::Sprite &spaceSprite, int highScore, sf::Text hiscore, sf::Sprite controls)
{
	if (globalEnableHackVision)
	{
		window.draw(spaceSprite);
		window.draw(vertical);
		vertical.setPosition(sf::Vector2f(GRAPHICAL_OFFSET*GAME_WIDTH_SCALE,0));
		window.draw(vertical);
	}

	for (int i = 0; i <= GAME_WIDTH_SCALE&&!globalEnableHackVision; i++)
	{
		vertical.setPosition(sf::Vector2f(GRAPHICAL_OFFSET*i, 0));
		window.draw(vertical);
	}

	for (int i = 0; i <= GAME_HEIGHT_SCALE && !globalEnableHackVision; i++) // GAME_HEIGHT + 1 BORDERS
	{
		horizontal.setPosition(sf::Vector2f(0, GRAPHICAL_OFFSET*i));
		window.draw(horizontal);
	}

	window.draw(textScore);
	window.draw(numberScore);
	window.draw(textLine);
	window.draw(numberLine);
	window.draw(controls);
	window.draw(hiscore);
}

void drawMap(char map[HEIGHT][WIDTH+1], sf::RenderWindow &window, sf::Sprite sprite)
{
	for (int y = 0; y <= HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			if (map[y][x] == BLOCK)
			{
				sprite.setPosition((x - 1)*GRAPHICAL_OFFSET, (y - 3)*GRAPHICAL_OFFSET); // x-1 because we have one border ont the left, y-3 because we have 3 "invisible" spaces up
				window.draw(sprite);
			}
		}
	}
}

void SmoothUserGravity(char map[HEIGHT][WIDTH+1], char currentBlock[BLOCK_HEIGHT][BLOCK_WIDTH+1], int x0, int y0, sf::RenderWindow &window, sf::Sprite sprite, sf::RectangleShape vertical, sf::RectangleShape horizontal, sf::Text textScore, sf::Text &numberScore, sf::Text textLine, sf::Text numberLine, int delta, sf::Sprite spaceSprite, sf::Text &hiscore, sf::Sprite &controlsSprite)
{
	removeShape(map, currentBlock, x0, y0, BLOCK_CENTRE); // Clearing console map for using drawMap multiple times without the current block

	int ry = y0 - 1;
	int errors = 0;

	for (int i = 1; i < globalUpdate; i++)
	{
		sf::Clock deltaTime;
		window.clear();
		drawStaticInterface(window, vertical, horizontal, textScore, numberScore, textLine, numberLine,spaceSprite,highScore,hiscore,controlsSprite);
		drawMap(map, window, sprite);

		for (int y = 0; y <= BLOCK_HEIGHT; y++)
		{
			int spriteY = (y + ry - BLOCK_CENTRE - 3)*(GRAPHICAL_OFFSET)+(GRAPHICAL_OFFSET*i) / globalUpdate;

			for (int x = 0; x <= BLOCK_WIDTH; x++)
			{
				int spriteX = (x + x0 - BLOCK_CENTRE - 1)*(GRAPHICAL_OFFSET);
				if (currentBlock[y][x] == BLOCK)
				{
					sprite.setPosition(spriteX, spriteY);
					window.draw(sprite); // Draw new sprite
				}

			}
		}

		window.display();

		int doubleDelta = delta / (globalUpdate-1) - deltaTime.getElapsedTime().asMilliseconds();
		if (doubleDelta >= 0)
		{
			Sleep(doubleDelta);
		}
		else
		{
			errors++;
			if (errors > globalUpdate / 2)
			{
				if (globalUpdate >= 4) //31->16->8->4->2 // Trying to adjust optimal values for framerate and speed.
				{
					globalUpdate = (globalUpdate + 1) / 2;
					cout << "Setting global update to " << globalUpdate<<endl;
				}
				errors = 0;	
			}
		}
	}

	overwriteMap(map, currentBlock, x0, y0, BLOCK_CENTRE); // Adding the block we removed earlier
}

bool isWindowClosed(sf::RenderWindow &window) // Handling events
{
	sf::Event eventFromWindow;
	while (window.pollEvent(eventFromWindow))
	{
		if (eventFromWindow.type == sf::Event::EventType::Closed)
		{
			window.close();
			return true;
		}
	}
	return false;
}

bool tryLoadFiles(sf::Font &font, sf::Texture &brickTexture, sf::SoundBuffer &lineClearSoundData, sf::SoundBuffer &rotateData, sf::Music &theme, sf::Texture &spaceTexture, sf::Texture &controls)
{
	bool error = false;
	// loading files
	if (!font.loadFromFile("stuff/Anke.ttf"))
	{
		cout << "stuff/Ankle.ttf is missing" << endl;
		error = true;
	}

	if (!brickTexture.loadFromFile("stuff/brick.png"))
	{
		cout << "stuff/brick.png is missing" << endl;
		error = true;
	}

	if (!lineClearSoundData.loadFromFile("stuff/LineClearSound.wav"))
	{
		cout << "stuff/LineClearSound.wav is missing" << endl;
		error = true;
	}

	if (!rotateData.loadFromFile("stuff/click.wav"))
	{
		cout << "stuff/click.wav is missing" << endl;
		error = true;
	}

	if (!theme.openFromFile("stuff/Tetris_theme.wav"))
	{
		cout << "stuff/Tetris_theme.wav is missing" << endl;
		error = true;
	}

	if (!spaceTexture.loadFromFile("stuff/space.png"))
	{
		cout << "stuff/Tetris_theme.wav is missing" << endl;
		error = true;
	}

	if (!controls.loadFromFile("stuff/lazyInterface.png"))
	{
		cout << "stuff/lazyInterface.png is missing" << endl;
		error = true;
	}

	return error;
}

void buildInterface(sf::Music &theme, sf::Sound &rotatePlayer, sf::Sound &lineClearSoundPlayer, sf::Sprite &brickSprite, sf::Text &textScore, sf::Text &textLine, sf::Text &numberScore, sf::Text &numberLine, sf::Text &gameOverText, sf::Text &pause, sf::RectangleShape &horizontalBorder, sf::RectangleShape &verticalBorder,sf::SoundBuffer &rotateData, sf::SoundBuffer &lineClearSoundData, sf::Texture &brickTexture, sf::Font &font, sf::Texture &spaceTexture, sf::Sprite &spaceSprite, sf::Texture &controlsTexture, sf::Sprite &controlsSprite, sf::Text &hiscore)
{
	theme.setLoop(true);
	theme.setVolume(75);
	theme.play();

	rotatePlayer.setBuffer(rotateData);
	lineClearSoundPlayer.setBuffer(lineClearSoundData);

	brickSprite.setTexture(brickTexture);
	brickSprite.setScale(0.5, 0.5);

	textScore.setString("SCORE:");
	textScore.setFont(font);
	textScore.setOrigin((textScore.getLocalBounds().width) / 2, (textScore.getLocalBounds().height) / 2); // Sets text origin position in the middle
	textScore.setPosition(sf::Vector2f((8.45 * WINDOW_WIDTH) / 16, 150-52));
	textScore.setCharacterSize(50);
	textScore.setFillColor(sf::Color::White);

	textLine.setString("LINES:");
	textLine.setFont(font);
	textLine.setOrigin((textScore.getLocalBounds().width) / 2, (textScore.getLocalBounds().height) / 2); // Sets text origin position in the middle
	textLine.setPosition(sf::Vector2f((10 * WINDOW_WIDTH) / 16, 240-65));
	textLine.setCharacterSize(50);
	textLine.setFillColor(sf::Color::White);

	numberScore.setFont(font);
	numberScore.setCharacterSize(50);
	numberScore.setFillColor(sf::Color::White);
	numberScore.setOrigin((numberScore.getLocalBounds().width) / 2, (numberScore.getLocalBounds().height) / 2); // Sets text origin position in the middle
	numberScore.setPosition(sf::Vector2f((11.5 * WINDOW_WIDTH) / 16,138-50));


	numberLine.setCharacterSize(50);
	numberLine.setFillColor(sf::Color::White);
	numberLine.setFont(font);
	numberLine.setOrigin((numberScore.getLocalBounds().width) / 2, 0);
	numberLine.setPosition(sf::Vector2f((11.5 * WINDOW_WIDTH) / 16, 4 * 50 -45));

	gameOverText.setCharacterSize(50);
	gameOverText.setString("GAME OVER");
	gameOverText.setPosition(0, 200);
	gameOverText.setFillColor(sf::Color::Cyan);
	gameOverText.setFont(font);

	hiscore.setCharacterSize(30);
	hiscore.setFillColor(sf::Color::White);
	hiscore.setFont(font);
	hiscore.setOrigin((numberScore.getLocalBounds().width) / 2, 0);
	hiscore.setString("Hi-SCORE: " + toString(highScore));
	hiscore.setPosition(sf::Vector2f((7.5 * WINDOW_WIDTH) / 16, 25));

	pause.setCharacterSize(50);
	pause.setString("PAUSED");
	pause.setPosition(55, 200);
	pause.setFillColor(sf::Color::Cyan);
	pause.setFont(font);

	horizontalBorder.setSize(sf::Vector2f(GRAPHICAL_OFFSET*GAME_WIDTH_SCALE + BORDER_SLIM, BORDER_SLIM));
	verticalBorder.setSize(sf::Vector2f(BORDER_SLIM, GRAPHICAL_OFFSET*GAME_HEIGHT_SCALE + BORDER_SLIM));

	horizontalBorder.setFillColor(sf::Color(200, 100, 50, 255));
	verticalBorder.setFillColor(sf::Color(200, 100, 50, 255));

	spaceSprite.setTexture(spaceTexture);
	spaceSprite.setScale(0.5, 0.5);

	controlsSprite.setTexture(controlsTexture);
	controlsSprite.setPosition(400,300);
}

void muteManager(bool mute, sf::Music &theme)
{
	if (mute)
	{
		theme.pause();
	}
	else
	{
		if (!(theme.getStatus() == sf::Music::Status::Playing)) 
		{
			theme.play();
		}
	}
}

void hardCoreMode(bool &toIncreaseHardcoreNumber, int &hardCoreNumber, sf::Sprite &brickSprite)
{
	if (toIncreaseHardcoreNumber)
	{
		if (hardCoreNumber < 250) hardCoreNumber += 10;
		else toIncreaseHardcoreNumber = false;
	}
	else
	{
		if (hardCoreNumber > -70) hardCoreNumber -= 10;
		else toIncreaseHardcoreNumber = true;
	}
	brickSprite.setColor(sf::Color(255, 255, 255, (hardCoreNumber>0) ? hardCoreNumber : 0));
}


// Save and load functions
void loadMap(char map[HEIGHT][WIDTH+1], int &x, int &y, char &blockType, int &rotation,long long &score,long long &linesCleared)
{
	ifstream reader;
	reader.open("stuff/data.txt");

	if (reader.is_open())
	{
		char inChar;

		for (int y = 0; y < HEIGHT; y++)
		{
			for (int x = 0; x < WIDTH; x++)
			{
				reader.get(inChar);
				if (inChar == '\n') reader >> inChar;
				map[y][x] = inChar;
			}
		}

		while (inChar != ';')
		{
			reader >> inChar;
		}

		reader >> inChar; // To remove ;

		string scoreFromFile, fileX, fileY, lines;

		while (inChar != ';')
		{
			fileX = fileX + inChar;
			reader >> inChar;
		}
		reader >> inChar;

		x = atoi(fileX.c_str());

		while (inChar != ';')
		{
			fileY = fileY + inChar;
			reader >> inChar;
		}

		y = atoi(fileY.c_str());

		reader >> inChar;
		blockType = inChar;

		reader >> inChar; // Will be ;
		reader >> inChar;

		rotation = inChar - '0';

		reader >> inChar; // Will be ;
		reader >> inChar; 

		while (inChar != ';')
		{
			scoreFromFile = scoreFromFile + inChar;
			reader>>inChar;
		}
		score = atoi(scoreFromFile.c_str());

		reader >> inChar;

		while (inChar != ';')
		{
			lines += inChar;
			reader >> inChar;
		}
		linesCleared = atoi(lines.c_str());
		reader.close();

	}
}

void saveMap(char map[HEIGHT][WIDTH+1], int &x, int &y, char &blockType, int &rotation, long long &score, long long &linesCleared)
{
	ofstream writer;
	writer.open("stuff/data.txt",ofstream::trunc);
	if (writer.is_open())
	{
		for (int y = 0; y < HEIGHT; y++)
		{
			for (int x = 0; x < WIDTH; x++)
			{
				writer << map[y][x];
			}
			writer << endl;
		}
		writer << ';' << x << ';' << y << ';' << blockType << ';' << rotation << ';' << score << ';' << linesCleared << ';';
		writer.close();
	}
}

void loadHighScore(long long &highScore)
{
	fstream reader;
	reader.open("stuff/score.txt");
	if (reader.is_open())
	{
		char ch;
		string hs;

		reader >> ch;

		while (ch != ';')
		{
			hs = hs + ch;
			reader >> ch;
		}

		highScore = atoi(hs.c_str());
		reader.close();
	}
}

void saveHighScore(long long highScore)
{
	ofstream writer;
	writer.open("stuff/score.txt",ofstream::trunc);

	if (writer.is_open())
	{
		writer << highScore << ";";
		writer.close();
	}
}

int main()
{
	sf::Event eventFromRenderWindow;
	sf::RenderWindow renderWindow(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "TETRIS", sf::Style::Titlebar | sf::Style::Close);
	sf::RectangleShape horizontalBorder, verticalBorder;
	sf::Text textScore, numberScore, textLine, numberLine, gameOverText, pause, hiscore;
	sf::Font font;
	sf::Texture brickTexture, spaceTexture, controlsTexture;
	sf::Sprite brickSprite, spaceSprite, controlsSprite;
	sf::Sound lineClearSoundPlayer, rotatePlayer;
	sf::SoundBuffer rotateData, lineClearSoundData;
	sf::Music tetrisTheme;
	sf::Clock deltaTime, deltaRotationTime, deltaHardDropTime, deltaMuteTime, deltaHacks, deltaHardcore, deltaSave;


	if (tryLoadFiles(font, brickTexture, lineClearSoundData, rotateData, tetrisTheme, spaceTexture,controlsTexture)) // Gonna output the error and close
	{
		Sleep(10000);
		return -1;
	}

	loadHighScore(highScore);
	buildInterface(tetrisTheme, rotatePlayer, lineClearSoundPlayer, brickSprite, textScore, textLine, numberScore, numberLine, gameOverText, pause, horizontalBorder, verticalBorder, rotateData, lineClearSoundData, brickTexture, font,spaceTexture,spaceSprite,controlsTexture,controlsSprite, hiscore);


	bool mute = false; // We do not want to reset this values after restart
	bool hardCore = false;
	bool load = false;

restart: // Fast and easy way to restart

	char map[HEIGHT][WIDTH+1];
	char blockType;
	char currentBlock[BLOCK_HEIGHT][BLOCK_WIDTH+1];

	bool isBound = true;
	bool firstEntry = true;
	bool restarter = false;
	bool isPaused = false;
	bool toIncreaseHardcoreNumber = false, hasBeenInHardcoreEnough = false, toSave =false;

	long long score = 0;
	long long linesCleared = 0;

	int x = RESET_X, y = RESET_Y;
	int rotationCCW = 0; // CCW = counter clock wise
	int hardCoreNumber = 250;

	mapMaker(map);

	if (load)
	{
		firstEntry = false;
		isBound = false;
		loadMap(map,x,y,blockType,rotationCCW, score, linesCleared);
		rotationCCW--; // Cause rotation manager will increment it
		rotationManager(currentBlock, rotationCCW, blockType);
		load = false;
		numberLine.setString(toString(linesCleared));
		renderWindow.clear();
		drawStaticInterface(renderWindow, verticalBorder, horizontalBorder, textScore, numberScore, textLine, numberLine, spaceSprite, highScore,hiscore,controlsSprite);
		drawMap(map, renderWindow, brickSprite);
		renderWindow.display();
	}

	// Gameloop
	while (true)
	{
		if (isWindowClosed(renderWindow))
		{
			return 0;
		}

		if (deltaHardcore.getElapsedTime().asSeconds() > 20 && hardCore)
		{
			hasBeenInHardcoreEnough = true;
		}
		else
		{
			hasBeenInHardcoreEnough = false;
		}

		bool playSoundLineClear = false;
		bool userGravity, hasRotated;

		if (isBound)
		{
			int currentLinesCleared = 0;

			if (gameOver(map, currentBlock, x, y) && !firstEntry)
			{
				break;
			}

			// LineCheck
			for (int i = -2; i <= 2; i++) // Only 4 full lines can be created with one figure but it is safer to check 5 lines because of possible future changes
			{
				if (isFullLine(map, y + i))
				{
					currentLinesCleared++;
					linesCleared++;
					removeLineAndMoveDown(map, y + i);
					score = score + 11 + linesCleared + currentLinesCleared * 3 + 3*hardCore*currentLinesCleared*hasBeenInHardcoreEnough; // To have more random score
					playSoundLineClear = true;
				}
			}

			numberLine.setString(toString(linesCleared));
			respawnRandomBlock(map, x, y, blockType, isBound, rotationCCW, currentBlock);
			overwriteMap(map, currentBlock, x, y, BLOCK_CENTRE);

			if (score > highScore)
			{
				highScore = score;
				saveHighScore(highScore);
				hiscore.setString("Hi-SCORE: " + toString(highScore));
			}
			firstEntry = false;
		}

		if (playSoundLineClear)
		{
			playSoundLineClear = false;
			lineClearSoundPlayer.play();
		}

		for (int i = 0; i < USER_INPUT_CHANCES; i++)
		{
			deltaTime.restart();

			if(hardCore) hardCoreMode(toIncreaseHardcoreNumber, hardCoreNumber, brickSprite);
			else brickSprite.setColor(sf::Color::White);

			hasRotated = false;
			userGravity = false;

			userInput(map, currentBlock, x, y, BLOCK_CENTRE, userGravity, rotationCCW, blockType, isBound, deltaRotationTime, hasRotated, restarter, isPaused, deltaHardDropTime, deltaMuteTime, mute, tetrisTheme, deltaHacks,hardCore, deltaHardcore,load,toSave,deltaSave);
			
			if (restarter)
			{
				goto restart;
			}
				
			if (hasRotated)
			{
				rotatePlayer.play();
			}

			if (toSave)
			{
				saveMap(map, x, y, blockType, rotationCCW, score, linesCleared);
				toSave = false;
			}

			while (isPaused)
			{
				if (isWindowClosed(renderWindow))
				{
					return 0;
				}

				renderWindow.clear(); // Redrawing cause minimization can remove graphic
				drawStaticInterface(renderWindow, verticalBorder, horizontalBorder, textScore, numberScore, textLine, numberLine, spaceSprite,highScore,hiscore, controlsSprite);
				drawMap(map, renderWindow, brickSprite);
				renderWindow.draw(pause);
				renderWindow.display();

				Sleep(200);
				
				if (GetAsyncKeyState(VK_ESCAPE))
				{
					Sleep(200); // Human reaction is 0.2 sec so 0.2 is a good number?
					isPaused = false;
					deltaTime.restart(); // Doesnt matter much but maybe is better to restart it for not immediate movement
					break;
				}

				if (GetAsyncKeyState('R'))
				{
					Sleep(100);
					goto restart;
				}

				if (GetAsyncKeyState('M')&&deltaMuteTime.getElapsedTime().asMilliseconds()>500)
				{
					deltaMuteTime.restart();
					mute = !mute;
					muteManager(mute, tetrisTheme);
				}

				if (GetAsyncKeyState('S'))
				{
					saveMap(map, x, y, blockType, rotationCCW, score, linesCleared);
				}

				if (GetAsyncKeyState('L'))
				{
					load = true;
					Sleep(200);
					goto restart;
				}
			}
			
			if (userGravity)
			{
				if (isBound) break;
				applyGravity(map, currentBlock, x, y, BLOCK_CENTRE, isBound);
			}

			int delta = (TIME_DELAY)/USER_INPUT_CHANCES - deltaTime.getElapsedTime().asMilliseconds();

			if (!userGravity)//in both cases we sleep pretty much TIME_DELAY
			{
				if(delta>1) Sleep(delta);
			}
			else  if (!isBound)
			{
				SmoothUserGravity(map, currentBlock, x, y, renderWindow, brickSprite, verticalBorder, horizontalBorder, textScore, numberScore, textLine, numberLine, (delta > 5) ? delta : 5, spaceSprite, hiscore, controlsSprite);
			}

			if (load) 
			{
				goto restart;
			}

			//Update window after all calculations
			numberScore.setString(toString(score));
			renderWindow.clear();
			drawStaticInterface(renderWindow, verticalBorder, horizontalBorder, textScore, numberScore, textLine, numberLine, spaceSprite,highScore,hiscore,controlsSprite);
			drawMap(map, renderWindow, brickSprite);
			renderWindow.display();
		}

		if (!isBound && !userGravity)
		{
			applyGravity(map, currentBlock, x, y, BLOCK_CENTRE, isBound);
		}
	}// Game loop end

	// Game over loop
	while (true)
	{
		renderWindow.clear(); // Redrawing because minimization can remove graphics
		drawStaticInterface(renderWindow, verticalBorder, horizontalBorder, textScore, numberScore, textLine, numberLine,spaceSprite,highScore,hiscore,controlsSprite);
		drawMap(map, renderWindow, brickSprite);
		renderWindow.draw(gameOverText);
		renderWindow.display();

		if (isWindowClosed(renderWindow)) // Not to freeze
		{
			return 0;
		}

		if (GetAsyncKeyState('R'))
		{
			goto restart;
		}

		if (GetAsyncKeyState('M') && deltaMuteTime.getElapsedTime().asMilliseconds() > 500)
		{
			deltaMuteTime.restart();
			mute = !mute;
			muteManager(mute, tetrisTheme);
		}

		if (GetAsyncKeyState('L'))
		{
			load = true;
			Sleep(200);
			goto restart;
		}

		if (GetAsyncKeyState(VK_ESCAPE))
		{
			break;
		}

		Sleep(100);
	}

	return 0;
}

