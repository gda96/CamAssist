package com.example.cam_assist.data.api

import com.example.cam_assist.data.model.ControlResponse
import com.example.cam_assist.data.model.DeviceData
import com.example.cam_assist.data.model.ScanStatus
import retrofit2.Response
import retrofit2.http.*

interface DeviceApi {
    @GET("data")
    suspend fun getDeviceData(): Response<DeviceData>
    
    @GET("control")
    suspend fun controlDevice(
        @Query("action") action: String // "toggle", "auto", or "off"
    ): Response<ControlResponse>
    
    @GET("resetwifi")
    suspend fun resetWifi(): Response<String>
    
    @GET("restartwifi")
    suspend fun restartWifi(): Response<String>
    
    @GET("resetcalibration")
    suspend fun resetCalibration(): Response<String>
    
    @POST("sensorcalibrate")
    @FormUrlEncoded
    suspend fun calibrateSensor(
        @Field("tempOffset") tempOffset: String,
        @Field("humOffset") humOffset: String
    ): Response<String>
    
    @POST("calibrate")
    @FormUrlEncoded
    suspend fun calibrateThresholds(
        @Field("tempThreshold") tempThreshold: String,
        @Field("humThreshold") humThreshold: String
    ): Response<String>
    
    @POST("savehideinfo")
    @FormUrlEncoded
    suspend fun saveHideInfo(
        @Field("hideConnectionInfo") hideConnectionInfo: String // "1" to hide, "0" to show
    ): Response<String>
    
    @GET("startscan")
    suspend fun startScan(): Response<String>
    
    @GET("scanstatus")
    suspend fun getScanStatus(): Response<ScanStatus>
    
    @POST("updatewifi")
    @FormUrlEncoded
    suspend fun updateWifi(
        @Field("mode") mode: String, // "ap" or "sta"
        @Field("staSSID") staSSID: String? = null,
        @Field("staPassword") staPassword: String? = null,
        @Field("apSSID") apSSID: String? = null,
        @Field("apPassword") apPassword: String? = null,
        @Field("hideConnectionInfo") hideConnectionInfo: String? = null,
        @Field("staIP") staIP: String? = null
    ): Response<String>
}

