@echo off
REM Диагностический скрипт для проверки конфигурации Android Auto

set PACKAGE_NAME=com.example.cam_assist
set ANDROID_AUTO_PACKAGE=com.google.android.projection.gearhead

echo ========================================
echo ДИАГНОСТИКА Android Auto
echo ========================================
echo.

echo [1] Проверка установки приложения...
adb shell pm list packages | findstr %PACKAGE_NAME%
if %errorlevel% neq 0 (
    echo ОШИБКА: Приложение не установлено!
) else (
    echo OK: Приложение установлено
)
echo.

echo [2] Проверка регистрации CarAppService...
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"CarAppService" /C:"androidx.car.app"
if %errorlevel% neq 0 (
    echo ОШИБКА: CarAppService не зарегистрирован!
) else (
    echo OK: CarAppService зарегистрирован
)
echo.

echo [3] Проверка queries...
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"queries" /C:"gearhead"
if %errorlevel% neq 0 (
    echo ОШИБКА: Queries не найдены!
) else (
    echo OK: Queries найдены
)
echo.

echo [4] Проверка режима разработчика Android Auto...
set INSTALLER_MODE=
for /f "tokens=*" %%i in ('adb shell settings get global car_app_installer_mode') do set INSTALLER_MODE=%%i
echo Режим установщика: %INSTALLER_MODE%
if "%INSTALLER_MODE%"=="1" (
    echo OK: Режим разработчика включен
) else (
    echo ОШИБКА: Режим разработчика НЕ включен!
    echo Выполните: adb shell settings put global car_app_installer_mode 1
)
echo.

echo [5] Проверка белого списка...
set WHITELIST=
for /f "tokens=*" %%i in ('adb shell settings get global car_app_whitelist') do set WHITELIST=%%i
echo Белый список: %WHITELIST%
echo %WHITELIST% | findstr %PACKAGE_NAME% >nul
if %errorlevel% equ 0 (
    echo OK: Приложение в белом списке
) else (
    echo ОШИБКА: Приложение НЕ в белом списке!
    echo Выполните: adb shell settings put global car_app_whitelist %PACKAGE_NAME%
)
echo.

echo [6] Проверка версии Android Auto...
adb shell dumpsys package %ANDROID_AUTO_PACKAGE% | findstr "versionName"
echo.

echo [7] Проверка метаданных приложения...
adb shell dumpsys package %PACKAGE_NAME% | findstr /C:"androidx.car.app" /C:"PRODUCTIVITY"
echo.

echo [8] Последние логи Android Auto (последние 20 строк)...
adb logcat -d | findstr /C:"car.app" /C:"gearhead" /C:"cam_assist" | more
echo.

echo ========================================
echo Диагностика завершена
echo ========================================
echo.
echo Сохраните вывод этого скрипта для анализа
echo.
pause

