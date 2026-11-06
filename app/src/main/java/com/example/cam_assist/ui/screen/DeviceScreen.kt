package com.example.cam_assist.ui.screen

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.Offset
import kotlin.math.pow
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.cam_assist.R
import com.example.cam_assist.ui.components.*
import com.example.cam_assist.ui.theme.*
import com.example.cam_assist.ui.viewmodel.DeviceViewModel
import com.example.cam_assist.ui.utils.scaledDp
import com.example.cam_assist.ui.utils.scaledSp

@Composable
fun DeviceScreen(
    viewModel: DeviceViewModel
) {
    val uiState by viewModel.uiState.collectAsState()
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.screenWidthDp > configuration.screenHeightDp
    
    var showCalibrationDialog by remember { mutableStateOf(false) }
    var showWiFiDialog by remember { mutableStateOf(false) }
    
    // Полноэкранный режим - контент на весь экран
    Box(
        modifier = Modifier.fillMaxSize()
    ) {
        // Фоновое изображение
        Image(
            painter = painterResource(id = R.drawable.background),
            contentDescription = null,
            modifier = Modifier
                .fillMaxSize()
                .graphicsLayer(alpha = 0.9f),
            contentScale = ContentScale.Crop
        )
        
        // Затемнение для лучшей читаемости
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(
                Brush.verticalGradient(
                        colors = listOf(
                            Color.Black.copy(alpha = 0.2f),
                            Color.Black.copy(alpha = 0.4f)
                        )
                    )
                )
        )
        if (isLandscape) {
            LandscapeLayout(uiState, viewModel, onCalibrationClick = { showCalibrationDialog = true }, onWiFiClick = { showWiFiDialog = true })
        } else {
            PortraitLayout(uiState, viewModel, onCalibrationClick = { showCalibrationDialog = true }, onWiFiClick = { showWiFiDialog = true })
        }
        
        // Индикатор загрузки
        if (uiState.isLoading) {
            CircularProgressIndicator(
                modifier = Modifier.align(Alignment.Center),
                color = ModeAuto
            )
        }
        
        // Убрали показ ошибок - теперь статус подключения отображается в статус-баре
        
        // Диалоги
        if (showCalibrationDialog) {
            CalibrationDialog(
                viewModel = viewModel,
                onDismiss = { showCalibrationDialog = false }
            )
        }
        
        if (showWiFiDialog) {
            WiFiSettingsDialog(
                viewModel = viewModel,
                onDismiss = { showWiFiDialog = false }
            )
        }
    }
}

@Composable
fun PortraitLayout(
    uiState: com.example.cam_assist.ui.viewmodel.DeviceUiState,
    viewModel: DeviceViewModel,
    onCalibrationClick: () -> Unit,
    onWiFiClick: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(scaledDp(16f))
            .verticalScroll(
                state = rememberScrollState(),
                enabled = true,
                flingBehavior = null,
                reverseScrolling = false
            ),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(scaledDp(12f))
    ) {
        // Заголовок с мягким световым ореолом статуса подключения
        Spacer(modifier = Modifier.height(scaledDp(8f)))
        val connectionStatusColor = when {
            uiState.isConnected && uiState.error == null && !uiState.isLoading -> 
                HeaterOn // Зеленый цвет при успешном подключении (ONLINE)
            else -> 
                HeaterOff // Красный цвет если подключение не удалось или продолжается (OFFLINE)
        }
        Box(
            modifier = Modifier.padding(bottom = scaledDp(8f)),
            contentAlignment = Alignment.Center
        ) {
            // Вычисляем радиусы размытия заранее для плавного градиента
            val density = LocalDensity.current
            // Увеличиваем радиусы для более широкого свечения
            val blurRadii = listOf(
                with(density) { scaledDp(35f).toPx() }, // Внешний слой - самый широкий
                with(density) { scaledDp(28f).toPx() },
                with(density) { scaledDp(22f).toPx() },
                with(density) { scaledDp(16f).toPx() },
                with(density) { scaledDp(11f).toPx() },
                with(density) { scaledDp(7f).toPx() },
                with(density) { scaledDp(4f).toPx() }  // Внутренний слой
            )
            // Плавные альфа-каналы для градиента (от внешнего к внутреннему)
            val alphaValues = listOf(0.25f, 0.35f, 0.45f, 0.55f, 0.65f, 0.75f, 0.85f)
            
            // Создаем плавное неоновое свечение через множество слоев
            blurRadii.forEachIndexed { index, blurRadius ->
                Text(
                    text = "Обогрев камеры ассистентов",
                    style = MaterialTheme.typography.headlineLarge.copy(
                        fontSize = scaledSp(27f)
                    ),
                    color = connectionStatusColor.copy(alpha = alphaValues[index]),
                    modifier = Modifier
                        .align(Alignment.Center)
                        .graphicsLayer {
                            renderEffect = androidx.compose.ui.graphics.BlurEffect(
                                radiusX = blurRadius,
                                radiusY = blurRadius,
                                edgeTreatment = androidx.compose.ui.graphics.TileMode.Clamp
                            )
                        }
                )
            }
            // Основной текст поверх всех слоев
            Text(
                text = "Обогрев камеры ассистентов",
                style = MaterialTheme.typography.headlineLarge.copy(
                    fontSize = scaledSp(27f)
                ),
                color = TextPrimary,
                modifier = Modifier.align(Alignment.Center)
            )
        }
        
        // Статус бар
        StatusBar(uiState, viewModel)
        
        // Карточки с данными
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(scaledDp(12f))
        ) {
            DataCard(
                title = "Температура",
                value = uiState.deviceData?.temperature?.let { "%.1f".format(it) } ?: "--",
                unit = "°C",
                icon = "🌡️",
                modifier = Modifier.weight(1f)
            )
            DataCard(
                title = "Влажность",
                value = uiState.deviceData?.humidity?.let { "%.1f".format(it) } ?: "--",
                unit = "%",
                icon = "💧",
                modifier = Modifier.weight(1f)
            )
        }
        
        // Кнопки управления - базовая высота для вертикальной ориентации
        val configuration = LocalConfiguration.current
        val baseButtonHeightPortrait = 110f // Оригинальное значение
        
        // Кнопка обогрева: тап = вкл/выкл (ручной 5 мин), долгое нажатие = безусловный обогрев (ручной без автосброса)
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(scaledDp(baseButtonHeightPortrait))
                .pointerInput(Unit) {
                    detectTapGestures(
                        onTap = { viewModel.toggleHeater() },
                        onLongPress = { viewModel.setHeaterPersistent() }
                    )
                }
        ) {
            ControlButton(
                text = if (uiState.deviceData?.heaterState == true) "Выключить обогрев" else "Включить обогрев",
                onClick = {},
                backgroundColor = if (uiState.deviceData?.heaterState == true) HeaterOff else HeaterOn,
                enabled = uiState.deviceData?.heaterOffMode != true,
                modifier = Modifier.fillMaxSize(),
                useClickable = false
            )
        }
        
        // Кнопка показывает режим, на который можно переключиться (в статус-баре — текущий)
        ControlButton(
            text = if (uiState.deviceData?.autoMode == true) "Ручной режим" else "Автоматический режим",
            onClick = { viewModel.setAutoMode() },
            backgroundColor = if (uiState.deviceData?.autoMode == true) ModeManual else ModeAuto,
            enabled = uiState.deviceData?.heaterOffMode != true,
            modifier = Modifier.height(scaledDp(baseButtonHeightPortrait))
        )
        
        ControlButton(
            text = if (uiState.deviceData?.heaterOffMode == true) "Включить" else "Выключить",
            onClick = { viewModel.setOffMode() },
            backgroundColor = ModeOff,
            modifier = Modifier.height(scaledDp(baseButtonHeightPortrait))
        )
        
        // Кнопки настроек
        ControlButton(
            text = "Калибровка",
            onClick = onCalibrationClick,
            backgroundColor = DarkGray,
            modifier = Modifier.height(scaledDp(baseButtonHeightPortrait))
        )
        
        ControlButton(
            text = "Настройки WiFi",
            onClick = onWiFiClick,
            backgroundColor = DarkGray,
            modifier = Modifier.height(scaledDp(baseButtonHeightPortrait))
        )
    }
}

@Composable
fun LandscapeLayout(
    uiState: com.example.cam_assist.ui.viewmodel.DeviceUiState,
    viewModel: DeviceViewModel,
    onCalibrationClick: () -> Unit,
    onWiFiClick: () -> Unit
) {
    val configuration = LocalConfiguration.current
    val screenWidthDp = configuration.screenWidthDp.toFloat()
    val screenHeightDp = configuration.screenHeightDp.toFloat()
    val aspectRatio = screenWidthDp / screenHeightDp
    
    // Определяем категорию экрана для адаптации отступов
    val isWideScreen = screenWidthDp > 1000f && screenHeightDp < 800f
    val isTablet = aspectRatio < 1.5f
    
    // Единый отступ между всеми элементами
    val uniformSpacing = when {
        isWideScreen -> 12f                      // Стандартный отступ для широких экранов
        isTablet -> 10f                          // Меньше отступ для планшетов
        else -> 12f                              // Стандартный отступ
    }
    
    // Отступы от краев экрана
    val screenPadding = 16f
    
    // Вычисляем высоту заголовка (приблизительно)
    val titleFontSize = 23f
    val titleLineHeight = titleFontSize * 1.2f // Примерная высота строки
    val titlePadding = uniformSpacing
    val titleHeight = titleLineHeight + titlePadding * 2f
    
    // Вычисляем высоту статус-бара (приблизительно)
    val statusBarPaddingV = 15f * 2f // vertical padding
    val statusBarContentHeight = 50f // Примерная высота содержимого статус-бара
    val statusBarHeight = statusBarPaddingV + statusBarContentHeight
    
    // Вычисляем доступную высоту для кнопок
    // Формула: (высота экрана - (статус бар + надпись + отступы)) / 5
    // Учитываем: верхний отступ, заголовок, статус бар, отступы, нижний отступ
    // 5 кнопок: включение, режим, калибровка, WiFi, выключить
    val usedHeight = screenPadding + // верхний отступ
                     uniformSpacing + // отступ перед заголовком
                     titleHeight + // заголовок
                     uniformSpacing + // отступ после заголовка
                     statusBarHeight + // статус бар
                     uniformSpacing + // отступ после статус-бара
                     screenPadding // нижний отступ
    
    // Вычисляем доступную высоту для основного контента (Row с кнопками и плитками)
    val availableHeight = screenHeightDp - usedHeight
    
    // Вычисляем базовую высоту кнопок динамически
    // Делим доступную высоту на 3.5 (4 кнопки: включение, режим, WiFi, калибровка)
    // Кнопка выключения будет отдельно, поэтому не учитываем её здесь
    // Все кнопки будут иметь одинаковую базовую высоту
    val baseButtonHeight = (availableHeight / 3.5f).coerceIn(100f, 250f) // Увеличено: деление на 3.5 вместо 4, минимум 100 вместо 80, максимум 250 вместо 200
    
    // Базовая высота плиток = базовая высота кнопки включение/выключение + промежуток + базовая высота кнопки ручного/автоматического режима
    // Формула: baseButtonHeight + uniformSpacing + baseButtonHeight
    val baseCardHeight = baseButtonHeight + uniformSpacing + baseButtonHeight
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(scaledDp(16f))
    ) {
        // Заголовок по центру с мягким световым ореолом статуса подключения
        Spacer(modifier = Modifier.height(scaledDp(uniformSpacing)))
        val connectionStatusColor = when {
            uiState.isConnected && uiState.error == null && !uiState.isLoading -> 
                HeaterOn // Зеленый цвет при успешном подключении (ONLINE)
            else -> 
                HeaterOff // Красный цвет если подключение не удалось или продолжается (OFFLINE)
        }
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = scaledDp(uniformSpacing)),
            contentAlignment = Alignment.Center
        ) {
            // Вычисляем радиусы размытия заранее для плавного градиента
            val density = LocalDensity.current
            // Увеличиваем радиусы для более широкого свечения
            val blurRadii = listOf(
                with(density) { scaledDp(35f).toPx() }, // Внешний слой - самый широкий
                with(density) { scaledDp(28f).toPx() },
                with(density) { scaledDp(22f).toPx() },
                with(density) { scaledDp(16f).toPx() },
                with(density) { scaledDp(11f).toPx() },
                with(density) { scaledDp(7f).toPx() },
                with(density) { scaledDp(4f).toPx() }  // Внутренний слой
            )
            // Плавные альфа-каналы для градиента (от внешнего к внутреннему)
            val alphaValues = listOf(0.25f, 0.35f, 0.45f, 0.55f, 0.65f, 0.75f, 0.85f)
            
            // Создаем плавное неоновое свечение через множество слоев
            blurRadii.forEachIndexed { index, blurRadius ->
                Text(
                    text = "Обогрев камеры ассистентов",
                    style = MaterialTheme.typography.headlineMedium.copy(
                        fontSize = scaledSp(23f)
                    ),
                    color = connectionStatusColor.copy(alpha = alphaValues[index]),
                    modifier = Modifier
                        .align(Alignment.Center)
                        .graphicsLayer {
                            renderEffect = androidx.compose.ui.graphics.BlurEffect(
                                radiusX = blurRadius,
                                radiusY = blurRadius,
                                edgeTreatment = androidx.compose.ui.graphics.TileMode.Clamp
                            )
                        },
                    textAlign = androidx.compose.ui.text.style.TextAlign.Center
                )
            }
            // Основной текст поверх всех слоев
            Text(
                text = "Обогрев камеры ассистентов",
                style = MaterialTheme.typography.headlineMedium.copy(
                    fontSize = scaledSp(23f)
                ),
                color = TextPrimary,
                modifier = Modifier.align(Alignment.Center),
                textAlign = androidx.compose.ui.text.style.TextAlign.Center
            )
        }
        
        // Статус бар
        StatusBar(uiState, viewModel)
        
        Spacer(modifier = Modifier.height(scaledDp(uniformSpacing)))
        
        // Основной контент с кнопками - Row с двумя колонками
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f),
            horizontalArrangement = Arrangement.spacedBy(scaledDp(uniformSpacing))
        ) {
            // Левая колонка - карточки датчиков и кнопка калибровки
            Column(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight(),
                verticalArrangement = Arrangement.spacedBy(scaledDp(uniformSpacing))
            ) {
                // Карточки с данными (вверху) - высота точно рассчитывается по формуле:
                // высота плиток = высота кнопки включения/выключения + отступ + высота кнопки ручной/автоматический режим
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(scaledDp(baseCardHeight)), // Фиксированная высота по формуле: baseButtonHeight + uniformSpacing + baseButtonHeight
                    horizontalArrangement = Arrangement.spacedBy(scaledDp(uniformSpacing))
                ) {
                    DataCard(
                        title = "Температура",
                        value = uiState.deviceData?.temperature?.let { "%.1f".format(it) } ?: "--",
                        unit = "°C",
                        icon = "🌡️",
                        cardHeightDp = baseCardHeight, // Передаем расчетную высоту для пропорционального масштабирования содержимого
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight() // Заполняем всю высоту Row
                    )
                    DataCard(
                        title = "Влажность",
                        value = uiState.deviceData?.humidity?.let { "%.1f".format(it) } ?: "--",
                        unit = "%",
                        icon = "💧",
                        cardHeightDp = baseCardHeight, // Передаем расчетную высоту для пропорционального масштабирования содержимого
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight() // Заполняем всю высоту Row
                    )
                }
                
                // Spacer для заполнения пространства
                Spacer(modifier = Modifier.weight(1f))
                
                // Кнопка калибровки (внизу левой колонки)
                ControlButton(
                    text = "Калибровка",
                    onClick = onCalibrationClick,
                    backgroundColor = DarkGray,
                    buttonHeightDp = baseButtonHeight,
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(scaledDp(baseButtonHeight))
                )
            }
            
            // Правая колонка - кнопки управления
            Column(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight(),
                verticalArrangement = Arrangement.spacedBy(scaledDp(uniformSpacing))
            ) {
                // Кнопка обогрева: тап = вкл/выкл, долгое нажатие = безусловный обогрев (ручной без автосброса)
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(scaledDp(baseButtonHeight))
                        .pointerInput(Unit) {
                            detectTapGestures(
                                onTap = { viewModel.toggleHeater() },
                                onLongPress = { viewModel.setHeaterPersistent() }
                            )
                        }
                ) {
                    ControlButton(
                        text = if (uiState.deviceData?.heaterState == true) "Выключить обогрев" else "Включить обогрев",
                        onClick = {},
                        backgroundColor = if (uiState.deviceData?.heaterState == true) HeaterOff else HeaterOn,
                        enabled = uiState.deviceData?.heaterOffMode != true,
                        buttonHeightDp = baseButtonHeight,
                        modifier = Modifier.fillMaxSize(),
                        useClickable = false
                    )
                }
                
                // Кнопка показывает режим, на который можно переключиться (в статус-баре — текущий)
                ControlButton(
                    text = if (uiState.deviceData?.autoMode == true) "Ручной режим" else "Автоматический режим",
                    onClick = { viewModel.setAutoMode() },
                    backgroundColor = if (uiState.deviceData?.autoMode == true) ModeManual else ModeAuto,
                    enabled = uiState.deviceData?.heaterOffMode != true,
                    buttonHeightDp = baseButtonHeight, // Передаем реальную высоту для пропорционального масштабирования
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(scaledDp(baseButtonHeight)) // Фиксированная высота, как у остальных кнопок
                )
                
                // Spacer для заполнения пространства
                Spacer(modifier = Modifier.weight(1f))
                
                // Кнопка настройки WiFi (внизу правой колонки)
                ControlButton(
                    text = "Настройки WiFi",
                    onClick = onWiFiClick,
                    backgroundColor = DarkGray,
                    buttonHeightDp = baseButtonHeight,
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(scaledDp(baseButtonHeight))
                )
            }
        }
        
        // Отступ перед кнопкой выключения
        Spacer(modifier = Modifier.height(scaledDp(uniformSpacing)))
        
        // Кнопка выключения режимов - занимает всю ширину (оба столбца), как статус бар
        ControlButton(
            text = if (uiState.deviceData?.heaterOffMode == true) "Включить" else "Выключить",
            onClick = { viewModel.setOffMode() },
            backgroundColor = ModeOff,
            buttonHeightDp = baseButtonHeight, // Передаем реальную высоту для пропорционального масштабирования
            modifier = Modifier
                .fillMaxWidth()
                .height(scaledDp(baseButtonHeight)) // Такая же высота, как остальные кнопки
        )
    }
}

@Composable
fun StatusBar(
    uiState: com.example.cam_assist.ui.viewmodel.DeviceUiState,
    viewModel: DeviceViewModel
) {
    val data = uiState.deviceData
    
    // Компактный статус бар точно как в веб-портале
    // Всегда отображается, даже если данных еще нет (как в веб-портале)
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(
                color = androidx.compose.ui.graphics.Color(0x4D000000), // rgba(0, 0, 0, 0.3)
                shape = androidx.compose.foundation.shape.RoundedCornerShape(16.dp) // Такое же скругление, как у кнопок и плиток
            )
            .border(
                width = scaledDp(1f),
                color = CardBorder, // rgba(255, 255, 255, 0.1)
                shape = androidx.compose.foundation.shape.RoundedCornerShape(16.dp) // Такое же скругление, как у кнопок и плиток
            )
            .padding(horizontal = scaledDp(27f), vertical = scaledDp(15f)), // Уменьшено на 5 единиц
        horizontalArrangement = Arrangement.SpaceAround,
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Обогрев
        CompactStatusItem(
            label = "Обогрев",
            value = if (data?.heaterState == true) "Вкл" else "Выкл",
            valueColor = if (data?.heaterState == true) HeaterOn else HeaterOff,
            valueBackground = if (data?.heaterState == true) HeaterOn else HeaterOff
        )
        
        // Режим
        CompactStatusItem(
            label = "Режим",
            value = when {
                data?.heaterOffMode == true -> "Выкл"
                data?.autoMode == true -> "Авто"
                data != null -> "Ручной"
                else -> "Авто" // Дефолтное значение как в веб-портале
            },
            valueColor = when {
                data?.heaterOffMode == true -> ModeOff
                data?.autoMode == true -> ModeAuto
                data != null -> ModeManual
                else -> ModeAuto // Дефолтное значение
            },
            valueBackground = when {
                data?.heaterOffMode == true -> ModeOff
                data?.autoMode == true -> ModeAuto
                data != null -> ModeManual
                else -> ModeAuto // Дефолтное значение
            }
        )
        
        // WiFi - показываем SSID при подключении к домашней сети, иначе режим доступа
        CompactStatusItem(
            label = "WiFi",
            value = when {
                data?.wifiMode == "sta" && !data.staNetwork.isNullOrBlank() -> {
                    // Показываем SSID сети (обрезаем если слишком длинный)
                    val ssid = data.staNetwork
                    if (ssid.length > 12) ssid.take(12) + "..." else ssid
                }
                data?.wifiMode == "sta" -> "STA"
                else -> "AP"
            },
            valueColor = if (data?.wifiMode == "sta") WiFiSTA else WiFiAP,
            valueBackground = if (data?.wifiMode == "sta") WiFiSTA else WiFiAP
        )
    }
}

@Composable
fun CompactStatusItem(
    label: String,
    value: String,
    valueColor: androidx.compose.ui.graphics.Color,
    valueBackground: androidx.compose.ui.graphics.Color
) {
    // Базовые размеры для эталонного экрана - уменьшены на 5 единиц
    val baseHorizontalPadding = 3f // Уменьшено на 5 единиц
    val baseMinWidth = 155f // Уменьшено на 5 единиц
    val baseMaxWidth = 195f // Уменьшено на 5 единиц
    val baseLabelFontSize = 19f // Уменьшено на 5 единиц
    val baseLabelLineHeight = 23f // Уменьшено на 5 единиц
    val baseSpacerHeight = 3f // Уменьшено на 5 единиц
    val baseValueMinWidth = 125f // Уменьшено на 5 единиц
    val baseValueMaxWidth = 165f // Уменьшено на 5 единиц
    val baseValuePaddingH = 11f // Уменьшено на 5 единиц
    val baseValuePaddingV = 5f // Уменьшено на 5 единиц
    val baseValueFontSize = 25f // Уменьшено на 5 единиц
    val baseValueLineHeight = 31f // Уменьшено на 5 единиц
    val baseCornerRadius = 11f // Уменьшено на 5 единиц
    
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
        modifier = Modifier
            .padding(horizontal = scaledDp(baseHorizontalPadding))
            .widthIn(min = scaledDp(baseMinWidth), max = scaledDp(baseMaxWidth))
    ) {
        // Метка (сверху) - точно как в веб-версии
        Text(
            text = label.uppercase(),
            fontSize = scaledSp(baseLabelFontSize),
            color = TextTertiary, // rgba(255, 255, 255, 0.7)
            fontWeight = FontWeight.Normal, // font-weight: normal в веб-версии (не medium)
            letterSpacing = scaledSp(0.3f),
            lineHeight = scaledSp(baseLabelLineHeight)
        )
        
        Spacer(modifier = Modifier.height(scaledDp(baseSpacerHeight)))
        
        // Значение (снизу) с цветным фоном - точно как в веб-версии
        Box(
            modifier = Modifier
                .background(
                    color = valueBackground.copy(alpha = 0.2f), // rgba(color, 0.2)
                    shape = androidx.compose.foundation.shape.RoundedCornerShape(scaledDp(baseCornerRadius))
                )
                .widthIn(min = scaledDp(baseValueMinWidth), max = scaledDp(baseValueMaxWidth))
                .padding(horizontal = scaledDp(baseValuePaddingH), vertical = scaledDp(baseValuePaddingV)),
            contentAlignment = Alignment.Center
        ) {
            Text(
                text = value,
                fontSize = scaledSp(baseValueFontSize),
                color = valueColor,
                fontWeight = FontWeight.Bold, // font-weight: 700
                textAlign = TextAlign.Center,
                minLines = 1,
                maxLines = 1,
                lineHeight = scaledSp(baseValueLineHeight)
            )
        }
    }
}

