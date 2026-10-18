#define TURN_LIMIT_SEC 10                    // thời gian suy nghĩ mỗi lượt (giây)
// ---------- Dong ho dem nguoc moi luot ----------
ULONGLONG gTurnLeftMs = TURN_LIMIT_SEC * 1000ULL;  // số mili-giây CÒN LẠI của lượt này
bool gTurnOn = false;       // true: đang đếm ngược (chỉ tính lượt của NGƯỜI)
bool gTurnPause = false;    // tạm dừng khi đang mở hộp thoại (hỏi Có/Không...)
int  gTurnShown = -1;       // số giây đang hiển thị (chỉ vẽ lại khi đổi giây)
void ResetTurnTimer()
{
    gTurnLeftMs = TURN_LIMIT_SEC * 1000ULL;   // nạp lại đầy 10 giây
    gTurnOn = !(gMode == 2 && !_TURN);        // lượt máy thì KHÔNG tính giờ
    gTurnShown = -1;                          // ép vẽ lại lần đầu
}
// Ve dong ho dem nguoc o hang 6 cua bang THONG TIN
void ShowTurnTimer()
{
    char buf[40], bar[11];
    if (!gTurnOn)                              // không đếm (lượt máy / hết ván)
    {
        gTurnShown = -1;
        PanelLine(6, "Con lai : --", 8);       // hiện "--" màu xám
        GotoXY(_X, _Y);
        return;
    }
    int s = (int)((gTurnLeftMs + 999) / 1000); // đổi ms -> giây, LÀM TRÒN LÊN
    gTurnShown = s;
    int fill = (s * 10 + TURN_LIMIT_SEC - 1) / TURN_LIMIT_SEC;  // số ô '#' của thanh (0..10)
    if (fill > 10) fill = 10;
    for (int k = 0; k < 10; k++) bar[k] = (k < fill) ? '#' : '-';
    bar[10] = 0;                               // ký tự kết thúc chuỗi
    sprintf(buf, "Con lai : %2ds [%s]", s, bar);
    PanelLine(6, buf, s <= 3 ? 12 : (s <= 5 ? 14 : 10));   // đỏ <=3s, vàng <=5s, xanh còn lại
    GotoXY(_X, _Y);
}
// Cap nhat dong ho (goi thuong xuyen); chi ve lai khi doi giay
void UpdateClock()
{
    ULONGLONG now = GetTickCount64();          // giờ hiện tại (ms)
    ULONGLONG d = now - gLastTick;             // thời gian trôi qua từ lần gọi trước
    if (gClockOn)
    {
        gElapsedMs += d;                       // đồng hồ cả ván (cộng dồn)
        if (gTurnOn && !gTurnPause)            // đếm ngược: đang bật VÀ không tạm dừng
            gTurnLeftMs = (gTurnLeftMs > d) ? gTurnLeftMs - d : 0;   // trừ, không cho âm
    }
    gLastTick = now;                           // ghi mốc cho lần sau
    if ((int)(gElapsedMs / 1000) != gShownSec) ShowTime();
    if (gTurnOn && (int)((gTurnLeftMs + 999) / 1000) != gTurnShown) ShowTurnTimer();
}
// (a) Bắt đầu ván mới (cuối hàm ResetData)
ResetTurnTimer();               // dong ho dem nguoc luot

// (b) Nạp ván đã lưu (cuối hàm LoadGame)
ResetTurnTimer();               // dat lai dong ho dem nguoc theo luot vua nap

// (c) Đổi lượt sau mỗi nước đi (trong ProcessFinish, case 2)
case 2:
    _TURN = !_TURN;
    ShowTurn();
    ResetTurnTimer();           // lượt mới -> nạp lại 10 giây
    ShowTurnTimer();
    break;

    // (d) Lùi nước đi (cuối hàm UndoMove)
    ResetTurnTimer();
    ShowTurnTimer();
    gTurnPause = true;            // tam dung dong ho dem nguoc khi co hop thoai
    ... (hiện hộp thoại, chờ người chơi chọn) ...
        gTurnPause = false;           // chay tiep dong ho dem nguoc
    for (;;)
    {
        UpdateClock();

        // ---------- Het gio suy nghi ----------
        if (gTurnOn && gTurnLeftMs == 0)
        {
            int t = TimeoutLose();
            if (t >= 0) return t;
            continue;
        }
        ...
            for (;;)
            {
                UpdateClock();

                // ---------- Het gio suy nghi ----------
                if (gTurnOn && gTurnLeftMs == 0)
                {
                    int t = TimeoutLose();
                    if (t >= 0) return t;
                    continue;
                }
                ...