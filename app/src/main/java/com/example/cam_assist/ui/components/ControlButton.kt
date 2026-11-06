package com.example.cam_assist.ui.components

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.cam_assist.ui.theme.*
import com.example.cam_assist.ui.utils.scaledDp
import com.example.cam_assist.ui.utils.scaledSp

@Composable
fun ControlButton(
    text: String,
    onClick: () -> Unit,
    enabled: Boolean = true,
    backgroundColor: Color = ModeAuto,
    modifier: Modifier = Modifier,
    isCompact: Boolean = false,
    buttonHeightDp: Float? = null, // Реальная высота кнопки для пропорционального масштабирования
    useClickable: Boolean = true // false = только визуал, события обрабатывает родитель (для кнопки обогрева с долгим нажатием)
) {
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.screenWidthDp > configuration.screenHeightDp
    
    // Определяем широкоформатный экран для дополнительного увеличения размеров
    val isWideScreen = configuration.screenWidthDp > 1000 && configuration.screenHeightDp < 800
    
    // Базовая высота кнопок для расчета масштаба
    val baseButtonHeight = when {
        isWideScreen -> 137f
        isLandscape -> 117f
        else -> 110f
    }
    
    // Вычисляем масштаб на основе реальной высоты кнопки
    val heightScale = if (buttonHeightDp != null && buttonHeightDp > 0f && isLandscape) {
        (buttonHeightDp / baseButtonHeight).coerceIn(0.5f, 2.0f) // Ограничиваем разумными пределами
    } else {
        1f // Если высота не передана или не landscape, используем базовый масштаб
    }
    
    // Базовые размеры для эталонного экрана
    // Увеличиваем базовые размеры для широкоформатных экранов
    val basePadding = when {
        isCompact -> 14f
        isWideScreen -> 28f // Увеличено для широкоформатных экранов
        isLandscape -> 24f // Увеличено для горизонтальной ориентации
        else -> 20f
    }
    val baseFontSize = when {
        isCompact -> 28f // Увеличено на 6 единиц
        isWideScreen -> 40f // Увеличено на 6 единиц
        isLandscape -> 38f // Увеличено на 6 единиц
        else -> 36f // Увеличено на 6 единиц
    }
    // Используем такое же скругление, как у плиток с показаниями датчиков (16.dp)
    val cornerRadius = 16.dp
    
    // Применяем автомасштабирование и масштаб на основе высоты кнопки
    val padding = scaledDp(basePadding * heightScale)
    val fontSize = scaledSp(baseFontSize * heightScale)
    
    // Взаимодействие для отслеживания нажатия
    val interactionSource = remember { MutableInteractionSource() }
    val isPressed by interactionSource.collectIsPressedAsState()
    
    // Анимация масштаба при нажатии (как в веб-портале: scale(0.96))
    // Увеличиваем разницу для более заметной анимации в вертикальной ориентации
    val scale by animateFloatAsState(
        targetValue = if (isPressed && enabled) 0.94f else 1f, // Увеличено с 0.96 до 0.94 для большей заметности
        animationSpec = tween(
            durationMillis = if (isPressed) 50 else 200, // Увеличено время возврата для плавности
            easing = androidx.compose.animation.core.FastOutSlowInEasing
        ),
        label = "button_scale"
    )
    
    // Создаем градиентный фон (как в веб-портале: linear-gradient(135deg, ...))
    // 135 градусов = диагональ от левого верхнего угла к правому нижнему
    val gradientColors = remember(backgroundColor) {
        listOf(
            backgroundColor,
            // Создаем более темный оттенок для градиента (как в веб-портале)
            Color(
                red = (backgroundColor.red * 0.85f).coerceIn(0f, 1f),
                green = (backgroundColor.green * 0.85f).coerceIn(0f, 1f),
                blue = (backgroundColor.blue * 0.85f).coerceIn(0f, 1f),
                alpha = backgroundColor.alpha
            )
        )
    }
    
    // Угол 135 градусов: от левого верхнего угла к правому нижнему
    // В Compose это означает: start = (0, 0), end = (width, height)
    val gradientStart = androidx.compose.ui.geometry.Offset(0f, 0f)
    val gradientEnd = androidx.compose.ui.geometry.Offset(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)
    
    // Используем более мягкую тень без темного прямоугольника
    Box(
        modifier = modifier
            .fillMaxWidth()
            .shadow(
                elevation = if (enabled) 4.dp else 0.dp, // Уменьшена высота тени
                shape = RoundedCornerShape(cornerRadius),
                spotColor = Color.Black.copy(alpha = 0.2f), // Более мягкая тень
                ambientColor = Color.Black.copy(alpha = 0.1f) // Более мягкая тень
            )
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .fillMaxHeight() // Заполняем всю доступную высоту
                .scale(scale) // Применяем анимацию масштаба
                .clip(RoundedCornerShape(cornerRadius)) // Clip применяем только к содержимому
                .then(
                    if (useClickable) {
                        Modifier.clickable(
                            enabled = enabled,
                            onClick = onClick,
                            interactionSource = interactionSource,
                            indication = null // Отключаем стандартный ripple, используем свою анимацию
                        )
                    } else Modifier
                )
                .background(
                    brush = if (enabled) {
                        Brush.linearGradient(
                            colors = gradientColors,
                            start = gradientStart,
                            end = gradientEnd
                        )
                    } else {
                        Brush.linearGradient(
                            colors = listOf(CardBackground, CardBackground)
                        )
                    }
                )
                .border(
                    width = 1.dp,
                    color = if (enabled) backgroundColor.copy(alpha = 0.3f) else CardBorder,
                    shape = RoundedCornerShape(cornerRadius)
                )
                .padding(padding),
            contentAlignment = Alignment.Center
        ) {
            Text(
                text = text,
                fontSize = fontSize,
                color = if (enabled) Color.White else TextSecondary,
                fontWeight = FontWeight.Bold,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                // Оптимальный lineHeight для предотвращения обрезания без разрезания текста пополам
                lineHeight = fontSize * 1.25f,
                modifier = Modifier
            )
        }
    }
}

