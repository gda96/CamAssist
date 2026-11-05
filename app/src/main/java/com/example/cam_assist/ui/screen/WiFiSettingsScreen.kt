package com.example.cam_assist.ui.screen

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import com.example.cam_assist.ui.components.ControlButton
import com.example.cam_assist.ui.theme.*
import com.example.cam_assist.ui.viewmodel.DeviceViewModel

@Composable
fun WiFiSettingsDialog(
    viewModel: DeviceViewModel,
    onDismiss: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val data = uiState.deviceData
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.screenWidthDp > configuration.screenHeightDp
    
    var wifiMode by remember { mutableStateOf(data?.wifiMode ?: "ap") }
    var staSSID by remember { mutableStateOf(data?.staNetwork ?: "") }
    var staPassword by remember { mutableStateOf("") }
    var staIP by remember { mutableStateOf(data?.staIP?.takeIf { it.isNotBlank() && it != "0.0.0.0" } ?: "") }
    var apSSID by remember { mutableStateOf(data?.currentSSID ?: "Camera_Heater_AP") }
    var apPassword by remember { mutableStateOf("12345678") }
    var showPassword by remember { mutableStateOf(false) }
    
    // Обновляем значения при изменении данных (только при первом показе или при явном изменении)
    LaunchedEffect(data) {
        data?.let {
            wifiMode = it.wifiMode
            staSSID = it.staNetwork
            staIP = it.staIP.takeIf { ip -> ip.isNotBlank() && ip != "0.0.0.0" } ?: ""
            apSSID = it.currentSSID
            
            // Автозаполнение паролей из полученных данных
            if (!it.staPassword.isNullOrBlank()) {
                staPassword = it.staPassword
            }
            if (!it.apPassword.isNullOrBlank()) {
                apPassword = it.apPassword
            }
        }
    }
    
    Dialog(
        onDismissRequest = onDismiss,
        properties = androidx.compose.ui.window.DialogProperties(
            usePlatformDefaultWidth = false,
            decorFitsSystemWindows = false
        )
    ) {
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(Color.Black.copy(alpha = 0.7f)) // Полупрозрачная подложка
                .clickable { onDismiss() },
            contentAlignment = Alignment.Center
        ) {
            Card(
            modifier = Modifier
                .widthIn(max = if (isLandscape) 600.dp else Int.MAX_VALUE.dp)
                .fillMaxWidth(if (isLandscape) 0.9f else 1f)
                .fillMaxHeight(if (isLandscape) 0.95f else 0.9f)
                .padding(if (isLandscape) 8.dp else 16.dp),
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(
                containerColor = CardBackground
            )
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(if (isLandscape) 10.dp else 16.dp)
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(if (isLandscape) 6.dp else 12.dp)
            ) {
                Text(
                    text = "Настройки WiFi",
                    style = if (isLandscape) MaterialTheme.typography.titleMedium else MaterialTheme.typography.headlineMedium,
                    color = TextPrimary
                )
                
                // Режим WiFi
                Text(
                    text = "Режим работы",
                    style = if (isLandscape) MaterialTheme.typography.titleSmall else MaterialTheme.typography.titleMedium,
                    color = TextSecondary
                )
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(if (isLandscape) 8.dp else 12.dp)
                ) {
                    FilterChip(
                        selected = wifiMode == "ap",
                        onClick = { wifiMode = "ap" },
                        label = { 
                            Text(
                                "Точка доступа",
                                maxLines = 1,
                                overflow = TextOverflow.Ellipsis
                            ) 
                        },
                        modifier = Modifier.weight(1f)
                    )
                    FilterChip(
                        selected = wifiMode == "sta",
                        onClick = { wifiMode = "sta" },
                        label = { 
                            Text(
                                "Домашняя сеть",
                                maxLines = 1,
                                overflow = TextOverflow.Ellipsis
                            ) 
                        },
                        modifier = Modifier.weight(1f)
                    )
                }
                
                Divider()
                
                // Настройки точки доступа
                Text(
                    text = "Точка доступа",
                    style = if (isLandscape) MaterialTheme.typography.titleSmall else MaterialTheme.typography.titleMedium,
                    color = TextSecondary
                )
                
                if (isLandscape) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        OutlinedTextField(
                            value = apSSID,
                            onValueChange = { apSSID = it },
                            label = { Text("SSID") },
                            modifier = Modifier
                                .weight(1f)
                                .heightIn(min = 48.dp),
                            singleLine = true
                        )
                        
                        OutlinedTextField(
                            value = apPassword,
                            onValueChange = { apPassword = it },
                            label = { Text("Пароль") },
                            modifier = Modifier
                                .weight(1f)
                                .heightIn(min = 48.dp),
                            singleLine = true,
                            visualTransformation = if (showPassword) VisualTransformation.None else PasswordVisualTransformation()
                        )
                    }
                } else {
                    OutlinedTextField(
                        value = apSSID,
                        onValueChange = { apSSID = it },
                        label = { Text("SSID точки доступа") },
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(min = 56.dp),
                        singleLine = true
                    )
                    
                    OutlinedTextField(
                        value = apPassword,
                        onValueChange = { apPassword = it },
                        label = { Text("Пароль точки доступа") },
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(min = 56.dp),
                        singleLine = true,
                        visualTransformation = if (showPassword) VisualTransformation.None else PasswordVisualTransformation()
                    )
                }
                
                // Настройки домашней сети
                if (wifiMode == "sta") {
                    Divider()
                    
                    Text(
                        text = "Домашняя сеть",
                        style = if (isLandscape) MaterialTheme.typography.titleSmall else MaterialTheme.typography.titleMedium,
                        color = TextSecondary
                    )
                    
                    // Кнопка сканирования сетей
                    Button(
                        onClick = {
                            viewModel.startScan()
                        },
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(min = if (isLandscape) 40.dp else 48.dp)
                    ) {
                        Text(
                            "Сканировать сети",
                            fontSize = if (isLandscape) 12.sp else 14.sp
                        )
                    }
                    
                    // Показываем результаты сканирования
                    uiState.scanStatus?.networks?.let { networks ->
                        networks.forEach { network ->
                            Card(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(vertical = 4.dp)
                                    .clickable {
                                        staSSID = network.ssid
                                    },
                                colors = CardDefaults.cardColors(
                                    containerColor = CardBackground
                                )
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(12.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Column {
                                        Text(
                                            text = network.ssid,
                                            style = MaterialTheme.typography.bodyMedium,
                                            color = TextPrimary
                                        )
                                        Text(
                                            text = "Сила сигнала: ${network.rssi} dBm",
                                            style = MaterialTheme.typography.bodySmall,
                                            color = TextSecondary
                                        )
                                    }
                                    Text(
                                        text = if (network.encryption == "none") "Открытая" else "Защищена",
                                        style = MaterialTheme.typography.bodySmall,
                                        color = TextSecondary
                                    )
                                }
                            }
                        }
                    }
                    
                    if (isLandscape) {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            OutlinedTextField(
                                value = staSSID,
                                onValueChange = { staSSID = it },
                                label = { Text("SSID") },
                                modifier = Modifier
                                    .weight(1f)
                                    .heightIn(min = 48.dp),
                                singleLine = true
                            )
                            
                            OutlinedTextField(
                                value = staPassword,
                                onValueChange = { staPassword = it },
                                label = { Text("Пароль") },
                                modifier = Modifier
                                    .weight(1f)
                                    .heightIn(min = 48.dp),
                                singleLine = true,
                                visualTransformation = if (showPassword) VisualTransformation.None else PasswordVisualTransformation()
                            )
                        }
                        
                        OutlinedTextField(
                            value = staIP,
                            onValueChange = { staIP = it },
                            label = { Text("IP адрес") },
                            modifier = Modifier
                                .fillMaxWidth()
                                .heightIn(min = 48.dp),
                            singleLine = true,
                            placeholder = { Text("192.168.1.100") }
                        )
                    } else {
                        OutlinedTextField(
                            value = staSSID,
                            onValueChange = { staSSID = it },
                            label = { Text("SSID сети") },
                            modifier = Modifier
                                .fillMaxWidth()
                                .heightIn(min = 56.dp),
                            singleLine = true
                        )
                        
                        OutlinedTextField(
                            value = staPassword,
                            onValueChange = { staPassword = it },
                            label = { Text("Пароль сети") },
                            modifier = Modifier
                                .fillMaxWidth()
                                .heightIn(min = 56.dp),
                            singleLine = true,
                            visualTransformation = if (showPassword) VisualTransformation.None else PasswordVisualTransformation()
                        )
                        
                        OutlinedTextField(
                            value = staIP,
                            onValueChange = { staIP = it },
                            label = { Text("IP адрес устройства") },
                            modifier = Modifier
                                .fillMaxWidth()
                                .heightIn(min = 56.dp),
                            singleLine = true,
                            placeholder = { Text("192.168.1.100") }
                        )
                    }
                }
                
                Divider()
                
                // Первая строка: Сохранить и Закрыть
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(if (isLandscape) 8.dp else 12.dp)
                ) {
                    ControlButton(
                        text = "Сохранить",
                        onClick = {
                            viewModel.updateWifi(
                                mode = wifiMode,
                                staSSID = if (wifiMode == "sta") staSSID else null,
                                staPassword = if (wifiMode == "sta") staPassword else null,
                                apSSID = apSSID,
                                apPassword = apPassword,
                                hideConnectionInfo = null,
                                staIP = if (wifiMode == "sta" && staIP.isNotBlank()) staIP else null
                            )
                            onDismiss()
                        },
                        backgroundColor = ModeAuto,
                        modifier = Modifier.weight(1f),
                        isCompact = isLandscape
                    )
                    
                    ControlButton(
                        text = "Закрыть",
                        onClick = onDismiss,
                        backgroundColor = TextSecondary,
                        modifier = Modifier.weight(1f),
                        isCompact = isLandscape
                    )
                }
                
                // Вторая строка: Перезагрузить и Сбросить
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(if (isLandscape) 8.dp else 12.dp)
                ) {
                    ControlButton(
                        text = "Перезагрузить",
                        onClick = {
                            viewModel.restartWifi()
                            onDismiss()
                        },
                        backgroundColor = ModeManual,
                        modifier = Modifier.weight(1f),
                        isCompact = isLandscape
                    )
                    
                    ControlButton(
                        text = "Сброс",
                        onClick = {
                            viewModel.resetWifi()
                            onDismiss()
                        },
                        backgroundColor = androidx.compose.ui.graphics.Color(0xFFE74C3C),
                        modifier = Modifier.weight(1f),
                        isCompact = isLandscape
                    )
                }
            }
        }
        }
    }
}

