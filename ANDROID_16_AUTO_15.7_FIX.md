# Исправление для Android 16 и Android Auto 15.7+

## Проблема
Приложение не отображается в списке доступных приложений Android Auto на устройствах с Android 16 и Android Auto 15.7.654634-release и выше.

## Критически важные настройки

### 1. Queries в манифесте (ОБЯЗАТЕЛЬНО для Android 11+)

```xml
<queries>
    <package android:name="com.google.android.projection.gearhead" />
    <intent>
        <action android:name="androidx.car.app.CarAppService" />
    </intent>
</queries>
```

**Без этого блока Android Auto НЕ СМОЖЕТ обнаружить приложение на Android 11+!**

### 2. Полная конфигурация сервиса

```xml
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
```

### 3. Минимальный уровень API

Для Android Auto 15.7+ рекомендуется использовать `minCarApiLevel="2"` вместо `1`.

### 4. Версия библиотеки

Используется `androidx.car.app:app:1.4.0` - последняя стабильная версия.

## Важные моменты для Android 16 и Android Auto 15.7+

### 1. Режим разработчика ОБЯЗАТЕЛЕН

Android Auto 15.7+ по умолчанию показывает **ТОЛЬКО** приложения из Google Play Store.

**Для разработки необходимо:**

1. Откройте **Настройки → Android Auto**
2. Нажмите на **"Версия"** **7 раз подряд**
3. Появится сообщение "Вы стали разработчиком!"
4. Нажмите на **три точки** (меню) → **"Для разработчиков"**
5. Включите **"Неизвестные источники"**
6. **Перезапустите Android Auto** (полностью закройте и откройте)

### 2. Установка приложения

```bash
# Удалить старую версию
adb uninstall com.example.cam_assist

# Установить новую версию
adb install app/build/outputs/apk/release/app-release.apk
```

### 3. После установки

1. **Полностью закройте Android Auto** (не просто сверните)
2. **Откройте Android Auto снова**
3. Подождите 10-15 секунд
4. Откройте **Настройки → Настройка панели запуска**
5. Приложение должно появиться в списке

### 4. Если приложение все еще не появляется

#### Проверка через ADB:

```bash
# Проверка регистрации сервиса
adb shell dumpsys package com.example.cam_assist | grep -A 20 "CarAppService"

# Проверка queries
adb shell dumpsys package com.example.cam_assist | grep -A 10 "queries"

# Логи Android Auto
adb logcat | grep -i "car.app\|gearhead\|cam_assist"
```

#### Дополнительные шаги:

1. **Очистите кэш Android Auto:**
   - Настройки → Приложения → Android Auto → Хранилище → Очистить кэш

2. **Перезагрузите устройство**

3. **Проверьте версию Android Auto:**
   - Должна быть 15.7.654634-release или выше
   - Обновите из Google Play Store, если нужно

4. **Проверьте версию Android:**
   - Должна быть Android 11+ (API 30+)
   - Для Android 16 требуется полная конфигурация

## Известные ограничения Android Auto 15.7+

1. **Только приложения из Play Store** - по умолчанию
2. **Режим разработчика обязателен** - для тестирования
3. **Строгая проверка** - все метаданные должны быть правильными
4. **Queries обязательны** - для Android 11+

## Проверка конфигурации

### Чек-лист:

- [ ] Queries добавлены в манифест
- [ ] Сервис экспортирован (`android:exported="true"`)
- [ ] Сервис включен (`android:enabled="true"`)
- [ ] Intent-filter правильный
- [ ] Все метаданные присутствуют
- [ ] minCarApiLevel = 2
- [ ] Категория = PRODUCTIVITY
- [ ] Описание добавлено
- [ ] Иконка настроена
- [ ] Режим разработчика включен
- [ ] Неизвестные источники включены
- [ ] Android Auto перезапущен

## Версия приложения

- **Версия:** 1.4
- **Version Code:** 5
- **Дата:** 26.12.2025

## Если ничего не помогает

1. **Проверьте логи Android Auto:**
   ```bash
   adb logcat | grep -i "car.app\|gearhead\|cam_assist" > android_auto_log.txt
   ```

2. **Проверьте, что приложение установлено правильно:**
   ```bash
   adb shell pm list packages | grep cam_assist
   adb shell dumpsys package com.example.cam_assist
   ```

3. **Попробуйте на другом устройстве** (если возможно)

4. **Обратитесь к документации Google:**
   - https://developer.android.com/training/cars/apps

## Заключение

Для Android 16 и Android Auto 15.7+ требуется:
1. ✅ Полная конфигурация queries
2. ✅ Все метаданные
3. ✅ Режим разработчика включен
4. ✅ Правильная версия библиотеки (1.4.0)
5. ✅ minCarApiLevel = 2

**Приложение должно появиться в списке после выполнения всех шагов!**

