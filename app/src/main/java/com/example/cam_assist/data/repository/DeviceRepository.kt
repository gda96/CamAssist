package com.example.cam_assist.data.repository

import com.example.cam_assist.data.api.ApiClient
import com.example.cam_assist.data.api.DeviceApi
import com.example.cam_assist.data.model.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.util.Locale

class DeviceRepository(private var baseUrl: String = "http://192.168.4.1/") {
    private var api: DeviceApi = ApiClient.create(baseUrl)
    
    suspend fun getDeviceData(): Result<DeviceData> = withContext(Dispatchers.IO) {
        try {
            val response = api.getDeviceData()
            if (response.isSuccessful && response.body() != null) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to get device data: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun toggleHeater(): Result<ControlResponse> = withContext(Dispatchers.IO) {
        try {
            val response = api.controlDevice("toggle")
            if (response.isSuccessful && response.body() != null) {
                val body = response.body()!!
                // Если в ответе есть ошибка, возвращаем её как ошибку
                if (!body.success && body.error != null) {
                    Result.failure(Exception(body.error))
                } else {
                    Result.success(body)
                }
            } else {
                Result.failure(Exception("Failed to toggle heater: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun setAutoMode(): Result<ControlResponse> = withContext(Dispatchers.IO) {
        try {
            val response = api.controlDevice("auto")
            if (response.isSuccessful && response.body() != null) {
                val body = response.body()!!
                // Если в ответе есть ошибка, возвращаем её как ошибку
                if (!body.success && body.error != null) {
                    Result.failure(Exception(body.error))
                } else {
                    Result.success(body)
                }
            } else {
                Result.failure(Exception("Failed to set auto mode: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun controlDevice(action: String): Result<ControlResponse> = withContext(Dispatchers.IO) {
        try {
            val response = api.controlDevice(action)
            if (response.isSuccessful && response.body() != null) {
                val body = response.body()!!
                // Если в ответе есть ошибка, возвращаем её как ошибку
                if (!body.success && body.error != null) {
                    Result.failure(Exception(body.error))
                } else {
                    Result.success(body)
                }
            } else {
                Result.failure(Exception("Failed to control device: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun calibrateSensor(tempOffset: Float, humOffset: Float): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            // Отправляем с точкой как десятичным разделителем (Locale.US), чтобы веб-портал и скетч получали те же значения
            val response = api.calibrateSensor(
                String.format(Locale.US, "%.1f", tempOffset),
                String.format(Locale.US, "%.1f", humOffset)
            )
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                val msg = response.errorBody()?.string()?.takeIf { it.isNotBlank() }
                    ?: "Ошибка коррекции датчика: ${response.code()}"
                Result.failure(Exception(msg))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun calibrateThresholds(tempThreshold: Float, humThreshold: Float): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            // Отправляем с точкой как десятичным разделителем (Locale.US), чтобы веб-портал и скетч получали те же значения
            val response = api.calibrateThresholds(
                String.format(Locale.US, "%.1f", tempThreshold),
                String.format(Locale.US, "%.1f", humThreshold)
            )
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                val msg = response.errorBody()?.string()?.takeIf { it.isNotBlank() }
                    ?: "Ошибка сохранения порогов: ${response.code()}"
                Result.failure(Exception(msg))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun resetCalibration(): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = api.resetCalibration()
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to reset calibration: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun saveHideInfo(hide: Boolean): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = api.saveHideInfo(if (hide) "1" else "0")
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to save hide info: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun startScan(): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = api.startScan()
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to start scan: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun getScanStatus(): Result<ScanStatus> = withContext(Dispatchers.IO) {
        try {
            val response = api.getScanStatus()
            if (response.isSuccessful && response.body() != null) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to get scan status: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun updateWifi(
        mode: String,
        staSSID: String? = null,
        staPassword: String? = null,
        apSSID: String? = null,
        apPassword: String? = null,
        hideConnectionInfo: Boolean? = null,
        staIP: String? = null
    ): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = api.updateWifi(
                mode = mode,
                staSSID = staSSID,
                staPassword = staPassword,
                apSSID = apSSID,
                apPassword = apPassword,
                hideConnectionInfo = hideConnectionInfo?.let { if (it) "1" else "0" },
                staIP = staIP
            )
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to update WiFi: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun resetWifi(): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = api.resetWifi()
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to reset WiFi: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    suspend fun restartWifi(): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = api.restartWifi()
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to restart WiFi: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    fun updateBaseUrl(newBaseUrl: String) {
        baseUrl = newBaseUrl
        api = ApiClient.create(baseUrl)
    }
}

