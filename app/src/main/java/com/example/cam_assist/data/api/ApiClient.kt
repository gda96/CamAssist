package com.example.cam_assist.data.api

import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import java.util.concurrent.TimeUnit

object ApiClient {
    private const val DEFAULT_BASE_URL = "http://192.168.4.1/" // AP mode default
    private const val CONNECTION_TIMEOUT = 10L // Увеличено для ESP32-C3
    private const val READ_TIMEOUT = 10L // Увеличено для ESP32-C3
    
    private val loggingInterceptor = HttpLoggingInterceptor().apply {
        level = HttpLoggingInterceptor.Level.BODY
    }
    
    private val okHttpClient = OkHttpClient.Builder()
        .connectTimeout(CONNECTION_TIMEOUT, TimeUnit.SECONDS)
        .readTimeout(READ_TIMEOUT, TimeUnit.SECONDS)
        .addInterceptor(loggingInterceptor)
        .build()
    
    fun create(baseUrl: String = DEFAULT_BASE_URL): DeviceApi {
        val retrofit = Retrofit.Builder()
            .baseUrl(baseUrl)
            .client(okHttpClient)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
        
        return retrofit.create(DeviceApi::class.java)
    }
    
    // Helper to build base URL from IP address
    fun buildBaseUrl(ipAddress: String): String {
        return if (ipAddress.startsWith("http://") || ipAddress.startsWith("https://")) {
            ipAddress
        } else {
            "http://$ipAddress/"
        }
    }
}

