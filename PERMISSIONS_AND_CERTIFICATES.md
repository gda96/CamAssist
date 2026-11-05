# Разрешения и сертификаты для Android Auto

## Добавленные разрешения

### Основные разрешения (для работы приложения)
- ✅ `INTERNET` - доступ к интернету
- ✅ `ACCESS_NETWORK_STATE` - проверка состояния сети
- ✅ `ACCESS_WIFI_STATE` - доступ к информации о WiFi
- ✅ `CHANGE_WIFI_STATE` - изменение настроек WiFi

### Разрешения для Android Auto
- ✅ `WAKE_LOCK` - удержание устройства в активном состоянии
- ✅ `FOREGROUND_SERVICE` - работа в фоновом режиме
- ✅ `FOREGROUND_SERVICE_DATA_SYNC` - синхронизация данных в фоне
- ✅ `POST_NOTIFICATIONS` - отправка уведомлений (Android 13+)

### Разрешения для совместимости
- ✅ `BLUETOOTH` - для Android до версии 11 (maxSdkVersion="30")
- ✅ `BLUETOOTH_CONNECT` - для Android 12+ (может потребоваться для Android Auto)
- ✅ `ACCESS_FINE_LOCATION` - для Android до версии 12 (maxSdkVersion="32")
- ✅ `ACCESS_COARSE_LOCATION` - для Android до версии 12 (maxSdkVersion="32")

## Конфигурация сервиса Android Auto

### Полная конфигурация CarAppService:

```xml
<service
    android:name=".car.CarAppService"
    android:exported="true"
    android:enabled="true"
    android:label="@string/app_name"
    android:icon="@mipmap/ic_launcher"
    android:foregroundServiceType="dataSync">
    <intent-filter>
        <action android:name="androidx.car.app.CarAppService" />
    </intent-filter>
    <meta-data
        android:name="androidx.car.app.minCarApiLevel"
        android:value="2" />
    <meta-data
        android:name="androidx.car.app.category"
        android:value="PRODUCTIVITY" />
    <meta-data
        android:name="androidx.car.app.description"
        android:value="@string/car_app_description" />
    <meta-data
        android:name="androidx.car.app.icon"
        android:resource="@mipmap/ic_launcher" />
    <meta-data
        android:name="androidx.car.app.template"
        android:value="true" />
</service>
```

### Ключевые атрибуты:
- ✅ `android:exported="true"` - сервис доступен для Android Auto
- ✅ `android:enabled="true"` - сервис включен
- ✅ `android:foregroundServiceType="dataSync"` - тип фонового сервиса
- ✅ Все необходимые метаданные присутствуют

## Queries для Android Auto

```xml
<queries>
    <package android:name="com.google.android.projection.gearhead" />
    <intent>
        <action android:name="androidx.car.app.CarAppService" />
    </intent>
</queries>
```

**Критически важно для Android 11+ (API 30+)!**

## Сертификаты и подпись

### Текущая конфигурация:
- **Тип подписи:** Debug keystore (для тестирования)
- **Путь к keystore:** `~/.android/debug.keystore`
- **Alias:** `androiddebugkey`
- **Пароль:** `android`

### Для продакшена:
1. Создайте production keystore:
   ```bash
   keytool -genkey -v -keystore app/release.keystore -alias cam_assist -keyalg RSA -keysize 2048 -validity 10000
   ```

2. Обновите `app/build.gradle.kts`:
   ```kotlin
   signingConfigs {
       create("release") {
           storeFile = file("app/release.keystore")
           storePassword = "your_password"
           keyAlias = "cam_assist"
           keyPassword = "your_password"
       }
   }
   ```

## Network Security Config

Конфигурация для работы с локальными сетями (ESP32):

```xml
<network-security-config>
    <base-config cleartextTrafficPermitted="true">
        <trust-anchors>
            <certificates src="system" />
        </trust-anchors>
    </base-config>
    <domain-config cleartextTrafficPermitted="true">
        <domain includeSubdomains="true">192.168.4.1</domain>
        <!-- другие локальные IP -->
    </domain-config>
</network-security-config>
```

## Версия приложения

- **Версия:** 1.5
- **Version Code:** 6
- **Дата:** 26.12.2025

## Проверка разрешений

После установки приложения проверьте разрешения:

```bash
# Проверка установленных разрешений
adb shell dumpsys package com.example.cam_assist | grep permission

# Проверка разрешений Android Auto
adb shell dumpsys package com.google.android.projection.gearhead | grep permission
```

## Важно для Android Auto 15.7+

1. **Режим разработчика обязателен:**
   - Настройки → Android Auto → Версия (7 раз)
   - Меню → Для разработчиков → Неизвестные источники

2. **После установки:**
   - Полностью закройте Android Auto
   - Откройте снова
   - Подождите 10-15 секунд

3. **Проверка:**
   - Настройки Android Auto → Настройка панели запуска
   - Приложение должно появиться в списке

## Устранение проблем

Если приложение все еще не появляется:

1. **Проверьте логи:**
   ```bash
   adb logcat | grep -i "car.app\|gearhead\|cam_assist" > log.txt
   ```

2. **Проверьте регистрацию сервиса:**
   ```bash
   adb shell dumpsys package com.example.cam_assist | grep -A 30 "CarAppService"
   ```

3. **Очистите кэш Android Auto:**
   - Настройки → Приложения → Android Auto → Хранилище → Очистить кэш

4. **Перезагрузите устройство**

## Заключение

Все необходимые разрешения и конфигурации добавлены. Приложение готово к использованию с Android Auto 15.7+ на Android 16+.

**Статус:** ✅ Все разрешения добавлены, приложение пересобрано

