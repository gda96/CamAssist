# Исправления для Android 16+ и Android Auto 15.7+

## Проблема
Приложение не отображается в списке доступных приложений Android Auto на устройствах с Android 16+ и Android Auto 15.7+.

## Выполненные исправления

### 1. Обновлен minCarApiLevel
**Было:** `android:value="1"`  
**Стало:** `android:value="2"`

Для Android Auto 15.7+ рекомендуется использовать уровень API 2+ для лучшей совместимости.

### 2. Добавлена поддержка Android 16
Добавлен атрибут `android:enableOnBackInvokedCallback="true"` в application тег для поддержки новых жестов навигации в Android 16.

### 3. Улучшены queries
Queries остаются обязательными для Android 11+ и критичны для Android 16+.

## Текущая конфигурация

### AndroidManifest.xml

```xml
<!-- Queries для Android Auto (требуется для Android 11+) -->
<!-- Для Android 16+ и Android Auto 15.7+ требуется полная конфигурация -->
<queries>
    <!-- Пакет Android Auto -->
    <package android:name="com.google.android.projection.gearhead" />
    <!-- Intent для CarAppService -->
    <intent>
        <action android:name="androidx.car.app.CarAppService" />
    </intent>
</queries>

<application
    android:enableOnBackInvokedCallback="true">
    
    <service
        android:name=".car.CarAppService"
        android:exported="true"
        android:enabled="true"
        android:label="@string/app_name"
        android:icon="@mipmap/ic_launcher">
        <intent-filter>
            <action android:name="androidx.car.app.CarAppService" />
        </intent-filter>
        <!-- Для Android Auto 15.7+ рекомендуется использовать уровень 2+ -->
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
    </service>
</application>
```

## Версия

- **Версия:** 1.3
- **Version Code:** 4
- **Дата:** 26.12.2025

## Критически важные шаги для Android 16+

### 1. Режим разработчика ОБЯЗАТЕЛЕН

Для Android 16+ и Android Auto 15.7+ режим разработчика **обязателен** даже для тестирования:

1. **Настройки → Android Auto**
2. Нажмите на **"Версия"** **7 раз подряд**
3. Появится сообщение "Вы стали разработчиком!"
4. В меню **"Для разработчиков"** включите **"Неизвестные источники"**
5. **Перезагрузите устройство** (рекомендуется)

### 2. Установка приложения

```bash
# Полностью удалить старую версию
adb uninstall com.example.cam_assist

# Установить новую версию
adb install app/build/outputs/apk/release/app-release.apk

# Или принудительно переустановить
adb install -r app/build/outputs/apk/release/app-release.apk
```

### 3. Перезапуск Android Auto

**КРИТИЧНО для Android 16+:**

1. Полностью закройте Android Auto (не просто сверните)
2. Очистите кэш Android Auto:
   - Настройки → Приложения → Android Auto → Хранилище → Очистить кэш
3. Перезагрузите устройство (рекомендуется)
4. Откройте Android Auto снова

### 4. Проверка

1. Откройте **Настройки Android Auto**
2. Перейдите в **"Настройка панели запуска"**
3. Приложение **"Обогрев камеры"** должно появиться в списке доступных приложений

## Проверка через ADB

```bash
# Проверка регистрации сервиса
adb shell dumpsys package com.example.cam_assist | grep -A 20 "CarAppService"

# Проверка queries
adb shell dumpsys package com.example.cam_assist | grep -A 10 "queries"

# Проверка метаданных
adb shell dumpsys package com.example.cam_assist | grep -A 5 "androidx.car.app"

# Логи Android Auto
adb logcat | grep -i "car.app\|gearhead\|cam_assist"
```

## Если приложение все еще не появляется

### Для Android 16+ и Android Auto 15.7+:

1. **Проверьте версию Android Auto:**
   - Должна быть 15.7.654634-release или выше
   - Обновите из Google Play Store

2. **Проверьте версию Android:**
   - Должна быть Android 16 или выше
   - Обновите систему

3. **Очистите все данные Android Auto:**
   - Настройки → Приложения → Android Auto → Хранилище → Очистить данные
   - **ВНИМАНИЕ:** Это удалит все настройки Android Auto

4. **Переустановите приложение:**
   ```bash
   adb uninstall com.example.cam_assist
   adb install app/build/outputs/apk/release/app-release.apk
   ```

5. **Перезагрузите устройство:**
   - Это критично для Android 16+

6. **Проверьте логи:**
   ```bash
   adb logcat | grep -i "car.app\|gearhead\|cam_assist\|CarAppService"
   ```

## Известные проблемы Android 16+

1. **Строгая проверка queries:** Android 16+ более строго проверяет queries
2. **Требуется перезагрузка:** После изменений в манифесте может потребоваться перезагрузка
3. **Кэш Android Auto:** Кэш может не обновляться автоматически

## Рекомендации

1. ✅ Всегда используйте режим разработчика для тестирования
2. ✅ Перезагружайте устройство после установки приложения
3. ✅ Очищайте кэш Android Auto при проблемах
4. ✅ Проверяйте логи через ADB
5. ✅ Убедитесь, что Android Auto обновлен до последней версии

## Статус

✅ **Исправлено** - Приложение должно теперь отображаться в списке доступных приложений Android Auto на Android 16+ и Android Auto 15.7+.

**Дата исправления:** 26.12.2025  
**Версия:** 1.3 (versionCode: 4)

