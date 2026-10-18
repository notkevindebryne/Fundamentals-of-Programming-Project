#include "config.h"

bool _turn = true;
int _winner = 2;
_point _a[boardsize][boardsize];

bool _sound = true;
bool _block2ends = true;

vector<CellPos> winCells;

void resetdata()
{
    for (int i = 0; i < boardsize; i++)
    {
        for (int j = 0; j < boardsize; j++)
        {
            _a[i][j].x = j;
            _a[i][j].y = i;
            _a[i][j].c = 0;
        }
    }
    _turn = true;
    _winner = 2;
    winCells.clear();
}

int testboard()
{
    winCells.clear();
    int d_c[4] = {1, 0, 1, -1};
    int d_r[4] = {0, 1, 1, 1};
    int checkedcell = 0;

    for (int i = 0; i < boardsize; i++)
    {
        for (int j = 0; j < boardsize; j++)
        {
            int player = _a[i][j].c;
            if (player == 0)
                continue;
            checkedcell++;

            for (int k = 0; k < 4; k++)
            {
                int wincondition = 1;
                for (int h = 1; h <= 4; h++)
                {
                    int r = i + h * d_r[k];
                    int c = j + h * d_c[k];
                    if (r >= 0 && r < boardsize && c >= 0 && c < boardsize && _a[r][c].c == player)
                        wincondition++;
                    else
                        break;
                }

                if (wincondition == 5)
                {
                    if (_block2ends)
                    {
                        int head_r = i - d_r[k];
                        int head_c = j - d_c[k];
                        int tail_r = i + 5 * d_r[k];
                        int tail_c = j + 5 * d_c[k];
                        bool headBlocked = (head_r >= 0 && head_r < boardsize && head_c >= 0 && head_c < boardsize && _a[head_r][head_c].c == -player);
                        bool tailBlocked = (tail_r >= 0 && tail_r < boardsize && tail_c >= 0 && tail_c < boardsize && _a[tail_r][tail_c].c == -player);
                        if (headBlocked && tailBlocked)
                            continue;
                    }

                    for (int h = 0; h < 5; h++)
                    {
                        winCells.push_back({i + h * d_r[k], j + h * d_c[k]});
                    }
                    return player;
                }
            }
        }
    }

    if (checkedcell == boardsize * boardsize)
        return 0; // Hòa
    return 2;     // Tiếp tục
}

int checkboard(int row, int col)
{
    if (row >= 0 && row < boardsize && col >= 0 && col < boardsize)
    {
        if (_a[row][col].c == 0)
        {
            _a[row][col].c = _turn ? 1 : -1;
            _turn = !_turn;
            return _a[row][col].c;
        }
    }
    return 0;
}

bool saveGameData(const string &filename)
{
    ofstream outFile(filename.c_str());
    if (!outFile.is_open())
        return false;
    outFile << _turn << "\n";
    for (int i = 0; i < boardsize; i++)
    {
        for (int j = 0; j < boardsize; j++)
        {
            outFile << _a[i][j].c << " ";
        }
        outFile << "\n";
    }
    outFile.close();
    return true;
}

bool loadGameData(const string &filename)
{
    ifstream inFile(filename.c_str());
    if (!inFile.is_open())
        return false;
    inFile >> _turn;
    for (int i = 0; i < boardsize; i++)
    {
        for (int j = 0; j < boardsize; j++)
        {
            inFile >> _a[i][j].c;
        }
    }
    inFile.close();
    winCells.clear();
    _winner = testboard();
    return true;
} // Kết thúc hàm loadGameData

// ===== Trạng thái Music Player (Định nghĩa biến toàn cục) =====
int currentSong = 0;
int playState = 0; // 0 = STOP, 1 = PLAY, 2 = PAUSE
bool playerOpen = false;
bool draggingThumb = false;
bool justOpened = false;