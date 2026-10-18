#include "config.h"

// Gameplay
Texture2D texBackground;
Texture2D texX;
Texture2D texO;

// Main Menu
Texture2D texMainBackground;
Texture2D texLogo;
Texture2D texBar;
Texture2D texBarHover;
Font fontPixel;

// Chèn ảnh vào đây ->
void LoadGameAssets() {
    texBackground = LoadTexture("assets/background.png");
    texX = LoadTexture("assets/x.png");
    texO = LoadTexture("assets/o.png");

    // --- Tài nguyên Main Menu ---
    texMainBackground = LoadTexture("assets/menu_background.png");
    texLogo = LoadTexture("assets/menu_logo.png");
    texBar = LoadTexture("assets/menu_bar.png");
    texBarHover = LoadTexture("assets/menu_bar_hover.png");

    // Giữ nét pixel sắc cạnh khi phóng to/thu nhỏ
    SetTextureFilter(texMainBackground, TEXTURE_FILTER_POINT);
    SetTextureFilter(texLogo, TEXTURE_FILTER_POINT);
    SetTextureFilter(texBar, TEXTURE_FILTER_POINT);
    SetTextureFilter(texBarHover, TEXTURE_FILTER_POINT);

    // Font pixel: nạp ở cỡ 16 (bội số của 8 - đúng lưới pixel của Press Start 2P).
    // Muốn chữ to hơn thì vẽ ở 32, 48... (gấp nguyên lần) để vẫn sắc nét.
    fontPixel = LoadFontEx("assets/PressStart2P-Regular.ttf", 16, 0, 0);
    SetTextureFilter(fontPixel.texture, TEXTURE_FILTER_POINT);

    // --- Cửa sổ nghe nhạc (MusicPlayer.cpp) ---
    LoadMusicPlayer();
}

void UnloadGameAssets() {
    UnloadTexture(texBackground);
    UnloadTexture(texX);
    UnloadTexture(texO);

    // --- Giải phóng tài nguyên Main Menu ---
    UnloadTexture(texMainBackground);
    UnloadTexture(texLogo);
    UnloadTexture(texBar);
    UnloadTexture(texBarHover);
    UnloadFont(fontPixel);
    UnloadMusicPlayer();
}

void DrawBoardUI() {
    // 1. Kéo dãn Background cho phủ kín toàn bộ màn hình (1000x700)
    Rectangle sourceBg = { 0.0f, 0.0f, (float)texBackground.width, (float)texBackground.height };
    Rectangle destBg = { 0.0f, 0.0f, 1000.0f, 700.0f };
    DrawTexturePro(texBackground, sourceBg, destBg, { 0.0f, 0.0f }, 0.0f, WHITE);

    // 2. Vẽ các đường kẻ lưới bàn cờ màu đen
    for (int i = 0; i <= boardsize; i++) {
        DrawLine(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_SIZE,
            BOARD_OFFSET_X + boardsize * CELL_SIZE, BOARD_OFFSET_Y + i * CELL_SIZE, BLACK);
        DrawLine(BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y,
            BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y + boardsize * CELL_SIZE, BLACK);
    }

    // 3. Quét toàn bộ mảng dữ liệu để vẽ quân X và O
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            // Tính toán tọa độ góc trên bên trái của ô cờ hiện tại
            int posX = BOARD_OFFSET_X + j * CELL_SIZE;
            int posY = BOARD_OFFSET_Y + i * CELL_SIZE;

            // TẠO KHUNG ĐÍCH ĐỂ ÉP KÍCH THƯỚC ẢNH
            // Khung này ép ảnh nhỏ lại, lùi vào 2 pixel và nhỏ hơn ô cờ 4 pixel để tạo viền hở cho đẹp
            Rectangle destRect = { (float)posX + 2.0f, (float)posY + 2.0f, (float)CELL_SIZE - 4.0f, (float)CELL_SIZE - 4.0f };

            // Nếu ô này được đánh X
            if (_a[i][j].c == 1) {
                Rectangle sourceX = { 0.0f, 0.0f, (float)texX.width, (float)texX.height };
                DrawTexturePro(texX, sourceX, destRect, { 0.0f, 0.0f }, 0.0f, WHITE);
            }
            // Nếu ô này được đánh O
            else if (_a[i][j].c == -1) {
                Rectangle sourceO = { 0.0f, 0.0f, (float)texO.width, (float)texO.height };
                DrawTexturePro(texO, sourceO, destRect, { 0.0f, 0.0f }, 0.0f, WHITE);
            }
        }
    }
}

// ===== Bố cục Main Menu (khung 1000x700) =====
static const int   MENU_LOGO_TOP = 14;   // Logo cách mép trên
static const int   MENU_LOGO_GAP = 14;   // Logo -> nút đầu tiên
static const int   MENU_BTN_GAP = 8;    // Khoảng cách giữa các nút
static const int   MENU_FONT_SIZE = 16;
static const float MENU_FONT_SPACING = 1.0f;
static const Color MENU_TEXT_COLOR = { 176, 64, 30, 255 };
static const Color MENU_TEXT_HOVER_COLOR = { 110, 38, 12, 255 };

static const char* MENU_LABELS[MENU_BUTTON_COUNT] = {
    "NEW GAME", "LOAD GAME", "SETTING", "MUSIC", "CREDIT", "EXIT"
};

// Nút luôn nằm giữa màn hình, kích thước lấy theo ảnh thanh gỗ
Rectangle GetMenuButtonRect(int index) {
    float x = (1000.0f - texBar.width) / 2.0f;
    float y = (float)(MENU_LOGO_TOP + texLogo.height + MENU_LOGO_GAP + index * (texBar.height + MENU_BTN_GAP));
    return { x, y, (float)texBar.width, (float)texBar.height };
}

int GetHoveredMenuButton() {
    if (IsMusicPlayerOpen()) return -1;   // cửa sổ nhạc đang che menu
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        if (CheckCollisionPointRec(mouse, GetMenuButtonRect(i))) return i;
    }
    return -1;
}

void DrawMenuUI() {
    // 1. Nền lớp học (đã cắt sẵn đúng 1000x700 nên vẽ 1:1)
    DrawTexture(texMainBackground, 0, 0, WHITE);

    // 2. Logo CARO ở giữa phía trên
    DrawTexture(texLogo, (1000 - texLogo.width) / 2, MENU_LOGO_TOP, WHITE);

    int hovered = GetHoveredMenuButton();

    // 3. Các thanh gỗ ở trạng thái thường
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        Rectangle r = GetMenuButtonRect(i);
        DrawTexture(texBar, (int)r.x, (int)r.y, WHITE);
    }

    // 4. Thanh đang hover vẽ đè lên sau cùng (ảnh hover lớn hơn do có quầng sáng)
    if (hovered >= 0) {
        Rectangle r = GetMenuButtonRect(hovered);
        int marginX = (texBarHover.width - texBar.width) / 2;
        int marginY = (texBarHover.height - texBar.height) / 2;
        DrawTexture(texBarHover, (int)r.x - marginX, (int)r.y - marginY, WHITE);
    }

    // 5. Chữ trên thanh gỗ (căn giữa; lệch lên 3px vì thanh gỗ có gờ dày ở đáy)
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        Rectangle r = GetMenuButtonRect(i);
        Vector2 ts = MeasureTextEx(fontPixel, MENU_LABELS[i], (float)MENU_FONT_SIZE, MENU_FONT_SPACING);
        Vector2 pos = { (float)(int)(r.x + (r.width - ts.x) / 2.0f),
                        (float)(int)(r.y + (r.height - ts.y) / 2.0f) - 3.0f };
        DrawTextEx(fontPixel, MENU_LABELS[i], pos, (float)MENU_FONT_SIZE, MENU_FONT_SPACING,
            (i == hovered) ? MENU_TEXT_HOVER_COLOR : MENU_TEXT_COLOR);
    }
}

void DrawSettingsUI() {
    DrawText("CAI DAT GAME", 380, 150, 36, DARKBLUE);

    DrawText(TextFormat("Am thanh: %s (Click de doi)", _sound ? "ON" : "OFF"), 350, 250, 22, DARKGRAY);
    DrawText(TextFormat("Luat chan 2 dau: %s (Click de doi)", _block2ends ? "ON" : "OFF"), 350, 310, 22, DARKGRAY);

    DrawRectangle(380, 420, 240, 45, LIGHTGRAY);
    DrawText("QUAY LAI MENU", 415, 432, 20, BLACK);
}

void DrawGame() {
    UpdateMusicPlayer();              // nhạc + chuột/phím của cửa sổ nhạc (chạy mỗi frame)

    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch (currentState) {
    case STATE_MENU:
        DrawMenuUI();
        break;
    case STATE_GAMEPLAY:
        DrawBoardUI();
        break;
    case STATE_SETTINGS:
        DrawSettingsUI();
        break;
    }

    DrawMusicPlayer();                // vẽ đè lên trên cùng khi đang mở

    EndDrawing();
}
