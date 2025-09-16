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

// -----------------------------
// Константы игры и управления
// -----------------------------

namespace GameConfig
{
    //constexpr (от constant expression) — это ключевое слово в C++, которое указывает, 
    // что значение функции или переменной может быть вычислено на этапе компиляции.
    // 
    // Настройки камеры/зум-режима
    constexpr float ZoomScale = 3.0f;     // во сколько раз увеличиваем при удержании W

    // Скорости мяча под горячими клавишами
    constexpr float BallSpeedSlow = 1.0f;  // при удержании S
    constexpr float BallSpeedFast = 90.0f; // при удержании Q
    constexpr float BallSpeedNormal = 20.0f; // по умолчанию

    // Параметры мяча
    constexpr float BallRadius = 25.0f;
    constexpr float BallInitialSpeed = 20.0f;

    constexpr float balltraceRadius = 15.0f;


    // Размеры и скорость платформы
    constexpr float PlatformWidth = 300.0f;
    constexpr float PlatformHeight = 100.0f;
    constexpr float PlatformSpeedNormal = 20.0f;
    constexpr float PlatformSpeedFast = 40.0f; // при удержании Shift

    // Сетка блоков
    constexpr int BlockWidth = 80;
    constexpr int BlockHeight = 40;
    constexpr int BlocksPerRow = 1;
    constexpr int BlockRows = 1;
    constexpr int BlockGap = 1;
    constexpr int BlocksStartY = 100;
}

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
        dx(0), dy(0), speed(0), hBitmap(nullptr)
    {
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
    // Отрисовка с учётом смещения и масштаба вида
    void DrawView(HDC hdc, float viewX, float viewY, float viewScale)
    {
        if (hBitmap)
        {
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP old = (HBITMAP)SelectObject(memDC, hBitmap);

            BITMAP bm;
            GetObject(hBitmap, sizeof(BITMAP), &bm);

            int dstX = (int)((x - viewX) * viewScale);
            int dstY = (int)((y - viewY) * viewScale);
            int dstW = (int)(width * viewScale);
            int dstH = (int)(height * viewScale);

            TransparentBlt(
                hdc, dstX, dstY, dstW, dstH,
                memDC, 0, 0, bm.bmWidth, bm.bmHeight,
                RGB(255, 255, 255)
            );

            SelectObject(memDC, old);
            DeleteDC(memDC);
        }
        else {
            int dstX = (int)((x - viewX) * viewScale);
            int dstY = (int)((y - viewY) * viewScale);
            int dstW = (int)(width * viewScale);
            int dstH = (int)(height * viewScale);
            Rectangle(hdc, dstX, dstY, dstX + dstW, dstY + dstH);
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


    // Управление скоростью мяча горячими клавишами (S/Q/по умолчанию)
    void SlowBall()
    {
        if (GetAsyncKeyState('S') & 0x8000)
            speed = GameConfig::BallSpeedSlow;
        //«Присвоить переменной speed значение BallSpeedSlow из пространства имён GameConfig»
        else if (GetAsyncKeyState('Q') & 0x8000)
            speed = GameConfig::BallSpeedFast;
        else
            speed = GameConfig::BallSpeedNormal;
    }
    void SetRadius(float r) { radius = r; width = r * 2; height = r * 2; }

    // Переопределяем Draw, чтобы рисовать игровой шарик
    void Draw(HDC hdc)
    {
        Ellipse(hdc,
            (int)(x - radius),
            (int)(y - radius),
            (int)(x + radius),
            (int)(y + radius));
    }
    // Отрисовка шара с учётом вида (смещения и масштаба)
    void DrawView(HDC hdc, float viewX, float viewY, float viewScale)
    {
        float left = x - radius;
        float top = y - radius;
        float size = radius * 2.0f;
        int l = (int)((left - viewX) * viewScale);
        int t = (int)((top - viewY) * viewScale);
        int r = l + (int)(size * viewScale);
        int b = t + (int)(size * viewScale);
        Ellipse(hdc, l, t, r, b);
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
    // Переключение скорости платформы: обычная или ускоренная при Shift
    void MoveShift(bool shiftPressed)
    {
        speed = shiftPressed ? GameConfig::PlatformSpeedFast : GameConfig::PlatformSpeedNormal;
    }
};

// Кирпич (Block) — не наследуется, он статический объект

class Block : public Sprite
{
public:
    bool active;

    Block(int x = 0, int y = 0, int w = 40, int h = 20, COLORREF color = RGB(255, 0, 0))
        : Sprite(x, y, w, h), active(true) {
    }

};

//class TracePoint : public Sprite
//{
//    float x, y;
//    /*void DrawView(HDC hdc, float viewX, float viewY, float viewScale) //Трассировка
//    {
//        GetDC()->SetPixel(x, y, RGB(1, 2, 3))
//    }*/
//    //int radius = 5;
//    //TracePoint(float x = 0, float y = 0, float r = 10)
//    //    : Sprite(x, y, r * 2, r * 2), radius(r)
//    //{
//    //}
//    void Draw(HDC hdc)
//    {
//        SetPixel(hdc, x, y, RGB(1, 1, 1));
//    }
//};


// Игровое окно (для HDC и размеров)

struct GameWindow
{
    HWND hWnd;
    HDC dc;
    HDC buffer;
    int width, height;
    HBITMAP back;

    GameWindow() : hWnd(nullptr), dc(nullptr), buffer(nullptr),
        width(800), height(600), back(nullptr) {
    }
};


//const int TracePoints = 4;
//int TracePoints[TracePoints];     // содержит 4 элемента

// Глобальные объекты игры

GameWindow window;
PlayerPlatform player(0, 0, 0, 0);
Ball ball(0, 0, 0);
Ball balltrace(0, 0, 0);

std::vector<Block> blocks; // Массив блоков вместо одного
HBITMAP hBack;
std::vector<POINT> ballTrace;
//std::vector<TracePoint> ballTracePath;
bool ballactive = false;

// Параметры вида (камера/зум)
bool zoomMode = false;
float viewX = 0.0f;
float viewY = 0.0f;
float viewScale = 1.0f;
const float zoomScale = GameConfig::ZoomScale; // масштаб при удержании W

void UpdateView()
{
    zoomMode = (GetAsyncKeyState('W') & 0x8000) != 0;
    if (zoomMode)
    {
        viewScale = zoomScale;
        float visibleW = window.width / viewScale;
        float visibleH = window.height / viewScale;
        viewX = ball.GetX() - visibleW * 0.5f;
        viewY = ball.GetY() - visibleH * 0.5f;
        // Ограничение в пределах размера сцены (здесь = размер окна)
        if (viewX < 0.0f) viewX = 0.0f;
        if (viewY < 0.0f) viewY = 0.0f;
        float maxVX = (float)window.width - visibleW;
        float maxVY = (float)window.height - visibleH;
        if (viewX > maxVX) viewX = maxVX;
        if (viewY > maxVY) viewY = maxVY;
    }
    else
    {
        viewScale = 1.0f;
        viewX = 0.0f;
        viewY = 0.0f;
    }
}

float RandomFloat(float a, float b)
{
    return a + static_cast<float>(rand()) / (RAND_MAX / (b - a));
}

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
    player.SetSize(GameConfig::PlatformWidth, GameConfig::PlatformHeight);
    player.SetSpeed(GameConfig::PlatformSpeedNormal);
    player.SetPosition(window.width / 2.0f, window.height - 120.0f);

    //Трассировочная точка
    balltrace.SetRadius(GameConfig::balltraceRadius);
    balltrace.SetPosition(window.width / 2.0f, window.height - 120.0f);

    // Мяч
    ball.SetRadius(GameConfig::BallRadius);
    ball.SetSpeed(GameConfig::BallInitialSpeed);
    ball.SetDirection(-1.0f, 1.0f);
    ball.SetPosition(window.width / 2.0f, window.height / 2.0f);

    // Создаем массив блоков в виде сетки
    blocks.clear();
    int blockWidth = GameConfig::BlockWidth;
    int blockHeight = GameConfig::BlockHeight;
    int blocksPerRow = GameConfig::BlocksPerRow;
    int rows = GameConfig::BlockRows;
    int startX = (window.width - (blocksPerRow * blockWidth + (blocksPerRow - 1) * GameConfig::BlockGap)) / 2; // центрируем
    int startY = GameConfig::BlocksStartY;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < blocksPerRow; col++) {
            Block newBlock;
            newBlock.SetSize(blockWidth, blockHeight);
            newBlock.SetPosition(startX + col * (blockWidth + GameConfig::BlockGap), startY + row * (blockHeight + GameConfig::BlockGap));
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

void CheckBallBlocksCollision(Ball& ball, std::vector<Block>& blocks)
{
    ballactive = true;

    float bx = ball.GetX();
    float by = ball.GetY();
    float r = ball.GetRadius();

    for (auto& block : blocks)
    {
        if (!block.active) continue; // пропускаем неактивные блоки

        float blx = block.GetX();
        float bly = block.GetY();
        float blw = block.GetW();
        float blh = block.GetH();

        // Проверяем столкновение с учётом радиуса мяча
        bool collisionX = (bx + r >= blx) && (bx - r <= blx + blw);
        bool collisionY = (by + r >= bly) && (by - r <= bly + blh);

        if (collisionX && collisionY)
        {
            // Определяем центр мяча и центра блока
            float ballCenterX = bx;
            float ballCenterY = by;

            float blockCenterX = blx + blw / 2.0f;
            float blockCenterY = bly + blh / 2.0f;

            float deltaX = ballCenterX - blockCenterX;
            float deltaY = ballCenterY - blockCenterY;

            // Определяем сторону столкновения и корректируем позицию
            if (abs(deltaX) > abs(deltaY))
            {
                // столкновение по горизонтали — отражаем X
                ball.SetDirection(-ball.GetDX(), ball.GetDY());
                if (deltaX > 0)
                    ball.SetPosition(blx + blw + r, by); // справа
                else
                    ball.SetPosition(blx - r, by);       // слева
            }
            else {
                // столкновение по вертикали — отражаем Y
                ball.SetDirection(ball.GetDX(), -ball.GetDY());
                if (deltaY > 0)
                    ball.SetPosition(bx, bly + blh + r); // снизу
                else
                    ball.SetPosition(bx, bly - r);       // сверху
            }

            // Деактивируем блок
            /*block.active = false;*/
            break; // выходим после первого столкновения
        }
    }
}

void CheckBallPlatformCollision(Ball& ball, PlayerPlatform& platform)
{
    float px = platform.GetX();
    float py = platform.GetY();
    float pw = platform.GetW();
    //float ph = platform.GetH(); // ph не нужен для верхней стены

    float bx = ball.GetX(); // центр мяча
    float by = ball.GetY();
    float r = ball.GetRadius();

    // условие: нижняя точка мяча коснулась или прошла через верх платформы,
    // и центр мяча сверху платформы (чтобы не ловить столкновения снизу).
    if ((by + r >= py) && (by - r < py) && (bx + r >= px) && (bx - r <= px + pw))
    {
        // вычисляем относительное попадание по X (0..1)
        float hitRelative = (bx - px) / pw;
        if (hitRelative < 0.0f) hitRelative = 0.0f;
        if (hitRelative > 1.0f) hitRelative = 1.0f;

        // угол отскока: от -60 до +60 градусов (в радианах)
        float angleDeg = (hitRelative - 0.5f) * 120.0f;// 0.0 (левый край) до 1.0 (правый край)
        float rad = angleDeg * 3.14159265f / 180.0f; // -60° до +60°
        /*Чем ближе к краю - больше угол
         Центр платформы → вертикальный отскок
        новая направляющая (dx, dy), dy должно быть отрицательным — вверх*/
        float ndx = sinf(rad);
        float ndy = -cosf(rad);

        // нормализуем (чтобы сохранять постоянную скорость)
        float len = sqrtf(ndx * ndx + ndy * ndy);
        if (len < 1e-6f) len = 1.0f;
        ball.SetDirection(ndx / len, ndy / len);
    }
}

void MouseMove(Ball& ball, std::vector<Block>& blocks)
{
    POINT mousePos;
    if (GetCursorPos(&mousePos) && ballactive == true)
    {
       
        ball.SetPosition(mousePos.x, mousePos.y);
        balltrace.SetPosition(mousePos.x, mousePos.y);


        float dx = balltrace.GetDX();
        float dy = balltrace.GetDY();
        float x = mousePos.x;
        float y = mousePos.y;

        for (int i = 0; i < 50; i++)
        {
            x += dx * 10;
            y += dy * 10;

            POINT p{ (LONG)x, (LONG)y };
            ballTrace.push_back(p);
        }

        // Проверяем столкновения настоящего шара
        CheckBallBlocksCollision(ball, blocks);
    }
}


void BallStepMove(Ball& ball, float stepSize = 1.0f)
{
    // Мяч двигается по маленьким шагам (sub-steps).
    // Это позволяет не "пролетать" сквозь платформу или блоки при большой скорости.

        // Сколько всего пикселей нужно пройти за кадр (скорость шара)
    float totalMove = ball.GetSpeed();

    // Текущее направление движения шара
    float dx = ball.GetDX();
    float dy = ball.GetDY();

    // Считаем, сколько маленьких шагов нужно сделать
    // Например: скорость 10, шаг 1 → будет 10 проверок
    int steps = static_cast<int>(ceil(totalMove / stepSize));
    //количество шагов = общая дистанция / размер одного шага
    //Функция ceil(x) = округление вверх до ближайшего целого. И оно возвращает double,
    //оператор Static_cast преобразует его в int

    for (int i = 0; i < steps; i++)
    {
        // Двигаем мяч на один маленький шаг
        ball.SetPosition
        (
            ball.GetX() + dx * stepSize,
            ball.GetY() + dy * stepSize
        );

        // Проверяем столкновения на этом шаге
        // Если мяч коснётся платформы или блока — тут же обработаем
        CheckBallPlatformCollision(ball, player);
        CheckBallBlocksCollision(ball, blocks);

        // Проверка выхода за стены окна
        float bx = ball.GetX();    // центр мяча по X
        float by = ball.GetY();    // центр мяча по Y
        float r = ball.GetRadius();// радиус мяча

        // Столкновение с левой стенкой
        if (bx - r <= 0.0f) {
            ball.SetPosition(r, by); // возвращаем мяч внутрь
            ball.SetDirection(fabs(dx), dy); // отражаем по X вправо
        }

        // Столкновение с правой стенкой
        if (bx + r >= window.width) {
            ball.SetPosition(window.width - r, by); // возвращаем внутрь
            ball.SetDirection(-fabs(dx), dy); // отражаем по X влево
        }

        // Столкновение с верхней стенкой
        if (by - r <= 0.0f) {
            ball.SetPosition(bx, r); // возвращаем внутрь
            ball.SetDirection(dx, fabs(dy)); // отражаем по Y вниз
        }

        // "Проигрыш": мяч улетел за нижнюю границу
        if (by + r >= window.height) {
            // Сбрасываем мяч в центр
            ball.SetPosition(window.width / 2.0f, window.height / 2.0f);

            // Генерируем случайное направление вниз
            float randomDX = RandomFloat(-0.7f, 0.7f);
            float len = sqrtf(randomDX * randomDX + 1.0f);
            ball.SetDirection(randomDX / len, 1.0f / len);
        }

        // Обновляем dx и dy, потому что мяч мог отразиться
        dx = ball.GetDX();
        dy = ball.GetDY();
    }
}

// Ограничение платформы

void LimitPlatform()
{
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
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    while (!GetAsyncKeyState(VK_ESCAPE)) {
        // Очистка экрана
        PatBlt(window.buffer, 0, 0, window.width, window.height, BLACKNESS);

        // Обновляем вид (камера/зум)
        UpdateView();

        // Рисуем фон
        if (hBack) {
            HDC memDC = CreateCompatibleDC(window.buffer);
            HBITMAP old = (HBITMAP)SelectObject(memDC, hBack);
            BITMAP bm; GetObject(hBack, sizeof(BITMAP), &bm);
            if (zoomMode)
            {
                // Вырезаем из фоновой текстуры область под камеру и растягиваем на окно
                int srcX = (int)viewX;
                int srcY = (int)viewY;
                int srcW = (int)(window.width / viewScale);
                int srcH = (int)(window.height / viewScale);
                // Подстрахуем рамки в пределах текстуры
                if (srcX < 0) srcX = 0;
                if (srcY < 0) srcY = 0;
                if (srcX + srcW > bm.bmWidth) srcX = bm.bmWidth - srcW;
                if (srcY + srcH > bm.bmHeight) srcY = bm.bmHeight - srcH;
                StretchBlt(window.buffer, 0, 0, window.width, window.height,
                    memDC, srcX, srcY, srcW, srcH, SRCCOPY);
            }
            else
            {
                StretchBlt(window.buffer, 0, 0, window.width, window.height,
                    memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            }
            SelectObject(memDC, old);
            DeleteDC(memDC);
        }

        // Рисуем платформу, мяч и блоки с учётом вида
        player.DrawView(window.buffer, viewX, viewY, viewScale);
        ball.DrawView(window.buffer, viewX, viewY, viewScale);
        balltrace.DrawView(window.buffer, viewX, viewY, viewScale);

        // Рисуем трассировку
        for (auto& p : ballTrace)
        {
            Ellipse(window.buffer, p.x - 2, p.y - 2, p.x + 2, p.y + 2);
            // маленькие кружочки радиусом 2
        }

        for (auto& block : blocks)
        {
            if (block.active) {
                block.DrawView(window.buffer, viewX, viewY, viewScale);
            }
        }

        // Выводим на экран
        BitBlt(window.dc, 0, 0, window.width, window.height, window.buffer, 0, 0, SRCCOPY);

        // Обновляем управление платформой
        bool shift = GetAsyncKeyState(VK_LSHIFT) & 0x8000;
        player.MoveShift(shift);
        if (GetAsyncKeyState('A') & 0x8000) player.MoveLeft();
        if (GetAsyncKeyState('D') & 0x8000) player.MoveRight();

        // Граничные условия платформы
        LimitPlatform();

        // Двигаем мяч дискретными шагами (предотвращает пролет сквозь объекты)
        BallStepMove(ball);
        BallReset(ball);
        ball.SlowBall();
        // Проверяем столкновения
        CheckBallPlatformCollision(ball, player);
        CheckBallBlocksCollision(ball, blocks);
        MouseMove(ball, blocks);
        Sleep(3); // ~60 FPS
    }
    return 0;
}
