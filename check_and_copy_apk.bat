@echo off
cd /d "%~dp0"
echo Проверка сборки Android приложения...
echo.

if exist "app\build\outputs\apk\release\app-release.apk" (
    echo APK найден!
    echo.
    dir "app\build\outputs\apk\release\app-release.apk" | findstr "app-release"
    echo.
    echo Копирую в корень проекта...
    copy /Y "app\build\outputs\apk\release\app-release.apk" "Cam_assist_v1.27.apk" >nul 2>&1
    if exist "Cam_assist_v1.27.apk" (
        echo.
        echo ========================================
        echo APK успешно скопирован!
        echo ========================================
        echo Файл: Cam_assist_v1.27.apk
        dir "Cam_assist_v1.27.apk" | findstr "Cam_assist"
        echo ========================================
    ) else (
        echo Ошибка: файл не скопирован
    )
) else (
    echo APK файл не найден.
    echo Возможно, сборка еще не завершена.
    echo Проверьте процесс сборки в другом окне.
)
echo.
pause
