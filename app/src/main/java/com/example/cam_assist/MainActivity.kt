package com.example.cam_assist

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.View
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.cam_assist.ui.screen.DeviceScreen
import com.example.cam_assist.ui.theme.Cam_assistTheme
import com.example.cam_assist.ui.viewmodel.DeviceViewModel
import com.example.cam_assist.ui.viewmodel.DeviceViewModelFactory

class MainActivity : ComponentActivity() {
    private var hasPermissions by mutableStateOf(false)
    
    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        hasPermissions = permissions.all { it.value }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        
        // Полноэкранный режим - скрываем системные шторки
        setupFullScreen()
        
        // Добавляем слушатель для автоматического скрытия системных панелей
        setupSystemBarsListener()
        
        // Запрашиваем разрешения
        requestPermissions()
        
        val viewModelFactory = DeviceViewModelFactory(application)
        
        setContent {
            Cam_assistTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    if (hasPermissions) {
                        // Обработчик кнопки "Назад" для корректного сворачивания
                        BackHandler {
                            moveTaskToBack(true)
                        }
                        DeviceScreen(
                            viewModel = viewModel { viewModelFactory.create(DeviceViewModel::class.java) }
                        )
                    } else {
                        // Показываем экран запроса разрешений
                        PermissionRequestScreen {
                            requestPermissions()
                        }
                    }
                }
            }
        }
    }
    
    private fun requestPermissions() {
        val permissions = mutableListOf<String>().apply {
            add(Manifest.permission.INTERNET)
            add(Manifest.permission.ACCESS_NETWORK_STATE)
            add(Manifest.permission.ACCESS_WIFI_STATE)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                // Для Android 10+ может потребоваться дополнительное разрешение
            } else {
                add(Manifest.permission.CHANGE_WIFI_STATE)
            }
        }
        
        val permissionsToRequest = permissions.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        
        if (permissionsToRequest.isNotEmpty()) {
            requestPermissionLauncher.launch(permissionsToRequest.toTypedArray())
        } else {
            hasPermissions = true
        }
    }
    
    private fun setupFullScreen() {
        // Полноэкранный режим - скрываем все системные шторки
        WindowCompat.setDecorFitsSystemWindows(window, false)
        
        val windowInsetsController = WindowCompat.getInsetsController(window, window.decorView)
        
        // Скрываем системные бары (статус бар и навигационная панель)
        windowInsetsController?.apply {
            hide(WindowInsetsCompat.Type.statusBars())
            hide(WindowInsetsCompat.Type.navigationBars())
            // Позволяем показывать бары при свайпе для корректного сворачивания
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
        
        // Дополнительно для старых версий Android (до API 30)
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            window.decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            )
        }
        
        // Для Android 11+ (API 30+) используем более надежный метод
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false)
            window.insetsController?.let { controller ->
                controller.hide(android.view.WindowInsets.Type.statusBars() or android.view.WindowInsets.Type.navigationBars())
                controller.systemBarsBehavior = android.view.WindowInsetsController.BEHAVIOR_DEFAULT
            }
        }
        
        // Сохраняем экран включенным
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    }
    
    private fun setupSystemBarsListener() {
        // Периодическая проверка и автоматическое скрытие системных панелей
        val handler = android.os.Handler(android.os.Looper.getMainLooper())
        val checkRunnable = object : Runnable {
            override fun run() {
                val windowInsetsController = WindowCompat.getInsetsController(window, window.decorView)
                val rootView = window.decorView.rootView
                
                // Проверяем видимость системных панелей через WindowInsets
                val insets = ViewCompat.getRootWindowInsets(rootView)
                if (insets != null) {
                    val statusBarsVisible = insets.isVisible(WindowInsetsCompat.Type.statusBars())
                    val navigationBarsVisible = insets.isVisible(WindowInsetsCompat.Type.navigationBars())
                    
                    if (statusBarsVisible || navigationBarsVisible) {
                        // Скрываем панели с небольшой задержкой
                        handler.postDelayed({
                            windowInsetsController?.apply {
                                hide(WindowInsetsCompat.Type.statusBars())
                                hide(WindowInsetsCompat.Type.navigationBars())
                                systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                            }
                            
                            // Для Android 11+ (API 30+)
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                                window.insetsController?.let { controller ->
                                    controller.hide(android.view.WindowInsets.Type.statusBars() or android.view.WindowInsets.Type.navigationBars())
                                    controller.systemBarsBehavior = android.view.WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                                }
                            }
                        }, 200) // Задержка 200мс для лучшей видимости
                    }
                }
                
                // Проверяем каждые 300мс
                handler.postDelayed(this, 300)
            }
        }
        
        // Запускаем проверку после небольшой задержки
        handler.postDelayed(checkRunnable, 500)
    }
}

@Composable
fun PermissionRequestScreen(onRequestPermissions: () -> Unit) {
    Box(
        modifier = Modifier.fillMaxSize(),
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text(
                text = "Требуются разрешения",
                style = MaterialTheme.typography.headlineMedium
            )
            Text(
                text = "Приложению необходимы разрешения для работы с сетью",
                style = MaterialTheme.typography.bodyMedium
            )
        }
    }
    
    LaunchedEffect(Unit) {
        onRequestPermissions()
    }
}
