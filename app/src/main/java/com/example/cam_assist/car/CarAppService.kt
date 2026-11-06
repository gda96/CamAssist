package com.example.cam_assist.car

import androidx.car.app.CarAppService
import androidx.car.app.Session
import androidx.car.app.validation.HostValidator

/**
 * Сервис для интеграции с Android Auto.
 * Обеспечивает правильное отображение приложения в списке доступных приложений
 * для настройки панели запуска в Android Auto.
 */
class CarAppService : CarAppService() {
    
    /**
     * Создает валидатор хостов для Android Auto.
     * Разрешает все хосты для упрощения разработки.
     * В production можно настроить более строгий валидатор.
     */
    override fun createHostValidator(): HostValidator {
        return HostValidator.ALLOW_ALL_HOSTS_VALIDATOR
    }

    /**
     * Создает новую сессию для Android Auto.
     * Вызывается при каждом подключении к Android Auto.
     */
    override fun onCreateSession(): Session {
        return CarSession()
    }
}

