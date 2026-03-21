#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <mmsystem.h>
#include <conio.h>
#include <easyx.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <tchar.h>
#pragma comment(lib, "winmm.lib")


// 双子星的颜色常量
#define COLOR_YANG_OUTER  RGB(0, 150, 255) // 青色：暗部（光晕）
#define COLOR_YANG_CORE   RGB(0, 255, 255) // 青色：亮部（核心）
#define COLOR_YIN_OUTER   RGB(200, 0, 200) // 洋红：暗部（光晕）
#define COLOR_YIN_CORE    RGB(255, 100, 255) // 洋红：亮部（核心）
#define COLOR_CORE_WHITE  RGB(255, 255, 255) // 极亮中心点
bool bgmState = false; // 音乐开关

// 双子星	
 struct Twin {
	float x, y; // 位置
	int radius; // 碰撞半径
	COLORREF color;
	// 用于制作尾迹
	float trail_x[10];   // 过去 10 帧的 X 坐标
	float trail_y[10];   // 过去 10 帧的 Y 坐标
};

 // 障碍物
 struct Obstacle {
	 float x, y;
	 float w, h;      // 障碍物的宽和高
	 bool active;     // 是否在屏幕上（存活状态）
	 COLORREF color;  // 障碍物颜色
 };
#define MAX_OBS 8   // 同屏最多 8 个障碍物
 struct Obstacle obs[MAX_OBS] = { 0 }; // 初始化为空
 int spawnTimer = 0; // 障碍物生成计时器

 enum State {
	 MENU, PLAYING, GAMEOVER
 };

TCHAR* numtoarr(int n) {
	static TCHAR arr[20];
	_stprintf(arr, _T("%d"), n);
	return arr;
}

// 创建按钮
void CreatButton(POINT pos, int width, int height, LPCTSTR text, COLORREF col) {
	setfillcolor(col);
	fillroundrect(pos.x, pos.y, pos.x + width, pos.y + height, 10, 10);
	setlinecolor(RGB(255, 255, 255)); // 白色边框
	setlinestyle(PS_SOLID, 2);        // 2像素宽
	roundrect(pos.x, pos.y, pos.x + width, pos.y + height, 10, 10);
	settextcolor(BLACK);
	settextstyle(24, 0, _T("微软雅黑"));
	int tw = textwidth(text);
	outtextxy(pos.x + (width - tw) / 2, pos.y + (height - textheight(text)) / 2, text);
}
// 点击区域检测
// 判定点 (mx, my) 是否在左上角为 (x, y)，宽 w 高 h 的矩形内
int isMouseInArea(int mx, int my, int x, int y, int w, int h) {
	if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
		return 1; // 在范围内
	}
	return 0; // 不在范围内
}

// 开始界面
void DrawHello(int mouseX, int mouseY) {
	POINT center = { getwidth() / 2, getheight() / 2 };
	COLORREF btnColor = RGB(255, 255, 255);
	if (isMouseInArea(mouseX, mouseY, center.x - 60, center.y - 30, 120, 40)) {
		btnColor = RGB(200, 200, 200); // 悬停变灰色
	}
	CreatButton({ center.x - 60, center.y - 30 }, 120, 40, _T("开始游戏"), btnColor);
	btnColor = RGB(255, 255, 255);
	if (isMouseInArea(mouseX, mouseY, center.x - 60, center.y + 30, 120, 40)) {
		btnColor = RGB(200, 200, 200); // 悬停变灰色
	}
	CreatButton({ center.x - 60, center.y + 30 }, 120, 40, _T("查看教程"), btnColor);

	COLORREF bgmBtnColor = RGB(100, 100, 100);
	if (bgmState)
		bgmBtnColor = RGB(100, 255, 100);
	CreatButton({ getwidth() - 60, 30 }, 45, 25, "音乐", bgmBtnColor);
}

// 背景音乐
void PlayBGM(const char* music) {
	static int id = 0;
	char cmd[256];
	char err[128] = "";
	sprintf(cmd, "open %s alias bgm%d", music, id);
	MCIERROR ret =  mciSendString(cmd, NULL, 0, NULL);
	if (ret) {
		mciGetErrorString(ret, err, sizeof(err));
		printf("[ERROR]: Failed to open music '%s' - %s\n", music, err);
	}
	sprintf(cmd, "play bgm%d repeat", id);
	ret = mciSendString(cmd, NULL, 0, NULL);
	if (ret) {
		mciGetErrorString(ret, err, sizeof(err));
		printf("[ERROR]: Failed to play music '%s' - %s\n", music, err);
	}
	id++;
}

// 绘制阳星
void DrawYangStar(struct Twin* yang) {
	// 1. 绘制光晕（稍大，颜色较暗）
	setfillcolor(COLOR_YANG_OUTER);
	setlinecolor(COLOR_YANG_OUTER);
	solidcircle((int)yang->x, (int)yang->y, yang->radius + 3); // 半径比实际大

	// 2. 绘制核心（实际碰撞半径，高亮霓虹色）
	setfillcolor(COLOR_YANG_CORE);
	setlinecolor(COLOR_YANG_CORE);
	solidcircle((int)yang->x, (int)yang->y, yang->radius);

	// 3. 绘制极亮中心点（非常小，纯白，模拟“超频”质感）
	setfillcolor(COLOR_CORE_WHITE);
	setlinecolor(COLOR_CORE_WHITE);
	solidcircle((int)yang->x, (int)yang->y, 3); // 极小
}
// 绘制阴星
void DrawYinStar(struct Twin* yin) {
	// 1. 绘制光晕（稍大，颜色较暗）
	setfillcolor(COLOR_YIN_OUTER);
	setlinecolor(COLOR_YIN_OUTER);
	solidcircle((int)yin->x, (int)yin->y, yin->radius + 3); // 半径比实际大
	// 2. 绘制核心（实际碰撞半径，高亮霓虹色）
	setfillcolor(COLOR_YIN_CORE);
	setlinecolor(COLOR_YIN_CORE);
	solidcircle((int)yin->x, (int)yin->y, yin->radius);
	// 3. 绘制极亮中心点（非常小，纯白，模拟“超频”质感）
	setfillcolor(COLOR_CORE_WHITE);
	setlinecolor(COLOR_CORE_WHITE);
	solidcircle((int)yin->x, (int)yin->y, 3); // 极小
}

void HandleInput(struct Twin* yang, struct Twin* yin) {
	float moveSpeed = 5.0f;

	// === 玩家 1：阳星 (W A S D) ===
	if (GetAsyncKeyState('W') & 0x8000) yang->y -= moveSpeed;
	if (GetAsyncKeyState('S') & 0x8000) yang->y += moveSpeed;
	if (GetAsyncKeyState('A') & 0x8000) yang->x -= moveSpeed;
	if (GetAsyncKeyState('D') & 0x8000) yang->x += moveSpeed;

	// === 玩家 2：阴星 (方向键) ===
	if (GetAsyncKeyState(VK_UP) & 0x8000)    yin->y -= moveSpeed;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)  yin->y += moveSpeed;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)  yin->x -= moveSpeed;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) yin->x += moveSpeed;

	if (yang->x < 0) yang->x = 0;
	if (yang->x > 800) yang->x = 800;
	if (yang->y < 0) yang->y = 0;
	if (yang->y > 600) yang->y = 600;

	if (yin->x < 0) yin->x = 0;
	if (yin->x > 800) yin->x = 800;
	if (yin->y < 0) yin->y = 0;
	if (yin->y > 600) yin->y = 600;
}

// 1. 简易碰撞检测 (AABB 矩形碰撞)
// 将小球视为一个正方形，判断是否与障碍物矩形重叠
bool CheckCollision(struct Twin* star, struct Obstacle* ob) {
	if (!ob->active) return false;

	// 碰撞判定逻辑（简单粗暴且高效）
	if (star->x + star->radius > ob->x &&
		star->x - star->radius < ob->x + ob->w &&
		star->y + star->radius > ob->y &&
		star->y - star->radius < ob->y + ob->h) {
		return true;
	}
	return false;
}

// 2. 生成与更新障碍物 (核心灵魂：打破对称！)
void UpdateObstacles() {
	float obsSpeed = 6.0f; // 障碍物飞过来的速度

	// 计时器：每隔 45 帧生成一个新障碍物
	spawnTimer++;
	if (spawnTimer >= 45) {
		spawnTimer = 0;
		// 在数组里找一个处于“非激活”状态的空位生成新障碍物
		for (int i = 0; i < MAX_OBS; i++) {
			if (!obs[i].active) {
				obs[i].active = true;
				obs[i].w = 30 + rand() % 50; // 宽度随机 30~80
				obs[i].h = 30 + rand() % 50; // 高度随机 30~80
				obs[i].x = 800; // 从屏幕最右侧出现

				// 【核心设计】：Y轴完全随机，迫使玩家上下乱窜躲避！
				obs[i].y = rand() % (600 - (int)obs[i].h);

				// 给个危险的红色霓虹感
				obs[i].color = RGB(255, 50, 50);
				break; // 生成一个就够了，跳出循环
			}
		}
	}

	// 更新存活障碍物的位置
	for (int i = 0; i < MAX_OBS; i++) {
		if (obs[i].active) {
			obs[i].x -= obsSpeed; // 向左移动
			// 如果移出了屏幕左侧，回收它
			if (obs[i].x + obs[i].w < 0) {
				obs[i].active = false;
			}
		}
	}
}

// 3. 绘制障碍物
void DrawObstacles() {
	for (int i = 0; i < MAX_OBS; i++) {
		if (obs[i].active) {
			setfillcolor(obs[i].color);
			setlinecolor(RGB(255, 255, 255)); // 加个白边框更明显
			fillrectangle((int)obs[i].x, (int)obs[i].y,
				(int)(obs[i].x + obs[i].w), (int)(obs[i].y + obs[i].h));
		}
	}
}

int main() {
	srand((unsigned)time(NULL));
	initgraph(800, 600, EX_NOMINIMIZE | EX_SHOWCONSOLE | EX_DBLCLKS); //EX_NOCLOSE | EX_NOMINIMIZE
	setbkcolor(RGB(77, 194, 195));	
	setbkmode(TRANSPARENT);
	State gameState = MENU; // 游戏状态为菜单界面

	// 帧率控制
	int totalFrames = 0; // 总帧数
	char str[100] = "";
	const clock_t FPS = 1000 / 60; // 目标帧率为60 FPS | 16ms / frame
	clock_t startTime = 0;
	clock_t frametime = 0;

	Twin yang = { 400, 150, 5, COLOR_YANG_CORE }; // 初始化阳星
	Twin yin = { 400, 450, 5, COLOR_YIN_CORE }; // 初始化阴星
	int score = 0;

	int curMouseX = 0;
	int curMouseY = 0;
	ExMessage msg = { 0 };
	// 游戏主循环
	while (true)
	{
		startTime = clock(); // 记录开始时间
		sprintf(str, "Frame: %d", totalFrames++);

		settextcolor(WHITE);
		settextstyle(20, 0, _T("Arial"));

		cleardevice();
		BeginBatchDraw(); // 双缓冲开始

		outtextxy(20, 20, str);// 写入帧数信息

		// 读取鼠标消息
		while (peekmessage(&msg,EX_MOUSE | EX_KEY)) {
			if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {
				printf("[LOG]:按下 ESC 键，退出游戏\n"); 
				exit(0);
			}
			switch (msg.message) {
			case WM_MOUSEMOVE:
				curMouseX = msg.x;
				curMouseY = msg.y;
				break;
			case WM_LBUTTONDOWN: 
				printf("[LOG]:鼠标左键按下 pos (%d, %d)\n", msg.x, msg.y);
				if(isMouseInArea(msg.x, msg.y, getwidth() / 2 - 60, getheight() / 2 - 30, 120, 40) && gameState == MENU) {
					printf("[LOG]:点击了开始游戏按钮\n");
					gameState = PLAYING; // 切换到游戏状态
				}
				else if(isMouseInArea(msg.x, msg.y, getwidth() / 2 - 60, getheight() / 2 + 30, 120, 40)) {
					printf("[LOG]:点击了查看教程按钮\n");
					MessageBox(NULL, _T("这是一个双子星游戏，使用 WASD 键控制阳星移动，阴星会镜像同步。你的目的是尽可能的得到更高的分数！"), _T("游戏教程"), MB_OK);
				}
				else if (isMouseInArea(msg.x, msg.y, getwidth() - 60, 30, 45, 25)) {
					printf("[LOG]:点击了音乐开关\n");
					if (bgmState) {
						bgmState = false;
						mciSendString("close all", NULL, 0, NULL);
					}
					else {
						bgmState = true;
						PlayBGM("assets/M500003kNDLh2UjjDP.mp3");
					}
				}
				break;
			case WM_RBUTTONDOWN:
				printf("[LOG]:鼠标右键按下 pos (%d, %d)\n", msg.x, msg.y);
				break;
			case WM_MOUSEWHEEL:
				printf("[LOG]:鼠标滚轮滚动 pos (%d, %d) dir (%d)\n", msg.x, msg.y, msg.wheel);
				break;
			case WM_LBUTTONDBLCLK:
				printf("[LOG]:鼠标左键双击 pos (%d, %d)\n", msg.x, msg.y);
				break;
			}
		}

		// 菜单界面逻辑 
		if (gameState == MENU)
			DrawHello(msg.x, msg.y);
		// 游戏时界面
		else if (gameState == PLAYING) {
			setbkcolor(RGB(10, 10, 25));
			HandleInput(&yang, &yin);
			// 障碍物逻辑
			UpdateObstacles(); // 更新障碍物坐标并随机生成
			DrawObstacles();
			// 死亡判定
			for (int i = 0; i < MAX_OBS; i++) {
				if (CheckCollision(&yang, &obs[i]) || CheckCollision(&yin, &obs[i])) {
					printf("[LOG]: 玩家撞毁！Game Over！\n");
					gameState = GAMEOVER; // 切换到死亡状态
				}
			}
			DrawYangStar(&yang);
			DrawYinStar(&yin);
		// 游戏结束界面
		} else if (gameState == GAMEOVER) {
			settextcolor(RED);
			settextstyle(40, 0, _T("Arial"));
			outtextxy(getwidth() / 2 - 100, getheight() / 2 - 20, _T("游戏结束！"));
		}
			
		EndBatchDraw(); // 双缓冲结束

		// 帧率控制
		frametime = clock() - startTime;
		if(totalFrames % 60==0)
			printf("[LOG]:帧 %d 耗时 %ld ms time(s):%d\n", totalFrames, frametime, totalFrames /60);
		if (frametime < FPS) {
			Sleep(FPS-frametime); // 控制帧率约为60 FPS
		}
	}
	closegraph();
	return 0;
}