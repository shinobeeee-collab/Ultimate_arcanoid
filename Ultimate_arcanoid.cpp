// Настройки линкера:
// Подсистема: Windows (/SUBSYSTEM:WINDOWS)
// Дополнительные зависимости: Msimg32.lib; Winmm.lib
#pragma comment(lib, "Msimg32.lib")

#include <windows.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib> // для rand()
#include <ctime>   // time
#include <wingdi.h> // для TransparentBlt

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
    void Draw(HDC hdc) 
    {
        if (hBitmap) {
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP old = (HBITMAP)SelectObject(memDC, hBitmap);

            BITMAP bm;
            GetObject(hBitmap, sizeof(BITMAP), &bm);

            // Рисуем с прозрачностью (белый = прозрачный)
            TransparentBlt(
                hdc, (int)x, (int)y, (int)width, (int)height, // куда
                memDC, 0, 0, bm.bmWidth, bm.bmHeight,         // откуда
                RGB(255, 255, 255)                            // цвет прозрачности
            );

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
    bool active;
    float radius;
public:
    Ball(float x = 0, float y = 0, float r = 10)
        : Sprite(x, y, r * 2, r * 2), radius(r), active(true)
    {
    }

    float GetRadius() const { return radius; }
    float GetSpeed() const { return speed; }
    void StopSpeed()
    {
        if (GetAsyncKeyState('S') & 0x8000)
            speed = 0;
        else
            speed = 10;
    }
    void SetRadius(float r) { radius = r; width = r * 2; height = r * 2; }

    // Переопределяем Draw, чтобы рисовать круг
    void Draw(HDC hdc) {
        Ellipse(hdc,
            (int)(x - radius),
            (int)(y - radius),
            (int)(x + radius),
            (int)(y + radius));
    }
    float Clamp(float value, float minVal, float maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }
    // Функция для столкновения с прямоугольником
    bool CheckCollisionRect(float rx, float ry, float rw, float rh) {
        // ближайшая точка прямоугольника к центру круга
        float closestX = Clamp(x, rx, rx + rw);
        float closestY = Clamp(y, ry, ry + rh);

        float dx = x - closestX;
        float dy = y - closestY;

        return (dx * dx + dy * dy) <= radius * radius;
    }
};


// Платформа игрока

class PlayerPlatform : public Sprite
{

public:
    PlayerPlatform(float x = 0, float y = 0, float w = 100, float h = 20)
        : Sprite(x, y, w, h) {
    }


    void MoveLeft() { x -= speed; }
    void MoveRight() { x += speed; }

    // Метод обновления скорости
    void MoveShift(bool shiftPressed)
    {
        if (shiftPressed)
        {
            // скорость от 30 до 60 это прикол такой
            speed = 30.0f + static_cast<float>(std::rand() % 10);
        }
        else
        {
            speed = 20.0f;
        }
    }
};

// Кирпич (Block) — не наследуется, он статический объект

// Исправлено:
class Block : public Sprite {
public:
    bool active;

    Block(int x = 0, int y = 0, int w = 40, int h = 20, COLORREF color = RGB(255, 0, 0))
        : Sprite(x, y, w, h), active(true) {
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
PlayerPlatform player(0, 0, 0, 0);
Ball ball(0, 0, 0);
std::vector<Block> blocks; // Массив блоков вместо одного
HBITMAP hBack;



// Инициализация игры

void InitGame() 
{
    // Загрузка картинок
    HBITMAP playerBmp = (HBITMAP)LoadImageA(NULL, "player_platform.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    HBITMAP ballBmp = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    HBITMAP blockBmp = (HBITMAP)LoadImageA(NULL, "block.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "forest_bg.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    
    // Проверяем загрузку и устанавливаем битмапы
    if (playerBmp) 
    {
        player.SetBitmap(playerBmp);
    }
    if (ballBmp) 
    {
        ball.SetBitmap(ballBmp);
    }
    // Битмап для блоков будет установлен при создании каждого блока

    // Платформа
    player.SetSize(300.0f, 100.0f);
    player.SetSpeed(15.0f);
    player.SetPosition(window.width / 2.0f, window.height - 120.0f);

    // Мяч
    ball.SetRadius(25.0f);
    ball.SetSpeed(20.0f);
    ball.SetDirection(-1.0f, 1.0f);
    ball.SetPosition(window.width / 2.0f, window.height / 2.0f);
    ball.StopSpeed();

    // Создаем массив блоков в виде сетки
    blocks.clear();
    int blockWidth = 80;
    int blockHeight = 40;
    int blocksPerRow = 8;
    int rows = 4;
    int startX = (window.width - (blocksPerRow * blockWidth + (blocksPerRow - 1) * 10)) / 2; // центрируем
    int startY = 100;
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < blocksPerRow; col++) {
            Block newBlock;
            newBlock.SetSize(blockWidth, blockHeight);
            newBlock.SetPosition(startX + col * (blockWidth + 10), startY + row * (blockHeight + 10));
            newBlock.active = true;
            
            // Устанавливаем битмап для блока, если он загружен
            if (blockBmp) {
                newBlock.SetBitmap(blockBmp);
            }
            
            blocks.push_back(newBlock);
        }
    }

   
}
// Функция проверки столкновения мяча с платформой
// Движение мяча с отражениями
void BallReset(Ball& ball)
{
    if (GetAsyncKeyState('R') & 0x8000)
    {
        ball.SetPosition(window.width / 2.0f, window.height / 2.0f);
    }
}

void BallMove(Ball& ball) {
    ball.Move();

    float r = ball.GetRadius();

    // Отражение от стен
    if (ball.GetX() - r <= 0) { ball.SetDirection(-ball.GetDX(), ball.GetDY()); ball.SetPosition(r, ball.GetY()); }
    if (ball.GetX() + r >= window.width) { ball.SetDirection(-ball.GetDX(), ball.GetDY()); ball.SetPosition(window.width - r, ball.GetY()); }
    if (ball.GetY() - r <= 0) { ball.SetDirection(ball.GetDX(), -ball.GetDY()); ball.SetPosition(ball.GetX(), r); }

    // Отражение от платформы
    float px = player.GetX(), py = player.GetY(), pw = player.GetW(), ph = player.GetH();
    if (ball.CheckCollisionRect(px, py, pw, ph)) {
        // устанавливаем мяч над платформой
        ball.SetPosition(ball.GetX(), py - r);
        // угол отскока по X
        float hitPos = (ball.GetX() - px) / pw; // 0-1
        float newDX = (hitPos - 0.5f) * 2.0f;
        float newDY = -fabs(ball.GetDY());
        float len = sqrt(newDX * newDX + newDY * newDY);
        ball.SetDirection(newDX / len, newDY / len);
    }

    // Столкновение с блоками
    for (auto& block : blocks) {
        if (!block.active) continue;
        if (ball.CheckCollisionRect(block.GetX(), block.GetY(), block.GetW(), block.GetH())) {
            block.active = false;
            // простое отражение по Y
            ball.SetDirection(ball.GetDX(), -ball.GetDY());
            break;
        }
    }

    // Мяч упал вниз
    if (ball.GetY() - r >= window.height) {
        ball.SetPosition(window.width / 2.0f, window.height / 2.0f);
        float randomDX = (rand() % 100) / 100.0f;
        if (rand() % 2 == 0) randomDX = -randomDX;
        ball.SetDirection(randomDX, 1.0f);
    }
}

void CheckBallBlocksCollision(Ball& ball, std::vector<Block>& blocks)
{
    float bax = ball.GetX();
    float bay = ball.GetY();
    float baw = ball.GetW();
    float bah = ball.GetH();

    for (auto& block : blocks) {
        if (!block.active) continue; // пропускаем неактивные блоки
        
        float blx = block.GetX();
        float bly = block.GetY();
        float blw = block.GetW();
        float blh = block.GetH();

        if (bax + baw >= blx && bax <= blx + blw &&
            bay + bah >= bly && bay <= bly + blh)
        {
            // Определяем, с какой стороны произошло столкновение
            float ballCenterX = bax + baw / 2;
            float ballCenterY = bay + bah / 2;
            float blockCenterX = blx + blw / 2;
            float blockCenterY = bly + blh / 2;
            
            float deltaX = ballCenterX - blockCenterX;
            float deltaY = ballCenterY - blockCenterY;
            
            // Если столкновение больше по горизонтали - отскок по вертикали
            if (abs(deltaX) > abs(deltaY)) {
                ball.SetDirection(-ball.GetDX(), ball.GetDY()); // меняем X
            } else {
                ball.SetDirection(ball.GetDX(), -ball.GetDY()); // меняем Y
            }
            
            // Уничтожаем блок
            block.active = false;
            break; // выходим из цикла после первого столкновения
        }
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

// Точка входа

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
        // Рисуем платформу и мяч
        player.Draw(window.buffer);
        ball.Draw(window.buffer);
        
        // Рисуем все активные блоки
        for (auto& block : blocks) 
        {
            if (block.active) {
                block.Draw(window.buffer);
            }
        }
       
        
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
        BallReset(ball);
        ball.StopSpeed();
        // Проверяем столкновение с платформой
        CheckBallPlatformCollision(ball, player);
        CheckBallBlocksCollision(ball, blocks);
        Sleep(3); // ~60 FPS
    }
    return 0;
}
