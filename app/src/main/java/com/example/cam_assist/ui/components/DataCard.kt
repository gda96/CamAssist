package com.example.cam_assist.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.cam_assist.ui.theme.*
import com.example.cam_assist.ui.utils.scaledDp
import com.example.cam_assist.ui.utils.scaledSp

@Composable
fun DataCard(
    title: String,
    value: String,
    unit: String,
    icon: String = "",
    modifier: Modifier = Modifier,
    cardHeightDp: Float? = null // Реальная высота плитки для пропорционального масштабирования
) {
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.screenWidthDp > configuration.screenHeightDp
    val screenWidthDp = configuration.screenWidthDp.toFloat()
    val screenHeightDp = configuration.screenHeightDp.toFloat()
    val aspectRatio = if (isLandscape) screenWidthDp / screenHeightDp else screenHeightDp / screenWidthDp
    
    // Определяем категорию экрана для адаптации размеров
    val isWideScreen = isLandscape && screenWidthDp > 1000f && screenHeightDp < 800f
    val isUltraWide = isLandscape && aspectRatio >= 2.5f
    val isStandardWide = isLandscape && aspectRatio >= 2.0f && aspectRatio < 2.5f
    val isStandard = isLandscape && aspectRatio >= 1.5f && aspectRatio < 2.0f
    val isTablet = isLandscape && aspectRatio < 1.5f
    
    // Базовая высота плиток для расчета масштаба
    val baseCardHeight = when {
        isWideScreen && isUltraWide -> 294f
        isWideScreen -> 286f
        isUltraWide -> 274f
        isStandardWide -> 264f
        isStandard -> 246f
        isTablet -> 230f
        isLandscape -> 246f
        else -> 160f
    }
    
    // Вычисляем масштаб на основе реальной высоты плитки
    val heightScale = if (cardHeightDp != null && cardHeightDp > 0f) {
        (cardHeightDp / baseCardHeight).coerceIn(0.5f, 2.0f) // Ограничиваем разумными пределами
    } else {
        1f // Если высота не передана, используем базовый масштаб
    }
    
    // Базовые размеры для эталонного экрана
    val baseCardPadding = when {
        isWideScreen && isUltraWide -> 22f
        isWideScreen -> 20f
        isUltraWide -> 18f
        isStandardWide -> 17f
        isStandard -> 16f
        isTablet -> 14f
        else -> 16f
    }
    
    val baseIconSize = when {
        isWideScreen && isUltraWide -> 76f
        isWideScreen -> 72f
        isUltraWide -> 68f
        isStandardWide -> 66f
        isStandard -> 64f
        isTablet -> 60f
        else -> 64f
    }
    
    val baseValueFontSize = when {
        isWideScreen && isUltraWide -> 88f
        isWideScreen -> 84f
        isUltraWide -> 78f
        isStandardWide -> 76f
        isStandard -> 72f
        isTablet -> 68f
        else -> 72f
    }
    
    val baseValueLineHeight = when {
        isWideScreen && isUltraWide -> 104f
        isWideScreen -> 100f
        isUltraWide -> 92f
        isStandardWide -> 90f
        isStandard -> 84f
        isTablet -> 80f
        else -> 84f
    }
    
    val baseTitleFontSize = 28f
    val baseUnitFontSize = if (isWideScreen) 40f else 36f
    
    // Применяем автомасштабирование и масштаб на основе высоты плитки
    val cardPadding = scaledDp(baseCardPadding * heightScale)
    val iconSize = scaledSp(baseIconSize * heightScale)
    val valueFontSize = scaledSp(baseValueFontSize * heightScale)
    val valueLineHeight = scaledSp(baseValueLineHeight * heightScale)
    val titleFontSize = scaledSp(baseTitleFontSize * heightScale)
    val unitFontSize = scaledSp(baseUnitFontSize * heightScale)
    val elementSpacing = scaledDp(8f * heightScale)
    
    Card(
        modifier = modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(16.dp)),
        colors = CardDefaults.cardColors(
            containerColor = CardBackground
        ),
        shape = RoundedCornerShape(16.dp),
        border = androidx.compose.foundation.BorderStroke(1.dp, CardBorder)
    ) {
        Column(
            modifier = Modifier
                .padding(cardPadding)
                .fillMaxWidth()
                .fillMaxHeight(),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            if (icon.isNotEmpty()) {
                Text(
                    text = icon,
                    fontSize = iconSize,
                    modifier = Modifier.padding(bottom = elementSpacing)
                )
            }
            Text(
                text = title,
                fontSize = titleFontSize,
                color = TextSecondary,
                fontWeight = FontWeight.Medium,
                modifier = Modifier.padding(bottom = elementSpacing)
            )
            Row(
                verticalAlignment = Alignment.Bottom,
                horizontalArrangement = Arrangement.Center
            ) {
                Text(
                    text = value,
                    fontSize = valueFontSize,
                    color = TextPrimary,
                    fontWeight = FontWeight.Bold,
                    letterSpacing = scaledSp(-1f * heightScale),
                    lineHeight = valueLineHeight,
                    modifier = Modifier.padding(bottom = scaledDp(if (isWideScreen) 4f * heightScale else 2f * heightScale))
                )
                Text(
                    text = unit,
                    fontSize = unitFontSize,
                    color = TextSecondary,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(start = scaledDp(4f * heightScale), bottom = scaledDp(if (isWideScreen) 6f * heightScale else 4f * heightScale))
                )
            }
        }
    }
}

