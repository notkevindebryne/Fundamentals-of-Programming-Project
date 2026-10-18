#ifndef CONFIG_H
#define CONFIG_H
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>

// Thư viện Raylib
#include "raylib.h"

using namespace std;

// Cấu hình bàn cờ & giao diện
const int boardsize = 12;
const int CELL_SIZE = 45;
const int BOARD_OFFSET_X = 230;
const int BOARD_OFFSET_Y = 80;

// Trạng thái Game
enum GameState
{
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_SETTINGS
};

extern GameState currentState;

// Cấu trúc điểm cờ
struct _point
{
    int x, y, c; // c = 0: Trống, 1: X, -1: O
};

struct CellPos
{
    int r, c;
};

// Biến toàn cục bàn cờ & cài đặt
extern _point _a[boardsize][boardsize];
extern vector<CellPos> winCells;
extern bool _turn;  // true: Lượt X, false: Lượt O
extern int _winner; // 1: X thắng, -1: O thắng, 0: Hòa, 2: Đang chơi
extern bool _sound;
extern bool _block2ends;

// Các hàm Logic (Model)
void resetdata();
int testboard();
int checkboard(int row, int col);
bool saveGameData(const string &filename);
bool loadGameData(const string &filename);

// Các hàm Đồ họa (View)
void DrawGame();
void DrawMenuUI();
void DrawBoardUI();
void DrawSettingsUI();

// Các hàm Điều khiển (Control)
void UpdateGame();
void HandleMouseClick();

// ===== Tài nguyên màn hình Gameplay =====
extern Texture2D texBackground;
extern Texture2D texX;
extern Texture2D texO;

// ===== Tài nguyên màn hình chính (Main Menu) =====
extern Texture2D texMainBackground; // Nền lớp học pixel
extern Texture2D texLogo;           // Logo "CARO"
extern Texture2D texBar;            // Thanh gỗ - trạng thái thường
extern Texture2D texBarHover;       // Thanh gỗ - trạng thái hover (phát sáng)
extern Font fontPixel;              // Font pixel Press Start 2P

// ===== Các nút ở Main Menu (thứ tự từ trên xuống) =====
enum MenuButton
{
    MENU_NEW_GAME,
    MENU_LOAD_GAME,
    MENU_SETTINGS,
    MENU_MUSIC,
    MENU_CREDIT,
    MENU_EXIT,
    MENU_BUTTON_COUNT
};
Rectangle GetMenuButtonRect(int index); // Vị trí + kích thước nút (dùng chung cho vẽ và bấm chuột)
int GetHoveredMenuButton();             // Nút đang được trỏ chuột vào, -1 nếu không có

// ===== Music Player (file MusicPlayer.cpp) =====
void LoadMusicPlayer();   // Nạp ảnh giao diện + ảnh bìa + nhạc (gọi sau InitWindow)
void UnloadMusicPlayer(); // Giải phóng + CloseAudioDevice
void OpenMusicPlayer();   // Mở cửa sổ nhạc (gọi khi bấm nút MUSIC)
bool IsMusicPlayerOpen(); // Đang mở? (khi mở thì menu phía sau bị khoá)
void UpdateMusicPlayer(); // Gọi MỖI FRAME: nuôi stream nhạc + xử lý chuột/phím
void DrawMusicPlayer();   // Vẽ cửa sổ nhạc (nằm trong BeginDrawing/EndDrawing)

void LoadGameAssets();
void UnloadGameAssets();

// ===== Trạng thái Music Player (Đưa sang Model quản lý) =====
extern int currentSong;
extern int playState; // 0: STOPPED, 1: PLAYING, 2: PAUSED
extern bool playerOpen;
extern bool draggingThumb;
extern bool justOpened;

// ===== Hàm giao tiếp cho Control =====
void MusicTogglePause();
void MusicStep(int dir);
void MusicProcessMouse(Vector2 m, bool clicked, bool down);

#endif