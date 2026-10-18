void ComputerMove()
{
    if (gBlockRule)                       // chế độ chặn 2 đầu -> dùng bot riêng
    {
        int bi, bj;
        Block2::Choose(&bi, &bj);
        PlaceAt(bi, bj);
        return;
    }
    // ======================= BOT CHO LUAT "CHAN 2 DAU" =======================
    // Negamax + Alpha-Beta + Iterative Deepening + chấm điểm tăng dần,
    // tất cả đều hiểu luật "chặn 2 đầu" (dùng RunWins / WouldWin ở trên)
    namespace Block2
    {
        const int ME = 1;                // máy luôn là O (=1)
        const int OPP = -1;               // người là X (=-1)

        const int WIN_SCORE = 1000000;
        const int INF_SCORE = 2000000;
        const int WINDOW_SCORE[6] = { 0, 10, 100, 1000, 10000, 100000 };
        const int MAX_CAND = 12;          // mỗi nút chỉ xét 12 ô triển vọng nhất

        // Cấu hình từng mức
        const int EASY_DEPTH = 1, EASY_MS = 50;
        const int NORMAL_DEPTH = 3, NORMAL_MS = 300;
        const int HARD_DEPTH = 12, HARD_MS = 1800;

        int gEval = 0;                                  // điểm cả bàn (dương = lợi cho máy)
        int gNear[BOARD_SIZE][BOARD_SIZE];              // số quân trong phạm vi 2 ô quanh mỗi ô
        ULONGLONG gDeadline = 0;
        bool gTimeUp = false;
        unsigned long gNodes = 0;
        int gRootI = -1, gRootJ = -1;                   // nước tốt nhất ở gốc
        int gHintI = -1, gHintJ = -1;                   // nước tốt nhất của độ sâu trước (xét trước)
        int gLastDepth = 0;                             // độ sâu hoàn thành gần nhất

        // ---------- Chấm điểm cửa sổ 5 ô, BIẾT LUẬT chặn 2 đầu ----------
        int EvalWindow(int i, int j, int di, int dj)
        {
            int nMe = 0, nOpp = 0;
            for (int k = 0; k < WIN_COUNT; k++)
            {
                int c = _A[i + di * k][j + dj * k].c;
                if (c == ME) nMe++;
                else if (c == OPP) nOpp++;
            }
            if ((nMe > 0 && nOpp > 0) || (nMe == 0 && nOpp == 0)) return 0;

            int who = (nMe > 0) ? ME : OPP;
            if (gBlockRule)
            {
                // 2 ô ngay ngoài cửa sổ: nếu cả hai đều chặn -> dãy 5 này vô dụng ("cửa sổ chết")
                bool b1 = IsBlocked(i - di, j - dj, who);
                bool b2 = IsBlocked(i + di * WIN_COUNT, j + dj * WIN_COUNT, who);
                if (b1 && b2) return 0;
            }
            if (nMe > 0) return WINDOW_SCORE[nMe];
            return -WINDOW_SCORE[nOpp] * 11 / 10;
        }

        // Chấm cả bàn: chỉ gọi 1 lần ở đầu mỗi lượt nghĩ
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

        // Tổng điểm mọi cửa sổ có thể bị ảnh hưởng khi ô (i, j) đổi trạng thái.
        // t = 0..4: cửa sổ CHỨA ô này. t = -1 và t = 5: cửa sổ nằm ngay cạnh, ô này là "đầu" của nó
        // (vì luật chặn 2 đầu, đầu của cửa sổ cũng làm đổi điểm) => 7 cửa sổ mỗi hướng
        int WindowsAround(int i, int j)
        {
            int total = 0;
            for (int d = 0; d < 4; d++)
                for (int t = -1; t <= WIN_COUNT; t++)
                {
                    int si = i - DI[d] * t, sj = j - DJ[d] * t;                         // ô đầu cửa sổ
                    int ei = si + DI[d] * (WIN_COUNT - 1), ej = sj + DJ[d] * (WIN_COUNT - 1);
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

        void SearchPut(int i, int j, int who)           // đặt quân thử
        {
            int before = WindowsAround(i, j);
            _A[i][j].c = who;
            gEval += WindowsAround(i, j) - before;      // chỉ cộng phần chênh lệch
            AddNear(i, j, +1);
        }

        void SearchRemove(int i, int j)                 // gỡ quân thử
        {
            int before = WindowsAround(i, j);
            _A[i][j].c = 0;
            gEval += WindowsAround(i, j) - before;
            AddNear(i, j, -1);
        }

        // ---------- Ô ứng viên ----------
        struct Cand { int i, j, order; bool win, oppWin; };

        void SortCands(Cand* c, int n)                  // insertion sort giảm dần theo order
        {
            for (int a = 1; a < n; a++)
            {
                Cand t = c[a];
                int b = a - 1;
                while (b >= 0 && c[b].order < t.order) { c[b + 1] = c[b]; b--; }
                c[b + 1] = t;
            }
        }

        // Mọi ô trống gần quân cờ, kèm: who thắng ngay? đối thủ thắng ngay nếu đánh ô này?
        int Gen(Cand* out, int who, int hintI, int hintJ)
        {
            int n = 0;
            for (int i = 0; i < BOARD_SIZE; i++)
                for (int j = 0; j < BOARD_SIZE; j++)
                {
                    if (_A[i][j].c != 0 || gNear[i][j] == 0) continue;
                    Cand& c = out[n++];
                    c.i = i;
                    c.j = j;
                    c.win = WouldWin(i, j, who);      // luật thật, không phải ước lượng
                    c.oppWin = WouldWin(i, j, -who);
                    c.order = ForkScore(i, j, who) + ForkScore(i, j, -who);   // điểm sơ bộ (chỉ để xếp thứ tự)
                    if (c.win) c.order += 100000000;
                    else if (c.oppWin) c.order += 50000000;
                    else if (i == hintI && j == hintJ) c.order += 10000000;
                }
            SortCands(out, n);
            return n;
        }

        // Các nước CỨU được: sau khi đi nước đó, đối thủ hết ô thắng.
        // Có 2 kiểu cứu: (1) chiếm ngay ô thắng, (2) chặn đầu còn lại để dãy 5 bị chặn 2 đầu
        int Defenders(Cand* out, const int* wi, const int* wj, int wn, int who)
        {
            bool mark[BOARD_SIZE][BOARD_SIZE] = {};
            int n = 0;
            for (int w = 0; w < wn; w++)
                for (int d = 0; d < 4; d++)
                    for (int s = -WIN_COUNT; s <= WIN_COUNT; s++)   // 4 đường qua ô thắng, cách <= 5 ô
                    {
                        int a = wi[w] + DI[d] * s, b = wj[w] + DJ[d] * s;
                        if (!InBoard(a, b) || _A[a][b].c != 0 || mark[a][b]) continue;
                        mark[a][b] = true;

                        _A[a][b].c = who;                           // thử đặt quân của mình
                        bool ok = true;
                        for (int x = 0; x < wn && ok; x++)          // các ô thắng cũ còn thắng được không?
                            if (_A[wi[x]][wj[x]].c == 0 && WouldWin(wi[x], wj[x], -who)) ok = false;
                        _A[a][b].c = 0;

                        if (ok)
                        {
                            out[n].i = a;
                            out[n].j = b;
                            out[n].win = out[n].oppWin = false;
                            out[n].order = ForkScore(a, b, who) + ForkScore(a, b, -who);
                            n++;
                        }
                    }
            SortCands(out, n);
            return n;
        }

        // ---------- Tìm kiếm Negamax + Alpha-Beta ----------
        int Search(int depth, int alpha, int beta, int who, int ply)
        {
            if ((++gNodes & 1023) == 0 && GetTickCount64() > gDeadline) gTimeUp = true;
            if (gTimeUp) return 0;
            if (depth <= 0) return (who == ME) ? gEval : -gEval;

            Cand cand[MAX_MOVES];
            int n = Gen(cand, who, ply == 0 ? gHintI : -1, ply == 0 ? gHintJ : -1);
            if (n == 0) return 0;                                   // hết ô -> hòa

            if (cand[0].win) return WIN_SCORE - ply - 1;            // đi 1 nước là thắng (đúng luật)

            // Đối thủ có ô thắng? -> BẮT BUỘC cứu
            int wi[16], wj[16], wn = 0;
            for (int k = 0; k < n && wn < 16; k++)
                if (cand[k].oppWin) { wi[wn] = cand[k].i; wj[wn] = cand[k].j; wn++; }

            int ext = 0;
            if (wn > 0)
            {
                n = Defenders(cand, wi, wj, wn, who);               // ghi đè danh sách bằng các nước cứu được
                if (n == 0) return -(WIN_SCORE - ply - 2);          // không cứu nổi -> thua
                ext = 1;                                            // nước bị ép không tính vào độ sâu
            }
            else if (n > MAX_CAND) n = MAX_CAND;

            int best = -INF_SCORE;
            for (int k = 0; k < n; k++)
            {
                int i = cand[k].i, j = cand[k].j;
                SearchPut(i, j, who);
                int s = -Search(depth - 1 + ext, -beta, -alpha, -who, ply + 1);
                SearchRemove(i, j);
                if (gTimeUp) return 0;

                if (s > best)
                {
                    best = s;
                    if (ply == 0) { gRootI = i; gRootJ = j; }
                }
                if (best > alpha) alpha = best;
                if (alpha >= beta) break;                           // CẮT TỈA
            }
            return best;
        }

        // ---------- Chọn nước đi ----------
        // sloppy = true: thỉnh thoảng đánh bừa (mức Dễ)
        void ChooseMove(int maxDepth, int ms, bool sloppy, int* bi, int* bj)
        {
            // Nước thứ 2 của ván: đánh ngẫu nhiên quanh quân đầu của người cho đỡ đơn điệu
            if (gCount == 1)
            {
                int ci[8], cj[8], cn = 0;
                for (int a = -1; a <= 1; a++)
                    for (int b = -1; b <= 1; b++)
                    {
                        int i = gHist[0].i + a, j = gHist[0].j + b;
                        if ((a != 0 || b != 0) && InBoard(i, j) && _A[i][j].c == 0)
                        {
                            ci[cn] = i; cj[cn] = j; cn++;
                        }
                    }
                if (cn > 0) { int r = rand() % cn; *bi = ci[r]; *bj = cj[r]; return; }
            }

            // Khởi tạo trạng thái tìm kiếm từ bàn cờ thật
            gEval = EvalBoard();
            memset(gNear, 0, sizeof(gNear));
            for (int i = 0; i < BOARD_SIZE; i++)
                for (int j = 0; j < BOARD_SIZE; j++)
                    if (_A[i][j].c != 0) AddNear(i, j, +1);

            Cand cand[MAX_MOVES];
            int n = Gen(cand, ME, -1, -1);
            if (n == 0)                                             // bàn trống -> đánh vào tâm
            {
                *bi = BOARD_SIZE / 2;
                *bj = BOARD_SIZE / 2;
                gLastDepth = 0;
                return;
            }
            *bi = cand[0].i;                                        // nước dự phòng
            *bj = cand[0].j;
            gLastDepth = 0;

            if (sloppy && rand() % 100 < 30)                        // mức Dễ: 30% đánh bừa
            {
                int r = rand() % n;
                *bi = cand[r].i;
                *bj = cand[r].j;
                return;
            }
            if (cand[0].win) return;                                // thắng ngay (đúng luật) thì đánh luôn

            // Đào sâu dần
            gDeadline = GetTickCount64() + (ULONGLONG)ms;
            gTimeUp = false;
            gNodes = 0;
            gHintI = gHintJ = -1;
            for (int depth = 1; depth <= maxDepth; depth++)
            {
                gRootI = gRootJ = -1;
                int s = Search(depth, -INF_SCORE, INF_SCORE, ME, 0);
                if (gTimeUp) break;                                 // dở dang -> giữ kết quả độ sâu trước
                if (gRootI >= 0)
                {
                    *bi = gRootI; *bj = gRootJ;
                    gHintI = gRootI; gHintJ = gRootJ;
                    gLastDepth = depth;
                }
                if (s > WIN_SCORE - 1000 || s < -WIN_SCORE + 1000) break;   // thấy thắng/thua chắc rồi
            }
        }

        // Hàm duy nhất bên ngoài cần gọi
        void Choose(int* bi, int* bj)
        {
            if (gDiff == 0)      ChooseMove(EASY_DEPTH, EASY_MS, true, bi, bj);
            else if (gDiff == 1) ChooseMove(NORMAL_DEPTH, NORMAL_MS, false, bi, bj);
            else                 ChooseMove(HARD_DEPTH, HARD_MS, false, bi, bj);
        }
    }   // hết namespace Block2