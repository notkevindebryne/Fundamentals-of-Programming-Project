#include "config.h"

// =====================================================================
//  MUSIC PLAYER - cửa sổ nghe nhạc mở ra từ nút MUSIC ở Main Menu
//  * Giao diện được ghép từ nhiều ảnh nhỏ (mp_*.png) bằng DrawNineSlice
//  * Phát nhạc bằng Music stream của raylib (LoadMusicStream ...)
// =====================================================================

// ---------------------------------------------------------------------
// 1) DANH SÁCH 5 BÀI HÁT  <-- CHỈ CẦN SỬA Ở ĐÂY ĐỂ ĐỔI NHẠC / TÊN / ẢNH BÌA
//    musicPath: đường dẫn file nhạc. raylib đọc được .mp3 .ogg .wav .flac
//               (KHÔNG đọc được .mp4 -> phải đổi đuôi sang .mp3 trước)
//    coverPath: ảnh bìa, kích thước 432x214
//    Font Press Start 2P không có dấu tiếng Việt -> viết tên không dấu.
// ---------------------------------------------------------------------
struct Song
{
    const char *title;
    const char *artist;
    const char *musicPath;
    const char *coverPath;
};

static const int SONG_COUNT = 5;

static Song songs[SONG_COUNT] = {
    {"Chuyen Kenh", "Ngot", "assets/song1.mp3", "assets/cover1.png"},
    {"Da Tan", "Ngot", "assets/song2.mp3", "assets/cover2.png"},
    {"Lan Cuoi", "Ngot", "assets/song3.mp3", "assets/cover3.png"},
    {"Thay Chua", "Ngot", "assets/song4.mp3", "assets/cover4.png"},
    {"Mat Tich", "Ngot", "assets/song5.mp3", "assets/cover5.png"},
};

// ---------------------------------------------------------------------
// 2) BỐ CỤC (toạ độ tính trong cửa sổ nhạc, gốc ở góc trên trái của nó)
// ---------------------------------------------------------------------
static const int PW = 560, PH = 628;   // kích thước cửa sổ nhạc
static const int PX = (1000 - PW) / 2; // căn giữa màn hình 1000x700
static const int PY = (700 - PH) / 2;

static const int TITLE_H = 46; // thanh tiêu đề gỗ
static const int PAD_L = 28;
static const int CONTENT_W = 468;            // chiều rộng ô ảnh / ô tên / ô danh sách
static const int FRAME_B = 18;               // độ dày viền khung (mp_frame.png)
static const int ART_Y = 62, ART_H = 250;    // ô ảnh bìa (đã kéo thành chữ nhật)
static const int INFO_Y = 324, INFO_H = 62;  // ô tên bài + tên tác giả
static const int LIST_Y = 398, LIST_H = 214; // ô danh sách 5 bài
static const int ROW_H = 34;

static const int SB_X = PAD_L + CONTENT_W + 14; // thanh cuộn
static const int SB_W = 26;
static const int SB_TOP = ART_Y, SB_BOT = LIST_Y + LIST_H;
static const int CAP_H = 31; // chiều cao nút mũi tên trên / dưới
static const int TRACK_Y = SB_TOP + CAP_H;
static const int TRACK_H = (SB_BOT - CAP_H) - TRACK_Y; // 488px, chia đều cho 5 bài

static const int MP_FONT = 16;
static const float MP_SPACING = 1.0f;

static const Color COL_TEXT_DARK = {70, 30, 22, 255};
static const Color COL_TEXT_MID = {110, 56, 40, 255};
static const Color COL_TEXT_LIGHT = {255, 236, 205, 255};
static const Color COL_ROW_NOW = {232, 160, 86, 255};
static const Color COL_ROW_HOVER = {240, 205, 150, 255};
static const Color COL_BORDER = {92, 45, 38, 255};

// ---------------------------------------------------------------------
// 3) TRẠNG THÁI & TÀI NGUYÊN
// ---------------------------------------------------------------------

static Music songMusic[SONG_COUNT];
static bool songLoaded[SONG_COUNT];
static Texture2D songCover[SONG_COUNT];
static bool coverLoaded[SONG_COUNT];

// Các mảnh ảnh giao diện
static Texture2D texMpTitle, texMpMin, texMpClose, texMpTile, texMpFrame, texMpInfo;
static Texture2D texMpUp, texMpDown, texMpTrack, texMpThumbTop, texMpKnob, texMpThumbBot;

static Texture2D LoadPixelTexture(const char *path)
{
    Texture2D t = LoadTexture(path);
    SetTextureFilter(t, TEXTURE_FILTER_POINT); // giữ nét pixel
    return t;
}

// ---------------------------------------------------------------------
// 4) HÀM VẼ PHỤ: GHÉP ẢNH
// ---------------------------------------------------------------------
// Kéo dãn kiểu 9-slice: 4 góc giữ nguyên, 4 cạnh + phần giữa co giãn.
// l,t,r,b = độ dày viền (px, tính trên chính ảnh gốc). Đặt 0 để không chia chiều đó.
static void DrawNineSlice(Texture2D tex, Rectangle dst, int l, int t, int r, int b)
{
    float w = (float)tex.width, h = (float)tex.height;
    float sx[4] = {0.0f, (float)l, w - r, w};
    float sy[4] = {0.0f, (float)t, h - b, h};
    float dx[4] = {dst.x, dst.x + l, dst.x + dst.width - r, dst.x + dst.width};
    float dy[4] = {dst.y, dst.y + t, dst.y + dst.height - b, dst.y + dst.height};
    for (int j = 0; j < 3; j++)
    {
        for (int i = 0; i < 3; i++)
        {
            Rectangle src = {sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j]};
            Rectangle out = {dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]};
            if (src.width <= 0 || src.height <= 0 || out.width <= 0 || out.height <= 0)
                continue;
            DrawTexturePro(tex, src, out, {0.0f, 0.0f}, 0.0f, WHITE);
        }
    }
}

// Kéo cả ảnh vừa khít một hình chữ nhật
static void DrawStretch(Texture2D tex, Rectangle dst)
{
    Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
    DrawTexturePro(tex, src, dst, {0.0f, 0.0f}, 0.0f, WHITE);
}

static Rectangle Rc(int x, int y, int w, int h)
{
    return {(float)x, (float)y, (float)w, (float)h};
}

// Cắt chữ dài và thêm ".." cho vừa chiều rộng maxW
static string FitText(const string &text, float maxW)
{
    if (MeasureTextEx(fontPixel, text.c_str(), (float)MP_FONT, MP_SPACING).x <= maxW)
        return text;
    string s = text;
    while (s.size() > 1)
    {
        s.pop_back();
        string t = s + "..";
        if (MeasureTextEx(fontPixel, t.c_str(), (float)MP_FONT, MP_SPACING).x <= maxW)
            return t;
    }
    return s;
}

static void DrawMpText(const string &text, int x, int y, Color col)
{
    DrawTextEx(fontPixel, text.c_str(), {(float)x, (float)y}, (float)MP_FONT, MP_SPACING, col);
}

// Biểu tượng pixel: tam giác (play) và 2 vạch (pause)
static void DrawPlayIcon(int x, int y)
{
    for (int c = 0; c < 5; c++)
    {
        int h = 16 - 3 * c;
        DrawRectangle(x + c * 3, y + (16 - h) / 2, 3, h, COL_TEXT_LIGHT);
    }
}
static void DrawPauseIcon(int x, int y)
{
    DrawRectangle(x, y, 5, 16, COL_TEXT_LIGHT);
    DrawRectangle(x + 10, y, 5, 16, COL_TEXT_LIGHT);
}

// ---------------------------------------------------------------------
// 5) CÁC HÀM PHÁT NHẠC (Được Control gọi vào)
// ---------------------------------------------------------------------
static void PlaySong(int index)
{
    if (index < 0 || index >= SONG_COUNT)
        return;
    if (songLoaded[currentSong])
        StopMusicStream(songMusic[currentSong]);
    currentSong = index;
    playState = 1; // 1 = PLAYING
    if (songLoaded[currentSong])
        PlayMusicStream(songMusic[currentSong]);
}

void TogglePause()
{
    if (playState == 0)
    { // 0 = STOPPED
        PlaySong(currentSong);
    }
    else if (playState == 1)
    { // 1 = PLAYING
        if (songLoaded[currentSong])
            PauseMusicStream(songMusic[currentSong]);
        playState = 2; // 2 = PAUSED
    }
    else
    {
        if (songLoaded[currentSong])
            ResumeMusicStream(songMusic[currentSong]);
        playState = 1;
    }
}

// Hàm StepSong đổi tên / bọc lại để config.h gọi
void StepSong(int dir, bool wrap)
{
    int next = currentSong + dir;
    if (next < 0 || next >= SONG_COUNT)
    {
        if (!wrap)
            return;
        next = (next + SONG_COUNT) % SONG_COUNT;
    }
    PlaySong(next);
}

static void StopPlayback()
{
    if (songLoaded[currentSong])
        StopMusicStream(songMusic[currentSong]);
    playState = 0; // 0 = STOPPED
}

// Gọi từ Control
void MusicTogglePause() { TogglePause(); }
void MusicStep(int dir) { StepSong(dir, false); }

// ---------------------------------------------------------------------
// XỬ LÝ CHUỘT (Control bắt tín hiệu rồi truyền tọa độ vào đây xử lý logic UI)
// ---------------------------------------------------------------------
static float SlotHeight() { return (float)TRACK_H / SONG_COUNT; }
static int SlotTop(int i) { return (int)(i * SlotHeight() + 0.5f); }

static void SelectBySliderY(float mouseY)
{
    int idx = (int)((mouseY - (PY + TRACK_Y)) / SlotHeight());
    if (idx < 0)
        idx = 0;
    if (idx >= SONG_COUNT)
        idx = SONG_COUNT - 1;
    if (idx != currentSong)
        PlaySong(idx);
}

void MusicProcessMouse(Vector2 m, bool clicked, bool down)
{
    if (justOpened)
    {
        if (!down)
            justOpened = false;
        return;
    }

    // Xử lý kéo thanh trượt
    if (draggingThumb)
    {
        if (down)
            SelectBySliderY(m.y);
        else
            draggingThumb = false;
    }

    if (!clicked)
        return; // Nếu không click thì kết thúc hàm

    // Xử lý các nút click
    Rectangle btnClose = Rc(PX + PW - 8 - 26, PY + 10, 26, 26);
    Rectangle btnMin = Rc(PX + PW - 8 - 26 - 6 - 26, PY + 10, 26, 26);
    if (CheckCollisionPointRec(m, btnClose))
    {
        StopPlayback();
        playerOpen = false;
        return;
    }
    if (CheckCollisionPointRec(m, btnMin))
    {
        playerOpen = false;
        return;
    }

    // Click ảnh bìa (Pause/Play)
    Rectangle cover = Rc(PX + PAD_L + FRAME_B, PY + ART_Y + FRAME_B, CONTENT_W - 2 * FRAME_B, ART_H - 2 * FRAME_B);
    if (CheckCollisionPointRec(m, cover))
    {
        TogglePause();
        return;
    }

    // Click vào từng bài hát
    for (int i = 0; i < SONG_COUNT; i++)
    {
        Rectangle row = Rc(PX + PAD_L + FRAME_B + 2, PY + LIST_Y + FRAME_B + 4 + ROW_H * i, CONTENT_W - 2 * FRAME_B - 4, ROW_H - 2);
        if (CheckCollisionPointRec(m, row))
        {
            if (i == currentSong)
                TogglePause();
            else
                PlaySong(i);
            return;
        }
    }

    // Click khu vực thanh cuộn
    Rectangle up = Rc(PX + SB_X, PY + SB_TOP, SB_W, CAP_H);
    Rectangle downBtn = Rc(PX + SB_X, PY + SB_BOT - CAP_H, SB_W, CAP_H);
    Rectangle track = Rc(PX + SB_X, PY + TRACK_Y, SB_W, TRACK_H);
    if (CheckCollisionPointRec(m, up))
    {
        StepSong(-1, false);
        return;
    }
    if (CheckCollisionPointRec(m, downBtn))
    {
        StepSong(+1, false);
        return;
    }
    if (CheckCollisionPointRec(m, track))
    {
        draggingThumb = true;
        SelectBySliderY(m.y);
        return;
    }
}

// ---------------------------------------------------------------------
// 6) NẠP / GIẢI PHÓNG (Giữ nguyên như cũ)
// ---------------------------------------------------------------------
void LoadMusicPlayer()
{
    // --- các mảnh giao diện ---
    texMpTitle = LoadPixelTexture("assets/mp_titlebar.png");
    texMpMin = LoadPixelTexture("assets/mp_btn_min.png");
    texMpClose = LoadPixelTexture("assets/mp_btn_close.png");
    texMpTile = LoadPixelTexture("assets/mp_paper_tile.png");
    texMpFrame = LoadPixelTexture("assets/mp_frame.png");
    texMpInfo = LoadPixelTexture("assets/mp_infobox.png");
    texMpUp = LoadPixelTexture("assets/mp_sb_up.png");
    texMpDown = LoadPixelTexture("assets/mp_sb_down.png");
    texMpTrack = LoadPixelTexture("assets/mp_sb_track.png");
    texMpThumbTop = LoadPixelTexture("assets/mp_sb_thumb_top.png");
    texMpKnob = LoadPixelTexture("assets/mp_sb_knob.png");
    texMpThumbBot = LoadPixelTexture("assets/mp_sb_thumb_bot.png");

    // --- âm thanh ---
    if (!IsAudioDeviceReady())
        InitAudioDevice();

    for (int i = 0; i < SONG_COUNT; i++)
    {
        songLoaded[i] = false;
        coverLoaded[i] = false;

        if (FileExists(songs[i].coverPath))
        {
            songCover[i] = LoadTexture(songs[i].coverPath);
            SetTextureFilter(songCover[i], TEXTURE_FILTER_BILINEAR);
            coverLoaded[i] = (songCover[i].id > 0);
        }

        if (IsAudioDeviceReady() && FileExists(songs[i].musicPath))
        {
            songMusic[i] = LoadMusicStream(songs[i].musicPath);
            songLoaded[i] = (songMusic[i].frameCount > 0);
            if (songLoaded[i])
                songMusic[i].looping = false;
        }
    }
}

void UnloadMusicPlayer()
{
    for (int i = 0; i < SONG_COUNT; i++)
    {
        if (songLoaded[i])
        {
            StopMusicStream(songMusic[i]);
            UnloadMusicStream(songMusic[i]);
        }
        if (coverLoaded[i])
            UnloadTexture(songCover[i]);
        songLoaded[i] = false;
        coverLoaded[i] = false;
    }
    UnloadTexture(texMpTitle);
    UnloadTexture(texMpMin);
    UnloadTexture(texMpClose);
    UnloadTexture(texMpTile);
    UnloadTexture(texMpFrame);
    UnloadTexture(texMpInfo);
    UnloadTexture(texMpUp);
    UnloadTexture(texMpDown);
    UnloadTexture(texMpTrack);
    UnloadTexture(texMpThumbTop);
    UnloadTexture(texMpKnob);
    UnloadTexture(texMpThumbBot);
    if (IsAudioDeviceReady())
        CloseAudioDevice();
}

// ---------------------------------------------------------------------
// 7) MỞ / ĐÓNG
// ---------------------------------------------------------------------
void OpenMusicPlayer()
{
    playerOpen = true;
    justOpened = true;
    draggingThumb = false;
    if (playState == 0)
        PlaySong(currentSong); // 0 = STOPPED
}

bool IsMusicPlayerOpen() { return playerOpen; }

// ---------------------------------------------------------------------
// 8) CẬP NHẬT MỖI FRAME: Bây giờ CHỈ CÒN nhiệm vụ stream nhạc
// ---------------------------------------------------------------------
void UpdateMusicPlayer()
{
    if (IsAudioDeviceReady())
        SetMasterVolume(_sound ? 1.0f : 0.0f);

    // Không còn phần lấy input chuột/phím ở đây nữa.
    if (playState == 1 && songLoaded[currentSong])
    { // 1 = PLAYING
        UpdateMusicStream(songMusic[currentSong]);
        if (!IsMusicStreamPlaying(songMusic[currentSong]))
            StepSong(+1, true); // Tự động chuyển bài
    }
}

// ---------------------------------------------------------------------
// 9) VẼ  (ghép các mảnh ảnh lại thành cửa sổ nhạc)
// ---------------------------------------------------------------------
void DrawMusicPlayer()
{
    if (!playerOpen)
        return;
    Vector2 mouse = GetMousePosition();

    // Làm tối nền menu + đổ bóng cho cửa sổ
    DrawRectangle(0, 0, 1000, 700, Fade(BLACK, 0.45f));
    DrawRectangle(PX + 6, PY + 6, PW, PH, Fade(BLACK, 0.35f));

    // --- Giấy kẻ dòng: lát tile cho kín thân cửa sổ, cắt gọn bằng Scissor ---
    Rectangle body = Rc(PX, PY + TITLE_H, PW, PH - TITLE_H);
    BeginScissorMode((int)body.x, (int)body.y, (int)body.width, (int)body.height);

    // FIX LỖI TREO MÁY: Kiểm tra nếu ảnh load thành công (width, height > 0) thì mới lặp
    if (texMpTile.width > 0 && texMpTile.height > 0)
    {
        for (int y = 0; y < (int)body.height; y += texMpTile.height)
        {
            for (int x = 0; x < (int)body.width; x += texMpTile.width)
            {
                DrawTexture(texMpTile, (int)body.x + x, (int)body.y + y, WHITE);
            }
        }
    }
    else
    {
        // Vẽ màu nền dự phòng nếu bị thiếu file ảnh mp_paper_tile.png
        DrawRectangleRec(body, RAYWHITE);
    }

    EndScissorMode();
    DrawRectangleLinesEx(body, 2.0f, COL_BORDER);

    // --- Thanh tiêu đề + 2 nút ---
    DrawNineSlice(texMpTitle, Rc(PX, PY, PW, TITLE_H), 14, 0, 14, 0);
    DrawMpText("MUSIC", PX + 14, PY + 15, COL_TEXT_LIGHT);

    Rectangle btnClose = Rc(PX + PW - 8 - 26, PY + 10, 26, 26);
    Rectangle btnMin = Rc(PX + PW - 8 - 26 - 6 - 26, PY + 10, 26, 26);
    Color tMin = CheckCollisionPointRec(mouse, btnMin) ? Color{225, 200, 170, 255} : WHITE;
    Color tClose = CheckCollisionPointRec(mouse, btnClose) ? Color{225, 200, 170, 255} : WHITE;
    DrawTexture(texMpMin, (int)btnMin.x, (int)btnMin.y, tMin);
    DrawTexture(texMpClose, (int)btnClose.x, (int)btnClose.y, tClose);

    // --- Khung ảnh bìa (kéo thành chữ nhật) + ảnh ---
    DrawNineSlice(texMpFrame, Rc(PX + PAD_L, PY + ART_Y, CONTENT_W, ART_H), FRAME_B, FRAME_B, FRAME_B, FRAME_B);
    Rectangle cover = Rc(PX + PAD_L + FRAME_B, PY + ART_Y + FRAME_B, CONTENT_W - 2 * FRAME_B, ART_H - 2 * FRAME_B);
    if (coverLoaded[currentSong])
    {
        DrawStretch(songCover[currentSong], cover);
    }
    else
    {
        DrawRectangleRec(cover, Color{222, 200, 165, 255});
    }
    if (!songLoaded[currentSong])
    { // chưa có file nhạc -> báo cho dễ thấy
        DrawRectangle((int)cover.x, (int)cover.y + (int)cover.height / 2 - 16, (int)cover.width, 32, Fade(BLACK, 0.6f));
        string msg = string("NO FILE: ") + songs[currentSong].musicPath;
        msg = FitText(msg, cover.width - 16);
        Vector2 ts = MeasureTextEx(fontPixel, msg.c_str(), (float)MP_FONT, MP_SPACING);
        DrawMpText(msg, (int)(cover.x + (cover.width - ts.x) / 2), (int)cover.y + (int)cover.height / 2 - 8, COL_TEXT_LIGHT);
    }
    // Biểu tượng play/pause ở góc dưới phải ảnh bìa
    int ix = (int)(cover.x + cover.width) - 40, iy = (int)(cover.y + cover.height) - 40;
    DrawRectangle(ix, iy, 32, 32, Fade(BLACK, 0.55f));
    if (playState == 1)
        DrawPauseIcon(ix + 9, iy + 8);
    else
        DrawPlayIcon(ix + 9, iy + 8);

    // --- Ô tên bài + tác giả (+ thời gian) ---
    DrawNineSlice(texMpInfo, Rc(PX + PAD_L, PY + INFO_Y, CONTENT_W, INFO_H), 11, 10, 11, 10);
    int tx = PX + PAD_L + 16;
    float maxW = CONTENT_W - 32;
    DrawMpText(FitText(songs[currentSong].title, maxW), tx, PY + INFO_Y + 11, COL_TEXT_DARK);

    string timeStr = "";
    if (songLoaded[currentSong])
    {
        int t = (int)GetMusicTimePlayed(songMusic[currentSong]);
        int len = (int)GetMusicTimeLength(songMusic[currentSong]);
        if (t > len)
            t = len;
        timeStr = TextFormat("%d:%02d/%d:%02d", t / 60, t % 60, len / 60, len % 60);
    }
    float timeW = timeStr.empty() ? 0.0f : MeasureTextEx(fontPixel, timeStr.c_str(), (float)MP_FONT, MP_SPACING).x;
    DrawMpText(FitText(songs[currentSong].artist, maxW - timeW - 12), tx, PY + INFO_Y + 33, COL_TEXT_MID);
    if (!timeStr.empty())
        DrawMpText(timeStr, tx + (int)(maxW - timeW), PY + INFO_Y + 33, COL_TEXT_MID);

    // --- Khung danh sách + 5 dòng ---
    DrawNineSlice(texMpFrame, Rc(PX + PAD_L, PY + LIST_Y, CONTENT_W, LIST_H), FRAME_B, FRAME_B, FRAME_B, FRAME_B);
    for (int i = 0; i < SONG_COUNT; i++)
    {
        Rectangle row = Rc(PX + PAD_L + FRAME_B + 2, PY + LIST_Y + FRAME_B + 4 + ROW_H * i, CONTENT_W - 2 * FRAME_B - 4, ROW_H - 2);
        if (i == currentSong)
            DrawRectangleRec(row, COL_ROW_NOW);
        else if (CheckCollisionPointRec(mouse, row))
            DrawRectangleRec(row, COL_ROW_HOVER);
        string label = string(TextFormat("%d. ", i + 1)) + songs[i].title;
        DrawMpText(FitText(label, row.width - 20), (int)row.x + 8, (int)row.y + 8, COL_TEXT_DARK);
    }

    // --- Thanh cuộn: mũi tên trên + rãnh + thanh trượt (1/5 rãnh) + mũi tên dưới ---
    DrawTexture(texMpUp, PX + SB_X, PY + SB_TOP, WHITE);
    DrawStretch(texMpTrack, Rc(PX + SB_X, PY + TRACK_Y, SB_W, TRACK_H));
    DrawTexture(texMpDown, PX + SB_X, PY + SB_BOT - CAP_H, WHITE);

    int thumbY = PY + TRACK_Y + SlotTop(currentSong);
    int thumbH = SlotTop(currentSong + 1) - SlotTop(currentSong); // = TRACK_H / 5
    int restH = thumbH - texMpKnob.height;
    int topH = restH / 2, botH = restH - topH;
    int thumbX = PX + SB_X + 3;
    DrawStretch(texMpThumbTop, Rc(thumbX, thumbY, texMpThumbTop.width, topH));                           // phần nâu
    DrawTexture(texMpKnob, thumbX, thumbY + topH, WHITE);                                                // núm kem ở giữa
    DrawStretch(texMpThumbBot, Rc(thumbX, thumbY + topH + texMpKnob.height, texMpThumbBot.width, botH)); // phần nâu đỏ
}
