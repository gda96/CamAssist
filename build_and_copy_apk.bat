@echo off
chcp 65001 >nul
cd /d "%~dp0"
set VERSION=1.82
set VERSIONCODE=83
set APKNAME=Cam_assist_v%VERSION%.apk
set TARGET=RELEASE\Android_App\%APKNAME%

echo ========================================
echo Сборка и копирование Android приложения
echo Название: Обогрев камеры
echo Версия: %VERSION% (versionCode %VERSIONCODE%)
echo ========================================
echo.

echo [1/3] Очистка предыдущей сборки...
call gradlew.bat clean
echo.

echo [2/3] Сборка release APK...
call gradlew.bat :app:assembleRelease -x lintVitalAnalyzeRelease --no-daemon
echo.

if exist "app\build\outputs\apk\release\app-release.apk" (
    echo [3/3] Копирование в RELEASE\Android_App...
    if not exist "RELEASE\Android_App" mkdir "RELEASE\Android_App"
    copy /Y "app\build\outputs\apk\release\app-release.apk" "%TARGET%" >nul 2>&1
    if exist "%TARGET%" (
        echo.
        echo ========================================
        echo [УСПЕХ] Сборка завершена!
        echo ========================================
        echo Файл: %TARGET%
        for %%F in ("%TARGET%") do (
            echo Размер: %%~zF байт
            echo Дата: %%~tF
        )
        echo ========================================
    ) else (
        echo [ОШИБКА] Не удалось скопировать APK
    )
) else (
    echo [ОШИБКА] APK не найден после сборки
    echo Проверьте ошибки сборки выше
)
echo.
pause
