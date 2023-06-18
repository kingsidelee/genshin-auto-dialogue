#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
  template <class... TArgs>
  overload(TArgs... args) : TArgs(std::forward<TArgs>(args))... {}
};

template <class... Ts>
overload(Ts...) -> overload<std::decay_t<Ts>...>;

class Screenshot {
  public:
  Screenshot() {
    // get current display device's resolution
    DEVMODE dm{};
    dm.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm)) {
      width_ = dm.dmPelsWidth;
      height_ = dm.dmPelsHeight;
      std::cout << "Set screenshot resolution to: " << dm.dmPelsWidth << "x" << dm.dmPelsHeight << std::endl;
    } else {
      std::cout << "Can't retrive display settings, terminating. " << std::endl;
      std::exit(1);
    }

    // init device context
    hScreenDC_ = GetDC(nullptr);
    hMemoryDC_ = CreateCompatibleDC(hScreenDC_);
    // init ddb
    hBitmap_ = CreateCompatibleBitmap(hScreenDC_, width_, height_);
    GetObject(hBitmap_, sizeof(BITMAP), &bmp_);
    // fill bmp header
    bi_.biSize = sizeof(BITMAPINFOHEADER);
    bi_.biWidth = bmp_.bmWidth;
    bi_.biHeight = bmp_.bmHeight;
    bi_.biPlanes = 1;
    bi_.biBitCount = 32;
    bi_.biCompression = BI_RGB;
    bi_.biSizeImage = ((bmp_.bmWidth * bi_.biBitCount + 31) / 32) * 4 * bmp_.bmHeight;
  }

  // bmp data array size in byte
  int bmpsize() const { return bi_.biSizeImage; }

  // width and height getter
  int width() const { return width_; }
  int height() const { return height_; }

  // copy bmp pixel data to buffer
  void capture(std::unique_ptr<std::byte[]>& arr) {
    hOldBitmap_ = static_cast<HBITMAP>(SelectObject(hMemoryDC_, hBitmap_));
    BitBlt(hMemoryDC_, 0, 0, width_, height_, hScreenDC_, 0, 0, SRCCOPY);
    GetDIBits(hMemoryDC_, hBitmap_, 0, bmp_.bmHeight, arr.get(), reinterpret_cast<LPBITMAPINFO>(&bi_), DIB_RGB_COLORS);
    hBitmap_ = static_cast<HBITMAP>(SelectObject(hMemoryDC_, hOldBitmap_));
  }

  ~Screenshot() {
    DeleteDC(hMemoryDC_);
    ReleaseDC(nullptr, hScreenDC_);
    DeleteObject(hBitmap_);
  }

  private:
  int width_, height_;
  HDC hScreenDC_, hMemoryDC_;
  HBITMAP hBitmap_, hOldBitmap_;
  BITMAP bmp_;
  BITMAPINFOHEADER bi_;
};

class Skipper {
  public:
  Skipper() : state_(Waiting()) {
    buf_ = std::make_unique<std::byte[]>(s_.bmpsize());
    remap_coordinate();
  }

  void wait() {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto match = [&](const Waiting&) {
      std::cout << duration_cast<seconds>(system_clock::now().time_since_epoch()).count()
                << " Matching \U000025b6 or \U0001f4ac...\n";
      while (true) {
        std::this_thread::sleep_for(3s);
        s_.capture(buf_);
        if (match_tri() || match_dot()) {
          std::cout << duration_cast<seconds>(system_clock::now().time_since_epoch()).count() << " Matched.\n";
          state_ = FastForwarding();
          return;
        }
      }
    };
    auto stay = [](const auto&) {};

    std::visit(overload(match, stay), state_);
  }

  void fastforward() {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto match = [&](const FastForwarding&) {
      std::atomic_bool flag = true;

      auto keypress = [&flag] {
        INPUT ip{};
        // set up a generic keyboard event
        ip.type = INPUT_KEYBOARD;

        while (flag) {
          std::this_thread::sleep_for(500ms);

          // press the "f" key
          ip.ki.wVk = 0x46;   // virtual-key code for the "f" key
          ip.ki.dwFlags = 0;  // 0 for key press
          SendInput(1, &ip, sizeof(INPUT));

          // release the "f" key
          ip.ki.dwFlags = KEYEVENTF_KEYUP;  // KEYEVENTF_KEYUP for key release
          SendInput(1, &ip, sizeof(INPUT));
        }

        std::cout << duration_cast<seconds>(system_clock::now().time_since_epoch()).count()
                  << " \'F\' pressing thread stopped.\n";
        return;
      };

      auto mousepress = [pos = dialogueClickPos, x = s_.width(), y = s_.height()] {
        INPUT ip{};
        // set up a generic mouse event
        ip.type = INPUT_MOUSE;

        // move the cursor to dialogue icon position
        ip.mi.dx = (pos.first / static_cast<double>(x)) * 65535;
        ip.mi.dy = (pos.second / static_cast<double>(y)) * 65535;
        ip.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

        // press and release mouse left button
        SendInput(1, &ip, sizeof(INPUT));
        ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &ip, sizeof(INPUT));
        ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &ip, sizeof(INPUT));
      };

      std::thread kt(keypress);
      std::cout << duration_cast<seconds>(system_clock::now().time_since_epoch()).count()
                << " \'F\' pressing thread started.\n";
      std::cout << duration_cast<seconds>(system_clock::now().time_since_epoch()).count() << " Matching \U0001f4ac...\n";
      while (true) {
        std::this_thread::sleep_for(3s);
        s_.capture(buf_);
        auto dot = match_dot();
        auto tri = match_tri();
        if (dot && !tri) {
          std::cout << duration_cast<seconds>(system_clock::now().time_since_epoch()).count() << " \U0001f4ac Matched.\n";
          mousepress();
          std::cout << duration_cast<seconds>(system_clock::now().time_since_epoch()).count() << " Mouse click sent.\n";
          continue;
        } else if (!dot && tri)
          continue;
        else {
          flag = false;
          kt.join();
          state_ = Waiting();
          return;
        }
      }
    };
    auto stay = [](const auto&) {};

    std::visit(overload(match, stay), state_);
  }

  private:
  bool match_tri() const {
    auto pixels = reinterpret_cast<const unsigned*>(buf_.get());
    auto greyTriLeftTanPix = pixels[get_index(greyTriLeftTanPixPos)];    // 0xffece5d8
    auto greyTriPix = pixels[get_index(greyTriPixPos)];                  // 0xff3*4*5*
    auto greyTriRightTanPix = pixels[get_index(greyTriRightTanPixPos)];  // 0xffece5d8
    if (greyTriLeftTanPix == 0xffece5d8 && greyTriRightTanPix == 0xffece5d8 && greyTriPix >= 0xff304050 && greyTriPix <= 0xff3f4f5f)
      return true;
    else
      return false;
  }

  bool match_dot() const {
    auto pixels = reinterpret_cast<const unsigned*>(buf_.get());
    auto greyDotLeftWhitePix = pixels[get_index(greyDotLeftWhitePixPos)];    // 0xffffffff
    auto greyDotPix = pixels[get_index(greyDotPixPos)];                      // 0xff6*6*7*
    auto greyDotRightWhitePix = pixels[get_index(greyDotRightWhitePixPos)];  // 0xffffffff
    if (greyDotLeftWhitePix == 0xffffffff && greyDotRightWhitePix == 0xffffffff && greyDotPix >= 0xff606070 &&
        greyDotPix <= 0xff6f6f7f)
      return true;
    else
      return false;
  }

  // coordinate (0, 0) corresponding to upper-left corner pixel
  using Coord = std::pair<unsigned, unsigned>;
  Coord greyTriLeftTanPixPos;
  Coord greyTriPixPos;
  Coord greyTriRightTanPixPos;
  Coord greyDotLeftWhitePixPos;
  Coord greyDotPixPos;
  Coord greyDotRightWhitePixPos;
  Coord dialogueClickPos;

  void remap_coordinate() {
    if (s_.width() == 3840 && s_.height() == 2160) {
      greyTriLeftTanPixPos = {130, 95};
      greyTriPixPos = {145, 95};
      greyTriRightTanPixPos = {160, 95};
      greyDotLeftWhitePixPos = {2590, 1605};
      greyDotPixPos = {2598, 1605};
      greyDotRightWhitePixPos = {2606, 1605};
      dialogueClickPos = {2645, 1605};
    } else if (s_.width() == 2560 && s_.height() == 1440) {
      greyTriLeftTanPixPos = {86, 63};
      greyTriPixPos = {96, 63};
      greyTriRightTanPixPos = {107, 63};
      greyDotLeftWhitePixPos = {1727, 1070};
      greyDotPixPos = {1732, 1070};
      greyDotRightWhitePixPos = {1737, 1070};
      dialogueClickPos = {1764, 1070};
    } else if (s_.width() == 1920 && s_.height() == 1080) {
      greyTriLeftTanPixPos = {64, 47};
      greyTriPixPos = {72, 47};
      greyTriRightTanPixPos = {80, 47};
      greyDotLeftWhitePixPos = {1295, 802};
      greyDotPixPos = {1298, 802};
      greyDotRightWhitePixPos = {1303, 802};
      dialogueClickPos = {1323, 802};
    } else {
      std::cout << "Unsupported resolution, terminating. " << std::endl;
      std::exit(1);
    }
    std::cout << "Mapped coordinates to resolution " << s_.width() << "x" << s_.height() << std::endl;
  }
  unsigned get_index(Coord coord) const { return (s_.height() - coord.second - 1) * s_.width() + coord.first; }

  // states
  struct Waiting {};
  struct FastForwarding {};

  using State = std::variant<Waiting, FastForwarding>;

  State state_;
  Screenshot s_;
  std::unique_ptr<std::byte[]> buf_;
};

int main() {
  Skipper s;
  while (true) {
    s.wait();
    s.fastforward();
  }
  return 0;
}