# Исправление проблемы с отображением приложения в Android Auto

## Проблема
Приложение "Обогрев камеры" не отображалось в списке доступных приложений для настройки панели запуска в Android Auto.

## Решение

### 1. Добавлены queries для Android 11+ (API 30+)

Для Android 11 и выше требуется явно объявить queries для Android Auto:

```xml
<queries>
    <package android:name="com.google.android.projection.gearhead" />
    <intent>
        <action android:name="androidx.car.app.CarAppService" />
    </intent>
</queries>
```

**Важно:** Это критично для Android 11+ (targetSdk 30+), без этого Android Auto не сможет обнаружить приложение.

### 2. Добавлена иконка в сервис

```xml
<service
    android:name=".car.CarAppService"
    android:icon="@mipmap/ic_launcher">
```

### 3. Исправлен intent-filter

Убрана неправильная категория из intent-filter (категория должна быть только в meta-data).

## Изменения в AndroidManifest.xml

### Добавлено:
1. ✅ `<queries>` блок для Android Auto
2. ✅ `android:icon` в сервисе
3. ✅ Исправлен intent-filter

### Полная конфигурация сервиса:

```xml
<!-- Android Auto -->
<service
    android:name=".car.CarAppService"
    android:exported="true"
    android:enabled="true"
    android:label="@string/app_name"
    android:icon="@mipmap/ic_launcher">
    <intent-filter>
        <action android:name="androidx.car.app.CarAppService" />
    </intent-filter>
    <meta-data
        android:name="androidx.car.app.minCarApiLevel"
        android:value="1" />
    <meta-data
        android:name="androidx.car.app.category"
        android:value="PRODUCTIVITY" />
    <meta-data
        android:name="androidx.car.app.description"
        android:value="@string/car_app_description" />
    <meta-data
        android:name="androidx.car.app.icon"
        android:resource="@mipmap/ic_launcher" />
</service>
```

## Версия

- **Версия:** 1.2
- **Version Code:** 3
- **Дата:** 26.12.2025

## Установка обновленного APK

```bash
# Удалить старую версию
adb uninstall com.example.cam_assist

# Установить новую версию
adb install app/build/outputs/apk/release/app-release.apk
```

## Проверка

После установки:

1. **Убедитесь, что режим разработчика включен:**
   - Настройки → Android Auto → Версия (нажмите 7 раз)
   - Включите "Для разработчиков" → "Неизвестные источники"

2. **Перезапустите Android Auto:**
   - Полностью закройте приложение
   - Откройте снова

3. **Проверьте в настройках:**
   - Настройки Android Auto → Настройка панели запуска
   - Приложение "Обогрев камеры" должно появиться в списке доступных приложений

## Проверка через ADB

```bash
# Проверка регистрации сервиса
adb shell dumpsys package com.example.cam_assist | grep -A 20 "CarAppService"

# Проверка queries
adb shell dumpsys package com.example.cam_assist | grep -A 10 "queries"

# Логи Android Auto
adb logcat | grep -i "car.app\|cam_assist"
```

## Если приложение все еще не появляется

1. **Проверьте версию Android:**
   - Queries требуются для Android 11+ (API 30+)
   - Если устройство Android 10 или ниже, queries не нужны, но могут не мешать

2. **Очистите кэш Android Auto:**
   - Настройки → Приложения → Android Auto → Хранилище → Очистить кэш

3. **Переустановите приложение:**
   ```bash
   adb uninstall com.example.cam_assist
   adb install app/build/outputs/apk/release/app-release.apk
   ```

4. **Проверьте логи:**
   ```bash
   adb logcat | grep -i "car.app\|gearhead\|cam_assist"
   ```

5. **Убедитесь, что Android Auto обновлен:**
   - Обновите Android Auto из Google Play Store

## Ключевые моменты

1. **Queries обязательны** для Android 11+ (targetSdk 30+)
2. **Режим разработчика** должен быть включен для тестирования
3. **Перезапуск Android Auto** необходим после установки
4. **Версия приложения** должна быть обновлена (1.2, versionCode: 3)

## Статус

✅ **Исправлено** - Приложение должно теперь отображаться в списке доступных приложений Android Auto.

**Дата исправления:** 26.12.2025  
**Версия:** 1.2 (versionCode: 3)

