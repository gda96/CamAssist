package com.example.cam_assist.car

import androidx.car.app.CarContext
import androidx.car.app.Screen
import androidx.car.app.model.Action
import androidx.car.app.model.ActionStrip
import androidx.car.app.model.CarColor
import androidx.car.app.model.Template
import androidx.car.app.model.MessageTemplate
import androidx.car.app.model.Pane
import androidx.car.app.model.PaneTemplate
import androidx.car.app.model.Row
import com.example.cam_assist.data.repository.DeviceRepository
import com.example.cam_assist.util.NetworkUtils
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.delay

class CarMainScreen(carContext: CarContext) : Screen(carContext) {
    private var deviceData: com.example.cam_assist.data.model.DeviceData? = null
    private var isLoading = false
    private var error: String? = null
    
    // Используем фиксированный URL для Android Auto (AP режим по умолчанию)
    private val repository = try {
        DeviceRepository("http://192.168.4.1/")
    } catch (e: Exception) {
        // Fallback на AP режим
        DeviceRepository("http://192.168.4.1/")
    }
    
    private val coroutineScope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    
    init {
        // Запускаем обновление только после первого отображения
        // Это гарантирует, что экран загрузится даже если сеть недоступна
        try {
            startPeriodicUpdate()
        } catch (e: Exception) {
            // Игнорируем ошибки инициализации для Android Auto
        }
    }
    
    private fun startPeriodicUpdate() {
        coroutineScope.launch {
            // Небольшая задержка перед первым запросом
            delay(1000)
            while (true) {
                try {
                    loadDeviceData()
                } catch (e: Exception) {
                    // Игнорируем ошибки сети в Android Auto
                }
                delay(3000) // Обновление каждые 3 секунды
            }
        }
    }
    
    private suspend fun loadDeviceData() {
        isLoading = true
        invalidate()
        
        try {
            repository.getDeviceData()
                .onSuccess { data ->
                    deviceData = data
                    error = null
                }
                .onFailure { e ->
                    error = e.message ?: "Ошибка подключения"
                }
        } catch (e: Exception) {
            error = "Ошибка: ${e.message}"
        }
        
        isLoading = false
        invalidate()
    }
    
    override fun onGetTemplate(): Template {
        if (isLoading) {
            return MessageTemplate.Builder("Загрузка данных...")
                .setTitle("Обогрев камеры")
                .setHeaderAction(Action.BACK)
                .build()
        }
        
        error?.let {
            return MessageTemplate.Builder(it)
                .setTitle("Ошибка подключения")
                .setHeaderAction(Action.BACK)
                .addAction(
                    Action.Builder()
                        .setTitle("Повторить")
                        .setOnClickListener {
                            coroutineScope.launch {
                                loadDeviceData()
                            }
                        }
                        .build()
                )
                .build()
        }
        
        val data = deviceData ?: return MessageTemplate.Builder("Нет данных")
            .setTitle("Обогрев камеры")
            .setHeaderAction(Action.BACK)
            .build()
        
        val rows = mutableListOf<Row>()
        
        // Температура
        rows.add(
            Row.Builder()
                .setTitle("Температура")
                .addText("${data.temperature}°C")
                .build()
        )
        
        // Влажность
        rows.add(
            Row.Builder()
                .setTitle("Влажность")
                .addText("${data.humidity}%")
                .build()
        )
        
        // Статус обогрева
        rows.add(
            Row.Builder()
                .setTitle("Обогрев")
                .addText(if (data.heaterState) "ВКЛ" else "ВЫКЛ")
                .build()
        )
        
        // Режим
        rows.add(
            Row.Builder()
                .setTitle("Режим")
                .addText(if (data.autoMode) "Автоматический" else "Ручной")
                .build()
        )
        
        val pane = Pane.Builder()
            .addRow(rows[0])
            .addRow(rows[1])
            .addRow(rows[2])
            .addRow(rows[3])
            .build()
        
        return PaneTemplate.Builder(pane)
            .setTitle("Обогрев камеры ассистентов")
            .setHeaderAction(Action.BACK)
            .setActionStrip(
                ActionStrip.Builder()
                    .addAction(
                        Action.Builder()
                            .setTitle(if (data.heaterState) "Выключить" else "Включить")
                            .setBackgroundColor(if (data.heaterState) CarColor.RED else CarColor.GREEN)
                        .setOnClickListener {
                            coroutineScope.launch {
                                repository.toggleHeater()
                                loadDeviceData()
                            }
                        }
                            .build()
                    )
                    .addAction(
                        Action.Builder()
                            .setTitle(if (data.autoMode) "Ручной" else "Авто") // Кнопка: режим, на который переключимся
                            .setOnClickListener {
                                coroutineScope.launch {
                                    repository.setAutoMode()
                                    loadDeviceData()
                                }
                            }
                            .build()
                    )
                    .build()
            )
            .build()
    }
}

