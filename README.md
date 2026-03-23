# 基于EaysX图形库和C++开发的小游戏《双球一败涂地》
## 游戏介绍
在《双球一败涂地中》中，需要两名玩家分别使用WASD和方向键来操控两个小球，游戏进行时右侧会随机生成障碍物，玩家需要尽可能的躲避越来越快的障碍物。
游戏时有概率生成具有特殊效果的道具，道具可以帮助玩家更顺利的度过袭来的障碍物
游戏目标：获取更高的分数并存活更久

## 注意事项
1. 游戏目录下 assets目录不可修改移动，内有游戏运行时bgm音频资源，如果修改无法再播放BGM
2. 如需更换音乐需手动替换 "assets/M500003kNDLh2UjjDP.mp3"文件，名字不可更改
3. GitHub：[仓库](https://github.com/inochiiya/Aurora-Twins-game/)

## 概览
使用 [EasyX绘图库](https://easyx.cn/) 实现界面UI，通过GetAsyncKeyState函数处理按键输入，支持多键同时按下处理

游戏围绕三个界面开发，分别是MENU、PLAYING、PAUSE、GAMEOVER，通过一个共用体变量来切换状态

MENU界面：调用DrawHello绘制基础界面

PLAYING界面：调用多个刷新和绘制函数，包括两个小球、障碍物、道具、背景、粒子效果、HUB等

PAUSE界面：调用DrawPause

GAMEOVER界面：输出相关游玩信息，支持按下 R “回溯” 