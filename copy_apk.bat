@echo off
cd /d "%~dp0"
if exist "app\build\outputs\apk\release\app-release.apk" (
    copy /Y "app\build\outputs\apk\release\app-release.apk" "Cam_assist_v1.24.apk"
    echo APK copied successfully!
    dir "Cam_assist_v1.24.apk"
) else (
    echo APK not found. Building...
    call gradlew.bat :app:assembleRelease -x lintVitalAnalyzeRelease
    if exist "app\build\outputs\apk\release\app-release.apk" (
        copy /Y "app\build\outputs\apk\release\app-release.apk" "Cam_assist_v1.24.apk"
        echo APK copied successfully!
        dir "Cam_assist_v1.24.apk"
    )
)
