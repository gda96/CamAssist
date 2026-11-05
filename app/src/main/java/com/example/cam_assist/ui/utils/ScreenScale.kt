package com.example.cam_assist.ui.utils

import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.runtime.Composable
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.TextUnit
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * Утилита для автомасштабирования элементов интерфейса
 * в зависимости от размера экрана
 */
object ScreenScale {
    // Базовые размеры для эталонного экрана (1920x720 - типичный автомобильный экран)
    private const val BASE_WIDTH_DP = 1920f
    private const val BASE_HEIGHT_DP = 720f
    
    // Минимальный и максимальный масштаб для предотвращения слишком маленьких/больших элементов
    private const val MIN_SCALE = 0.6f
    private const val MAX_SCALE = 2.0f // Увеличено для поддержки широкоформатных экранов
    
    /**
     * Вычисляет масштабный коэффициент для горизонтальной ориентации
     * на основе ширины и высоты экрана
     * Продвинутый алгоритм для всех возможных соотношений сторон Android устройств
     * 
     * Поддерживаемые соотношения сторон (в landscape):
     * - 4:3 = 1.33 (старые планшеты)
     * - 3:2 = 1.5 (некоторые планшеты)
     * - 16:10 = 1.6 (планшеты)
     * - 16:9 = 1.78 (стандартные смартфоны)
     * - 18:9 = 2.0 (современные смартфоны)
     * - 19.5:9 = 2.17 (iPhone X и подобные)
     * - 20:9 = 2.22 (современные флагманы)
     * - 21:9 = 2.33 (ультраширокие экраны)
     * - 2.67 (1920x720 - автомобильные экраны)
     * - 3.0+ (очень широкие автомобильные экраны)
     */
    @Composable
    fun getLandscapeScale(): Float {
        val configuration = LocalConfiguration.current
        val screenWidthDp = configuration.screenWidthDp.toFloat()
        val screenHeightDp = configuration.screenHeightDp.toFloat()
        
        // Вычисляем соотношение сторон
        val aspectRatio = screenWidthDp / screenHeightDp
        val baseAspectRatio = BASE_WIDTH_DP / BASE_HEIGHT_DP // 1920/720 = 2.67
        
        // Определяем категорию экрана по соотношению сторон
        val screenCategory = when {
            aspectRatio >= 3.0f -> "ultra_wide_car"      // Очень широкие автомобильные экраны (3.0+)
            aspectRatio >= 2.5f -> "wide_car"            // Широкие автомобильные экраны (2.5-3.0)
            aspectRatio >= 2.2f -> "ultra_wide_phone"      // Ультраширокие смартфоны (21:9, 20:9)
            aspectRatio >= 2.0f -> "wide_phone"           // Широкие смартфоны (18:9, 19.5:9)
            aspectRatio >= 1.7f -> "standard_phone"       // Стандартные смартфоны (16:9)
            aspectRatio >= 1.5f -> "tablet_wide"          // Широкие планшеты (3:2, 16:10)
            else -> "tablet_square"                       // Квадратные планшеты (4:3)
        }
        
        // Определяем широкоформатный экран (автомобильные экраны)
        val isWideScreen = screenWidthDp > 1000f && screenHeightDp < 800f
        
        // Базовые масштабы для ширины и высоты
        val widthScale = screenWidthDp / BASE_WIDTH_DP
        val heightScale = screenHeightDp / BASE_HEIGHT_DP
        
        // Адаптивные веса в зависимости от категории экрана
        val (widthWeight, heightWeight, aspectMultiplier) = when (screenCategory) {
            "ultra_wide_car" -> {
                // Очень широкие автомобильные экраны - приоритет ширине, большой множитель
                Triple(0.8f, 0.2f, 1.6f)
            }
            "wide_car" -> {
                // Широкие автомобильные экраны - приоритет ширине, средний множитель
                Triple(0.75f, 0.25f, 1.5f)
            }
            "ultra_wide_phone" -> {
                // Ультраширокие смартфоны (21:9, 20:9) - баланс с уклоном в ширину
                Triple(0.7f, 0.3f, 1.2f)
            }
            "wide_phone" -> {
                // Широкие смартфоны (18:9, 19.5:9) - стандартный баланс
                Triple(0.65f, 0.35f, 1.1f)
            }
            "standard_phone" -> {
                // Стандартные смартфоны (16:9) - стандартный баланс
                Triple(0.6f, 0.4f, 1.0f)
            }
            "tablet_wide" -> {
                // Широкие планшеты (3:2, 16:10) - больше учитываем высоту
                Triple(0.5f, 0.5f, 0.95f)
            }
            "tablet_square" -> {
                // Квадратные планшеты (4:3) - приоритет высоте
                Triple(0.4f, 0.6f, 0.9f)
            }
            else -> {
                // По умолчанию - стандартный баланс
                Triple(0.6f, 0.4f, 1.0f)
            }
        }
        
        // Вычисляем базовый масштаб
        var scale = (widthScale * widthWeight + heightScale * heightWeight) * aspectMultiplier
        
        // Дополнительные корректировки для экстремальных случаев
        when {
            // Для очень узких экранов (высота < 400dp) - уменьшаем масштаб
            screenHeightDp < 400f -> scale *= 0.85f
            
            // Для очень широких экранов (ширина > 2000dp) - немного увеличиваем
            screenWidthDp > 2000f -> scale *= 1.1f
            
            // Для очень маленьких экранов (ширина < 600dp) - уменьшаем масштаб
            screenWidthDp < 600f -> scale *= 0.9f
        }
        
        // Ограничиваем масштаб минимальным и максимальным значениями
        return scale.coerceIn(MIN_SCALE, MAX_SCALE)
    }
    
    /**
     * Вычисляет масштабный коэффициент для вертикальной ориентации
     * Улучшенный алгоритм для различных соотношений сторон
     */
    @Composable
    fun getPortraitScale(): Float {
        val configuration = LocalConfiguration.current
        val screenWidthDp = configuration.screenWidthDp.toFloat()
        val screenHeightDp = configuration.screenHeightDp.toFloat()
        
        // Вычисляем соотношение сторон (для портрета высота > ширины)
        val aspectRatio = screenHeightDp / screenWidthDp
        
        // Для вертикальной ориентации используем комбинацию ширины и высоты
        val widthScale = screenWidthDp / 1080f // Базовая ширина для портрета
        val heightScale = screenHeightDp / 1920f // Базовая высота для портрета
        
        // Адаптивные веса в зависимости от соотношения сторон
        // Для более узких экранов (большое соотношение) больше учитываем ширину
        // Для более широких экранов (малое соотношение) больше учитываем высоту
        val widthWeight = when {
            aspectRatio > 2.0f -> 0.4f // Очень узкий экран (например, 640x1264)
            aspectRatio > 1.8f -> 0.35f // Узкий экран
            aspectRatio < 1.5f -> 0.25f // Более широкий экран
            else -> 0.3f // Стандартное соотношение
        }
        val heightWeight = 1f - widthWeight
        
        var scale = (widthScale * widthWeight + heightScale * heightWeight)
        
        // Корректировка для экстремальных соотношений сторон
        if (aspectRatio > 2.0f) {
            // Для очень узких экранов немного уменьшаем масштаб, чтобы элементы поместились
            scale *= 0.85f
        } else if (aspectRatio < 1.4f) {
            // Для более широких экранов немного увеличиваем
            scale *= 1.05f
        }
        
        return scale.coerceIn(MIN_SCALE, MAX_SCALE)
    }
    
    /**
     * Применяет масштаб к Dp значению
     */
    fun scaleDp(baseDp: Float, scale: Float): Dp {
        return (baseDp * scale).dp
    }
    
    /**
     * Применяет масштаб к Sp значению (для шрифтов)
     */
    fun scaleSp(baseSp: Float, scale: Float): TextUnit {
        return (baseSp * scale).sp
    }
}

/**
 * Вспомогательные функции для удобного использования в Composable
 */
@Composable
fun scaledDp(baseDp: Float): Dp {
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.screenWidthDp > configuration.screenHeightDp
    val scale = if (isLandscape) {
        ScreenScale.getLandscapeScale()
    } else {
        ScreenScale.getPortraitScale()
    }
    return ScreenScale.scaleDp(baseDp, scale)
}

@Composable
fun scaledSp(baseSp: Float): TextUnit {
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.screenWidthDp > configuration.screenHeightDp
    val scale = if (isLandscape) {
        ScreenScale.getLandscapeScale()
    } else {
        ScreenScale.getPortraitScale()
    }
    return ScreenScale.scaleSp(baseSp, scale)
}
