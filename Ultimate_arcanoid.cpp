// Настройки линкера:
// Подсистема: Windows (/SUBSYSTEM:WINDOWS)
// Дополнительные зависимости: Msimg32.lib; Winmm.lib

#include <windows.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib> // для rand()

// Базовый класс Sprite — всё, что умеет двигаться и рисоваться

class Sprite
{
protected: //Ключевое слово Протектед - модификатор доступа только из наследуемых классов
    float x, y;          // Позиция
    float width, height; // Размеры
    float dx, dy;        // Направление движения
    float speed;         // Скорость
    HBITMAP hBitmap;     // Битмап для отрисовки

public://Ключевое слово Паблик - модификатор доступа из любой части программы 
        //Конструктор класса - специальная функция для создания объекта, чтобы инициализировать члены класса
    Sprite(float x = 0, float y = 0, float w = 0, float h = 0)
        : x(x), y(y), width(w), height(h),
        dx(0), dy(0), speed(0), hBitmap(nullptr) {
    }
    //virtual нужен, чтобы если объект наследника (например Ball) удаляется через указатель на Sprite, 
    // вызывался деструктор наследника, а не только базового класса.
    virtual ~Sprite() {}

    // Обновление позиции
    virtual void Move() 
    {
        x += dx * speed;
        y += dy * speed;
    }

    // Отрисовка
    virtual void Draw(HDC hdc) {
        if (hBitmap) {
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP old = (HBITMAP)SelectObject(memDC, hBitmap);

            BITMAP bm;
            GetObject(hBitmap, sizeof(BITMAP), &bm);
            StretchBlt(hdc, (int)x, (int)y, (int)width, (int)height,
                memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            SelectObject(memDC, old);
            DeleteDC(memDC);
        }
        else {
            Rectangle(hdc, (int)x, (int)y, (int)(x + width), (int)(y + height));
        }
    }

    // --- Геттеры/сеттеры ---
    void SetBitmap(HBITMAP bmp) { hBitmap = bmp; }
    void SetPosition(float nx, float ny) { x = nx; y = ny; }
    void SetSize(float w, float h) { width = w; height = h; }
    void SetSpeed(float s) { speed = s; }
    void SetDirection(float ndx, float ndy) { dx = ndx; dy = ndy; }

    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetW() const { return width; }
    float GetH() const { return height; }
    float GetDX() const { return dx; }
    float GetDY() const { return dy; }
};

// Мяч (Ball) — наследник Sprite, добавляет радиус и гравитацию

class Ball : public Sprite
{
    float radius;
public:
    Ball(float x = 0, float y = 0, float r = 10)
        : Sprite(x, y, r * 2, r * 2), radius(r) {
    }

    float GetRadius() const { return radius; }
    float GetSpeed()  const { return speed; }
    void SetRadius(float r) { radius = r; width = r * 2; height = r * 2; }
};

// Платформа игрока

class PlayerPlatform : public Sprite
{
    float baseSpeed;  // постоянная базовая скорость

public:
    PlayerPlatform(float x = 0, float y = 0, float w = 100, float h = 20)
        : Sprite(x, y, w, h) {
    }


    void MoveLeft() { x -= speed; }
    void MoveRight() { x += speed; }

    // Метод обновления скорости
    void MoveShift(bool shiftPressed)
    {
        speed = shiftPressed ? 50.0f : 20.0f; // 
    }
};

// Кирпич (Block) — не наследуется, он статический объект

class Block {
public:
    int x, y, w, h;
    COLORREF color;
    bool active;

    Block(int x = 0, int y = 0, int w = 40, int h = 20, COLORREF c = RGB(0, 255, 0))
        : x(x), y(y), w(w), h(h), color(c), active(true) {
    }

    void Draw(HDC hdc) {
        if (!active) return;
        HBRUSH brush = CreateSolidBrush(color);
        HBRUSH old = (HBRUSH)SelectObject(hdc, brush);
        Rectangle(hdc, x, y, x + w, y + h);
        SelectObject(hdc, old);
        DeleteObject(brush);
    }
};

// Игровое окно (для HDC и размеров)

struct GameWindow {
    HWND hWnd;
    HDC dc;
    HDC buffer;
    int width, height;
    HBITMAP back;

    GameWindow() : hWnd(nullptr), dc(nullptr), buffer(nullptr),
        width(800), height(600), back(nullptr) {
    }
};

// Глобальные объекты игры

GameWindow window;
PlayerPlatform player(0, 0, 100, 20);
Ball ball(0, 0, 10);
std::vector<Block> blocks;
const int BLOCK_ROWS = 5;
const int BLOCK_COLS = 10;
HBITMAP hBack;

// Инициализация игры

void InitGame() {
    // Загрузка картинок
    HBITMAP playerBmp = (HBITMAP)LoadImageA(NULL, "player_platform.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    HBITMAP ballBmp = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "forest_bg.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    
    // Проверяем загрузку и устанавливаем битмапы
    if (playerBmp) {
        player.SetBitmap(playerBmp);
    }
    if (ballBmp) {
        ball.SetBitmap(ballBmp);
    }

    // Платформа
    player.SetSize(300.0f, 100.0f);
    player.SetSpeed(15.0f);
    player.SetPosition(window.width / 2.0f, window.height - 120.0f);

    // Мяч
    ball.SetRadius(25.0f);
    ball.SetSpeed(20.0f);
    ball.SetDirection(-1.0f, 1.0f);
    ball.SetPosition(window.width / 2.0f, window.height / 2.0f);

    // Блоки
    int blockW = window.width / BLOCK_COLS;
    int blockH = 30;

    for (int r = 0; r < BLOCK_ROWS; r++) {
        for (int c = 0; c < BLOCK_COLS; c++) {
            COLORREF col = RGB(50 * r, 20 * c, 100);
            blocks.emplace_back(c * blockW, r * blockH + 50,
                blockW - 4, blockH - 4, col);
        }
    }
}

// Функция проверки столкновения мяча с платформой
// Движение мяча с отражениями
void BallMove(Ball& ball) {
    ball.Move();

    // Отражение от стен
    if (ball.GetX() <= 0) ball.SetDirection(abs(ball.GetDX()), ball.GetDY());
    if (ball.GetX() + ball.GetW() >= window.width) ball.SetDirection(-abs(ball.GetDX()), ball.GetDY());
    if (ball.GetY() <= 0) ball.SetDirection(ball.GetDX(), abs(ball.GetDY()));

    // Отражение от платформы
    float px = player.GetX(), py = player.GetY(), pw = player.GetW(), ph = player.GetH();
    float bx = ball.GetX(), by = ball.GetY(), bw = ball.GetW(), bh = ball.GetH();
    if (bx + bw >= px && bx <= px + pw && by + bh >= py && by <= py + ph) {
        ball.SetPosition(bx, py - bh);
        ball.SetDirection(ball.GetDX(), -abs(ball.GetDY()));
    }

    // Отражение от нижнего края
    if (ball.GetY() + ball.GetH() >= window.height) {
        // Reset ball
        ball.SetPosition(window.width / 2.0f, window.height / 2.0f);
        ball.SetDirection(0.0f, 0.0f);
    }
}

void CheckBallPlatformCollision(Ball& ball, PlayerPlatform& platform)
{
    float px = platform.GetX();
    float py = platform.GetY();
    float pw = platform.GetW();
    float ph = platform.GetH();

    float bx = ball.GetX();
    float by = ball.GetY();
    float bw = ball.GetW();
    float bh = ball.GetH();

    if (bx + bw >= px && bx <= px + pw &&
        by + bh >= py && by <= py + ph)
    {
        // ставим мяч прямо над платформой
        ball.SetPosition(bx, py - bh);

        // меняем направление по вертикали
        ball.SetDirection(ball.GetDX(), -abs(ball.GetDY()));
    }
}

// Ограничение платформы

void LimitPlatform() {
    if (player.GetX() < 0) player.SetPosition(0, player.GetY());
    if (player.GetX() + player.GetW() > window.width)
        player.SetPosition(window.width - player.GetW(), player.GetY());
}

// Инициализация окна

void InitWindow() {
    SetProcessDPIAware();
    window.hWnd = CreateWindowA("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE,
        0, 0, 0, 0, 0, 0, 0, 0);


    RECT r;
    GetClientRect(window.hWnd, &r);
    window.dc = GetDC(window.hWnd);
    window.width = r.right - r.left;
    window.height = r.bottom - r.top;
    window.buffer = CreateCompatibleDC(window.dc);
    SelectObject(window.buffer, CreateCompatibleBitmap(window.dc, window.width, window.height));
}

// Точка входа (без неё не работает)

int main() {
    return wWinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    InitWindow();
    InitGame();
    ShowCursor(FALSE);

    while (!GetAsyncKeyState(VK_ESCAPE)) {
        // Очистка экрана
        PatBlt(window.buffer, 0, 0, window.width, window.height, BLACKNESS);

        // Рисуем фон
        if (hBack) {
            HDC memDC = CreateCompatibleDC(window.buffer);
            HBITMAP old = (HBITMAP)SelectObject(memDC, hBack);
            BITMAP bm; GetObject(hBack, sizeof(BITMAP), &bm);
            StretchBlt(window.buffer, 0, 0, window.width, window.height,
                memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            SelectObject(memDC, old);
            DeleteDC(memDC);
        }

        // Рисуем блоки
        for (auto& b : blocks) b.Draw(window.buffer);

        // Рисуем платформу и мяч
        player.Draw(window.buffer);
        ball.Draw(window.buffer);

        // Выводим на экран
        BitBlt(window.dc, 0, 0, window.width, window.height, window.buffer, 0, 0, SRCCOPY);

        // Сначала двигаем платформу
        bool shift = GetAsyncKeyState(VK_LSHIFT) & 0x8000;
        player.MoveShift(shift);
        if (GetAsyncKeyState('A') & 0x8000) player.MoveLeft();
        if (GetAsyncKeyState('D') & 0x8000) player.MoveRight();

        // Обновляем платформу
        /*player.Update();*/
        LimitPlatform();

        // Двигаем мяч
        BallMove(ball);

        // Проверяем столкновение с платформой
        CheckBallPlatformCollision(ball, player);

        Sleep(16); // ~60 FPS
    }
    return 0;
}
