# Инструкция по настройке iOS приложения

## Требования

- macOS с установленным Xcode 14.0 или выше
- iOS устройство или симулятор (iOS 15.0+)
- Аккаунт Apple Developer (для установки на реальное устройство)

## Создание Xcode проекта

1. Откройте Xcode
2. Выберите File → New → Project
3. Выберите "App" в разделе iOS
4. Заполните:
   - Product Name: `CamAssist`
   - Team: Ваша команда разработчика
   - Organization Identifier: `com.example` (или ваш)
   - Interface: SwiftUI
   - Language: Swift
   - Minimum Deployment: iOS 15.0

## Добавление файлов

1. Скопируйте все файлы из папки `iOS_App/CamAssist/` в ваш Xcode проект
2. Убедитесь, что все файлы добавлены в Target "CamAssist"

## Структура файлов в Xcode

```
CamAssist/
├── Models/
│   └── DeviceData.swift
├── Services/
│   ├── NetworkService.swift
│   └── UdpService.swift
├── ViewModels/
│   └── DeviceViewModel.swift
├── Views/
│   ├── DeviceView.swift
│   ├── CalibrationDialog.swift
│   └── WiFiSettingsDialog.swift
├── Components/
│   ├── StatusBar.swift
│   ├── DataCard.swift
│   └── ControlButton.swift
└── CamAssistApp.swift
```

## Добавление фонового изображения

1. Добавьте файл `unnamed.png` в Assets.xcassets
2. Назовите его `background`
3. Или используйте градиент (уже реализован как fallback)

## Настройка Info.plist

Файл `Info.plist` уже настроен с необходимыми разрешениями:
- NSLocalNetworkUsageDescription - для доступа к локальной сети
- NSAppTransportSecurity - для HTTP подключений

## Зависимости

Приложение использует только стандартные фреймворки Apple:
- SwiftUI
- Foundation
- Network (для UDP)
- Combine

## Сборка и запуск

1. Выберите устройство или симулятор
2. Нажмите ⌘R для запуска
3. Приложение автоматически подключится к устройству ESP32

## CarPlay (опционально)

Для добавления поддержки CarPlay:
1. Добавьте Capability "CarPlay" в настройках проекта
2. Создайте CarPlay App в Apple Developer Portal
3. Добавьте CarPlay компоненты (см. документацию Apple)

## Тестирование

1. Убедитесь, что ESP32 устройство включено
2. Подключите iOS устройство к той же WiFi сети (или к AP устройства)
3. Запустите приложение
4. Данные должны начать обновляться автоматически
