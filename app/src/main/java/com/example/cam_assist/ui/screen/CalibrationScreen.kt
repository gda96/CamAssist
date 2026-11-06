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
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import com.example.cam_assist.ui.components.ControlButton
import com.example.cam_assist.ui.theme.*
import com.example.cam_assist.ui.viewmodel.DeviceViewModel

@Composable
fun CalibrationDialog(
    viewModel: DeviceViewModel,
    onDismiss: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val data = uiState.deviceData
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.screenWidthDp > configuration.screenHeightDp
    
    var tempThreshold by remember { mutableStateOf(data?.calibrationParams?.tempThreshold?.toString() ?: "10.0") }
    var humThreshold by remember { mutableStateOf(data?.calibrationParams?.humThreshold?.toString() ?: "70.0") }
    var tempOffset by remember { mutableStateOf(data?.calibrationParams?.tempOffset?.toString() ?: "0.0") }
    var humOffset by remember { mutableStateOf(data?.calibrationParams?.humOffset?.toString() ?: "0.0") }
    
    // Обновляем значения при изменении данных (в т.ч. после сброса калибровки)
    LaunchedEffect(data?.calibrationParams) {
        data?.calibrationParams?.let {
            tempThreshold = it.tempThreshold.toString()
            humThreshold = it.humThreshold.toString()
            tempOffset = it.tempOffset.toString()
            humOffset = it.humOffset.toString()
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
                    text = "Калибровка",
                    style = if (isLandscape) MaterialTheme.typography.titleMedium else MaterialTheme.typography.headlineMedium,
                    color = TextPrimary
                )
                
                Text(
                    text = "Пороги срабатывания обогрева",
                    style = if (isLandscape) MaterialTheme.typography.titleSmall else MaterialTheme.typography.titleMedium,
                    color = TextSecondary
                )
                
                if (isLandscape) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        OutlinedTextField(
                            value = tempThreshold,
                            onValueChange = { tempThreshold = it },
                            label = { Text("Температура (≤ °C)") },
                            supportingText = { Text("макс. 30") },
                            modifier = Modifier
                                .weight(1f)
                                .heightIn(min = 48.dp),
                            singleLine = true
                        )
                        
                        OutlinedTextField(
                            value = humThreshold,
                            onValueChange = { humThreshold = it },
                            label = { Text("Влажность (≥ %)") },
                            modifier = Modifier
                                .weight(1f)
                                .heightIn(min = 48.dp),
                            singleLine = true
                        )
                    }
                } else {
                    OutlinedTextField(
                        value = tempThreshold,
                        onValueChange = { tempThreshold = it },
                        label = { Text("Температура включения (≤ °C)") },
                        supportingText = { Text("макс. 30°C") },
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(min = 56.dp),
                        singleLine = true
                    )
                    
                    OutlinedTextField(
                        value = humThreshold,
                        onValueChange = { humThreshold = it },
                        label = { Text("Влажность включения (≥ %)") },
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(min = 56.dp),
                        singleLine = true
                    )
                }
                
                Divider()
                
                Text(
                    text = "Коррекция датчика",
                    style = if (isLandscape) MaterialTheme.typography.titleSmall else MaterialTheme.typography.titleMedium,
                    color = TextSecondary
                )
                
                if (isLandscape) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        OutlinedTextField(
                            value = tempOffset,
                            onValueChange = { tempOffset = it },
                            label = { Text("Коррекция темп. (°C)") },
                            modifier = Modifier
                                .weight(1f)
                                .heightIn(min = 48.dp),
                            singleLine = true
                        )
                        
                        OutlinedTextField(
                            value = humOffset,
                            onValueChange = { humOffset = it },
                            label = { Text("Коррекция влажн. (%)") },
                            modifier = Modifier
                                .weight(1f)
                                .heightIn(min = 48.dp),
                            singleLine = true
                        )
                    }
                } else {
                    OutlinedTextField(
                        value = tempOffset,
                        onValueChange = { tempOffset = it },
                        label = { Text("Коррекция температуры (°C)") },
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(min = 56.dp),
                        singleLine = true
                    )
                    
                    OutlinedTextField(
                        value = humOffset,
                        onValueChange = { humOffset = it },
                        label = { Text("Коррекция влажности (%)") },
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(min = 56.dp),
                        singleLine = true
                    )
                }
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(if (isLandscape) 8.dp else 12.dp)
                ) {
                    ControlButton(
                        text = "Сохранить пороги",
                        onClick = {
                            tempThreshold.toFloatOrNull()?.let { temp ->
                                humThreshold.toFloatOrNull()?.let { hum ->
                                    viewModel.calibrateThresholds(temp, hum)
                                }
                            }
                        },
                        backgroundColor = ModeAuto,
                        modifier = Modifier.weight(1f),
                        isCompact = isLandscape
                    )
                    
                    ControlButton(
                        text = "Применить коррекцию",
                        onClick = {
                            tempOffset.toFloatOrNull()?.let { temp ->
                                humOffset.toFloatOrNull()?.let { hum ->
                                    viewModel.calibrateSensor(temp, hum)
                                }
                            }
                        },
                        backgroundColor = ModeAuto,
                        modifier = Modifier.weight(1f),
                        isCompact = isLandscape
                    )
                }
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(if (isLandscape) 8.dp else 12.dp)
                ) {
                    ControlButton(
                        text = "Сброс",
                        onClick = {
                            viewModel.resetCalibration()
                        },
                        backgroundColor = Color(0xFFE74C3C),
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
            }
        }
        }
    }
}

