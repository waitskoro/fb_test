# fb_test

### Создание виртуального дисплея

1. Установка ffmpeg
##### sudo apt install ffmpeg
2. Создание дисплея
##### sudo modprobe vkms
3. Запуск виртуального дисплея
##### sudo ffplay -f fbdev /dev/fb0
