#!/usr/bin/env python3
import shutil
import os
import sys

source = r"app\build\outputs\apk\release\app-release.apk"
dest = r"Cam_assist_v1.25.apk"

if os.path.exists(source):
    try:
        shutil.copy2(source, dest)
        if os.path.exists(dest):
            size = os.path.getsize(dest)
            print(f"APK успешно скопирован: {dest}")
            print(f"Размер: {size / (1024*1024):.2f} MB")
        else:
            print("Ошибка: файл не скопирован")
    except Exception as e:
        print(f"Ошибка копирования: {e}")
else:
    print(f"APK не найден: {source}")
