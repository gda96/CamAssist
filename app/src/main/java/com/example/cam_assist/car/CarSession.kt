package com.example.cam_assist.car

import androidx.car.app.Session
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner

class CarSession : Session(), DefaultLifecycleObserver {
    override fun onCreateScreen(intent: android.content.Intent): androidx.car.app.Screen {
        return CarMainScreen(carContext)
    }

    override fun onCreate(owner: LifecycleOwner) {
        super.onCreate(owner)
    }
}

