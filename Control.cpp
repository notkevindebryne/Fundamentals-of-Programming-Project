#include "config.h"

GameState currentState = STATE_MENU;

void HandleInput()
{
    Vector2 mousePos = GetMousePosition();

    // 1. ƯU TIÊN: Nếu cửa sổ Music đang mở, Control sẽ "chiếm" input để điều khiển nhạc
    if (playerOpen)
    {
        // Đổi bài bằng WASD
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_A))
            MusicStep(-1);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D))
            MusicStep(1);

        // Enter để Play/Pause
        if (IsKeyPressed(KEY_ENTER))
            MusicTogglePause();

        // Cuộn chuột để đổi bài
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
            MusicStep((wheel > 0) ? -1 : 1);

        // Chuột (Click hoặc Kéo slider)
        bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        MusicProcessMouse(mousePos, clicked, down);

        return; // Dừng tại đây, ngăn không cho click xuyên xuống menu/game ở dưới
    }

    // 2. Xử lý input game bình thường
    if (currentState == STATE_MENU)
    {
        // Sử dụng hàm GetHoveredMenuButton từ View.cpp để đồng bộ click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            int hovered = GetHoveredMenuButton();
            if (hovered == MENU_NEW_GAME)
            {
                resetdata();
                currentState = STATE_GAMEPLAY;
            }
            else if (hovered == MENU_SETTINGS)
            {
                currentState = STATE_SETTINGS;
            }
            else if (hovered == MENU_MUSIC)
            {
                OpenMusicPlayer(); // Mở nhạc
            }
            else if (hovered == MENU_EXIT)
            {
                CloseWindow();
            }
        }
    }
    else if (currentState == STATE_SETTINGS)
    {
        if (CheckCollisionPointRec(mousePos, {350, 240, 300, 40}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            _sound = !_sound;
        }
        else if (CheckCollisionPointRec(mousePos, {350, 300, 300, 40}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            _block2ends = !_block2ends;
        }
        else if (CheckCollisionPointRec(mousePos, {380, 420, 240, 45}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            currentState = STATE_MENU;
        }
    }
    else if (currentState == STATE_GAMEPLAY)
    {
        if (_winner == 2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            int col = (mousePos.x - BOARD_OFFSET_X) / CELL_SIZE;
            int row = (mousePos.y - BOARD_OFFSET_Y) / CELL_SIZE;

            if (row >= 0 && row < boardsize && col >= 0 && col < boardsize)
            {
                if (checkboard(row, col) != 0)
                {
                    _winner = testboard();
                }
            }
        }

        if (IsKeyPressed(KEY_R))
            resetdata();
        if (IsKeyPressed(KEY_ESCAPE))
            currentState = STATE_MENU;
    }
}

void UpdateGame()
{
    HandleInput(); // Gọi hàm input tổng
}

int main()
{
    InitWindow(1000, 700, "GAME CARO - RAYLIB");
    SetTargetFPS(60);
    LoadGameAssets();
    resetdata();

    while (!WindowShouldClose())
    {
        UpdateGame();
        DrawGame();
    }

    UnloadGameAssets();
    CloseWindow();
    return 0;
}