# Имитация установки из Google Play Store для Android Auto

## Важно

**Технически невозможно полностью имитировать установку из Play Store через манифест.** Android Auto проверяет сертификат подписи и источник установки на системном уровне через Google Play Services.

Однако, можно использовать специальные команды ADB для настройки устройства, чтобы Android Auto распознал приложение как установленное из Play Store.

## Что было добавлено

### 1. Метаданные для Play Store совместимости

В `AndroidManifest.xml` добавлены:
- `com.google.android.gms.version` - версия Google Play Services
- `com.google.android.gms.car.application` - флаг приложения для Android Auto

### 2. Создан скрипт автоматической настройки

Файл `setup_android_auto.bat` автоматически:
- Устанавливает приложение
- Настраивает Android Auto через ADB
- Добавляет приложение в белый список
- Очищает кэш Android Auto

## Использование скрипта

### Windows:

```bash
# Просто запустите скрипт
setup_android_auto.bat
```

### Вручную через ADB:

```bash
# 1. Установка приложения
adb uninstall com.example.cam_assist
adb install app\build\outputs\apk\release\app-release.apk

# 2. Настройка Android Auto (требует режим разработчика)
adb shell pm grant com.google.android.projection.gearhead android.permission.WRITE_SECURE_SETTINGS
adb shell settings put global car_app_installer_mode 1
adb shell settings put global car_app_whitelist com.example.cam_assist

# 3. Очистка кэша Android Auto
adb shell pm clear com.google.android.projection.gearhead
adb shell am force-stop com.google.android.projection.gearhead
```

## Обязательные шаги

### 1. Включите режим разработчика Android Auto

**КРИТИЧНО:** Без этого ничего не сработает!

1. Откройте **Настройки → Android Auto**
2. Нажмите на **"Версия"** **7 раз подряд**
3. Появится сообщение "Вы стали разработчиком!"
4. Нажмите на **три точки** (меню) → **"Для разработчиков"**
5. Включите **"Неизвестные источники"**

### 2. Включите режим разработчика на устройстве

1. Откройте **Настройки → О телефоне**
2. Нажмите на **"Номер сборки"** **7 раз**
3. Вернитесь в **Настройки → Система → Для разработчиков**
4. Включите **"Отладка по USB"**

### 3. Запустите скрипт настройки

```bash
setup_android_auto.bat
```

### 4. После выполнения скрипта

1. **Откройте Android Auto** на устройстве
2. **Подождите 10-15 секунд**
3. Откройте **Настройки → Настройка панели запуска**
4. Приложение должно появиться в списке доступных приложений

## Проверка через ADB

### Проверка регистрации сервиса:

```bash
adb shell dumpsys package com.example.cam_assist | grep -A 30 "CarAppService"
```

### Проверка белого списка:

```bash
adb shell settings get global car_app_whitelist
```

Должно содержать: `com.example.cam_assist`

### Проверка режима установщика:

```bash
adb shell settings get global car_app_installer_mode
```

Должно вернуть: `1`

### Логи Android Auto:

```bash
adb logcat | grep -i "car.app\|gearhead\|cam_assist"
```

## Если приложение все еще не появляется

### 1. Проверьте все шаги:

- [ ] Режим разработчика Android Auto включен
- [ ] "Неизвестные источники" включены
- [ ] Режим разработчика устройства включен
- [ ] "Отладка по USB" включена
- [ ] Скрипт выполнен успешно
- [ ] Android Auto перезапущен

### 2. Дополнительные команды:

```bash
# Принудительное обновление списка приложений
adb shell am broadcast -a android.intent.action.MY_PACKAGE_REPLACED -n com.google.android.projection.gearhead/.CarAppService

# Перезапуск Android Auto с очисткой
adb shell am force-stop com.google.android.projection.gearhead
adb shell pm clear com.google.android.projection.gearhead
adb shell am start -n com.google.android.projection.gearhead/com.google.android.projection.gearhead.ui.MainActivity
```

### 3. Проверка версии Android Auto:

```bash
adb shell dumpsys package com.google.android.projection.gearhead | grep versionName
```

Должна быть 15.7.654634-release или выше.

### 4. Альтернативный метод (требует root):

Если у вас есть root доступ:

```bash
# Установка как системное приложение
adb root
adb remount
adb push app\build\outputs\apk\release\app-release.apk /system/app/CamAssist/CamAssist.apk
adb shell chmod 644 /system/app/CamAssist/CamAssist.apk
adb reboot
```

**ВНИМАНИЕ:** Это может повредить устройство! Используйте только на тестовых устройствах.

## Версия приложения

- **Версия:** 1.7
- **Version Code:** 8
- **Дата:** 26.12.2025

## Что делает скрипт

1. **Проверяет подключение** устройства через ADB
2. **Удаляет старую версию** приложения (если есть)
3. **Устанавливает новую версию** APK
4. **Предоставляет разрешения** Android Auto для записи настроек
5. **Включает режим установщика** приложений (`car_app_installer_mode = 1`)
6. **Добавляет приложение в белый список** Android Auto
7. **Очищает кэш** Android Auto
8. **Останавливает Android Auto** для перезапуска

## Ограничения

1. **Не работает без режима разработчика** - Android Auto 15.7+ требует явного включения режима разработчика
2. **Требует ADB** - нужен компьютер с ADB и USB подключение
3. **Не постоянное решение** - после перезагрузки устройства может потребоваться повторная настройка
4. **Только для разработки** - для продакшена нужно публиковать в Play Store

## Для продакшена

**Единственный способ гарантировать отображение в Android Auto без режима разработчика - публикация в Google Play Store.**

После публикации в Play Store:
- Приложение автоматически появится в Android Auto
- Режим разработчика не требуется
- Пользователи смогут найти приложение в настройках панели запуска

## Заключение

Скрипт `setup_android_auto.bat` автоматизирует процесс настройки Android Auto для распознавания приложения. После выполнения всех шагов приложение должно появиться в списке доступных приложений Android Auto.

**Статус:** ✅ Скрипт создан, приложение пересобрано с метаданными Play Store
