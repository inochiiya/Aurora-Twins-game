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

// 双星的颜色常量
#define COLOR_YANG_OUTER  RGB(0, 150, 255) // 光晕:青色，暗
#define COLOR_YANG_CORE   RGB(0, 255, 255) // 高亮核心:青色
#define COLOR_YIN_OUTER   RGB(200, 0, 200) // 光晕：洋红，暗
#define COLOR_YIN_CORE    RGB(255, 100, 255) // 高亮核心：杨红
#define COLOR_CORE_WHITE  RGB(255, 255, 255) // 中心点：白

// 全局变量
bool bgmState = false; // 音乐开启状态
bool wallActive = true; // 隔离墙是否激活
float totalScore = 0; // 计分
float currentDifficulty = 1.0f; // 难度系数

// 双星	
 struct Twin {
	float x, y; // 位置
	int radius; // 半径
	COLORREF color;
	// 用于制作尾迹
	float trail_x[10];   // 过去 10 帧的 X 坐标
	float trail_y[10];   // 过去 10 帧的 Y 坐标
};

 // 背景星空
#define STAR_COUNT 50
 struct Star {
	 float x, y;
	 float speed;
	 int brightness; // 亮度
 }stars[STAR_COUNT];

 // 道具
 enum PowerType {
	 WALL_BREAKER,SHIELD,SLOW_DOWN // 消墙、盾、减速
 };
 struct PowerUp {
	 float x, y;
	 int radius;
	 bool active;
	 //PowerType type;
	 COLORREF color;
 } wallBreaker = { 0, 0, 12, false, RGB(255, 215, 0) }; // 金黄色，显眼


 // 障碍物
 struct Obstacle {
	 float x, y;
	 float w, h;      // 宽和高
	 bool active;     // 是否在屏幕上（存活状态）
	 COLORREF color;  // 障碍物颜色
 };
#define MAX_OBS 32   // 同屏最多 8 个障碍物
 struct Obstacle obs[MAX_OBS] = { 0 };
 int spawnTimer = 0; // 生成计时器

 // 游戏状态
 enum State {
	 MENU, PLAYING,PAUSE, GAMEOVER
 };


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

// 判定点 (mx, my) 是否在左上角起始位置为 (x, y)，宽 w 高 h 的矩形内
int isMouseInArea(int mx, int my, int x, int y, int w, int h) {
	if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
		return 1;
	}
	return 0;
}

// 播放/暂停背景音乐
void PlayBGM(const char* music) {
	static bool isMusicOpened = false; // 音频是否已经打开
	MCIERROR ret = 0;
	char cmd[256];
	char err[128] = "";
	printf("[LOG]: bgmState '%d'; isMusicOpened '%d'\n", bgmState, isMusicOpened);
	if (!isMusicOpened) { // 第一次打开音乐
		sprintf(cmd, "open %s alias bgm", music);
		ret = mciSendString(cmd, NULL, 0, NULL);
		if (ret == 0) {
			isMusicOpened = true;
		}
		else {
			mciGetErrorString(ret, err, sizeof(err));
			printf("[ERROR]: Failed to open music '%s' - %s\n", music, err);
			return;
		}
	}
	// 切换播放/暂停状态
	if (bgmState) {
		ret = mciSendString("pause bgm", NULL, 0, NULL);
		bgmState = false;
	}
	else {
		ret = mciSendString("play bgm repeat", NULL, 0, NULL);
		bgmState = true;
	}
	if (ret != 0) {
		mciGetErrorString(ret, err, sizeof(err));
		printf("[ERROR]: MCI Control Error - %s\n", err);
	}
}

// 绘制开始界面
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

// 绘制暂停菜单
void DrawPause(int mouseX, int mouseY) {
	POINT center = { getwidth() / 2, getheight() / 2 };

	// 背景板遮罩
	setfillcolor(RGB(20, 20, 40));
	fillroundrect(center.x - 100, center.y - 120, center.x + 100, center.y + 120, 15, 15);

	// 继续游戏按钮
	COLORREF btnCol1 = isMouseInArea(mouseX, mouseY, center.x - 60, center.y - 80, 120, 40) ? RGB(200, 200, 200) : RGB(255, 255, 255);
	CreatButton({ center.x - 60, center.y - 80 }, 120, 40, _T("继续游戏"), btnCol1);

	// 音乐开关按钮
	COLORREF btnCol2 = bgmState ? RGB(100, 255, 100) : RGB(100, 100, 100); // 根据状态变色
	CreatButton({ center.x - 60, center.y - 20 }, 120, 40, _T("音乐开关"), btnCol2);

	// 退出游戏按钮
	COLORREF btnCol3 = isMouseInArea(mouseX, mouseY, center.x - 60, center.y + 40, 120, 40) ? RGB(255, 100, 100) : RGB(255, 255, 255);
	CreatButton({ center.x - 60, center.y + 40 }, 120, 40, _T("退出游戏"), btnCol3);
}

// 初始化星空背景
void InitStars() {
	for (int i = 0; i < STAR_COUNT; i++) {
		stars[i].x = rand() % getwidth();
		stars[i].y = rand() % getheight();
		stars[i].speed = (float)(1 + rand() % 5); // 进行一个速度的模拟
		stars[i].brightness = 50 + rand() % 200; // 进行一个亮度的模拟
	}
}
void DrawStars() {
	for (int i = 0; i < STAR_COUNT; i++) {
		// 颜色根据亮度变化
		COLORREF c = RGB(stars[i].brightness, stars[i].brightness, stars[i].brightness);
		setfillcolor(c);
		solidcircle((int)stars[i].x, (int)stars[i].y, 1); // 星星只是小点

		stars[i].x -= stars[i].speed; // 向左移动
		if (stars[i].x < 0) { // 循环
			stars[i].x = 800;
			stars[i].y = (float)(rand() % 600);
		}
	}
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

// 处理键盘输入
void HandleInput(struct Twin* yang, struct Twin* yin) {
	float moveSpeed = 5.0f;
	// 通过虚拟键判定，可以无冲
	// 玩家 1(W A S D)
	if (GetAsyncKeyState('W') & 0x8000) yang->y -= moveSpeed;
	if (GetAsyncKeyState('S') & 0x8000) yang->y += moveSpeed;
	if (GetAsyncKeyState('A') & 0x8000) yang->x -= moveSpeed;
	if (GetAsyncKeyState('D') & 0x8000) yang->x += moveSpeed;

	// 玩家 2(方向键)
	if (GetAsyncKeyState(VK_UP) & 0x8000)    yin->y -= moveSpeed;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)  yin->y += moveSpeed;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)  yin->x -= moveSpeed;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) yin->x += moveSpeed;

	// 对Y轴限制
	if (wallActive) {
		// 有墙的时候阳星不能超过 290，阴星不能小于 310
		if (yang->y > 290) yang->y = 290;
		if (yin->y < 310) yin->y = 310;
	}
	else {
		// 没墙
		if (yang->y > 600) yang->y = 600;
		if (yin->y < 0) yin->y = 0;
	}

	// 外围死角边界不变
	if (yang->y < 0) yang->y = 0;
	if (yin->y > 600) yin->y = 600;
	if (yang->x < 0) yang->x = 0;
	if (yang->x > 800) yang->x = 800;
	if (yin->x < 0) yin->x = 0;
	if (yin->x > 800) yin->x = 800;
}

// 更新尾迹坐标
void UpdateTrails(struct Twin* star) {
	for (int i = 9; i > 0; i--) {
		star->trail_x[i] = star->trail_x[i - 1];
		star->trail_y[i] = star->trail_y[i - 1];
	}
	star->trail_x[0] = star->x;
	star->trail_y[0] = star->y;
}
void DrawTrails(struct Twin* star, COLORREF col) {
	for (int i = 0; i < 10; i++) {
		// 颜色逐渐变暗
		int alpha = 255 - (i * 25);
		if (alpha < 0) alpha = 0;
		setfillcolor(RGB(GetRValue(col) * alpha / 255, GetGValue(col) * alpha / 255, GetBValue(col) * alpha / 255));
		solidcircle((int)star->trail_x[i], (int)star->trail_y[i], star->radius - i / 2);
	}
}

// 判断小球是否与障碍物重叠
bool CheckCollision(struct Twin* star, struct Obstacle* ob) {
	if (!ob->active) return false;
	// 碰撞判定
	if (star->x + star->radius > ob->x &&
		star->x - star->radius < ob->x + ob->w &&
		star->y + star->radius > ob->y &&
		star->y - star->radius < ob->y + ob->h) {
		return true;
	}
	return false;
}

// 生成与更新障碍物
void UpdateObstacles() {
	float obsSpeed = 6.0f * currentDifficulty; // 障碍物飞过来的速度
	// 每隔 45 帧生成一个新障碍物，大约0.75秒
	spawnTimer++;
	if (spawnTimer >= 45) {
		spawnTimer = 0;
		// 在数组里找“非激活”状态的空位生成新障碍物
		for (int i = 0; i < MAX_OBS; i++) {
			if (!obs[i].active) {
				obs[i].active = true;
				obs[i].w = 30 + rand() % 50; // 宽度随机 30~80
				obs[i].h = 30 + rand() % 50; // 高度随机 30~80
				obs[i].x = 800; // 从最右侧出现
				// Y轴随机
				obs[i].y = rand() % (600 - (int)obs[i].h);

				obs[i].color = RGB(255, 50, 50);
				break; // 生成一个就够
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

// 绘制障碍物
void DrawObstacles() {
	for (int i = 0; i < MAX_OBS; i++) {
		if (obs[i].active) {
			setfillcolor(obs[i].color);
			setlinecolor(RGB(255, 255, 255)); // 加个白边
			fillrectangle((int)obs[i].x, (int)obs[i].y,
				(int)(obs[i].x + obs[i].w), (int)(obs[i].y + obs[i].h));
		}
	}
}

void UpdateGameLogic(struct Twin* yang, struct Twin* yin) {
	// 基础得分
	totalScore += 0.1f * currentDifficulty;

	// 两星垂直距离小于100，分数加倍
	float dist = fabsf(yang->y - yin->y);
	if (dist < 100) totalScore += 0.1f;

	// 上难度了
	currentDifficulty = 1.0f + (float)totalScore / 500.0f;
}
void DrawHUD() {
    settextcolor(YELLOW);
    settextstyle(25, 0, _T("Consolas"));
    TCHAR scoreStr[50];
    _stprintf(scoreStr, _T("SCORE: %.0f  LEVEL: %.1f"), totalScore, currentDifficulty);
    outtextxy(20, 50, scoreStr);
}

int main() {
	srand((unsigned)time(NULL));
	initgraph(800, 600, EX_NOMINIMIZE | EX_SHOWCONSOLE); //EX_NOCLOSE | EX_NOMINIMIZE
	setbkmode(TRANSPARENT);
	State gameState = MENU; // 初始为菜单界面

	// 帧率控制
	int totalFrames = 0; // 总帧数
	char str[100] = "";
	const clock_t FPS = 1000 / 60; // 目标帧率为60 FPS | 16ms / frame
	clock_t startTime = 0;
	clock_t frametime = 0;

	InitStars(); // 初始化背景
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

		settextcolor(WHITE);
		settextstyle(20, 0, _T("Arial"));

		cleardevice();
		BeginBatchDraw(); // 双缓冲开始，绘图开始

		// 读取鼠标 and 键盘
		while (peekmessage(&msg,EX_MOUSE | EX_KEY)) {
			// 按下 ESC 键切换暂停/继续
			if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {
				if (gameState == PLAYING) {
					gameState = PAUSE; // 切换到暂停状态
					printf("[LOG]:按下 ESC 键，游戏暂停\n");
				} else if (gameState == PAUSE) {
					gameState = PLAYING; // 切换回游戏状态
					printf("[LOG]: 按下 ESC键，回到PLAYING\n");
				}
				else if (gameState == GAMEOVER) {
					printf("[LOG]:按下 ESC 键，退出游戏\n"); 
					exit(0);
				}
			}
			if(msg.message == WM_KEYDOWN && msg.vkcode == 'R' && gameState == GAMEOVER) {
				printf("[LOG]:按下 R 键，重置游戏\n");
				// 重置游戏状态
				gameState = MENU;
				printf("[LOG]:游戏状态切换到 MENU\n");
				totalScore = 0;
				currentDifficulty = 1.0f;
				totalFrames = 0;
				yang.x = 400; yang.y = 150;
				yin.x = 400; yin.y = 450;
				for (int i = 0; i < MAX_OBS; i++) obs[i].active = false; // 清除障碍物
				wallActive = true;    // 间隔恢复
				wallBreaker.active = true;
			}

			switch (msg.message) {
			case WM_MOUSEMOVE:
				curMouseX = msg.x;
				curMouseY = msg.y;
				break;
			case WM_LBUTTONDOWN: 
				// 菜单时
				if(gameState == MENU) {
					if (isMouseInArea(msg.x, msg.y, getwidth() / 2 - 60, getheight() / 2 - 30, 120, 40)) {
						printf("[LOG]:点击了开始游戏按钮\n");
						gameState = PLAYING; // 切换到游戏状态
						printf("[LOG]:游戏状态切换到 PLAYING\n");
					}
					else if (isMouseInArea(msg.x, msg.y, getwidth() / 2 - 60, getheight() / 2 + 30, 120, 40)) {
						printf("[LOG]:点击了查看教程按钮\n");
						MessageBox(NULL, _T("双人控制双星，躲避障碍并尽可能获取更高的分数"), _T("游戏教程"), MB_OK);
					}
					else if (isMouseInArea(msg.x, msg.y, getwidth() - 60, 30, 45, 25)) {
						printf("[LOG]:点击了音乐开关\n");
						printf("[LOG]:当前 bgmState '%d'\n", bgmState);
						PlayBGM("assets/M500003kNDLh2UjjDP.mp3");
					}
				}
				// 暂停时
				if (gameState == PAUSE) {
					POINT center = { getwidth() / 2, getheight() / 2 };
					if (isMouseInArea(msg.x, msg.y, center.x - 60, center.y - 80, 120, 40)) {
						printf("[LOG]:点击了继续游戏按钮\n");
						gameState = PLAYING; // 切换回游戏状态
						printf("[LOG]:游戏状态切换到 PLAYING\n");
					}
					else if (isMouseInArea(msg.x, msg.y, center.x - 60, center.y - 20, 120, 40)) {
						printf("[LOG]:点击了音乐开关\n");
						printf("[LOG]:当前 bgmState '%d'\n", bgmState);
						PlayBGM("assets/M500003kNDLh2UjjDP.mp3");
					}
					else if (isMouseInArea(msg.x, msg.y, center.x - 60, center.y + 40, 120, 40)) {
						printf("[LOG]:点击了退出游戏按钮\n");
						exit(0);
					}
				}
				break;
			case WM_RBUTTONDOWN:
				break;
			}
		}

		// 菜单界面逻辑 
		if (gameState == MENU) {
			setbkcolor(RGB(77, 194, 195));
			DrawHello(msg.x, msg.y);
		}
		// 暂停界面逻辑
		else if (gameState == PAUSE) {
			DrawPause(msg.x, msg.y);
		}
		// 游戏时界
		else if (gameState == PLAYING) {
			setbkcolor(RGB(10, 10, 25));
			DrawStars(); // 绘制背景
			if (wallActive) {
				setfillcolor(RGB(100, 100, 100)); // 灰色
				fillrectangle(0, 295, 800, 305); // 高度10
			}

			sprintf(str, "Frame: %d", totalFrames++/60);
			outtextxy(20, 20, str);// 写入帧数

			HandleInput(&yang, &yin);
			UpdateTrails(&yang);
			UpdateTrails(&yin);

			// 障碍物逻辑
			UpdateObstacles(); // 更新障碍物坐标并随机生成
			UpdateGameLogic(&yang, &yin); // 更新分数和难度
			DrawHUD();
			// 每 500 帧生成一次道具
			if (wallActive && totalFrames % 500 == 0 && !wallBreaker.active) {
				wallBreaker.active = true;
				wallBreaker.x = 800;
				wallBreaker.y = 100 + rand() % 400; // 随机位置
				printf("[LOG]: 道具生成 TIME: %d\n", totalFrames / 60);
			}
			if (wallBreaker.active) {
				wallBreaker.x -= 3.0f; // 道具向左飘
				setfillcolor(wallBreaker.color);
				solidcircle((int)wallBreaker.x, (int)wallBreaker.y, wallBreaker.radius);

				// 判断是否吃到了道具
				if ((abs(yang.x - wallBreaker.x) < 15 && abs(yang.y - wallBreaker.y) < 15) ||
					(abs(yin.x - wallBreaker.x) < 15 && abs(yin.y - wallBreaker.y) < 15)) {
					wallActive = false;    // 墙壁消失
					printf("[LOG]:道具被吃掉，墙壁消失\n");
					wallBreaker.active = false;
				}
				if (wallBreaker.x < 0) wallBreaker.active = false; // 飞出屏幕回收
			}
			DrawObstacles();

			// 死亡判定
			for (int i = 0; i < MAX_OBS; i++) {
				if (CheckCollision(&yang, &obs[i]) || CheckCollision(&yin, &obs[i])) {
					gameState = GAMEOVER;
					printf("[LOG]: 角色死亡\n");
				}
			}

			DrawTrails(&yang, COLOR_YANG_CORE);
			DrawYangStar(&yang);
			DrawTrails(&yin, COLOR_YIN_CORE);
			DrawYinStar(&yin);
		// 游戏结束界面
		} else if (gameState == GAMEOVER) {
			char finalScore[50];
			sprintf(finalScore, "最终得分：%.0f", totalScore);
			settextcolor(WHITE);
			settextstyle(40, 0, _T("Arial"));
			outtextxy(getwidth() / 2 - 100, getheight() / 2 - 100, _T("游戏结束！"));
			outtextxy(getwidth() / 2 - 100, getheight() / 2 - 50, finalScore);
			outtextxy(getwidth() / 2 - 150, getheight() / 2 + 20, _T("按 R 键重新开始"));
		}
			
		EndBatchDraw(); // 双缓冲结束，画面绘制结束

		// 帧率控制
		frametime = clock() - startTime;
		if(totalFrames % 60==0 && totalFrames)
			printf("[LOG]:帧 %d 耗时 %ld ms time(s):%d\n", totalFrames, frametime, totalFrames /60);
		if (frametime < FPS) {
			Sleep(FPS-frametime); // 60 FPS
		}
	}
	closegraph();
	return 0;
}