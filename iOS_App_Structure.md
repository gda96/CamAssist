# iOS приложение "Обогрев камеры ассистентов"

## Структура проекта

```
CamAssist-iOS/
├── CamAssist/
│   ├── Models/
│   │   ├── DeviceData.swift
│   │   ├── ControlResponse.swift
│   │   └── CalibrationParams.swift
│   ├── Services/
│   │   ├── DeviceService.swift
│   │   ├── UdpClient.swift
│   │   └── NetworkUtils.swift
│   ├── ViewModels/
│   │   └── DeviceViewModel.swift
│   ├── Views/
│   │   ├── ContentView.swift
│   │   ├── DeviceView.swift
│   │   ├── StatusBar.swift
│   │   ├── DataCard.swift
│   │   ├── ControlButton.swift
│   │   ├── CalibrationView.swift
│   │   └── WiFiSettingsView.swift
│   ├── CarPlay/
│   │   └── CarPlaySceneDelegate.swift
│   ├── App/
│   │   ├── CamAssistApp.swift
│   │   └── SceneDelegate.swift
│   └── Resources/
│       ├── Assets.xcassets/
│       └── Info.plist
├── CamAssistCarPlay/
│   └── CarPlayManager.swift
└── CamAssist.xcodeproj/
```

## Требования

- iOS 15.0+
- Xcode 14.0+
- Swift 5.7+
- CarPlay (опционально)

## Функциональность

- ✅ Полное управление устройством через WiFi
- ✅ Автоматическое определение режима подключения (AP/STA)
- ✅ Адаптивный дизайн для портретной и горизонтальной ориентации
- ✅ Поддержка CarPlay
- ✅ UDP коммуникация для автоматического обнаружения
- ✅ Управление обогревом и калибровкой
- ✅ Настройка WiFi сетей
