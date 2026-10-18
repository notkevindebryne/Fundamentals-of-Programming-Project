// ======================= BOT NÂNG CẤP LẦN 2 =======================
// Iterative Deepening + Negamax/Alpha-Beta + Zobrist/Bảng chuyển vị
// + Chấm điểm tăng dần + Nước bắt buộc (thắng ngay / chặn ngay)
#define BOT    1        // quân O (máy)
#define HUMAN (-1)      // quân X (người)

typedef unsigned long long u64;                 // số nguyên 64 bit không dấu

const int WIN_SCORE = 1000000;                 // điểm thắng (lớn hơn mọi thế cờ thường)
const int INF_SCORE = 2000000;                 // "vô cực"
const int FIVE_SCORE = 100000;                  // LineScore trả về giá trị này khi đủ 5 quân
const int WINDOW_SCORE[6] = { 0, 10, 100, 1000, 10000, 100000 };
const int MAX_CAND = 12;                      // mỗi nút chỉ xét 12 ô triển vọng nhất

// Cấu hình từng mức (đặt tên để khỏi dính "magic number")
const int NORMAL_DEPTH = 3, NORMAL_MS = 300;   // mức Thường
const int HARD_DEPTH = 12, HARD_MS = 1800;  // mức Khó

// ---------- Trạng thái dùng riêng cho tìm kiếm ----------
u64 gZobrist[BOARD_SIZE][BOARD_SIZE][2];        // số ngẫu nhiên cho (ô, loại quân)
u64 gHash = 0;                                  // "mã vân tay" của thế cờ hiện tại
int gEval = 0;                                  // điểm cả bàn cờ (dương = lợi cho máy)
int gNear[BOARD_SIZE][BOARD_SIZE];              // số quân nằm trong phạm vi 2 ô quanh mỗi ô

ULONGLONG gDeadline = 0;                        // hạn chót (mốc GetTickCount64)
bool gTimeUp = false;                           // đã hết giờ nghĩ chưa
unsigned long gNodes = 0;                       // đếm số thế cờ đã xét
int gRootI = -1, gRootJ = -1;                   // nước tốt nhất tìm được ở gốc
int gLastDepth = 0;                             // độ sâu hoàn thành gần nhất (để hiển thị)

// ---------- Bảng chuyển vị ----------
enum { TT_EXACT = 0, TT_LOWER, TT_UPPER };      // điểm chính xác / cận dưới / cận trên
struct TTEntry
{
    u64   key;      // mã thế cờ để kiểm tra đúng thế
    int   score;    // điểm đã tính
    short depth;    // đã tìm sâu bao nhiêu
    short move;     // nước tốt nhất (i * BOARD_SIZE + j)
    char  flag;     // loại điểm
};
const int TT_SIZE = 1 << 18;                    // 262144 ô, khoảng 6 MB
TTEntry gTT[TT_SIZE];                           // để GLOBAL: mảng lớn mà để trong hàm sẽ tràn stack

// Sinh số ngẫu nhiên 64 bit (thuật toán xorshift: đơn giản, nhanh)
u64 Rand64()
{
    static u64 x = 88172645463325252ULL;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
}

void InitZobrist()
{
    static bool done = false;
    if (done) return;
    done = true;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            gZobrist[i][j][0] = Rand64();
            gZobrist[i][j][1] = Rand64();
        }
}

// ---------- Chấm điểm cửa sổ 5 ô (như bản trước) ----------
int EvalWindow(int i, int j, int di, int dj)
{
    int nBot = 0, nHum = 0;
    for (int k = 0; k < WIN_COUNT; k++)
    {
        int c = _A[i + di * k][j + dj * k].c;
        if (c == BOT) nBot++;
        else if (c == HUMAN) nHum++;
    }
    if (nBot > 0 && nHum > 0) return 0;                  // cửa sổ "chết"
    if (nBot > 0) return WINDOW_SCORE[nBot];
    if (nHum > 0) return -WINDOW_SCORE[nHum] * 11 / 10;  // coi trọng mối đe dọa của người hơn một chút
    return 0;
}

// Chấm cả bàn: CHỈ gọi 1 lần ở đầu mỗi lượt nghĩ để lấy điểm xuất phát
int EvalBoard()
{
    int total = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            for (int d = 0; d < 4; d++)
            {
                int ei = i + DI[d] * (WIN_COUNT - 1);
                int ej = j + DJ[d] * (WIN_COUNT - 1);
                if (!InBoard(ei, ej)) continue;
                total += EvalWindow(i, j, DI[d], DJ[d]);
            }
    return total;
}

// Tổng điểm của MỌI cửa sổ 5 ô có chứa ô (i, j): tối đa 4 hướng x 5 cửa sổ = 20 cửa sổ
int WindowsAround(int i, int j)
{
    int total = 0;
    for (int d = 0; d < 4; d++)
        for (int k = 0; k < WIN_COUNT; k++)
        {
            int si = i - DI[d] * k, sj = j - DJ[d] * k;                       // ô đầu cửa sổ
            int ei = si + DI[d] * (WIN_COUNT - 1), ej = sj + DJ[d] * (WIN_COUNT - 1); // ô cuối
            if (!InBoard(si, sj) || !InBoard(ei, ej)) continue;
            total += EvalWindow(si, sj, DI[d], DJ[d]);
        }
    return total;
}

void AddNear(int i, int j, int delta)
{
    for (int a = -2; a <= 2; a++)
        for (int b = -2; b <= 2; b++)
            if (InBoard(i + a, j + b)) gNear[i + a][j + b] += delta;
}

// Đặt quân thử trong đầu: cập nhật cả bàn cờ, hash, điểm, bảng "gần quân"
void SearchPut(int i, int j, int who)
{
    int before = WindowsAround(i, j);            // điểm các cửa sổ liên quan TRƯỚC khi đặt
    _A[i][j].c = who;
    gHash ^= gZobrist[i][j][who == BOT ? 1 : 0]; // XOR thêm quân vào hash
    gEval += WindowsAround(i, j) - before;       // chỉ cộng phần chênh lệch
    AddNear(i, j, +1);
}

// Gỡ quân thử (hoàn tác chính xác SearchPut)
void SearchRemove(int i, int j)
{
    int who = _A[i][j].c;
    int before = WindowsAround(i, j);
    _A[i][j].c = 0;
    gHash ^= gZobrist[i][j][who == BOT ? 1 : 0]; // XOR lần nữa = gỡ quân khỏi hash
    gEval += WindowsAround(i, j) - before;
    AddNear(i, j, -1);
}

// ---------- Sinh và sắp xếp ô ứng viên ----------
struct Cand { int i, j, atk, def, order; };

// Trả về TẤT CẢ ô trống gần quân, xếp theo "độ hứa hẹn" giảm dần:
//   (1) ô giúp "who" thắng ngay  >  (2) ô phải chặn  >  (3) nước tốt nhất lần trước (ttMove)  >  (4) điểm sơ bộ
int GenCandidates(Cand* out, int who, int ttMove)
{
    int n = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (_A[i][j].c != 0 || gNear[i][j] == 0) continue;
            Cand& c = out[n++];
            c.i = i;
            c.j = j;
            c.atk = ForkScore(i, j, who);                // điểm tấn công của "who"
            c.def = ForkScore(i, j, -who);               // điểm đối thủ nhận được nếu chiếm ô này
            c.order = c.atk + c.def;
            if (c.atk >= FIVE_SCORE) c.order += 100000000;          // thắng ngay
            else if (c.def >= FIVE_SCORE) c.order += 50000000;      // đối thủ sắp thắng
            else if (i * BOARD_SIZE + j == ttMove) c.order += 10000000;
        }

    for (int a = 1; a < n; a++)                          // insertion sort giảm dần
    {
        Cand t = out[a];
        int b = a - 1;
        while (b >= 0 && out[b].order < t.order) { out[b + 1] = out[b]; b--; }
        out[b + 1] = t;
    }
    return n;
}

// ---------- Tìm kiếm Negamax + Alpha-Beta ----------
// who: bên đang đi. Trả về điểm theo góc nhìn của "who" (dương = who đang tốt)
// ply: số nước đã đi từ gốc (để ưu tiên thắng sớm / thua muộn)
int Search(int depth, int alpha, int beta, int who, int ply)
{
    // Cứ 2048 thế cờ thì xem đồng hồ 1 lần (gọi GetTickCount64 liên tục sẽ chậm)
    if ((++gNodes & 2047) == 0 && GetTickCount64() > gDeadline) gTimeUp = true;
    if (gTimeUp) return 0;

    if (depth <= 0) return (who == BOT) ? gEval : -gEval;   // hết độ sâu -> lấy điểm đã tính sẵn

    // ----- Tra bảng chuyển vị -----
    int alphaOrig = alpha;
    int ttMove = -1;
    TTEntry& e = gTT[gHash & (TT_SIZE - 1)];
    if (e.key == gHash)
    {
        ttMove = e.move;
        if (ply > 0 && e.depth >= depth)                    // đã từng tìm đủ sâu rồi
        {
            if (e.flag == TT_EXACT) return e.score;
            if (e.flag == TT_LOWER) { if (e.score > alpha) alpha = e.score; }
            else { if (e.score < beta)  beta = e.score; }
            if (alpha >= beta) return e.score;
        }
    }

    Cand cand[MAX_MOVES];
    int n = GenCandidates(cand, who, ttMove);
    if (n == 0) return 0;                                   // hết ô -> hòa

    // ----- Nước bắt buộc -----
    if (cand[0].atk >= FIVE_SCORE) return WIN_SCORE - ply - 1;      // đi 1 nước là thắng
    int ext = 0;
    if (cand[0].def >= FIVE_SCORE)                          // đối thủ sắp có 5 quân
    {
        if (n >= 2 && cand[1].def >= FIVE_SCORE)            // đối thủ có 2 chỗ thắng -> không chặn nổi
            return -(WIN_SCORE - ply - 2);
        n = 1;                                              // chỉ còn đúng 1 nước: chặn
        ext = 1;                                            // và không tính nước này vào độ sâu
    }
    else if (n > MAX_CAND) n = MAX_CAND;

    int best = -INF_SCORE, bestMove = -1;
    for (int k = 0; k < n; k++)
    {
        int i = cand[k].i, j = cand[k].j;
        SearchPut(i, j, who);
        int s = -Search(depth - 1 + ext, -beta, -alpha, -who, ply + 1);   // đổi dấu + đảo cửa sổ
        SearchRemove(i, j);
        if (gTimeUp) return 0;

        if (s > best)
        {
            best = s;
            bestMove = i * BOARD_SIZE + j;
            if (ply == 0) { gRootI = i; gRootJ = j; }       // ghi nhận nước tốt nhất ở gốc
        }
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;                           // CẮT TỈA
    }

    // ----- Ghi vào bảng chuyển vị -----
    e.key = gHash;
    e.score = best;
    e.depth = (short)depth;
    e.move = (short)bestMove;
    e.flag = (best <= alphaOrig) ? TT_UPPER : (best >= beta ? TT_LOWER : TT_EXACT);
    return best;
}

// ---------- Hàm chọn nước đi cho máy ----------
void ChooseBotMove(int maxDepth, int ms, int* bi, int* bj)
{
    InitZobrist();

    // Khởi tạo trạng thái tìm kiếm từ bàn cờ thật
    gHash = 0;
    gEval = EvalBoard();
    memset(gNear, 0, sizeof(gNear));
    memset(gTT, 0, sizeof(gTT));
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (_A[i][j].c != 0)
            {
                gHash ^= gZobrist[i][j][_A[i][j].c == BOT ? 1 : 0];
                AddNear(i, j, +1);
            }

    // Kiểm tra nhanh trước khi tìm kiếm
    Cand cand[MAX_MOVES];
    int n = GenCandidates(cand, BOT, -1);
    if (n == 0)                                             // bàn trống -> đánh vào tâm
    {
        *bi = BOARD_SIZE / 2;
        *bj = BOARD_SIZE / 2;
        gLastDepth = 0;
        return;
    }
    *bi = cand[0].i;                                        // nước dự phòng: ô sơ bộ tốt nhất
    *bj = cand[0].j;
    gLastDepth = 0;
    if (cand[0].atk >= FIVE_SCORE || cand[0].def >= FIVE_SCORE) return;  // thắng ngay hoặc phải chặn ngay

    // Đào sâu dần: độ sâu 1, 2, 3, ... cho tới khi hết giờ hoặc đạt độ sâu tối đa
    gDeadline = GetTickCount64() + (ULONGLONG)ms;
    gTimeUp = false;
    gNodes = 0;
    for (int depth = 1; depth <= maxDepth; depth++)
    {
        int s = Search(depth, -INF_SCORE, INF_SCORE, BOT, 0);
        if (gTimeUp) break;                                 // lần này dở dang -> bỏ, giữ kết quả độ sâu trước
        *bi = gRootI;
        *bj = gRootJ;
        gLastDepth = depth;
        if (s > WIN_SCORE - 1000 || s < -WIN_SCORE + 1000) break;   // đã thấy thắng/thua chắc chắn
    }
}

void ComputerMove()
{
    int bi = -1, bj = -1;

    if (gDiff == 0)
    {
        // ===== BOT DỄ: giữ nguyên cách cũ =====
        if (gCount > 0 && rand() % 100 < 35)
        {
            int ci[MAX_MOVES], cj[MAX_MOVES], cn = 0;
            for (int i = 0; i < BOARD_SIZE; i++)
                for (int j = 0; j < BOARD_SIZE; j++)
                    if (_A[i][j].c == 0 && NearStone(i, j, 2)) { ci[cn] = i; cj[cn] = j; cn++; }
            if (cn > 0) { int r = rand() % cn; bi = ci[r]; bj = cj[r]; }
        }
        if (bi < 0)
        {
            int bestScore = -1, n = 0;
            for (int i = 0; i < BOARD_SIZE; i++)
                for (int j = 0; j < BOARD_SIZE; j++)
                {
                    if (_A[i][j].c != 0) continue;
                    int s = CellScore(i, j, BOT) + CellScore(i, j, HUMAN) * 7 / 10 + rand() % 250;
                    if (s > bestScore) { bestScore = s; bi = i; bj = j; n = 1; }
                    else if (s == bestScore) { n++; if (rand() % n == 0) { bi = i; bj = j; } }
                }
        }
    }
    else if (gDiff == 1) ChooseBotMove(NORMAL_DEPTH, NORMAL_MS, &bi, &bj);   // Thường
    else                 ChooseBotMove(HARD_DEPTH, HARD_MS, &bi, &bj);   // Khó

    PlaceAt(bi, bj);
}