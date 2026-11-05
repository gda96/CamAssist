@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ========================================
echo Проверка сборки Android приложения v1.30
echo ========================================
echo.

if exist "app\build\outputs\apk\release\app-release.apk" (
    echo [OK] APK найден!
    echo.
    for %%F in ("app\build\outputs\apk\release\app-release.apk") do (
        echo Файл: %%~nxF
        echo Размер: %%~zF байт
        echo Дата: %%~tF
    )
    echo.
    echo Копирую в корень проекта...
    copy /Y "app\build\outputs\apk\release\app-release.apk" "Cam_assist_v1.30.apk" >nul 2>&1
    if exist "Cam_assist_v1.30.apk" (
        echo.
        echo ========================================
        echo [УСПЕХ] APK успешно скопирован!
        echo ========================================
        echo Файл: Cam_assist_v1.30.apk
        for %%F in ("Cam_assist_v1.30.apk") do (
            echo Размер: %%~zF байт
            echo Дата: %%~tF
        )
        echo ========================================
    ) else (
        echo [ОШИБКА] Файл не скопирован
    )
) else (
    echo [ОШИБКА] APK файл не найден.
    echo.
    echo Возможные причины:
    echo 1. Сборка еще не завершена
    echo 2. Произошла ошибка при сборке
    echo.
    echo Проверьте процесс сборки в другом окне.
)
echo.
pause
