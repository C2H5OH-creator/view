# view

`view` — маленький просмотрщик JPEG на C.

Программа:
- проверяет сигнатуру файла и тип (сейчас поддерживается только JPEG);
- парсит JPEG-маркеры и читает метаданные (ширина, высота, компоненты, precision);
- декодирует JPEG в RGB через `libjpeg`;
- открывает окно через `SDL2` и показывает изображение с сохранением пропорций при ресайзе.

## Зависимости

- `gcc`
- `pkg-config`
- `SDL2` (dev-пакет)
- `libjpeg` (dev-пакет)

## Сборка

```bash
mkdir -p bin
gcc -Isrc/include -std=c11 -Wall -Wextra src/source/*.c -o bin/view $(pkg-config --cflags --libs sdl2 libjpeg)
```

## Запуск

```bash
./bin/view [--verbose] [--info] <image.jpg>
```

Примеры:

```bash
./bin/view data/fall_wallpaper.jpg
./bin/view --info data/photo_2025-01-27_18-30-30.jpg
./bin/view --verbose --info data/fall_wallpaper.jpg
```

## Опции

- `-h, --help` — показать справку
- `-v, --verbose` — печатать подробный лог парсинга JPEG
- `-i, --info` — вывести извлечённые JPEG-параметры перед показом окна

## Ограничения (текущее состояние)

- поддерживается только JPEG;
- для неподдерживаемых форматов программа завершится с сообщением об ошибке;
- выход из окна: `Esc` или кнопка закрытия.
