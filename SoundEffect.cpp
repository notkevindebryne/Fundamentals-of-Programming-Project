#include <windows.h>
#include <iostream>
#include<vector>
#include<string>
#include <mmsystem.h>// MultiMedia System
#pragma comment(lib, "winmm.lib") // Tìm file thư viện winmm.lib (Windows Multimedia Library) của hệ điều hành và gắn nó vào chương trình này

using namespace std;

class WindowsAudio {
private:// hàm trong cùng class thay đổi được nhưng hàm hàm main k thay đổi được

    vector <string> backgroundmusicList;
    int selectedSoundIndex;
    bool isMuted;

public:// hàm main thay đổi được

    WindowsAudio() {// hàm khởi tạo
        backgroundmusicList = { "sound1.wav", "sound2.wav", "sound3.wav", "sound4.wav" };// chuẩn bị 4 background music
        selectedSoundIndex = 0;// chọn bài hát đầu tiên 
        isMuted = false; // false: đang bật nhạc, true: đang tắt nhạc
    }

    void selectSound(int index) {// hàm chọn background music
        if (index >= 0 && index < backgroundmusicList.size())
        {// bài hát chọn phải được đánh số trong vecotr kp là số âm và thuộc vector backgroundmusicList
            selectedSoundIndex = index;
        }
    }

    void toggleSound() {// hàm bật/ tắt âm thanh
        isMuted = !isMuted;
        if (isMuted) {// nếu bị tắt tiếng
            PlaySoundA(NULL, 0, 0);
            // BOOL PlaySoundA(pszSound,hmod, DWORD fdwSound);
            // pszSound: tên file nhạc, vd:"nhac.wav"
            // hmod: Dùng khi bạn muốn nhúng thẳng file âm thanh vào bên trong file .exe cho người khác khỏi copy. Vì chúng ta chỉ để file .wav ở ngoài thư mục bình thường, nên chỗ này chúng ta luôn điền NULL hoặc số 0 (bỏ qua).
            // fdwSound: quy định cách phát.Ví dụ: SND_FILENAME (báo đây là tên file), SND_ASYNC (phát nền không đơ máy), SND_LOOP (phát lặp đi lặp lại).Nếu bạn không cần quy định gì cả (như khi ra lệnh tắt nhạc), bạn cứ điền số 0.
        }
    }
    void play() {// phát âm thanh
        if (!isMuted) {// nếu không bị tắt tiếng
            string currentFile = backgroundmusicList[selectedSoundIndex];
            PlaySoundA(currentFile.c_str(), NULL, SND_FILENAME | SND_ASYNC);
        }
    }
};
int main() {
    // 1. TẠO ĐỐI TƯỢNG (Chế tạo máy phát nhạc)
    WindowsAudio audio;
    // Ngay tại dòng này, Constructor đã chạy. Nó nạp 4 bài nhạc vào danh sách, 
    // chọn bài số 0 và để chế độ Bật tiếng (isMuted = false).

    // 2. PHÁT BÀI HÁT MẶC ĐỊNH
    cout << "--- Dang phat bai so 0 ---" << endl;
    audio.play();
    Sleep(3000); // Tạm dừng màn hình 3000ms (3 giây) để nghe thử tiếng nhạc.
    // Nhờ có cờ SND_ASYNC, nhạc hát 3 giây, trong lúc đó lệnh Sleep đếm giờ.

// 3. ĐỔI BÀI HÁT KHÁC
    cout << "\n--- Chuyen sang bai so 2 ---" << endl;
    audio.selectSound(2); // Chọn bài "sound3.wav" (index 2)
    audio.play();         // Ra lệnh hát bài mới
    Sleep(3000);          // Nghe thử 3 giây

    // 4. KIỂM TRA TÍNH NĂNG MUTE
    cout << "\n--- BAM NUT TAT TIENG (MUTE) ---" << endl;
    audio.toggleSound();  // Gọi hàm lật công tắc. Lệnh PlaySoundA(NULL,0,0) sẽ chém đứt bài nhạc.
    Sleep(2000);          // Im lặng hoàn toàn trong 2 giây.

    // 5. CỐ TÌNH PHÁT NHẠC KHI ĐANG MUTE ĐỂ TEST LỖI
    cout << "\n--- Thu phat nhac khi dang Mute ---" << endl;
    audio.play();         // Gọi hàm play()
    // Nhưng lệnh if (!isMuted) bên trong hàm đã chặn đứng nó lại. Không có tiếng!
    Sleep(2000);

    // 6. BẬT TIẾNG LẠI (UNMUTE) VÀ PHÁT LẠI
    cout << "\n--- BAM NUT BAT TIENG LAI (UNMUTE) ---" << endl;
    audio.toggleSound();  // Lật công tắc về false
    audio.play();         // Phải tự gọi hàm play() thì nhạc mới lên tiếng lại

    // 7. KẾT THÚC
    cout << "\nNhan phim bat ky de thoat..." << endl;
    system("pause"); // Dừng cửa sổ Console lại, không cho nó tự đóng cái phụp.
    return 0;
}