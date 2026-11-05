@echo off
cd /d "%~dp0"
if exist "app\build\outputs\apk\release\app-release.apk" (
    copy /Y "app\build\outputs\apk\release\app-release.apk" "Cam_assist_v1.26.apk" >nul 2>&1
    if exist "Cam_assist_v1.26.apk" (
        echo APK успешно скопирован: Cam_assist_v1.26.apk
        dir "Cam_assist_v1.26.apk" | findstr "Cam_assist"
    ) else (
        echo Ошибка: файл не скопирован
    )
) else (
    echo APK файл не найден: app\build\outputs\apk\release\app-release.apk
    echo Запускаю сборку...
    call gradlew.bat :app:assembleRelease -x lintVitalAnalyzeRelease --no-daemon
    if exist "app\build\outputs\apk\release\app-release.apk" (
        copy /Y "app\build\outputs\apk\release\app-release.apk" "Cam_assist_v1.26.apk" >nul 2>&1
        echo APK скопирован после сборки
    )
)
