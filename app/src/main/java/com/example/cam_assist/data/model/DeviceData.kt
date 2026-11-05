package com.example.cam_assist.data.model

import com.google.gson.annotations.SerializedName

data class DeviceData(
    @SerializedName("temperature")
    val temperature: Float,
    
    @SerializedName("humidity")
    val humidity: Float,
    
    @SerializedName("heaterState")
    val heaterState: Boolean,
    
    @SerializedName("sensorConnected")
    val sensorConnected: Boolean,
    
    @SerializedName("autoMode")
    val autoMode: Boolean,
    
    @SerializedName("heaterOffMode")
    val heaterOffMode: Boolean? = null,
    
    @SerializedName("wifiMode")
    val wifiMode: String, // "ap" or "sta"
    
    @SerializedName("currentSSID")
    val currentSSID: String,
    
    @SerializedName("staIP")
    val staIP: String,
    
    @SerializedName("staNetwork")
    val staNetwork: String,
    
    @SerializedName("staPassword")
    val staPassword: String? = null,
    
    @SerializedName("apPassword")
    val apPassword: String? = null,
    
    @SerializedName("hideConnectionInfo")
    val hideConnectionInfo: Boolean,
    
    @SerializedName("calibrationParams")
    val calibrationParams: CalibrationParams
)

data class CalibrationParams(
    @SerializedName("tempThreshold")
    val tempThreshold: Float,
    
    @SerializedName("humThreshold")
    val humThreshold: Float,
    
    @SerializedName("tempOffset")
    val tempOffset: Float,
    
    @SerializedName("humOffset")
    val humOffset: Float
)

data class ControlResponse(
    @SerializedName("success")
    val success: Boolean,
    
    @SerializedName("heaterState")
    val heaterState: Boolean? = null,
    
    @SerializedName("autoMode")
    val autoMode: Boolean? = null,
    
    @SerializedName("heaterOffMode")
    val heaterOffMode: Boolean? = null,
    
    @SerializedName("error")
    val error: String? = null
)

data class ScanNetwork(
    @SerializedName("ssid")
    val ssid: String,
    
    @SerializedName("rssi")
    val rssi: Int,
    
    @SerializedName("encryption")
    val encryption: String
)

data class ScanStatus(
    @SerializedName("status")
    val status: String, // "scanning", "completed", "failed"
    
    @SerializedName("networks")
    val networks: List<ScanNetwork>? = null,
    
    @SerializedName("success")
    val success: Boolean? = null,
    
    @SerializedName("error")
    val error: String? = null
)

