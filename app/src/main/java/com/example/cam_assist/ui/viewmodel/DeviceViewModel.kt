package com.example.cam_assist.ui.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.example.cam_assist.data.model.DeviceData
import com.example.cam_assist.data.model.ScanStatus
import com.example.cam_assist.data.repository.DeviceRepository
import com.example.cam_assist.data.udp.UdpClient
import com.example.cam_assist.util.NetworkUtils
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

data class DeviceUiState(
    val deviceData: DeviceData? = null,
    val isLoading: Boolean = false,
    val isConnected: Boolean = false,
    val error: String? = null,
    val scanStatus: ScanStatus? = null,
    val isScanning: Boolean = false
)

class DeviceViewModel(application: Application) : AndroidViewModel(application) {
    private val repository: DeviceRepository
    private var updateJob: Job? = null
    private var udpJob: Job? = null
    private val udpClient = UdpClient()
    
    private val _uiState = MutableStateFlow(DeviceUiState())
    val uiState: StateFlow<DeviceUiState> = _uiState.asStateFlow()
    
    init {
        val baseUrl = NetworkUtils.getDeviceBaseUrl(application)
        repository = DeviceRepository(baseUrl)
        startPeriodicUpdate()
        startUdpListener()
    }
    
    private fun startUdpListener() {
        udpJob?.cancel()
        udpJob = viewModelScope.launch {
            udpClient.startListening(
                onDataReceived = { data ->
                    // Обновляем base URL если устройство в STA режиме
                    if (data.wifiMode == "sta" && !data.staIP.isNullOrBlank() && data.staIP != "0.0.0.0") {
                        val newBaseUrl = NetworkUtils.getDeviceBaseUrl(getApplication(), data.staIP)
                        repository.updateBaseUrl(newBaseUrl)
                    }
                    
                    _uiState.value = _uiState.value.copy(
                        deviceData = data,
                        isConnected = true,
                        error = null
                    )
                },
                onError = { error ->
                    // UDP ошибки не критичны, просто логируем
                    // Основное подключение через HTTP продолжит работать
                }
            )
        }
    }
    
    private fun startPeriodicUpdate() {
        updateJob?.cancel()
        updateJob = viewModelScope.launch {
            while (true) {
                loadDeviceData()
                delay(2000) // Обновление каждые 2 секунды
            }
        }
    }
    
    fun loadDeviceData() {
        viewModelScope.launch {
            val currentState = _uiState.value
            // Устанавливаем isLoading = true только если еще не было успешного подключения
            // Это предотвращает мигание статуса "ПОДКЛЮЕНИЕ" при периодических обновлениях
            val shouldShowLoading = !currentState.isConnected || currentState.deviceData == null
            
            if (shouldShowLoading) {
                _uiState.value = _uiState.value.copy(isLoading = true, error = null)
            }
            
            try {
            repository.getDeviceData()
                .onSuccess { data ->
                    // Обновляем base URL если устройство в STA режиме
                    if (data.wifiMode == "sta" && !data.staIP.isNullOrBlank() && data.staIP != "0.0.0.0") {
                        val newBaseUrl = NetworkUtils.getDeviceBaseUrl(getApplication(), data.staIP)
                        repository.updateBaseUrl(newBaseUrl)
                    }
                    
                    _uiState.value = _uiState.value.copy(
                        deviceData = data,
                        isLoading = false,
                            isConnected = true,
                            error = null
                    )
                }
                .onFailure { error ->
                        val errorMessage = when {
                            error.message?.contains("timeout", ignoreCase = true) == true -> 
                                "Таймаут подключения. Проверьте, что устройство включено и подключено к той же сети."
                            error.message?.contains("Unable to resolve host", ignoreCase = true) == true -> 
                                "Не удалось найти устройство. Подключитесь к сети 'Camera_Heater_AP' или проверьте IP адрес."
                            error.message?.contains("Connection refused", ignoreCase = true) == true -> 
                                "Устройство недоступно. Проверьте, что веб-сервер ESP32 запущен."
                            else -> error.message ?: "Ошибка подключения к устройству"
                        }
                        
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isConnected = false,
                            error = errorMessage
                        )
                    }
            } catch (e: Exception) {
                    _uiState.value = _uiState.value.copy(
                        isLoading = false,
                        isConnected = false,
                    error = "Критическая ошибка: ${e.message}"
                    )
                }
        }
    }
    
    fun toggleHeater() {
        viewModelScope.launch {
            val currentData = _uiState.value.deviceData
            if (currentData?.heaterOffMode == true) {
                _uiState.value = _uiState.value.copy(error = "Сначала включите режимы, нажав кнопку \"Включить\"")
                return@launch
            }
            
            repository.toggleHeater()
                .onSuccess { response ->
                    if (response.error != null) {
                        _uiState.value = _uiState.value.copy(error = response.error)
                    } else {
                    loadDeviceData()
                    }
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    /** Долгое нажатие «Включить обогрев» в ручном режиме: обогрев вкл без автосброса. Последовательность: 1) Ручной режим, 2) Долгое нажатие. */
    fun setHeaterPersistent() {
        viewModelScope.launch {
            val currentData = _uiState.value.deviceData
            if (currentData?.heaterOffMode == true) {
                _uiState.value = _uiState.value.copy(error = "Сначала включите режимы, нажав кнопку \"Включить\"")
                return@launch
            }
            if (currentData?.autoMode == true) {
                _uiState.value = _uiState.value.copy(error = "Сначала выберите ручной режим")
                return@launch
            }
            repository.controlDevice("toggle_persistent")
                .onSuccess { response ->
                    if (response.error != null) {
                        _uiState.value = _uiState.value.copy(error = response.error)
                    } else {
                        loadDeviceData()
                    }
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun setAutoMode() {
        viewModelScope.launch {
            val currentData = _uiState.value.deviceData
            // Если режим выключения активен, не позволяем переключать режимы
            if (currentData?.heaterOffMode == true) {
                _uiState.value = _uiState.value.copy(error = "Сначала включите режимы, нажав кнопку \"Включить\"")
                return@launch
            }
            
            repository.setAutoMode()
                .onSuccess { response ->
                    // Проверяем наличие ошибки в ответе
                    if (response.error != null) {
                        _uiState.value = _uiState.value.copy(error = response.error)
                    } else {
                        loadDeviceData() // Обновляем данные
                    }
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun setOffMode() {
        viewModelScope.launch {
            repository.controlDevice("off")
                .onSuccess {
                    loadDeviceData() // Обновляем данные
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun resetWifi() {
        viewModelScope.launch {
            repository.resetWifi()
                .onSuccess {
                    _uiState.value = _uiState.value.copy(error = "Настройки WiFi сброшены. Устройство перезагружается...")
                    // Устройство перезагрузится, поэтому через некоторое время попробуем переподключиться
                    delay(5000)
                    loadDeviceData()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun restartWifi() {
        viewModelScope.launch {
            repository.restartWifi()
                .onSuccess {
                    _uiState.value = _uiState.value.copy(error = "WiFi модуль перезагружается...")
                    // Небольшая задержка для перезапуска WiFi
                    delay(3000)
                    loadDeviceData()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun calibrateSensor(tempOffset: Float, humOffset: Float) {
        viewModelScope.launch {
            repository.calibrateSensor(tempOffset, humOffset)
                .onSuccess {
                    loadDeviceData()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun calibrateThresholds(tempThreshold: Float, humThreshold: Float) {
        viewModelScope.launch {
            repository.calibrateThresholds(tempThreshold, humThreshold)
                .onSuccess {
                    loadDeviceData()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun resetCalibration() {
        viewModelScope.launch {
            repository.resetCalibration()
                .onSuccess {
                    loadDeviceData()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun saveHideInfo(hide: Boolean) {
        viewModelScope.launch {
            repository.saveHideInfo(hide)
                .onSuccess {
                    loadDeviceData()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    fun startScan() {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isScanning = true)
            repository.startScan()
                .onSuccess {
                    // Начинаем проверку статуса сканирования
                    checkScanStatus()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(
                        isScanning = false,
                        error = error.message
                    )
                }
        }
    }
    
    private fun checkScanStatus() {
        viewModelScope.launch {
            while (_uiState.value.isScanning) {
                repository.getScanStatus()
                    .onSuccess { status ->
                        _uiState.value = _uiState.value.copy(scanStatus = status)
                        
                        if (status.status == "completed" || status.status == "failed") {
                            _uiState.value = _uiState.value.copy(isScanning = false)
                        } else {
                            delay(1000) // Проверяем каждую секунду
                        }
                    }
                    .onFailure {
                        delay(1000)
                    }
            }
        }
    }
    
    fun updateWifi(
        mode: String,
        staSSID: String? = null,
        staPassword: String? = null,
        apSSID: String? = null,
        apPassword: String? = null,
        hideConnectionInfo: Boolean? = null,
        staIP: String? = null
    ) {
        viewModelScope.launch {
            repository.updateWifi(mode, staSSID, staPassword, apSSID, apPassword, hideConnectionInfo, staIP)
                .onSuccess {
                    // Обновляем baseUrl если указан IP адрес
                    staIP?.takeIf { it.isNotBlank() && it != "0.0.0.0" }?.let { ip ->
                        val newBaseUrl = NetworkUtils.getDeviceBaseUrl(getApplication(), ip)
                        repository.updateBaseUrl(newBaseUrl)
                    }
                    loadDeviceData()
                }
                .onFailure { error ->
                    _uiState.value = _uiState.value.copy(error = error.message)
                }
        }
    }
    
    override fun onCleared() {
        super.onCleared()
        updateJob?.cancel()
        udpJob?.cancel()
        udpClient.stop()
    }
}

