@echo off
REM Скрипт для сборки release версии приложения
echo ========================================
echo Сборка Android приложения для Android Auto
echo ========================================
echo.

REM Очистка предыдущей сборки
echo [1/3] Очистка предыдущей сборки...
call gradlew.bat clean
if %errorlevel% neq 0 (
    echo Ошибка при очистке!
    pause
    exit /b 1
)

REM Сборка release версии
echo [2/3] Сборка release версии...
call gradlew.bat :app:assembleRelease -x lintVitalAnalyzeRelease
if %errorlevel% neq 0 (
    echo Ошибка при сборке!
    pause
    exit /b 1
)

REM Копирование APK в корневую папку
echo [3/3] Копирование APK файла...
if exist "app\build\outputs\apk\release\app-release.apk" (
    REM Получаем версию из build.gradle.kts (нужно обновлять вручную при изменении версии)
    set VERSION_NAME=1.12
    set VERSION_CODE=13
    set APK_NAME=Cam_assist_v%VERSION_NAME%.apk
    copy "app\build\outputs\apk\release\app-release.apk" "%APK_NAME%"
    echo.
    echo ========================================
    echo Сборка завершена успешно!
    echo ========================================
    echo.
    echo APK файл: %APK_NAME%
    echo Версия: %VERSION_NAME% (Code: %VERSION_CODE%)
    echo Расположение: %CD%\%APK_NAME%
    echo.
    echo Готово к распространению!
    echo.
) else (
    echo Ошибка: APK файл не найден!
    pause
    exit /b 1
)

pause
