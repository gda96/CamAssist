# ФИНАЛЬНОЕ РЕШЕНИЕ: Приложение не появляется в Android Auto

## КРИТИЧЕСКИ ВАЖНО

Если приложение **ВСЕ ЕЩЕ НЕ ПОЯВЛЯЕТСЯ** в списке, выполните ВСЕ шаги ниже **СТРОГО ПО ПОРЯДКУ**.

## ШАГ 1: Диагностика (ОБЯЗАТЕЛЬНО!)

Запустите диагностический скрипт:

```bash
diagnose_android_auto.bat
```

Этот скрипт покажет:
- Установлено ли приложение
- Зарегистрирован ли CarAppService
- Настроены ли queries
- Включен ли режим разработчика
- Есть ли приложение в белом списке

**СОХРАНИТЕ ВЫВОД СКРИПТА!**

## ШАГ 2: Проверка режима разработчика Android Auto

### На устройстве:

1. Откройте **Настройки → Android Auto**
2. Прокрутите вниз до **"О приложении"** или **"Версия"**
3. Нажмите на **"Версия"** **7 РАЗ ПОДРЯД** (не быстро, но и не медленно)
4. Должно появиться сообщение: **"Вы стали разработчиком!"**
5. Нажмите на **три точки** (меню) в правом верхнем углу
6. Выберите **"Для разработчиков"**
7. Прокрутите вниз и **ВКЛЮЧИТЕ** **"Неизвестные источники"**
8. Подтвердите предупреждение

### Проверка через ADB:

```bash
adb shell settings get global car_app_installer_mode
```

**Должно вернуть:** `1`

Если вернуло `null` или `0` - режим разработчика **НЕ ВКЛЮЧЕН!**

## ШАГ 3: Запуск агрессивного скрипта настройки

```bash
setup_android_auto.bat
```

Этот скрипт:
- Удалит старую версию
- Установит новую версию
- Настроит все разрешения
- Добавит в белый список
- Очистит кэш
- Перезапустит Android Auto

## ШАГ 4: ПЕРЕЗАГРУЗКА УСТРОЙСТВА

**КРИТИЧЕСКИ ВАЖНО!**

После выполнения скрипта:

1. **ПЕРЕЗАГРУЗИТЕ УСТРОЙСТВО** (не просто перезапустите Android Auto)
2. Подождите полной загрузки
3. Откройте Android Auto
4. Подождите **30 СЕКУНД** (Android Auto сканирует приложения)

## ШАГ 5: Проверка в настройках

1. Откройте **Настройки Android Auto**
2. Перейдите в **"Настройка панели запуска"**
3. Прокрутите список вниз
4. Приложение должно быть в списке **"Добавить ярлык на панель запуска"**

## Если ВСЕ ЕЩЕ не появляется

### Вариант A: Проверка через ADB вручную

```bash
# 1. Проверка регистрации сервиса
adb shell dumpsys package com.example.cam_assist | findstr "CarAppService"

# Должно показать:
# - android:name=".car.CarAppService"
# - android:exported="true"
# - android:enabled="true"
# - action: androidx.car.app.CarAppService

# 2. Проверка queries
adb shell dumpsys package com.example.cam_assist | findstr "queries"

# Должно показать:
# - package: com.google.android.projection.gearhead
# - action: androidx.car.app.CarAppService

# 3. Проверка метаданных
adb shell dumpsys package com.example.cam_assist | findstr "androidx.car.app"

# Должно показать все метаданные

# 4. Принудительное обновление
adb shell am broadcast -a android.intent.action.MY_PACKAGE_REPLACED
adb shell am broadcast -a android.intent.action.PACKAGE_REPLACED -d package:com.example.cam_assist
```

### Вариант B: Полная переустановка

```bash
# 1. Полное удаление
adb uninstall com.example.cam_assist
adb shell pm clear com.example.cam_assist

# 2. Очистка Android Auto
adb shell pm clear com.google.android.projection.gearhead
adb shell pm clear com.google.android.gms

# 3. Перезагрузка устройства
adb reboot

# 4. После перезагрузки - установка
adb install app\build\outputs\apk\release\app-release.apk

# 5. Настройка
setup_android_auto.bat
```

### Вариант C: Проверка версии Android Auto

```bash
adb shell dumpsys package com.google.android.projection.gearhead | findstr "versionName"
```

**Должна быть:** `15.7.654634-release` или выше

Если версия ниже - **ОБНОВИТЕ Android Auto из Play Store!**

### Вариант D: Альтернативный метод (требует root)

**ВНИМАНИЕ:** Только для тестовых устройств!

```bash
# Установка как системное приложение
adb root
adb remount
adb shell mkdir -p /system/app/CamAssist
adb push app\build\outputs\apk\release\app-release.apk /system/app/CamAssist/CamAssist.apk
adb shell chmod 644 /system/app/CamAssist/CamAssist.apk
adb shell chown system:system /system/app/CamAssist/CamAssist.apk
adb reboot
```

## Возможные причины проблемы

### 1. Режим разработчика не включен
- **Решение:** Включите через настройки Android Auto (7 раз на "Версия")

### 2. "Неизвестные источники" не включены
- **Решение:** Включите в настройках Android Auto → Для разработчиков

### 3. Android Auto не обновлен
- **Решение:** Обновите из Play Store до версии 15.7+

### 4. Кэш Android Auto не очищен
- **Решение:** Настройки → Приложения → Android Auto → Хранилище → Очистить кэш

### 5. Устройство не перезагружено
- **Решение:** ПЕРЕЗАГРУЗИТЕ устройство после установки

### 6. Недостаточно времени для сканирования
- **Решение:** Подождите 30-60 секунд после открытия Android Auto

### 7. Неправильная конфигурация манифеста
- **Решение:** Проверьте через `diagnose_android_auto.bat`

## Чек-лист перед обращением за помощью

- [ ] Режим разработчика Android Auto включен (проверено через ADB)
- [ ] "Неизвестные источники" включены
- [ ] Режим разработчика устройства включен
- [ ] "Отладка по USB" включена
- [ ] Приложение установлено (проверено через `adb shell pm list packages`)
- [ ] CarAppService зарегистрирован (проверено через `dumpsys package`)
- [ ] Queries настроены (проверено через `dumpsys package`)
- [ ] Белый список настроен (проверено через `settings get global car_app_whitelist`)
- [ ] Android Auto обновлен до версии 15.7+
- [ ] Устройство перезагружено после установки
- [ ] Прошло 30+ секунд после открытия Android Auto
- [ ] Диагностический скрипт выполнен и вывод сохранен

## Версия приложения

- **Версия:** 1.8
- **Version Code:** 9
- **Дата:** 26.12.2025

## Файлы для диагностики

1. **`setup_android_auto.bat`** - агрессивный скрипт настройки
2. **`diagnose_android_auto.bat`** - диагностический скрипт
3. **`app-release.apk`** - готовое приложение

## Если НИЧЕГО не помогает

1. **Сохраните вывод диагностики:**
   ```bash
   diagnose_android_auto.bat > diagnostic_output.txt
   ```

2. **Сохраните логи Android Auto:**
   ```bash
   adb logcat -d | findstr "car.app gearhead cam_assist" > android_auto_logs.txt
   ```

3. **Сохраните информацию о пакете:**
   ```bash
   adb shell dumpsys package com.example.cam_assist > package_info.txt
   ```

4. **Проверьте версию Android:**
   ```bash
   adb shell getprop ro.build.version.release
   adb shell getprop ro.build.version.sdk
   ```

5. **Обратитесь с этими файлами** к разработчикам или в поддержку

## Заключение

Если после выполнения **ВСЕХ** шагов приложение все еще не появляется, проблема может быть в:
- Слишком строгих настройках безопасности Android 16
- Особенностях конкретной версии Android Auto
- Ограничениях производителя устройства

**Единственное 100% решение:** Публикация в Google Play Store

