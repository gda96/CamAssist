# Настройка Android Auto для разработки

## Проблема: Приложение не появляется в списке Android Auto

Android Auto по умолчанию показывает **только приложения из Google Play Store**. Для разработки и тестирования собственных приложений необходимо включить режим разработчика.

## Решение: Включение режима разработчика Android Auto

### Шаг 1: Включение режима разработчика

1. Откройте **Настройки** на вашем Android устройстве
2. Найдите и откройте **Android Auto**
3. Прокрутите вниз до раздела **О приложении**
4. Нажмите на **Версия** (или **Version**) **7 раз подряд**
5. Появится сообщение "Вы стали разработчиком!" (или "You are now a developer!")

### Шаг 2: Включение поддержки неизвестных источников

1. В настройках Android Auto найдите раздел **Для разработчиков** (Developer options)
2. Прокрутите вниз до опции **Неизвестные источники** (Unknown sources)
3. **Включите** эту опцию
4. Подтвердите предупреждение о безопасности

### Шаг 3: Установка приложения

1. Установите APK на устройство:
   ```bash
   adb install -r app/build/outputs/apk/release/app-release.apk
   ```

2. Или скопируйте APK на устройство и установите вручную

### Шаг 4: Перезапуск Android Auto

1. **Полностью закройте** Android Auto (не просто сверните)
2. Или перезагрузите устройство
3. Откройте Android Auto снова

### Шаг 5: Проверка в настройках панели запуска

1. Откройте **Настройки Android Auto**
2. Перейдите в **Настройка панели запуска** (или **Customize launcher**)
3. Приложение **"Обогрев камеры"** должно появиться в списке **доступных приложений**
4. Приложение должно отображаться с:
   - Правильным названием
   - Описанием (если поддерживается)
   - Иконкой
   - Категорией "PRODUCTIVITY"

## Альтернативный способ (через ADB)

Если настройки недоступны, можно включить режим разработчика через ADB:

```bash
# Включить режим разработчика
adb shell pm grant com.google.android.projection.gearhead android.permission.WRITE_SECURE_SETTINGS

# Включить неизвестные источники
adb shell settings put global car_app_installer_mode 1
```

## Проверка конфигурации приложения

Убедитесь, что в `AndroidManifest.xml` правильно настроен сервис с полной конфигурацией:

```xml
<service
    android:name=".car.CarAppService"
    android:exported="true"
    android:enabled="true"
    android:label="@string/app_name">
    <intent-filter>
        <action android:name="androidx.car.app.CarAppService" />
    </intent-filter>
    <!-- Минимальный уровень API для Android Auto -->
    <meta-data
        android:name="androidx.car.app.minCarApiLevel"
        android:value="1" />
    <!-- Категория приложения для Android Auto -->
    <meta-data
        android:name="androidx.car.app.category"
        android:value="PRODUCTIVITY" />
    <!-- Описание приложения для Android Auto -->
    <meta-data
        android:name="androidx.car.app.description"
        android:value="@string/car_app_description" />
    <!-- Иконка приложения для Android Auto -->
    <meta-data
        android:name="androidx.car.app.icon"
        android:resource="@mipmap/ic_launcher" />
</service>
```

### Ключевые элементы конфигурации:

1. **`android:exported="true"`** - обязательно для Android Auto
2. **`android:enabled="true"`** - сервис должен быть включен
3. **`android:label`** - название приложения в Android Auto
4. **`androidx.car.app.category`** - категория приложения (PRODUCTIVITY, COMMUNICATION, etc.)
5. **`androidx.car.app.description`** - описание приложения (помогает в отображении)
6. **`androidx.car.app.icon`** - иконка приложения (опционально, но рекомендуется)

### Строковые ресурсы:

В `res/values/strings.xml` должно быть:

```xml
<string name="app_name">Обогрев камеры</string>
<string name="car_app_description">Управление системой обогрева камеры ассистентов. Мониторинг температуры и влажности, автоматическое и ручное управление обогревом.</string>
```

## Устранение проблем

### Приложение все еще не появляется

1. **Проверьте логи Android Auto:**
   ```bash
   adb logcat | grep -i "car.app"
   ```

2. **Очистите кэш Android Auto:**
   - Настройки → Приложения → Android Auto → Хранилище → Очистить кэш

3. **Переустановите приложение:**
   ```bash
   adb uninstall com.example.cam_assist
   adb install app/build/outputs/apk/release/app-release.apk
   ```

4. **Проверьте версию Android Auto:**
   - Убедитесь, что установлена последняя версия из Google Play

5. **Проверьте версию Android:**
   - Android Auto требует Android 6.0 (API 23) или выше
   - Car App Library требует Android 8.0 (API 26) или выше

### Проверка через ADB

Проверьте, что сервис зарегистрирован:

```bash
adb shell dumpsys package com.example.cam_assist | grep -A 10 "CarAppService"
```

## Важные замечания

⚠️ **Режим разработчика Android Auto:**
- Включает поддержку приложений из неизвестных источников
- Необходим только для разработки и тестирования
- Для публикации в Google Play Store режим разработчика не требуется

⚠️ **Безопасность:**
- Режим разработчика снижает уровень безопасности
- Используйте только на тестовых устройствах
- Не включайте на устройствах с важными данными

## Для продакшена

После публикации в Google Play Store:
- Приложение автоматически появится в Android Auto
- Режим разработчика не требуется
- Пользователи смогут найти приложение в настройках панели запуска





