package com.example.cam_assist.data.udp

import android.util.Log
import com.example.cam_assist.data.model.DeviceData
import com.google.gson.Gson
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress

class UdpClient(private val port: Int = 8888) {
    private var socket: DatagramSocket? = null
    private val gson = Gson()
    private val buffer = ByteArray(2048)
    
    suspend fun startListening(
        onDataReceived: (DeviceData) -> Unit,
        onError: (Exception) -> Unit
    ) = withContext(Dispatchers.IO) {
        try {
            socket = DatagramSocket(port).apply {
                broadcast = true
                reuseAddress = true
                soTimeout = 5000 // Таймаут 5 секунд для предотвращения блокировки
            }
            
            Log.d(TAG, "UDP клиент запущен на порту $port")
            
            while (socket?.isClosed == false) {
                try {
                    val packet = DatagramPacket(buffer, buffer.size)
                    socket?.receive(packet)
                    
                    val data = String(packet.data, 0, packet.length)
                    Log.d(TAG, "Получены UDP данные от ${packet.address.hostAddress}: $data")
                    
                    // Парсим JSON
                    val deviceData = gson.fromJson(data, DeviceData::class.java)
                    onDataReceived(deviceData)
                } catch (e: java.net.SocketTimeoutException) {
                    // Таймаут - это нормально, продолжаем слушать
                    continue
                } catch (e: Exception) {
                    if (socket?.isClosed == false) {
                        Log.e(TAG, "Ошибка при получении UDP пакета", e)
                        // Не вызываем onError для обычных ошибок, чтобы не прерывать работу
                    }
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка запуска UDP клиента", e)
            onError(e)
        }
    }
    
    fun stop() {
        try {
            socket?.close()
            socket = null
            Log.d(TAG, "UDP клиент остановлен")
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка при остановке UDP клиента", e)
        }
    }
    
    companion object {
        private const val TAG = "UdpClient"
    }
}
