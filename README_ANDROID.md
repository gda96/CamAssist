# Android приложение "Обогрев камеры ассистентов"

## Описание

Android приложение для управления устройством ESP32-C3, которое контролирует обогрев камеры ассистентов. Приложение работает как в режиме точки доступа (AP), так и в режиме подключения к домашней сети (STA).

## Возможности

- ✅ Полное управление устройством через WiFi
- ✅ Автоматическое определение режима подключения (AP/STA)
- ✅ Адаптивный дизайн для портретной и горизонтальной ориентации
- ✅ Оптимизация для планшетов и автомобильных магнитол
- ✅ Поддержка Android Auto
- ✅ Скрытие информации о подключении к домашней сети
- ✅ Отображение температуры и влажности в реальном времени
- ✅ Управление обогревом и режимами работы

## Требования

- Android 7.0 (API 24) и выше
- Подключение к WiFi сети устройства или домашней сети

## Сборка

1. Откройте проект в Android Studio
2. Синхронизируйте Gradle зависимости
3. Соберите APK или установите на устройство

### Debug версия

```bash
./gradlew assembleDebug
```

### Release версия

**Для тестирования (использует debug keystore):**
```bash
./gradlew assembleRelease
```

**Для продакшена (требуется production keystore):**

1. Создайте keystore файл (если еще не создан):
```bash
keytool -genkey -v -keystore app/release.keystore -alias cam_assist -keyalg RSA -keysize 2048 -validity 10000
```

2. Добавьте в `gradle.properties` (или создайте файл `keystore.properties` в папке `app/`):
```properties
RELEASE_STORE_FILE=release.keystore
RELEASE_STORE_PASSWORD=ваш_пароль
RELEASE_KEY_ALIAS=cam_assist
RELEASE_KEY_PASSWORD=ваш_пароль
```

3. Обновите `app/build.gradle.kts` для использования production keystore:
```kotlin
signingConfigs {
    create("release") {
        val keystorePropertiesFile = rootProject.file("keystore.properties")
        val keystoreProperties = java.util.Properties()
        if (keystorePropertiesFile.exists()) {
            keystoreProperties.load(java.io.FileInputStream(keystorePropertiesFile))
            storeFile = file(keystoreProperties["storeFile"] as String)
            storePassword = keystoreProperties["storePassword"] as String
            keyAlias = keystoreProperties["keyAlias"] as String
            keyPassword = keystoreProperties["keyPassword"] as String
        }
    }
}
```

4. Соберите release APK:
```bash
./gradlew assembleRelease
```

**Важно:** 
- Храните keystore файл и пароли в безопасном месте
- Без keystore вы не сможете обновить приложение в Google Play
- Добавьте `*.keystore` и `keystore.properties` в `.gitignore`

## Использование

### Подключение в режиме AP (точка доступа)

1. Убедитесь, что устройство ESP32-C3 создает точку доступа "Camera_Heater_AP"
2. Подключите Android устройство к этой сети
3. Запустите приложение
4. Приложение автоматически подключится к устройству по адресу 192.168.4.1

### Подключение в режиме STA (домашняя сеть)

1. Настройте устройство ESP32-C3 на подключение к вашей домашней WiFi сети через веб-интерфейс
2. Убедитесь, что Android устройство подключено к той же сети
3. Запустите приложение
4. Приложение автоматически определит IP адрес устройства и подключится

### Android Auto

1. Подключите Android устройство к автомобилю через USB или беспроводное подключение
2. Запустите Android Auto
3. Приложение появится в списке доступных приложений
4. Используйте голосовые команды или сенсорный экран для управления

## Структура проекта

```
app/src/main/java/com/example/cam_assist/
├── data/
│   ├── api/              # API клиент (Retrofit)
│   ├── model/            # Модели данных
│   └── repository/       # Репозиторий для работы с данными
├── ui/
│   ├── components/       # UI компоненты
│   ├── screen/           # Экраны приложения
│   ├── theme/            # Тема и стили
│   └── viewmodel/       # ViewModel для управления состоянием
├── car/                  # Android Auto компоненты
└── util/                 # Утилиты (NetworkUtils)
```

## API Endpoints

Приложение использует следующие API endpoints устройства:

- `GET /data` - получение данных устройства
- `GET /control?action=toggle` - переключение обогрева
- `GET /control?action=auto` - установка автоматического режима
- `POST /sensorcalibrate` - калибровка датчика
- `POST /calibrate` - калибровка порогов
- `POST /savehideinfo` - сохранение настройки скрытия информации
- `POST /updatewifi` - обновление WiFi настроек

## Адаптивный дизайн

Приложение автоматически адаптируется под ориентацию экрана:

- **Портретная ориентация**: данные и управление расположены вертикально
- **Горизонтальная ориентация**: данные слева, управление справа (оптимизировано для планшетов и магнитол)

Все элементы помещаются на одном экране без необходимости прокрутки.

## Разрешения

Приложение требует следующие разрешения:

- `INTERNET` - для подключения к устройству
- `ACCESS_NETWORK_STATE` - для проверки состояния сети
- `ACCESS_WIFI_STATE` - для определения режима подключения
- `CHANGE_WIFI_STATE` - для управления WiFi (если необходимо)

## Технологии

- **Jetpack Compose** - современный UI фреймворк
- **Retrofit** - HTTP клиент
- **Coroutines** - асинхронные операции
- **Material 3** - дизайн система
- **Android Auto** - интеграция с автомобильными системами

## Версия

- Version Code: 1
- Version Name: 1.0

## Лицензия

Проект создан для личного использования.

