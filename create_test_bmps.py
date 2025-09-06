from PIL import Image, ImageDraw
import os

# Создаем папку для изображений если её нет
if not os.path.exists('images'):
    os.makedirs('images')

# Создаем изображение платформы (синий прямоугольник)
platform = Image.new('RGB', (100, 20), color='blue')
draw = ImageDraw.Draw(platform)
draw.rectangle([2, 2, 98, 18], outline='white', width=2)
platform.save('player_platform.bmp')

# Создаем изображение мяча (красный круг)
ball = Image.new('RGB', (50, 50), color='black')
draw = ImageDraw.Draw(ball)
draw.ellipse([5, 5, 45, 45], fill='red', outline='white', width=2)
ball.save('ball.bmp')

# Создаем фон (зеленый градиент)
background = Image.new('RGB', (800, 600), color='darkgreen')
draw = ImageDraw.Draw(background)
for y in range(0, 600, 20):
    color = (0, min(255, 50 + y//3), 0)
    draw.rectangle([0, y, 800, y+20], fill=color)
background.save('forest_bg.bmp')

print("Созданы тестовые BMP файлы:")
print("- player_platform.bmp (синяя платформа)")
print("- ball.bmp (красный мяч)")
print("- forest_bg.bmp (зеленый фон)")
