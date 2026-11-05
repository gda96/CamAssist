package com.example.cam_assist.util

import android.content.Context
import android.net.wifi.WifiManager
import android.net.wifi.WifiInfo
import android.text.format.Formatter
import java.net.InetAddress

object NetworkUtils {
    private const val AP_MODE_IP = "192.168.4.1"
    private const val AP_MODE_SSID = "Camera_Heater_AP"
    
    /**
     * Определяет IP адрес устройства ESP32
     * Сначала пытается подключиться к AP режиму, затем к STA режиму
     */
    fun getDeviceBaseUrl(context: Context, staIP: String? = null): String {
        // Если передан IP из STA режима, используем его
        if (!staIP.isNullOrBlank() && staIP != "0.0.0.0") {
            return "http://$staIP/"
        }
        
        // Проверяем, подключены ли мы к AP сети устройства
        try {
            val wifiManager = context.applicationContext.getSystemService(Context.WIFI_SERVICE) as? WifiManager
            val wifiInfo: WifiInfo? = wifiManager?.connectionInfo
            
            wifiInfo?.let {
                val ssid = it.ssid?.removeSurrounding("\"") ?: ""
                // Проверяем как точное совпадение, так и частичное (на случай изменений в имени)
                if (ssid == AP_MODE_SSID || ssid.contains("Camera_Heater", ignoreCase = true)) {
                    return "http://$AP_MODE_IP/"
                }
            }
        } catch (e: Exception) {
            // В случае ошибки просто используем AP режим по умолчанию
        }
        
        // По умолчанию используем AP режим
        return "http://$AP_MODE_IP/"
    }
    
    /**
     * Проверяет, подключены ли мы к сети устройства
     */
    fun isConnectedToDevice(context: Context): Boolean {
        val wifiManager = context.applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
        val wifiInfo: WifiInfo? = wifiManager.connectionInfo
        
        wifiInfo?.let {
            val ssid = it.ssid?.removeSurrounding("\"") ?: ""
            return ssid == AP_MODE_SSID || wifiManager.isWifiEnabled
        }
        
        return false
    }
    
    /**
     * Получает локальный IP адрес устройства Android
     */
    fun getLocalIpAddress(context: Context): String? {
        val wifiManager = context.applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
        val wifiInfo: WifiInfo? = wifiManager.connectionInfo
        
        wifiInfo?.let {
            val ipAddress = it.ipAddress
            if (ipAddress != 0) {
                return Formatter.formatIpAddress(ipAddress)
            }
        }
        
        return null
    }
}

