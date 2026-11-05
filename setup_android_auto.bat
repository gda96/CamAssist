@echo off
REM АГРЕССИВНЫЙ скрипт для ПРИНУДИТЕЛЬНОЙ настройки Android Auto
REM Требует: ADB, режим разработчика на устройстве, режим разработчика Android Auto

set PACKAGE_NAME=com.example.cam_assist
set ANDROID_AUTO_PACKAGE=com.google.android.projection.gearhead

echo ========================================
echo АГРЕССИВНАЯ настройка Android Auto
echo ========================================
echo.

echo [1/10] Проверка подключения устройства...
adb devices
if %errorlevel% neq 0 (
    echo ОШИБКА: Устройство не подключено или ADB не найден
    pause
    exit /b 1
)
echo.

echo [2/10] Проверка режима разработчика Android Auto...
adb shell settings get global car_app_installer_mode
echo Если вернуло "null" или "0" - режим разработчика НЕ включен!
echo.
pause
echo.

echo [3/10] Удаление ВСЕХ версий приложения...
adb uninstall %PACKAGE_NAME%
adb shell pm clear %PACKAGE_NAME%
echo.

echo [4/10] Установка новой версии приложения...
adb install -r app\build\outputs\apk\release\app-release.apk
if %errorlevel% neq 0 (
    echo ОШИБКА: Не удалось установить приложение
    pause
    exit /b 1
)
echo.

echo [5/10] Предоставление ВСЕХ разрешений Android Auto...
adb shell pm grant %ANDROID_AUTO_PACKAGE% android.permission.WRITE_SECURE_SETTINGS
adb shell pm grant %ANDROID_AUTO_PACKAGE% android.permission.WRITE_SETTINGS
adb shell pm grant %ANDROID_AUTO_PACKAGE% android.permission.READ_LOGS
echo.

echo [6/10] Включение режима установщика приложений...
adb shell settings put global car_app_installer_mode 1
adb shell settings put global car_app_installer_mode_enabled true
echo.

echo [7/10] Добавление приложения в белый список Android Auto...
adb shell settings put global car_app_whitelist %PACKAGE_NAME%
adb shell settings put global car_app_whitelist_enabled true
echo.

echo [8/10] Принудительное обновление списка приложений...
adb shell am broadcast -a android.intent.action.MY_PACKAGE_REPLACED
adb shell am broadcast -a android.intent.action.PACKAGE_REPLACED -d package:%PACKAGE_NAME%
adb shell am broadcast -a android.intent.action.PACKAGE_ADDED -d package:%PACKAGE_NAME%
echo.

echo [9/10] ПОЛНАЯ очистка Android Auto...
adb shell pm clear %ANDROID_AUTO_PACKAGE%
adb shell pm clear com.google.android.gms
timeout /t 2 /nobreak >nul
echo.

echo [10/10] Принудительная остановка и перезапуск Android Auto...
adb shell am force-stop %ANDROID_AUTO_PACKAGE%
adb shell am force-stop com.google.android.gms
timeout /t 3 /nobreak >nul
adb shell am start -n %ANDROID_AUTO_PACKAGE%/.ui.MainActivity
echo.

echo ========================================
echo ПРОВЕРКА КОНФИГУРАЦИИ
echo ========================================
echo.

echo Проверка регистрации сервиса:
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"CarAppService" /C:"androidx.car.app"
echo.

echo Проверка белого списка:
adb shell settings get global car_app_whitelist
echo.

echo Проверка режима установщика:
adb shell settings get global car_app_installer_mode
echo.

echo Проверка queries:
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"queries" /C:"gearhead"
echo.

echo ========================================
echo Настройка завершена!
echo ========================================
echo.
echo КРИТИЧЕСКИ ВАЖНО:
echo 1. Режим разработчика Android Auto ДОЛЖЕН быть включен:
echo    - Настройки ^> Android Auto ^> Версия (нажмите 7 раз)
echo    - Меню ^> Для разработчиков ^> Неизвестные источники (ВКЛЮЧИТЕ!)
echo.
echo 2. Перезагрузите устройство (РЕКОМЕНДУЕТСЯ)
echo.
echo 3. После перезагрузки откройте Android Auto
echo.
echo 4. Подождите 30 секунд (Android Auto сканирует приложения)
echo.
echo 5. Проверьте: Настройки Android Auto ^> Настройка панели запуска
echo.
echo ========================================
echo.
echo Если приложение ВСЕ ЕЩЕ не появилось, запустите диагностику:
echo adb shell dumpsys package %PACKAGE_NAME% ^> diagnostic.txt
echo.
pause

