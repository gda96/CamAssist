# Инструкция по созданию iOS приложения

## Шаг 1: Создание Xcode проекта

1. Откройте Xcode
2. File → New → Project
3. Выберите **iOS** → **App**
4. Заполните:
   - **Product Name:** CamAssist
   - **Team:** Ваша команда разработчика
   - **Organization Identifier:** com.example (или ваш)
   - **Interface:** SwiftUI
   - **Language:** Swift
   - **Storage:** None (или Core Data если нужно)
5. Нажмите **Next** и выберите место для сохранения

## Шаг 2: Копирование файлов

Скопируйте все файлы из структуры `iOS_App/CamAssist/` в ваш Xcode проект:

1. **Models/** → Перетащите в проект (создайте группу Models)
2. **Services/** → Перетащите в проект (создайте группу Services)
3. **ViewModels/** → Перетащите в проект (создайте группу ViewModels)
4. **Views/** → Перетащите в проект (создайте группу Views)
5. **App/** → Замените существующий `CamAssistApp.swift`

## Шаг 3: Настройка Info.plist

Добавьте в `Info.plist`:

```xml
<key>NSAppTransportSecurity</key>
<dict>
    <key>NSAllowsArbitraryLoads</key>
    <true/>
</dict>
<key>NSLocalNetworkUsageDescription</key>
<string>Приложению необходим доступ к локальной сети для подключения к устройству ESP32</string>
<key>NSBonjourServices</key>
<array>
    <string>_http._tcp</string>
</array>
```

## Шаг 4: Добавление ресурсов

1. Добавьте фоновое изображение `background.png` в Assets.xcassets
2. Настройте иконку приложения

## Шаг 5: Настройка Capabilities

1. Выберите проект в навигаторе
2. Выберите target **CamAssist**
3. Перейдите на вкладку **Signing & Capabilities**
4. Добавьте **Background Modes**:
   - ✅ Background fetch
   - ✅ Background processing
5. (Опционально) Добавьте **CarPlay** capability для поддержки CarPlay

## Шаг 6: Настройка для CarPlay (опционально)

1. Добавьте CarPlay capability
2. Создайте CarPlay extension:
   - File → New → Target
   - Выберите **CarPlay** → **CarPlay App**
3. Настройте Info.plist для CarPlay согласно документации Apple

## Шаг 7: Сборка и запуск

1. Выберите устройство или симулятор
2. Нажмите **Run** (⌘R)
3. Приложение должно запуститься

## Требования

- **iOS Deployment Target:** 15.0
- **Xcode:** 14.0+
- **Swift:** 5.7+

## Примечания

- Для тестирования на реальном устройстве необходим Apple Developer аккаунт
- UDP функциональность требует реального устройства (не работает в симуляторе)
- CarPlay требует специальной сертификации Apple

## Следующие шаги

1. Протестируйте подключение к устройству ESP32
2. Проверьте работу UDP коммуникации
3. Настройте CarPlay (если требуется)
4. Добавьте обработку ошибок
5. Оптимизируйте UI для разных размеров экранов
