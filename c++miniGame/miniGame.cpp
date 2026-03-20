#define _CRT_SECURE_NO_WARNINGS
#include <conio.h>
#include <easyx.h>
#include <cstdio>
#include <ctime>
#include <tchar.h>

// 定义颜色常量（霓虹色调）
#define COLOR_YANG_OUTER  RGB(0, 150, 255) // 青色：暗部（光晕）
#define COLOR_YANG_CORE   RGB(0, 255, 255) // 青色：亮部（核心）
#define COLOR_YIN_OUTER   RGB(200, 0, 200) // 洋红：暗部（光晕）
#define COLOR_YIN_CORE    RGB(255, 100, 255) // 洋红：亮部（核心）
#define COLOR_CORE_WHITE  RGB(255, 255, 255) // 极亮中心点

 struct Twin {
	float x, y; // 位置
	int radius; // 碰撞半径
	COLORREF color;
	// 用于制作极光尾迹的数据
	float trail_x[10];   // 存储过去 10 帧的 X 坐标
	float trail_y[10];   // 存储过去 10 帧的 Y 坐标
};


TCHAR* numtoarr(int n) {
	// 将整数转换为 TCHAR 字符数组（适配 Unicode 和 ANSI）
	static TCHAR arr[20]; // 假设整数不会超过20位
	_stprintf(arr, _T("%d"), n); // 将整数转换为字符串（泛型文本映射）
	return arr;
}

void Test() {
	// 绘制游戏界面
	circle(getwidth()/2, getheight()/2, 50); // 示例：绘制一个圆

	setlinestyle(PS_SOLID | PS_JOIN_ROUND, 1); // 设置线条样式为虚线，宽度为2

	line(0, 0, getwidth(), getheight()); // 绘制线
	line(getwidth(), 0, 0, getheight());
	
	setlinecolor(WHITE);
	setfillcolor(RGB(77 / 2, 194 / 2, 195 / 2));
	fillrectangle(getwidth() / 4, getheight() / 4, getwidth() * 3 / 4, getheight() * 3 / 4); // 绘制矩形

	// 测试无边框矩形和圆角矩形
	setfillcolor(RGB(255, 0, 0));
	// 绘制无边框矩形
	solidrectangle(getwidth() / 4 + 10, getheight() / 4 + 10, getwidth() / 4 + 51, getheight() / 4 + 50);
	// 绘制圆角矩形
	roundrect(getwidth() / 4 + 60, getheight() / 4 + 60, getwidth() / 4 + 110, getheight() / 4 + 110,10,20);
	//还有fillroundrectangle和solidroundrectangle

	// 圆形
	circle(getwidth() / 2, getheight() / 2, 50);
	// 还有solidcircle和fillcircle

	// 椭圆，同样有fillellipse和solidelellipse
	ellipse(400, 300, 500, 350);
}

void CreatButton(POINT pos, int width, int height, LPCTSTR text) {
	setfillcolor(RGB(255, 255, 255));
	fillroundrect(pos.x, pos.y, pos.x + width, pos.y + height, 10, 10);
	settextcolor(BLACK);
	settextstyle(24, 0, _T("微软雅黑"));
	int tw = textwidth(text);
	outtextxy(pos.x + (width - tw) / 2, pos.y + (height - textheight(text)) / 2, text);
}

void DrawHello() {
	POINT center = { getwidth() / 2, getheight() / 2 };


	outtextxy(center.x - 100, 50, _T("双子星"));
	CreatButton({ center.x - 60, center.y - 30 }, 120, 40, _T("开始游戏"));
	CreatButton({ center.x - 60, center.y + 30 }, 120, 40, _T("查看教程"));
	
}
// 绘制阳星的专用函数
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

int main() {
	initgraph(800, 600, EX_NOMINIMIZE | EX_SHOWCONSOLE | EX_DBLCLKS); //EX_NOCLOSE | EX_NOMINIMIZE
	setbkcolor(RGB(77, 194, 195));	
	setbkmode(TRANSPARENT);

	// 游戏主循环
	int n = 0; // 总帧数
	char str[100] = "";

	// 帧率控制
	const clock_t FPS = 1000 / 60; // 目标帧率为60 FPS | 16ms / frame
	clock_t startTime = 0;
	clock_t frametime = 0;

	// 游戏主循环
	while (true)
	{
		startTime = clock(); // 记录开始时间

		sprintf(str, "Frame: %d", n++);
		settextcolor(WHITE);
		settextstyle(20, 0, _T("Arial"));

		cleardevice();
		BeginBatchDraw(); // 双缓冲开始

		outtextxy(20, 20, str);// 写入帧数信息

		// 绘制游戏界面
		DrawHello();

		Twin yang = { 100, 80, 20, COLOR_YANG_CORE }; // 初始化阳星
		DrawYangStar(&yang);
		
		IMAGE img;
		loadimage(&img,"assets\\1.jpg",128,128);
		//putimage(getwidth()/2 - 64, getheight()/2 -64, &img);

		// 读取鼠标消息
		ExMessage msg = { 0 };
		while (peekmessage(&msg,EX_MOUSE | EX_KEY)) {
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
			if (msg.message == WM_KEYDOWN) {
				printf("[LOG]:键盘按下 vkcode (%d)\n", msg.vkcode);
				switch (msg.vkcode) {
				case 'W':
					yang.y--;
					break;
				}
			}
		}

		EndBatchDraw(); // 双缓冲结束

		// 帧率控制
		frametime = clock() - startTime;
		if(n%60==0)
		printf("[LOG]:帧 %d 耗时 %ld ms time(s):%.2f\n", n, frametime,n/60.0);
		if (frametime < FPS) {
			Sleep(FPS-frametime); // 控制帧率约为60 FPS
		}
	}
	closegraph();
	return 0;
}