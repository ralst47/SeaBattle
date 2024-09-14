#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <gl/gl.h>
#include <math.h>
#include <wingdi.h>
#include <winuser.h>
#pragma comment(lib, "opengl32.lib")
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

typedef enum { COMPER, PLAYER } TEstep;

TEstep who = PLAYER;
int widthO, heightO;
int width = 1000;//размер окна по ширине
int height = 500;//размер окна по высоте
#define mapW 10//кол-во клеток по ширине
#define mapH 10//кол-во клеток по высоте
#define mapWS 0.99//масштаб по ширине 0-1.0 (float)
#define mapHS 0.99//масштаб по высоте 0-1.0 (float)
#define delay 0.5//задержка анимации
#define vectorDegree 2//степень вектора, 2 - по Пифагору на плоскости
//#define missToConer 4//ход, на котором бьёт по углам, если ни в одного не попал
//int missToConer = 4;
#define countStart 1//количество запусков, если >1, то комп бьет всегда
#define target TRUE//цель для стрельбы по моим кораблям FALSE или TRUE
BOOL smart = TRUE;//интелект компа FALSE или TRUE
int n;
#define shipSizes  { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 }
//#define shipSizes { 3 };
//#define shipSizes { 10, 9, 9, 8, 7, 6, 1, 1, 1, 1 };
//#define shipSizes { 10, 9, 8, 7, 6, 5, 5, 4, 4, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };
//#define shipSizes { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };
int ship_sizes[] = shipSizes;
int ship_count;
time_t seconds;

typedef struct
{
	BOOL ship;//корабль
	BOOL open;//клетка открыта
	BOOL miss;//мимо
	BOOL shot;//попал
	BOOL kill;//утопил
	signed char shipFront;//клеток до конца корабля с текущей
	signed char shipBack;//клеток до начала корабля с текущей
	BOOL dir;//напрвление FALSE-горизонт TRUE-вертикаль
	float theta;//угол поворота
	float scale;//масштаб при анимации
	time_t seconds;//начало анимации
} TCell;

TCell map[2][mapW][mapH];

typedef struct { int x; int y; BOOL shot; } TCoord;
struct { TCoord was; TCoord now; } go;

typedef enum { TOPGUNFLOAT, TOPGUNINT, TOPGUNSHIPS } TEtopGun;

typedef struct
{
	int shot;//подбито отсеков кораблей всего
	int keep;//осталось отсеков кораблей всего
	int miss;//количество ходов по промахам
	int miss1;//количество ходов по первичным промахам
	int all;//количество выстрелов всего
//	int gunF;//шаг, на котором будет GunFloat
//	BOOL coner;//выстрелы в углы промахов
	TEtopGun topGun;//название пушки
} TShips;
TShips step[2];//0 - поле PLAYER, 1 - поле COMPER

typedef struct
{
	float horizont;
	float vertical;
	float diagonal;
//	int ships;
} TDiag;

TDiag gun[mapW][mapH];

BOOL IsCellInMap(int x, int y)
{
	return x >= 0 && y >= 0 && x < mapW && y < mapH;
}

void ShowTopGun(int i, int j)//корабль
{
	float gap = 0.1;//зазор от края клетки 0-0,5
	glEnable(GL_LINE_SMOOTH);//сглаживание линий
	if (step[0].topGun == TOPGUNSHIPS) glLineWidth(sqrtf(mapWS * mapHS / mapW / mapH * widthO * heightO) * gap * 0.04);
	else glLineWidth(sqrtf(mapWS * mapHS / mapW / mapH * widthO * heightO) * gap * 0.01);
	if (map[0][i][j].ship) glColor3f(1.0, 0.0, 0.0);
	else
	{
		if (step[0].topGun == TOPGUNSHIPS) glColor3f(0.5, 1.0, 1.0);
		else glColor3f(0.5, 1.0, 0.5);
	}
	glBegin(GL_LINES);
	glVertex2f(0.5, gap);
	glVertex2f(0.5, 1 - gap);
	glVertex2f(gap, 0.5);
	glVertex2f(1 - gap, 0.5);
	glEnd();
	glDisable(GL_LINE_SMOOTH);
#define M_PI 3.14159265359
	float x, y, x0, y0;
	float cnt = (gun[i][j].diagonal);
	cnt = (cnt > 8) ? 8 : cnt;
	float a = M_PI * 2 / (cnt + 1);
	int dia = gun[i][j].diagonal;
	for (float jj = 0.01 + step[0].topGun * 0.1; jj <= 0.5 - gap; jj += (0.5 - gap - 0.01) / (cnt + 1))//0.1)
	{
		x0 = 0.5;
		y0 = jj + 0.5;
		glBegin(GL_LINE_LOOP);
		for (int ii = 0; ii < (cnt + 1) * 16; ii++)
		{
			x = sin(a * ii) * jj + 0.5;
			y = cos(a * ii) * jj + 0.5;
			if (x == x0 && y == y0 && ii > 0) break;
			glVertex2f(x, y);
		}
		glEnd();
	}
}

void ShowShip()//корабль
{
	float gap = 0.0;//зазор от края клетки 0-0,5
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(0.3, 1.0, 0.6); glVertex2f(gap, 1 - gap);
	glColor3f(0.2, 0.9, 0.5); glVertex2f(1 - gap, 1 - gap);
	glVertex2f(gap, gap);
	glColor3f(0.1, 0.8, 0.4); glVertex2f(1 - gap, gap);
	glEnd();
}

void ShowShipMiss()//(точка) промазал, мимо, холостой выстрел
{
	glEnable(GL_POINT_SMOOTH);
	glPointSize(sqrtf(mapWS * mapHS / mapW / mapH * widthO * heightO) * 0.15);
	glBegin(GL_POINTS);
	glColor3f(0, 0, 0);
	glVertex2f(0.5, 0.5);
	glEnd();
	glDisable(GL_POINT_SMOOTH);
}

void ShowShipShot()//попал, ранил, подбил
{
	float gap = 0.1;//зазор от края клетки 0-0,5
	glEnable(GL_LINE_SMOOTH);//сглаживание линий
	glLineWidth(sqrtf(mapWS * mapHS / mapW / mapH * widthO * heightO) * gap);
	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex2f(gap, gap);
	glVertex2f(1 - gap, 1 - gap);
	glVertex2f(gap, 1 - gap);
	glVertex2f(1 - gap, gap);
	glEnd();
	glDisable(GL_LINE_SMOOTH);
}

void ShowShipKill()//утопил
{
	float gap = 0.05;//зазор от края клетки 0-0,5
	//	glEnable(GL_POINT_SMOOTH);
	glPointSize(sqrtf(mapWS * mapHS / mapW / mapH * widthO * heightO) * gap * 1.45);
	glBegin(GL_POINTS);
	glColor3f(1, 0, 0);
	glVertex2f(gap, gap);
	glVertex2f(gap, 1 - gap);
	glVertex2f(1 - gap, 1 - gap);
	glVertex2f(1 - gap, gap);
	glEnd();
	//	glDisable(GL_POINT_SMOOTH);	glEnable(GL_LINE_SMOOTH);//сглаживание линий
	glLineWidth(sqrtf(mapWS * mapHS / mapW / mapH * widthO * heightO) * gap * 1.45);
	glColor3f(1, 0, 0);
	glBegin(GL_LINE_LOOP);
	glVertex2f(gap, gap);
	glVertex2f(gap, 1 - gap);
	glVertex2f(1 - gap, 1 - gap);
	glVertex2f(1 - gap, gap);
	glEnd();
	glDisable(GL_LINE_SMOOTH);


}

void ShowField()
{
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(0.3, 0.6, 1.0); glVertex2f(0, 1);
	glColor3f(0.2, 0.5, 0.9); glVertex2f(1, 1);	glVertex2f(0, 0);
	glColor3f(0.1, 0.4, 0.8); glVertex2f(1, 0);
	glEnd();
}

void ShowFieldOpen()
{
	float gap = 0.11;//зазор от края клетки 0-0,5
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(0, 0.2, 0.4); glVertex2f(gap, 1 - gap);
	glColor3f(0, 0.25, 0.5); glVertex2f(1 - gap, 1 - gap);	glVertex2f(gap, gap);
	glColor3f(0, 0.3, 0.6); glVertex2f(1 - gap, gap);
	glEnd();
}

void ScreeToOpenGL(HWND hwnd, int x, int y, float* ox, float* oy)//перевод координат окна в OPENGL
{
	RECT rct;
	GetClientRect(hwnd, &rct);
	*ox = ((2 * x / (float)rct.right - 1) - (1 - mapWS) / 2) * mapW / mapWS;
	*oy = ((1 - y / (float)rct.bottom) - (1 - mapHS) / 2) * mapH / mapHS;
	//	*ox = 2.0 * (x / (float)rct.right * mapW) - (float)width / height * mapW;//500*500
//	*ox = 2.0 * (x / (float)rct.right * mapW) - mapW;//1000*500
//	*oy = (mapH - y / (float)rct.bottom * mapH);// / mapHS - (1 - mapHS);
//	*ox = 2.0 * (x / (float)rct.right * mapW);
//	*oy = mapH - y / (float)rct.bottom * mapH;
}

void Generate_Ships()//Функция для генерации кораблей в случайных позициях на доске. Корабли могут быть различных размеров, например, 1, 2, 3, 4 клетки.
{
	if (countStart > 1)	srand(clock());
	else srand(time(NULL));
	memset(map, 0, sizeof(map));
	int ship_sizes[] = shipSizes;

	for (int n = 0; n < 2; n++)
	{
		//	int ship_sizes[] = { 1 };
		ship_count = sizeof(ship_sizes) / sizeof(ship_sizes[0]);// *(1 - n);
		for (int i = 0; i < ship_count; i++)
		{
			//if (n == 1) i = ship_count - 1;//1 корабль у компа
			int ship_size = ship_sizes[i];
			int x, y, dir;
			int iteration = 1000;//не помещаются корабли
			do
			{
				x = rand() % mapW;
				y = rand() % mapH;
				dir = rand() % 2;
				//dir = 0;
				iteration--;//не помещаются корабли
				if (iteration < 0) break;//не помещаются корабли
			} while (!is_valid(n, x, y, ship_size, dir));
			if (iteration < 0) continue;//не помещаются корабли
			for (int j = 0; j < ship_size; j++)
			{
				if (dir == 0)
				{
					map[n][x + j][y].ship = TRUE;
					map[n][x + j][y].shipFront = ship_size - j;
					map[n][x + j][y].shipBack = 1 + j;
					map[n][x + j][y].dir = FALSE;
				}
				else
				{
					map[n][x][y + j].ship = TRUE;
					map[n][x][y + j].shipFront = ship_size - j;
					map[n][x][y + j].shipBack = 1 + j;
					map[n][x][y + j].dir = TRUE;
				}
			}
			step[n].keep += ship_sizes[i];
		}
	}
}

int is_valid(int n, int x, int y, int size, int dir)//Функция проверяет, можно ли разместить корабль в данной позиции.Функция должна учитывать, что корабли не должны налегать друг на друга, и должны находиться в пределах доски
{
	if ((x + size > mapW) * (1 - dir) || (y + size * dir > mapH) * dir) return 0;//контроль размещения корабля в границах поля
	for (int i = x; i < x + size * (1 - dir) + dir; i++)//проверка горизонтального корабля
		for (int j = y; j < y + size * dir + (1 - dir); j++)//проверка вертикального корабля
			for (int ii = i - 1; ii <= i + 1; ii++)//проверка вокруг i
				for (int jj = j - 1; jj <= j + 1; jj++)//проверка вокруг j
					if (ii >= 0 && ii < mapW && jj >= 0 && jj < mapH)//контроль границ поля вокруг i и j
						if (map[n][ii][jj].ship != FALSE) return 0;
	return 1;
}

void Decorate_Game_Over(int i, int j)
{
	map[n][i][j].theta += 0.1;
	glTranslatef(0.5, 0.5, 0);
	map[n][i][j].scale = 0.1 * ((2 * n - 1) * cos(map[n][i][j].theta) - 1) + 1;
	glScalef(map[n][i][j].scale, map[n][i][j].scale, 0.0f);
	glTranslatef(-0.5, -0.5, 0);
}

void Decorate(int i, int j)//анимация стрельбы
{
	Sleep(1);
	glTranslatef(0.5, 0.5, 0);
	if (map[n][i][j].shot || map[n][i][j].kill)
	{
		map[n][i][j].theta += 1;
		map[n][i][j].scale = 0.1 * ((2 * n - 1) * cos(map[n][i][j].theta) - 1) + 1;
		glScalef(map[n][i][j].scale, map[n][i][j].scale, 0.0f);
	}
	else
	{
		srand(time(NULL));
		map[n][i][j].theta += 90 * (roundf(2 * (rand() % 2)) - 1);
		glRotatef(map[n][i][j].theta, 0, 0, 1);
		//	glRotatef(180, 0, 0, 1);
	}
	glTranslatef(-0.5, -0.5, 0);
}

BOOL Ship_Kill(int ii, int jj)
{
	n = who;
	int i, j;
	i = ii;
	j = jj;
	for (; map[n][i][j].shipFront > 0 && IsCellInMap(i, j);)
	{
		if (!map[n][i][j].shot) return FALSE;
		if (!map[n][i][j].dir) i++;
		else j++;
	}
	i = ii;
	j = jj;
	for (; map[n][i][j].shipBack > 0 && IsCellInMap(i, j);)
	{
		if (!map[n][i][j].shot) return FALSE;
		if (!map[n][i][j].dir) i--;
		else j--;
	}
	return TRUE;
}

int MaxFromArrayShips()
{
	int max = 0;
	for (int i = 0; i < ship_count; i++)
		if (ship_sizes[i] > max) max = ship_sizes[i];
	return max;
}

void DelFromArrayShips(int ship_size)
{
	for (int i = 0; i < ship_count; i++)
		if (ship_size == ship_sizes[i])
		{
			ship_sizes[i] = 0;
			break;
		}
}

void Ship_Kill_TRUE(int i, int j)
{
	n = who;
	int shipFront = map[n][i][j].shipFront;
	int shipBack = map[n][i][j].shipBack;
	int ship_size = shipBack + shipFront - 1;
	if (map[n][i][j].dir) j -= (shipBack - 1);
	else  i -= (shipBack - 1);
	for (int deck = 0; deck < ship_size; deck++)
	{
		map[n][i][j].kill = TRUE;
		map[n][i][j].seconds = time(NULL);
		if (map[n][i][j].dir) j++;
		else i++;
	}
	if (n == COMPER) DelFromArrayShips(ship_size);
}

void Miss_Around_Ship_TRUE(int i, int j)
{
	n = who;
	int shipFront = map[n][i][j].shipFront;
	int shipBack = map[n][i][j].shipBack;
	int ship_size = shipBack + shipFront - 1;
	if (map[n][i][j].dir) j -= (shipBack - 1);
	else  i -= (shipBack - 1);
	for (int deck = 0; deck < ship_size; deck++)
	{
		for (int ii = i - 1; ii <= i + 1; ii++)//проверка вокруг i
			for (int jj = j - 1; jj <= j + 1; jj++)//проверка вокруг j
				if (IsCellInMap(ii, jj))//контроль границ поля вокруг i и j
					if (!map[n][ii][jj].ship && !map[n][ii][jj].kill && !map[n][ii][jj].open)
					{
						map[n][ii][jj].open = TRUE;
						map[n][ii][jj].miss = TRUE;
						map[n][ii][jj].seconds = time(NULL);
					}
		if (map[n][i][j].dir) j++;
		else i++;
	}
}

void Miss_Coner_Ship_TRUE(int i, int j)
{
	n = who;
	for (int ii = i - 1; ii <= i + 1; ii++)//проверка вокруг i
		for (int jj = j - 1; jj <= j + 1; jj++)//проверка вокруг j
			if (IsCellInMap(ii, jj))//контроль границ поля вокруг i и j
				if (!map[n][ii][jj].miss && !map[n][ii][jj].open && abs(i - ii) == abs(j - jj))
				{
					map[n][ii][jj].open = TRUE;
					map[n][ii][jj].miss = TRUE;
					map[n][ii][jj].seconds = time(NULL);
				}
}

float TopGunShotMinMaxFloat(int i, int j, int minmax)//minmax = -1/+1 - ищет min/max
{
	int mm;
	if (minmax < 0) mm = TopGunInt();
	if (minmax > 0) mm = 0;
	for (int jj = -1; jj <= 1; jj++)
		for (int ii = -1; ii <= 1; ii++)
			if (IsCellInMap(i + ii, j + jj))
				if (!map[0][i + ii][j + jj].open && abs(ii) != abs(jj))
				{
					if (minmax < 0)
					{
						if (ii != 0 && gun[i + ii][j + jj].horizont < mm) mm = gun[i + ii][j + jj].horizont;
						if (jj != 0 && gun[i + ii][j + jj].vertical < mm) mm = gun[i + ii][j + jj].vertical;
					}
					else if (minmax > 0)
					{
						if (ii != 0 && gun[i + ii][j + jj].horizont > mm) mm = gun[i + ii][j + jj].horizont;
						if (jj != 0 && gun[i + ii][j + jj].vertical > mm) mm = gun[i + ii][j + jj].vertical;
					}
				}
	return mm;
}

BOOL IsCellInClose(int i, int j, int dir, int shipSize)
{
	if (dir)
	{
		for (int jj = 0; jj < shipSize; jj++)
			if (map[0][i][j + jj].open || !IsCellInMap(i, j + jj)) return FALSE;
	}
	else
		for (int ii = 0; ii < shipSize; ii++)
			if (map[0][i + ii][j].open || !IsCellInMap(i + ii, j)) return FALSE;
	return TRUE;
}

int TopGunShips()
{
	step[0].topGun = TOPGUNSHIPS;
	memset(gun, 0, sizeof(gun));
	int ii, jj;
	int shipSize = MaxFromArrayShips();
	int dir1 = (shipSize == 1) ? 1 : 2;
	for (int dir = 0; dir < dir1; dir++)
		for (int j = 0; j < mapH; j++)		
			for (int i = 0; i < mapW; i++)
			{
/*
				for (int deck = 0; deck < shipSize; deck++)
					if (dir)
					{
						if (IsCellInMap(i, j + deck))
						{
							if (!map[0][i][j + deck].open) gun[i][j + deck].ships++;
							else break;
						}
					}
					else
					{
						if (IsCellInMap(i + deck, j))
						{
							if (!map[0][i + deck][j].open) gun[i + deck][j].ships++;
							else break;
						}
					}
*/
				ii = 0;
				jj = 0;
				for (int deck = 0; deck < shipSize; deck++)
				{
//					if (IsCellInMap(i + ii, j + jj))
					if (IsCellInClose(i, j, dir, shipSize))
							if (!map[0][i + ii][j + jj].open) gun[i + ii][j + jj].diagonal++;
							else break;
					if (dir) jj++; else ii++;
				}
			}
	int max = 0;
	for (int j = 0; j < mapH; j++)
		for (int i = 0; i < mapW; i++)
		{
			if (gun[i][j].diagonal > max) max = gun[i][j].diagonal;
		}
	return max;
}

int TopGunInt()
{
	step[0].topGun = TOPGUNINT;
	memset(gun, 0, sizeof(gun));
	for (int j = 0; j < mapH; j++)
	{
		int sum = 0;
		for (int i = 0; i < mapW; i++)
		{
			if (map[0][i][j].open)
			{
				sum = 0;
			}
			else
			{
				sum++;
				gun[i][j].horizont = sum;
			}
		}
		for (int i = mapW - 1; i >= 0; i--)
		{
			if (gun[i][j].horizont == 0)
			{
				sum = 0;
			}
			else
			{
				if (gun[i][j].horizont > sum) sum = gun[i][j].horizont;
				else gun[i][j].horizont = sum;
			}
		}
	}
	for (int i = 0; i < mapW; i++)
	{
		int sum = 0;
		for (int j = 0; j < mapH; j++)
		{
			if (map[0][i][j].open)
			{
				sum = 0;
			}
			else
			{
				sum++;
				gun[i][j].vertical = sum;
			}
		}
		for (int j = mapH - 1; j >= 0; j--)
		{
			if (gun[i][j].vertical == 0)
			{
				sum = 0;
			}
			else
			{
				if (gun[i][j].vertical > sum) sum = gun[i][j].vertical;
				else gun[i][j].vertical = sum;
			}
		}
	}
	for (int j = 0; j < mapH; j++)
		for (int i = 0; i < mapW; i++)
		{
			gun[i][j].diagonal = powf(gun[i][j].horizont, vectorDegree) + powf(gun[i][j].vertical, vectorDegree);
			if (countStart == 1)
			{
				if (vectorDegree == 2) gun[i][j].diagonal = (int)sqrt(gun[i][j].diagonal);
				else gun[i][j].diagonal = (int)powf(gun[i][j].diagonal, 1.0 / vectorDegree);
			}
		}
	int max = 0;
	for (int j = 0; j < mapH; j++)
		for (int i = 0; i < mapW; i++)
		{
			if (gun[i][j].diagonal > max) max = gun[i][j].diagonal;
		}
	return max;
}

float TopGunFloat()
{
	step[0].topGun = TOPGUNFLOAT;
	memset(gun, 0, sizeof(gun));
	float sum, rate;
	for (int j = 0; j < mapH; j++)
	{
		sum = 0;
		rate = 1;
		for (int i = 0; i < mapW; i++)
		{
			if (map[0][i][j].open)
			{
				sum = 0;
			}
			else
			{
				sum++;
				gun[i][j].horizont = sum;
			}
		}
		for (int i = mapW - 1; i >= 0; i--)
		{
			if (gun[i][j].horizont == 0)
			{
				sum = 0;
				rate = 1;
			}
			else
			{
				if (gun[i][j].horizont >= sum) sum = gun[i][j].horizont;
				else
				{
					if (gun[i][j].horizont > sum * 0.5) rate *= 1.001;
					else if (gun[i][j].horizont < sum * 0.5) rate /= 1.001;
					gun[i][j].horizont = sum * rate;
				}
			}
		}
	}
	for (int i = 0; i < mapW; i++)
	{
		sum = 0;
		rate = 1;
		for (int j = 0; j < mapH; j++)
		{
			if (map[0][i][j].open)
			{
				sum = 0;
			}
			else
			{
				sum++;
				gun[i][j].vertical = sum;
			}
		}
		for (int j = mapH - 1; j >= 0; j--)
		{
			if (gun[i][j].vertical == 0)
			{
				sum = 0;
				rate = 1;
			}
			else
			{
				if (gun[i][j].vertical >= sum) sum = gun[i][j].vertical;
				else
				{
					if (gun[i][j].vertical > sum * 0.5) rate *= 1.001;
					else if (gun[i][j].vertical < sum * 0.5) rate /= 1.001;
					gun[i][j].vertical = sum * rate;
				}
			}
		}
	}
	for (int j = 0; j < mapH; j++)
		for (int i = 0; i < mapW; i++)
		{
			gun[i][j].diagonal = powf(gun[i][j].horizont, vectorDegree) + powf(gun[i][j].vertical, vectorDegree);
			if (countStart == 1)
			{
				if (vectorDegree == 2) gun[i][j].diagonal = sqrt(gun[i][j].diagonal);
				else gun[i][j].diagonal = powf(gun[i][j].diagonal, 1.0 / vectorDegree);
			}
		}
	float max = 0;
	for (int j = 0; j < mapH; j++)
		for (int i = 0; i < mapW; i++)
		{
			if (gun[i][j].diagonal > max) max = gun[i][j].diagonal;
		}
	return max;
}

float TopGun()
{
	return TopGunFloat();
//	if (step[0].miss1 < 8) return TopGunShips(); else return TopGunFloat();//Вывод_54.262_2.4 Вывод_54.258_2.3
//	if (step[0].miss1 < 8) return TopGunShips(); else if (MaxFromArrayShips() > 1) return TopGunFloat(); else return TopGunInt();//Вывод_54.414_2.3 Вывод_54.184_2.3 Вывод_54.412_2.3
//	if (step[0].miss1 < 8) return TopGunShips(); else if (MaxFromArrayShips() > 1 || step[0].keep > 3) return TopGunFloat(); else return TopGunInt();//Вывод_54.370_2.4 Вывод_54.305_2.4 Вывод_54.263_2.4
//	if (step[0].miss1 < 9) return TopGunShips(); else if (MaxFromArrayShips() > 1 || step[0].keep > 2) return TopGunFloat(); else return TopGunInt();//Вывод_54.128_2.3 Вывод_54.228_2.3 Вывод_54.324_2.4
//	if (step[0].miss1 < 9) return TopGunShips(); else if (MaxFromArrayShips() > 1 || step[0].keep > 3) return TopGunFloat(); else return TopGunInt();//Вывод_54.317_2.3 Вывод_54.186_2.3 Вывод_54.213_2.3 Вывод_54.156_2.3

//	if (step[0].miss1 < 9) return TopGunShips(); else if (MaxFromArrayShips() > 1 || step[0].keep > 4) return TopGunFloat(); else return TopGunInt();//Вывод_54.330_2.3 Вывод_54.119_2.3 Вывод_54.440_2.4
//	if (step[0].miss1 < 9) return TopGunShips(); else if (MaxFromArrayShips() > 1 || step[0].keep > 1) return TopGunFloat(); else return TopGunInt();//Вывод_54.409_2.3 Вывод_54.350_2.4 Вывод_54.260_2.3
//	if (step[0].miss1 < 7) return TopGunShips(); else if (MaxFromArrayShips() > 1 || step[0].keep > 2) return TopGunFloat(); else return TopGunInt();//Вывод_54.218_2.3 Вывод_54.280_2.3 Вывод_54.315_2.5
//	if (step[0].miss1 < 7) return TopGunShips(); else if (MaxFromArrayShips() > 1 || step[0].keep > 4) return TopGunFloat(); else return TopGunInt();//Вывод_54.320_2.3 Вывод_54.245_2.3 Вывод_54.341_2.4

//	if (step[0].shot < 4) return TopGunShips(); else  return TopGunInt();//Вывод_54.300_2.1 Вывод_54.667_2.3 Вывод_54.693_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].shot < 16) return TopGunFloat(); return TopGunInt();//Вывод_54.568_2.2
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].shot < 10) return TopGunFloat(); return TopGunInt();//Вывод_54.634_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].shot < 8) return TopGunFloat(); return TopGunInt();//Вывод_54.639_2.2
//	if (step[0].shot < 4) return TopGunShips(); else if (MaxFromArrayShips() > 1) return TopGunFloat(); return TopGunInt();//Вывод_54.560_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (MaxFromArrayShips() > 2) return TopGunFloat(); return TopGunInt();//Вывод_54.675_2.6
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep <= 2) return TopGunFloat(); return TopGunInt();//Вывод_54.472_2.4
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep <= 4) return TopGunFloat(); return TopGunInt();//Вывод_54.542_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep <= 8) return TopGunFloat(); return TopGunInt();//Вывод_54.451_2.4
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep <= 1) return TopGunFloat(); return TopGunInt();//Вывод_54.402_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep <= 0) return TopGunFloat(); return TopGunInt();//Вывод_54.602_2.3

//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep >= 0) return TopGunFloat(); return TopGunInt();//Вывод_54.461_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 0) return TopGunFloat(); return TopGunInt();//Вывод_54.470_2.2 Вывод_54.491_2.2
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 1) return TopGunFloat(); return TopGunInt();//Вывод_54.626_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 2) return TopGunFloat(); return TopGunInt();//Вывод_54.501_2.4
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 3) return TopGunFloat(); return TopGunInt();//Вывод_54.486_2.4
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 4) return TopGunFloat(); return TopGunInt();//Вывод_54.631_2.4
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 5) return TopGunFloat(); return TopGunInt();//Вывод_54.637_2.4
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 6) return TopGunFloat(); return TopGunInt();//Вывод_54.475_2.3
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 7) return TopGunFloat(); return TopGunInt();//Вывод_54.671_2.4
//	if (step[0].shot < 4) return TopGunShips(); else if (step[0].keep > 8) return TopGunFloat(); return TopGunInt();//Вывод_54.620_2.3

//	if (step[0].all < step[0].miss + 4) return TopGunShips(); else  return TopGunInt();//Вывод_54.300_2.1
//	if (step[0].all < step[0].miss + 4) return TopGunShips(); else  return TopGunFloat();//Вывод_54.231_2.0

//if (step[0].all < step[0].miss + step[0].gunF) return TopGunInt(); else  return TopGunFloat();
///	if (step[0].all < step[0].miss + step[0].gunF) return TopGunShips(); else  return TopGunFloat();//Вывод_54.670_2.5
//	if (step[0].all < step[0].miss + 0) return TopGunShips(); else  return TopGunFloat();//Вывод_55.187_3.9
//	if (step[0].all < step[0].miss + 1) return TopGunShips(); else  return TopGunFloat();//Вывод_54.376_2.0
//	if (step[0].all < step[0].miss + 2) return TopGunShips(); else  return TopGunFloat();//Вывод_54.538_2.1
//	if (step[0].all < step[0].miss + 3) return TopGunShips(); else  return TopGunFloat();//Вывод_54.519_2.2
//	if (step[0].all < step[0].miss + 4) return TopGunShips(); else  return TopGunFloat();//Вывод_54.109_2.0 Вывод_54.375_2.3
//	if (step[0].all < step[0].miss1 + 4) return TopGunShips(); else  return TopGunFloat();//Вывод_54.498_2.2
//	if (step[0].all < step[0].miss + 5) return TopGunShips(); else  return TopGunFloat();//Вывод_54.151_2.2
//	if (step[0].all < step[0].miss + 6) return TopGunShips(); else  return TopGunFloat();//Вывод_54.807_2.5
//	if (step[0].all < step[0].miss + 7) return TopGunShips(); else  return TopGunFloat();//Вывод_54.883_2.7
//	if (step[0].all < step[0].miss + 8) return TopGunShips(); else  return TopGunFloat();//Вывод_54.714_2.4
//	if (step[0].all < step[0].miss + 9) return TopGunShips(); else  return TopGunFloat();//Вывод_54.839_2.4
//	if (step[0].all < step[0].miss + 10) return TopGunShips(); else  return TopGunFloat();//Вывод_55.006_2.5
//	return TopGunInt();//Вывод_54.799_2.3 Вывод_54.404_2.2 Вывод_54.287_2.1
//	return TopGunFloat();//Вывод_55.344_4.1 Вывод_55.199_3.9 Вывод_55.273_4.0
//	return TopGunShips();//Вывод_55.963_2.4 Вывод_54.852_2.1 Вывод_55.400_2.5
///	if (step[0].all < step[0].miss + step[0].gunF || MaxFromArrayShips() < 2) return TopGunShips(); else  return TopGunFloat();//Вывод_54.987_2.1
///	if (MaxFromArrayShips() < 2 || 3 < MaxFromArrayShips()) return TopGunShips(); else  return TopGunFloat();//Вывод_54.727_2.1
///	if (MaxFromArrayShips() < 2 || 2 < MaxFromArrayShips()) return TopGunShips(); else  return TopGunFloat();//Вывод_54.517_2.0
///	if (MaxFromArrayShips() < 3 || 3 < MaxFromArrayShips()) return TopGunShips(); else  return TopGunFloat();//Вывод_54.913_2.3
///	if (MaxFromArrayShips() < 0 || 3 < MaxFromArrayShips()) return TopGunShips(); else  return TopGunFloat();//Вывод_55.234_3.9
///	if (MaxFromArrayShips() < 2 || 4 < MaxFromArrayShips()) return TopGunShips(); else  return TopGunFloat();//Вывод_55.573_2.4
//	return TopGunShips();
///	if (step[0].miss1 == 0) return TopGunInt(); else if (step[0].miss1 % 2) return TopGunShips(); else return TopGunFloat();//Вывод_54.951_2.4

}

int OutFile(int n0, int n1, int n2, int n3)
{
	//	char s[40] = "Привет мир\n";
	FILE* f;
	f = fopen("Вывод.txt", "a");
	if (f != NULL) fprintf(f, "%d%s%d%s%d%s%d%s", n0, "\t", n1, "\t", n2, "\t", n3, "\n");
	fclose(f);
}

int Direction(int i, int j, int dir, int minmax)//minmax = -1/+1 - бьёт в меньшую/большую сторону
{
	if (minmax == 0) minmax = rand() % 2 * 2 - 1;
	if (dir)
	{
		if (j == 0) return 1; else if (j == mapH - 1) return -1;
		if (map[0][i][j - 1].open) return 1;
		else if (map[0][i][j + 1].open) return -1;
		if (gun[i][j - 1].vertical < gun[i][j + 1].vertical) return minmax;
		else if (gun[i][j - 1].vertical > gun[i][j + 1].vertical) return -minmax;
		else return rand() % 2 * 2 - 1;
	}
	else
	{
		if (i == 0) return 1; else if (i == mapW - 1) return -1;
		if (map[0][i - 1][j].open) return 1;
		else if (map[0][i + 1][j].open) return -1;
		if (gun[i - 1][j].horizont < gun[i + 1][j].horizont) return minmax;
		else if (gun[i - 1][j].horizont > gun[i + 1][j].horizont) return -minmax;
		else return rand() % 2 * 2 - 1;
	}
}

int Direction0(int i, int j, int dir, int minmax)//minmax = -1/+1 - бьёт в меньшую/большую сторону
{
	//if (minmax == 0) minmax = rand() % 2 * 2 - 1;
	int MaxShips = MaxFromArrayShips();
	if (dir)
	{
		if (j == 0) return 1; else if (j == mapH - 1) return -1;
		if (map[0][i][j - 1].open) return 1;
		else if (map[0][i][j + 1].open) return -1;
		if ((int)gun[i][j - 1].vertical < MaxShips) return -1;//размер корабля больше чем до границы или miss
		else if ((int)gun[i][j + 1].vertical < MaxShips) return 1;//размер корабля больше чем до границы или miss
		if (gun[i][j - 1].vertical < gun[i][j + 1].vertical) return minmax;
		else if (gun[i][j - 1].vertical > gun[i][j + 1].vertical) return -minmax;
		else return rand() % 2 * 2 - 1;
	}
	else
	{
		if (i == 0) return 1; else if (i == mapW - 1) return -1;
		if (map[0][i - 1][j].open) return 1;
		else if (map[0][i + 1][j].open) return -1;
		if ((int)gun[i - 1][j].horizont < MaxShips) return -1;//размер корабля больше чем до границы или miss
		else if ((int)gun[i + 1][j].horizont < MaxShips) return 1;//размер корабля больше чем до границы или miss
		if (gun[i - 1][j].horizont < gun[i + 1][j].horizont) return minmax;
		else if (gun[i - 1][j].horizont > gun[i + 1][j].horizont) return -minmax;
		else return rand() % 2 * 2 - 1;
	}
}

/*
BOOL OpenConer(int i, int j)//для стрельбы по углам от miss (промахи)
{
	for (int ii = i - 1; ii <= i + 1; ii += 2)//проверка вокруг i
		for (int jj = j - 1; jj <= j + 1; jj += 2)//проверка вокруг j
			if (IsCellInMap(ii, jj))//контроль границ поля вокруг i и j
			{
				if (map[0][ii][jj].open) return TRUE;
			}
	return FALSE;
}
*/

void SetCells(int i, int j)//проверка и установка открытых клеток - мимо, попал, утопил и подсчёт выстрелов
{
	n = who;
	if (map[n][i][j].open)
	{
		if (!map[n][i][j].shot && !map[n][i][j].miss) step[n].all++;//подсчёт всех выстрелов
		if (!map[n][i][j].ship && !map[n][i][j].miss)
		{
			step[n].miss++;//подсчёт промахов (выстрелы мимо)
			if (n == 0 && !go.now.shot) step[n].miss1++;//подсчёт первичных промахов (выстрелы мимо)
			map[n][i][j].miss = TRUE;
			map[n][i][j].seconds = time(NULL);
		}
		else if (map[n][i][j].ship && !map[n][i][j].shot)
		{
			if (step[n].keep >= 0)
			{
				step[n].keep--;//подсчёт оставшихся кораблей по палубам
				step[n].shot++;//подсчёт подбитых кораблей по палубам
			}
			map[n][i][j].shot = TRUE;
			if (!map[n][i][j].kill && n == 1) Miss_Coner_Ship_TRUE(i, j);//открытие клеток по углам отсека корабля
			map[n][i][j].seconds = time(NULL);
			if (Ship_Kill(i, j))//проверка утопление корабля
			{
				map[n][i][j].seconds = time(NULL);
				Ship_Kill_TRUE(i, j);//установка утопления корабля
				Miss_Around_Ship_TRUE(i, j);//открытие клеток во круг корабля
			}
		}
	}
}

void CompStep()//Исскуственный интелект, почти
{
	int i, j, ii, jj, edit;
	if (countStart > 1)	srand(clock());
	else srand(time(NULL));
	if (step[0].keep == 0) return;
	if (map[0][go.now.x][go.now.y].kill)
	{
		go.now.shot = FALSE;
		go.was.shot = FALSE;
	}
	if (go.was.shot)
	{
		i = go.now.x;
		j = go.now.y;
		ii = go.now.x - go.was.x;
		jj = go.now.y - go.was.y;
		if (map[0][i + ii][j + jj].miss || !IsCellInMap(i + ii, j + jj))
		{
			go.now = go.was;
			go.was.x = i;
			go.was.y = j;
			i = go.now.x;
			j = go.now.y;
			ii *= -1;
			jj *= -1;
		}
		edit = -1;
		do
		{
			edit++;
			i += ii;
			j += jj;
		} while (map[0][i][j].shot);
		go.was.x = go.now.x + ii * edit;
		go.was.y = go.now.y + jj * edit;
		go.now.x = i;
		go.now.y = j;
		map[0][i][j].open = TRUE;//*************************************
		SetCells(i, j);
		if (!map[0][i][j].ship)
		{
			go.now.x = i - 2 * ii;
			go.now.y = j - 2 * jj;
			go.now.shot = TRUE;
			who = PLAYER;
		}
	}
	else
	{
		if (go.now.shot)
		{
			i = go.now.x;
			j = go.now.y;
			if (smart)
			{
				map[0][i][j].open = FALSE;
				TopGunInt();
				if (gun[i][j].horizont == gun[i][j].vertical)
				{
					map[0][i][j].open = TRUE;
					TopGunFloat();
					int minmax = TopGunShotMinMaxFloat(i, j, 1);
					do
					{
						do
						{
							ii = rand() % 3 - 1;
							jj = rand() % 3 - 1;
						} while (!IsCellInMap(i + ii, j + jj) || abs(ii) == abs(jj));
					} while (gun[i + ii][j + jj].horizont != minmax && gun[i + ii][j + jj].vertical != minmax);
				}
				else
				{
					if (gun[i][j].horizont > gun[i][j].vertical)
					{
						map[0][i][j].open = TRUE;
						TopGunFloat();
						ii = Direction(i, j, 0, 1);
						jj = 0;
					}
					else if (gun[i][j].horizont < gun[i][j].vertical)
					{
						map[0][i][j].open = TRUE;
						TopGunFloat();
						ii = 0;
						jj = Direction(i, j, 1, 1);
					}
				}
			}
			else
			{
				do
				{
					do
					{
						ii = rand() % 3 - 1;
						jj = rand() % 3 - 1;
					} while (!IsCellInMap(i + ii, j + jj) || abs(ii) == abs(jj));
				} while (map[0][i + ii][j + jj].open);
			}
			i += ii;
			j += jj;
			go.was = go.now;
			go.now.x = i;
			go.now.y = j;
			map[0][i][j].open = TRUE;//*************************************
			SetCells(i, j);
			if (!map[0][i][j].ship)
			{
				go.now = go.was;
				go.was.shot = FALSE;
				who = PLAYER;
			}
		}
		else
		{
			float topGun = TopGun();
			//BOOL openConer;
			//step[0].coner = (missToConer < step[0].all + 1 && missToConer * 2 >= step[0].all + 1 && step[0].shot == 0) ? TRUE : FALSE;
			do
			{
				i =  rand() % mapW;
				j =  rand() % mapH;
				//openConer = OpenConer(i, j);
			//} while (map[0][i][j].open || !openConer && step[0].all > 0 && step[0].coner && smart || gun[i][j].diagonal != topGun && !step[0].coner && smart);
			} while (map[0][i][j].open || gun[i][j].diagonal != topGun && smart);
		go.now.x = i;
			go.now.y = j;
			map[0][i][j].open = TRUE;//*************************************
			SetCells(i, j);
			if (map[0][i][j].ship) go.now.shot = TRUE;
			else
			{
				go.now.shot = FALSE;
				who = PLAYER;
			}
		}
	}
	seconds = time(NULL);
}

void GridBox()
{
	glPushMatrix();
	glLoadIdentity();
	glLineWidth(4);

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);

	for (float i = 0; i <= mapW; i += mapW)
	{
		glVertex2f((2 * who - 1) * (0.5 + 0.5 * mapWS - i * mapWS / mapW), -mapHS * (2 * who - 1));
		glVertex2f((2 * who - 1) * (0.5 + 0.5 * mapWS - i * mapWS / mapW), +mapHS * (2 * who - 1));
	}
	for (float j = 0; j <= mapH; j += mapH)
	{
		glVertex2f((2 * who - 1) * (0.5 + 0.5 * mapWS), -mapHS + 2 * j * mapHS / mapH);
		glVertex2f((2 * who - 1) * (0.5 - 0.5 * mapWS), -mapHS + 2 * j * mapHS / mapH);
	}

	glEnd();

	glPopMatrix();
}

void GridStep()
{
	glPushMatrix();
	glLoadIdentity();
	glLineWidth(1);

	glColor3f(0, 0, 1);
	glBegin(GL_LINES);

	for (float i = 0; i <= mapW; i++)
	{
		glVertex2f((2 * !who - 1) * (0.5 + 0.5 * mapWS - i * mapWS / mapW), -mapHS * (2 * !who - 1));
		glVertex2f((2 * !who - 1) * (0.5 + 0.5 * mapWS - i * mapWS / mapW), +mapHS * (2 * !who - 1));
	}
	for (float j = 0; j <= mapH; j++)
	{
		glVertex2f((2 * !who - 1) * (0.5 + 0.5 * mapWS), -mapHS + 2 * j * mapHS / mapH);
		glVertex2f((2 * !who - 1) * (0.5 - 0.5 * mapWS), -mapHS + 2 * j * mapHS / mapH);
	}

	glEnd();

	glPopMatrix();
}

void Progress(float p)
{
	glPushMatrix();
	glLoadIdentity();
	glLineWidth(mapHS / mapH * heightO);

	//glColor3f(1 - p, p, 0);
	glColor3f((p < 0.5) ? 1 : 2 - 2 * p, (p < 0.5) ? 2 * p : 1, 0);

	glEnable(GL_LINE_SMOOTH);//сглаживание линий
	glEnable(GL_LINE_STIPPLE);//прерывистая линия
	glLineStipple(2, 0b0101010101010101);//маска

	glBegin(GL_LINES);

	//glColor3f(1, 0, 0);
	glVertex2f(0, 0);
	//glColor3f((p < 0.5) ? 1 : 2 - 2 * p, (p < 0.5) ? 2 * p : 1, 0);
	glVertex2f(p, 0);

	glEnd();

	glDisable(GL_LINE_STIPPLE);
	glDisable(GL_LINE_SMOOTH);

	glPopMatrix();
}

void GameShow()
{
	float k = 1;
	glLoadIdentity();
	if (k > 1) glScalef(mapWS / mapW / k, 2.0 * mapHS / mapH, 1);
	else glScalef(mapWS / mapW, 2.0 * k * mapHS / mapH, 1);
	int nn = (countStart == 1) ? 1 : 0;
	for (n = nn; n >= 0; n--)
	{
		glPushMatrix();
		glTranslatef(-mapW * 0.5 * (1 - (2 * n - 1) / mapWS), -mapH * 0.5, 0);
		//	glTranslatef(-mapW * 0.5 * (1 + 1 / mapWS), -mapH * 0.5, 0);//левое поле
		//	glTranslatef(-mapW * 0.5 * (1 - 1 / mapWS), -mapH * 0.5, 0);//правое поле
		//**************************************************************************************************************
		for (int j = 0; j < mapH; j++)
			for (int i = 0; i < mapW; i++)
			{
				glPushMatrix();
				glTranslatef(i, j, 0);

				if ((time(NULL) - map[n][i][j].seconds) <= delay && countStart == 1) Decorate(i, j);
				ShowField();
				if (step[n].keep == 0 && (step[0].keep == 0 || step[1].keep == 0) && countStart == 1) Decorate_Game_Over(i, j);
				if (step[0].keep == 0 && n == 1 && map[n][i][j].ship) ShowShip();//отображение кораблей компа
				//if (map[n][i][j].ship) ShowShip();//отображение кораблей компа всегда
				if (n == 0 && map[n][i][j].ship) ShowShip();//отображение моих кораблей
				if (map[n][i][j].open)
				{
					if (map[n][i][j].ship) ShowShip();
					if (map[n][i][j].miss) ShowShipMiss();
					if (map[n][i][j].shot) ShowShipShot();
					if (map[n][i][j].kill) ShowShipKill();
				}
				if (target && smart && n == 0 && gun[i][j].diagonal == TopGun() && !go.now.shot && TopGun() > 1 && countStart == 1 && step[0].miss1 > 0) ShowTopGun(i, j);//цель для стрельбы по моим кораблям
				glPopMatrix();
			}
		//**************************************************************************************************************
		glPopMatrix();
	}
}

void WindowsSize(widthO, heightO)
{
	float k = 0.5 * widthO / heightO;
	glViewport(0, 0, widthO, heightO);
	glLoadIdentity();
	if (k >= 1) glOrtho(-k, k, -1, 1, -1, 1);
	else glOrtho(-1, 1, -1 / k, 1 / k, -1, 1);
}

void Axis()
{
	glPushMatrix();
	glLoadIdentity();
	glLineWidth(1);
	glColor3f(1, 0, 0);

	glEnable(GL_LINE_SMOOTH);//сглаживание линий
	glEnable(GL_LINE_STIPPLE);//прерывистая линия
	glLineStipple(2, 0b1111100110011111);//маска

	glBegin(GL_LINES);
	glVertex2f(-1, 0);
	glVertex2f(1, 0);
	glVertex2f(0, -1);
	glVertex2f(0, 1);
	glVertex2f(-0.5, -1);
	glVertex2f(-0.5, 1);
	glVertex2f(0.5, -1);
	glVertex2f(0.5, 1);
	glEnd();

	glDisable(GL_LINE_STIPPLE);
	glDisable(GL_LINE_SMOOTH);

	glPopMatrix();
}

void GridWindow()
{
	glPushMatrix();
	glLoadIdentity();
	glLineWidth(1);
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_LINES);
	for (float i = 0; i <= mapW; i++)
	{
		glVertex2f(0.5 + 0.5 * mapWS - i * mapWS / mapW, -mapHS);
		glVertex2f(0.5 + 0.5 * mapWS - i * mapWS / mapW, mapHS);
	}
	for (float j = 0; j <= mapH; j++)
	{
		glVertex2f(0.5 + 0.5 * mapWS, -mapHS + 2 * j * mapHS / mapH);
		glVertex2f(0.5 - 0.5 * mapWS, -mapHS + 2 * j * mapHS / mapH);
	}
	glEnd();
	glPopMatrix();
}

void Grid()
{
	glPushMatrix();
	glLoadIdentity();
	glLineWidth(1);
	glColor3f(1, 0, 0);

	glBegin(GL_LINE_LOOP);
	glVertex2f(1, 1);
	glVertex2f(1, -1);
	glVertex2f(-1, -1);
	glVertex2f(-1, 1);
	glEnd();

	glColor3f(0, 0, 1);
	glBegin(GL_LINES);
	for (int m = -1; m < 2; m += 2)
	{
		for (float i = 0; i <= mapW; i++)
		{
			glVertex2f(m * (0.5 + 0.5 * mapWS - i * mapWS / mapW), -mapHS);
			glVertex2f(m * (0.5 + 0.5 * mapWS - i * mapWS / mapW), mapHS);
		}
		for (float j = 0; j <= mapH; j++)
		{
			glVertex2f(m * (0.5 + 0.5 * mapWS), -mapHS + 2 * j * mapHS / mapH);
			glVertex2f(m * (0.5 - 0.5 * mapWS), -mapHS + 2 * j * mapHS / mapH);
		}
	}
	glEnd();

	glPopMatrix();
}

void CoordsText()
{
	glPushMatrix();
	glLoadIdentity();
//	glEnable(GL_POINT_SMOOTH);
	glPointSize(sqrtf(mapWS * mapHS / mapW / mapH * widthO * heightO) * 0.1);
	glBegin(GL_POINTS);
	glColor3f(0, 0, 0);

	for (int m = -1; m < 2; m += 2)
	{
		for (float i = 0; i < mapW; i++)
			glVertex2f(m * (0.5 + 0.5 * mapWS - (i + 0.5) * mapWS / mapW), mapHS + mapHS / mapH);
		for (float j = 0; j < mapH; j++)
			glVertex2f(m * (0.5 + 0.5 * (mapWS + mapWS / mapW)) , -mapHS + 2 * (j + 0.5) * mapHS / mapH);
	}
	glEnd();
//	glDisable(GL_POINT_SMOOTH);
	glPopMatrix();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcex;
	HWND hwnd;
	HDC hDC;
	HGLRC hRC;
	MSG msg;
	BOOL bQuit = FALSE;

		/* register window class */
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_OWNDC;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "GLSample";
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


	if (!RegisterClassEx(&wcex))
		return 0;

	/* create main window */
	hwnd = CreateWindowEx(0,
		"GLSample",
		"Морской бой - © red king",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,//размер окна по X
		height,//размер окна по Y
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hwnd, nCmdShow);//отображение окна

	/* enable OpenGL for the window */
	EnableOpenGL(hwnd, &hDC, &hRC);
	WindowsSize(widthO, heightO);//размер окна при старте
	
	if (mapWS > 1 || mapHS > 2) return 0;

//	int missToConerStart = (countStart == 1) ? missToConer : 0;
//	int missToConerFinish = (countStart == 1) ? missToConer : 8;
//	for (missToConer = missToConerStart; missToConer <= missToConerFinish; missToConer++)
//	{
		int repeatStart = 1; int repeatFinish = countStart;//количество запусков
		if (countStart > 1) remove("Вывод.txt");
		FILE* f;					//вывод в файл
		f = fopen("Вывод.txt", "a");//вывод в файл
		int sum = 0, sumShot1 = 0;
		float mid, midShot1;
		int repeat;
		for (repeat = repeatStart; repeat <= repeatFinish; repeat++)
		{
			memset(step, 0, sizeof(step));
			Generate_Ships();
			//step[0].gunF = rand() % 2 + 3;
			go.now.shot = FALSE;
			go.was.shot = FALSE;
			int step0shot = 0;
			//hhDC = hDC;
			if (bQuit) break;
			/* program main loop */
			while (!bQuit && (step[0].keep > 0 || repeatStart == repeatFinish))
				//while (!bQuit)
			{
				/* check for messages */
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					/* handle or dispatch messages */
					if (msg.message == WM_QUIT)
					{
						bQuit = TRUE;
					}
					else
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				else
				{
					/* OpenGL animation code goes here */

					glClearColor(0.0f, 0.0f, 0.5f, 0.0f);//цвет окна
					glClear(GL_COLOR_BUFFER_BIT);

					glPushMatrix();
					if (countStart == 1) GridBox();

					if (countStart > 1)//комп ходит сам по себе
					{
						who = COMPER;
						CompStep();
					}
					else if (who == COMPER && time(NULL) - seconds >= delay + 0.5) CompStep();//<---очередь компа

					if (countStart == 1 || step[0].keep == 0) GameShow();

					if (countStart == 1 && (who == PLAYER || step[0].keep == 0 || step[1].keep == 0)) SetCursor(LoadCursor(NULL, IDC_ARROW));//курсор стрелка
					else if (who == COMPER && countStart == 1) SetCursor(LoadCursor(NULL, IDC_WAIT));//курсор ожидание (часы)
					/*
	Символическое имя	Описание
	IDC_ARROW	Стандартный курсор в виде стрелки
	IDC_CROSS	Курсор в виде перекрещивающихся линий
	IDC_IBEAM	Текстовый курсор в виде буквы "I"
	IDC_ICON	Пустая пиктограмма
	IDC_SIZE	Курсор в виде четырех стрелок, указывающих в разных направлениях
	IDC_SIZENESW	Двойная стрелка, указывающая в северо-восточном и юго-западном направлении
	IDC_SIZENS	Двойная стрелка, указывающая в севером и южном направлении
	IDC_SIZENWSE	Двойная стрелка, указывающая в северо-западном и юго-восточном направлении
	IDC_SIZEWE	Двойная стрелка, указывающая в восточном и западном направлении
	IDC_UPARROW	Вертикальная стрелка
	IDC_WAIT	Курсор в виде песочных часов
		*/

					glPopMatrix();

					CoordsText();
					//Grid();
					//GridWindow();
					//Axis();
					if (countStart > 1) Progress((float)(repeat - repeatStart) / (repeatFinish - repeatStart));

					if (step[0].shot == 1 && step0shot == 0) step0shot = step[0].all;

					if (countStart == 1 || step[0].keep == 0) SwapBuffers(hDC);
					if (countStart == 1) Sleep(10);
				}
			}
			Sleep(10);
			sumShot1 += step0shot;
			sum += step[0].all;
			if (f != NULL) fprintf(f, "%d%s%d%s%d%s", repeat, "\t", step[0].all, "\t", step0shot, "\n");//вывод в файл
		}
		fclose(f);																							//вывод в фай;
		if (countStart > 1)
		{
			midShot1 = roundf((float)sumShot1 / (repeat - 1) * 10) / 10;
			mid = roundf((float)sum / (repeat - 1) * 1000) / 1000;
			char str[20];
	//		sprintf(str, "%s%2.d%s%.3f%s%.1f%s", "Вывод_", missToConer, "_", mid, "_", midShot1, ".txt");
			sprintf(str, "%s%.3f%s%.1f%s", "Вывод_", mid, "_", midShot1, ".txt");
			rename("Вывод.txt", str);
		}
//	}
	/* shutdown OpenGL */
	DisableOpenGL(hwnd, hDC, hRC);

	/* destroy the window explicitly */
	DestroyWindow(hwnd);
	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:
	{
		POINTFLOAT pf;
		ScreeToOpenGL(hwnd, LOWORD(lParam), HIWORD(lParam), &pf.x, &pf.y);
		int x = floorf(pf.x);
		int y = floorf(pf.y);
		BOOL canComp;
		if (IsCellInMap(x, y))
			if (map[1][x][y].open == FALSE && step[1].keep > 0 && who == PLAYER)
			{
				map[1][x][y].open = TRUE;
				canComp = !map[1][x][y].ship && !map[1][x][y].miss;
				SetCells(x, y);
				if (canComp || step[1].keep == 0)
				{
					who = COMPER;
					seconds = time(NULL);
				}
			}
	}
	break;

	case WM_SIZE://область вывода изображения OpenGL устанавливается в размер клиентского окна
		widthO = LOWORD(lParam);
		heightO = HIWORD(lParam);
		WindowsSize(widthO, heightO);
		break;

	case WM_DESTROY:
		return 0;

	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
	}
	break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
	PIXELFORMATDESCRIPTOR pfd;

	int iFormat;

	/* get the device context (DC) */
	*hDC = GetDC(hwnd);

	/* set the pixel format for the DC */
	ZeroMemory(&pfd, sizeof(pfd));

	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	iFormat = ChoosePixelFormat(*hDC, &pfd);

	SetPixelFormat(*hDC, iFormat, &pfd);

	/* create and enable the render context (RC) */
	*hRC = wglCreateContext(*hDC);

	wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL(HWND hwnd, HDC hDC, HGLRC hRC)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hwnd, hDC);
}