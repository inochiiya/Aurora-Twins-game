#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <mmsystem.h>
#include <conio.h>
#include <easyx.h>
#include <cstdio>
#include <ctime>
#include <tchar.h>
#pragma comment(lib, "winmm.lib")


// 定义颜色常量（霓虹）
#define COLOR_YANG_OUTER  RGB(0, 150, 255) // 青色：暗部（光晕）
#define COLOR_YANG_CORE   RGB(0, 255, 255) // 青色：亮部（核心）
#define COLOR_YIN_OUTER   RGB(200, 0, 200) // 洋红：暗部（光晕）
#define COLOR_YIN_CORE    RGB(255, 100, 255) // 洋红：亮部（核心）
#define COLOR_CORE_WHITE  RGB(255, 255, 255) // 极亮中心点

// 双子星	
 struct Twin {
	float x, y; // 位置
	int radius; // 碰撞半径
	COLORREF color;
	// 用于制作尾迹
	float trail_x[10];   // 过去 10 帧的 X 坐标
	float trail_y[10];   // 过去 10 帧的 Y 坐标
};


TCHAR* numtoarr(int n) {
	static TCHAR arr[20];
	_stprintf(arr, _T("%d"), n);
	return arr;
}

// 创建按钮
void CreatButton(POINT pos, int width, int height, LPCTSTR text) {
	setfillcolor(RGB(255, 255, 255));
	fillroundrect(pos.x, pos.y, pos.x + width, pos.y + height, 10, 10);
	settextcolor(BLACK);
	settextstyle(24, 0, _T("微软雅黑"));
	int tw = textwidth(text);
	outtextxy(pos.x + (width - tw) / 2, pos.y + (height - textheight(text)) / 2, text);
}

// 开始界面
void DrawHello() {
	POINT center = { getwidth() / 2, getheight() / 2 };


	outtextxy(center.x - 100, 50, _T("双子星"));
	CreatButton({ center.x - 60, center.y - 30 }, 120, 40, _T("开始游戏"));
	CreatButton({ center.x - 60, center.y + 30 }, 120, 40, _T("查看教程"));
	
}

// 背景音乐，原本打算使用的音频文件有点问题
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
	float moveSpeed = 5.0f; // 建议用 float，移动更平滑

	// 检测 W 键 (注意：GetAsyncKeyState 使用的是虚拟键码)
	if (GetAsyncKeyState('W') & 0x8000) {
		yang->y -= moveSpeed;
		yin->y -= moveSpeed; // 镜像同步
	}
	if (GetAsyncKeyState('S') & 0x8000) {
		yang->y += moveSpeed;
		yin->y += moveSpeed;
	}
	if (GetAsyncKeyState('A') & 0x8000) {
		yang->x -= moveSpeed;
		yin->x += moveSpeed; // 镜像逻辑：阳左阴右
	}
	if (GetAsyncKeyState('D') & 0x8000) {
		yang->x += moveSpeed;
		yin->x -= moveSpeed; // 镜像逻辑：阳右阴左
	}
}

int main() {
	initgraph(800, 600, EX_NOMINIMIZE | EX_SHOWCONSOLE | EX_DBLCLKS); //EX_NOCLOSE | EX_NOMINIMIZE
	setbkcolor(RGB(77, 194, 195));	
	setbkmode(TRANSPARENT);

	// 帧率控制
	int totalFrames = 0; // 总帧数
	char str[100] = "";
	const clock_t FPS = 1000 / 60; // 目标帧率为60 FPS | 16ms / frame
	clock_t startTime = 0;
	clock_t frametime = 0;

	Twin yang = { 100, 80, 5, COLOR_YANG_CORE }; // 初始化阳星
	Twin yin = { 200, 80, 5, COLOR_YIN_CORE }; // 初始化阴星
	int score = 0;

	PlayBGM("assets\\M500003kNDLh2UjjDP.mp3");

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

		// 绘制游戏界面
		DrawHello();

		// 绘制游戏主角
		DrawYangStar(&yang);
		DrawYinStar(&yin);
		
		// 图片加载示例
		//IMAGE img;
		//loadimage(&img,"assets\\1.jpg",128,128);
		//putimage(getwidth()/2 - 64, getheight()/2 -64, &img);

		// 读取鼠标消息
		ExMessage msg = { 0 };
		while (peekmessage(&msg,EX_MOUSE | EX_KEY)) {
			if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {
				printf("[LOG]:按下 ESC 键，退出游戏\n"); 
				exit(0);
			}
			switch (msg.message) {
			case WM_LBUTTONDOWN: 
				printf("[LOG]:鼠标左键按下 pos (%d, %d)\n", msg.x, msg.y);
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

		HandleInput(&yang, &yin);

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