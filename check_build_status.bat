@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ========================================
echo Проверка статуса сборки Android приложения
echo ========================================
echo.

if exist "app\build\outputs\apk\release\app-release.apk" (
    echo [✓] APK найден!
    echo.
    for %%F in ("app\build\outputs\apk\release\app-release.apk") do (
        echo Файл: app-release.apk
        echo Размер: %%~zF байт
        echo Дата: %%~tF
    )
    echo.
    echo Копирую в корень проекта...
    copy /Y "app\build\outputs\apk\release\app-release.apk" "Cam_assist_v1.30.apk" >nul 2>&1
    if exist "Cam_assist_v1.30.apk" (
        echo [✓] APK скопирован: Cam_assist_v1.30.apk
        for %%F in ("Cam_assist_v1.30.apk") do (
            echo Размер: %%~zF байт
            echo Дата: %%~tF
        )
        echo.
        echo ========================================
        echo [УСПЕХ] Сборка завершена!
        echo ========================================
    ) else (
        echo [✗] Ошибка копирования
    )
) else (
    echo [✗] APK еще не готов
    echo.
    echo Сборка все еще выполняется...
    echo Подождите несколько минут и запустите скрипт снова.
)
echo.
pause
