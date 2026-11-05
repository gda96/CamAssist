@echo off
REM Скрипт для проверки ВСЕХ настроек Android Auto

set PACKAGE_NAME=com.example.cam_assist
set ANDROID_AUTO_PACKAGE=com.google.android.projection.gearhead

echo ========================================
echo ПОЛНАЯ ПРОВЕРКА Android Auto
echo ========================================
echo.

echo [1] Проверка подключения устройства...
adb devices
if %errorlevel% neq 0 (
    echo ОШИБКА: Устройство не подключено!
    pause
    exit /b 1
)
echo.

echo [2] Проверка установки приложения...
adb shell pm list packages | findstr %PACKAGE_NAME%
if %errorlevel% neq 0 (
    echo ОШИБКА: Приложение НЕ установлено!
    echo Установите: adb install app\build\outputs\apk\release\app-release.apk
) else (
    echo OK: Приложение установлено
)
echo.

echo [3] Проверка регистрации CarAppService...
echo Вывод dumpsys package:
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"CarAppService" /C:"androidx.car.app" /C:"exported" /C:"enabled"
if %errorlevel% neq 0 (
    echo ОШИБКА: CarAppService не найден в выводе!
) else (
    echo OK: CarAppService найден
)
echo.

echo [4] Проверка queries...
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"queries" /C:"gearhead"
if %errorlevel% neq 0 (
    echo ОШИБКА: Queries не найдены!
) else (
    echo OK: Queries найдены
)
echo.

echo [5] Проверка режима разработчика Android Auto...
for /f "tokens=*" %%i in ('adb shell settings get global car_app_installer_mode') do set INSTALLER_MODE=%%i
echo Режим установщика: [%INSTALLER_MODE%]
if "%INSTALLER_MODE%"=="1" (
    echo OK: Режим разработчика включен
) else (
    echo ОШИБКА: Режим разработчика НЕ включен!
    echo.
    echo ВКЛЮЧИТЕ РЕЖИМ РАЗРАБОТЧИКА:
    echo 1. Настройки ^> Android Auto ^> Версия (нажмите 7 раз)
    echo 2. Меню ^> Для разработчиков ^> Неизвестные источники (включите)
    echo.
    echo Затем выполните:
    echo adb shell settings put global car_app_installer_mode 1
)
echo.

echo [6] Проверка белого списка...
for /f "tokens=*" %%i in ('adb shell settings get global car_app_whitelist') do set WHITELIST=%%i
echo Белый список: [%WHITELIST%]
echo %WHITELIST% | findstr %PACKAGE_NAME% >nul
if %errorlevel% equ 0 (
    echo OK: Приложение в белом списке
) else (
    echo ОШИБКА: Приложение НЕ в белом списке!
    echo.
    echo Добавьте в белый список:
    echo adb shell settings put global car_app_whitelist %PACKAGE_NAME%
)
echo.

echo [7] Проверка версии Android Auto...
adb shell dumpsys package %ANDROID_AUTO_PACKAGE% | findstr "versionName"
echo.

echo [8] Проверка версии Android...
adb shell getprop ro.build.version.release
adb shell getprop ro.build.version.sdk
echo.

echo [9] Проверка метаданных приложения...
echo Ищем метаданные androidx.car.app:
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"androidx.car.app" /C:"PRODUCTIVITY" /C:"minCarApiLevel"
echo.

echo [10] Проверка разрешений Android Auto...
adb shell pm list permissions -d -g | findstr "WRITE_SECURE_SETTINGS"
echo.

echo [11] Последние логи Android Auto (фильтр по car.app, gearhead, cam_assist)...
echo Сохраняем последние 50 строк логов...
adb logcat -d -t 50 | findstr /C:"car.app" /C:"gearhead" /C:"cam_assist" /C:"CarAppService"
echo.

echo ========================================
echo ИТОГОВАЯ ДИАГНОСТИКА
echo ========================================
echo.
echo Сохраните этот вывод для анализа!
echo.
echo Если приложение все еще не появляется:
echo 1. Убедитесь, что режим разработчика включен (шаг 5)
echo 2. Убедитесь, что приложение в белом списке (шаг 6)
echo 3. Перезагрузите устройство
echo 4. Подождите 30 секунд после открытия Android Auto
echo.
pause

