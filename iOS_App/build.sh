#!/bin/bash

# Скрипт для сборки iOS приложения
# Требуется: macOS с установленным Xcode

set -e

echo "🔨 Начинаем сборку iOS приложения..."

# Проверяем наличие Xcode
if ! command -v xcodebuild &> /dev/null; then
    echo "❌ Ошибка: xcodebuild не найден. Установите Xcode."
    exit 1
fi

# Переходим в директорию проекта
cd "$(dirname "$0")"

# Очистка предыдущей сборки
echo "🧹 Очистка предыдущей сборки..."
xcodebuild clean -project CamAssist.xcodeproj -scheme CamAssist

# Сборка для симулятора
echo "📱 Сборка для iOS Simulator..."
xcodebuild build \
    -project CamAssist.xcodeproj \
    -scheme CamAssist \
    -sdk iphonesimulator \
    -configuration Release \
    -derivedDataPath build

# Сборка для устройства (требует подписанный сертификат)
echo "📱 Сборка для iOS Device..."
xcodebuild build \
    -project CamAssist.xcodeproj \
    -scheme CamAssist \
    -sdk iphoneos \
    -configuration Release \
    -derivedDataPath build \
    CODE_SIGN_IDENTITY="" \
    CODE_SIGNING_REQUIRED=NO

echo "✅ Сборка завершена!"
echo "📦 Результаты находятся в: build/Build/Products/"
