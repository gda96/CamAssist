# Оптимизация иконок приложения

## Текущее состояние

Иконки созданы для всех разрешений экрана:
- ✅ mdpi (48x48)
- ✅ hdpi (72x72)
- ✅ xhdpi (96x96)
- ✅ xxhdpi (144x144)
- ✅ xxxhdpi (192x192)

Все иконки скопированы из исходного файла `iconcam.png` (192x192). Android будет автоматически масштабировать их для разных устройств.

## Оптимизация (опционально)

Для лучшего качества иконок на разных устройствах рекомендуется создать версии с точными размерами:

### Способ 1: Использование ImageMagick

Если у вас установлен ImageMagick, запустите скрипт:

```powershell
.\create_icons.ps1
```

Или вручную:

```powershell
# mdpi (48x48)
magick iconcam.png -resize 48x48 app\src\main\res\mipmap-mdpi\ic_launcher.png
magick iconcam.png -resize 48x48 app\src\main\res\mipmap-mdpi\ic_launcher_round.png

# hdpi (72x72)
magick iconcam.png -resize 72x72 app\src\main\res\mipmap-hdpi\ic_launcher.png
magick iconcam.png -resize 72x72 app\src\main\res\mipmap-hdpi\ic_launcher_round.png

# xhdpi (96x96)
magick iconcam.png -resize 96x96 app\src\main\res\mipmap-xhdpi\ic_launcher.png
magick iconcam.png -resize 96x96 app\src\main\res\mipmap-xhdpi\ic_launcher_round.png

# xxhdpi (144x144)
magick iconcam.png -resize 144x144 app\src\main\res\mipmap-xxhdpi\ic_launcher.png
magick iconcam.png -resize 144x144 app\src\main\res\mipmap-xxhdpi\ic_launcher_round.png

# xxxhdpi (192x192) - уже правильный размер
# Файл уже в правильном размере
```

### Способ 2: Использование онлайн-инструментов

1. Используйте [Android Asset Studio](https://romannurik.github.io/AndroidAssetStudio/icons-launcher.html)
2. Загрузите `iconcam.png`
3. Скачайте все размеры
4. Замените файлы в соответствующих папках

### Способ 3: Использование Android Studio

1. Откройте проект в Android Studio
2. Правый клик на `app/src/main/res`
3. New → Image Asset
4. Выберите `iconcam.png` как источник
5. Android Studio автоматически создаст все размеры

## Круглые иконки

Круглые иконки используют адаптивные иконки Android 8.0+ с круглой маской. Для устройств с Android 7.1 и ниже используются обычные квадратные иконки.

## Структура файлов

```
app/src/main/res/
├── drawable/
│   ├── iconcam.png              # Исходная иконка (192x192)
│   ├── iconcam_launcher.xml    # Drawable для обычной иконки
│   └── iconcam_round.xml       # Drawable для круглой иконки
├── mipmap-anydpi-v26/
│   ├── ic_launcher.xml          # Адаптивная иконка (обычная)
│   └── ic_launcher_round.xml    # Адаптивная иконка (круглая)
├── mipmap-mdpi/
│   ├── ic_launcher.png          # 48x48
│   └── ic_launcher_round.png    # 48x48
├── mipmap-hdpi/
│   ├── ic_launcher.png          # 72x72
│   └── ic_launcher_round.png    # 72x72
├── mipmap-xhdpi/
│   ├── ic_launcher.png          # 96x96
│   └── ic_launcher_round.png    # 96x96
├── mipmap-xxhdpi/
│   ├── ic_launcher.png          # 144x144
│   └── ic_launcher_round.png    # 144x144
└── mipmap-xxxhdpi/
    ├── ic_launcher.png          # 192x192
    └── ic_launcher_round.png    # 192x192
```

## Примечания

- Текущие иконки будут работать на всех устройствах
- Android автоматически масштабирует иконки при необходимости
- Для лучшего качества рекомендуется создать версии с точными размерами
- Круглые иконки автоматически применяются на устройствах с Android 8.0+

