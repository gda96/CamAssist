#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <pgmspace.h>

// ПРАВИЛЬНАЯ РАСПИНОВКА для ESP32-C3 Super Mini
// Согласно официальной схеме: GPIO8=SDA, GPIO9=SCL (стандартные I2C пины)
// ВНИМАНИЕ: GPIO8 подключен к встроенному синему LED - он будет мигать при работе I2C
// Если LED мешает, можно использовать альтернативные пины: GPIO4/GPIO5
#define HEATER_PIN 3      // GPIO3 (A3) - управление реле
#define BUTTON_PIN 2      // GPIO2 (A2) - сенсорная кнопка TTP-223 (A=1, B=0)
#define I2C_SDA 8         // GPIO8 (SDA) - для датчиков (официальный I2C SDA, но подключен к LED)
#define I2C_SCL 9         // GPIO9 (SCL) - для датчиков (официальный I2C SCL)

// Константы
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;
WiFiUDP udp;

// UDP настройки
const unsigned int UDP_PORT = 8888;
const unsigned long UDP_BROADCAST_INTERVAL = 2000; // Отправка каждые 2 секунды
unsigned long lastUdpBroadcast = 0;

// УБРАН МЬЮТЕКС: На ESP32-C3 (одно ядро) мьютекс вызывает ошибки xQueueGenericSend
// Используем простые переменные, так как нет реальной конкуренции за ресурсы

// Параметры WiFi (Пункт 6: String используется осторожно, избегаем фрагментации)
String apSSID = "Camera_Heater_AP";
String apPassword = "12345678";
String staSSID = "";
String staPassword = "";
bool wifiSTAEnabled = false;
bool wifiConnected = false;
unsigned long wifiReconnectTimer = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 60000; // Увеличил до 60 секунд

// Неблокирующее переподключение WiFi
struct WiFiReconnectState {
  bool isReconnecting = false;
  unsigned long reconnectStartTime = 0;
  int reconnectAttempts = 0;
  const int maxAttempts = 20;
  const unsigned long timeout = 10000; // 10 секунд
  unsigned long lastCheckTime = 0;
  const unsigned long checkInterval = 500; // Проверка каждые 500мс
};
WiFiReconnectState wifiReconnectState;

// Автоматическое переключение на AP при потере сети
unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 25000; // Оптимизировано: увеличено до 25 секунд для снижения нагрузки
int wifiDisconnectCount = 0;
const int MAX_DISCONNECT_COUNT = 5; // Увеличил до 5 проверок
bool forceAPMode = false; // Флаг принудительного режима AP

// Проверка имени точки доступа
unsigned long lastAPNameCheck = 0;
const unsigned long AP_NAME_CHECK_INTERVAL = 10000; // Проверка каждые 10 секунд

// Настройки отображения
bool showConnectionInfo = true; // По умолчанию показывать информацию о подключении

// Сканирование сетей (асинхронное) - Пункт 6: используем StaticJsonDocument для экономии памяти
// Уменьшен размер для экономии памяти
// Оптимизировано: уменьшен размер для экономии памяти (1536 байт достаточно для сканирования)
StaticJsonDocument<1536> scanResult;
bool isScanning = false;
bool scanCompleted = false;
unsigned long scanStartTime = 0;
const unsigned long SCAN_TIMEOUT = 30000; // 30 секунд на сканирование (увеличено для надежности)

// Мониторинг памяти
unsigned long lastMemoryCheck = 0;
const unsigned long MEMORY_CHECK_INTERVAL = 60000; // Оптимизировано: проверка каждые 60 секунд
const size_t MIN_FREE_HEAP = 30000; // Минимум 30KB свободной памяти

// Основные переменные
bool heaterState = false;
float temperature = 0;
float humidity = 0;
unsigned long lastSensorRead = 0;
bool sensorConnected = false;

// Кэширование данных для быстрого ответа
struct CachedData {
  char jsonBuffer[1024]; // Увеличено для передачи паролей WiFi
  unsigned long lastUpdate = 0;
  bool isValid = false;
};
CachedData cachedData;
const unsigned long CACHE_VALID_TIME = 500; // Кэш действителен 500мс

// Переменные для кнопки TTP-223 (A=1, B=0)
bool buttonPressed = false;
unsigned long buttonPressTime = 0;
static unsigned long lastButtonCheck = 0;
const unsigned long BUTTON_CHECK_INTERVAL = 5; // Проверка каждые 5мс для лучшего отклика
bool manualMode = false;
bool heaterOffMode = false; // Режим полного выключения обогрева (для мониторинга датчиков)
unsigned long manualModeStartTime = 0;
const unsigned long MANUAL_MODE_TIMEOUT = 5 * 60 * 1000;
const unsigned long AUTO_RETURN_DELAY = 30 * 1000; // 30 секунд

// Мониторинг состояния
bool lastCameraRiskState = false;
bool manualModeStartRisk = false;
unsigned long conditionChangeTime = 0;

// Калибровочные параметры
struct CalibrationParams {
  float tempThreshold = 10.0;
  float humThreshold = 70.0;
  float tempOffset = 0.0;    // Коррекция температуры
  float humOffset = 0.0;     // Коррекция влажности
};

CalibrationParams calParams;
CalibrationParams factoryCalibration;

// HTML страница без анимаций - хранится в PROGMEM (Flash) для экономии RAM
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=5.0, minimum-scale=0.5, user-scalable=yes, viewport-fit=cover">
    <title>Обогрев камеры ассистентов</title>
    <style>
        * {
            box-sizing: border-box;
            -webkit-tap-highlight-color: transparent;
            -webkit-text-size-adjust: 100%;
            -moz-text-size-adjust: 100%;
            -ms-text-size-adjust: 100%;
        }
        
        html {
            font-size: 16px;
            height: 100%;
            -webkit-font-smoothing: antialiased;
            -moz-osx-font-smoothing: grayscale;
        }
        
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            margin: 0;
            padding: 0;
            background: linear-gradient(135deg, rgba(26, 26, 46, 0.85) 0%, rgba(22, 33, 62, 0.85) 100%), url('/background.png') center center / cover no-repeat fixed;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            touch-action: manipulation;
            padding: env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left);
            position: relative;
        }
        
        body::before {
            content: '';
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.3);
            z-index: -1;
        }
        
        .container { 
            width: 100%;
            max-width: 500px;
            min-width: 280px;
            margin: 10px;
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            padding: clamp(12px, 4vw, 20px); 
            border-radius: clamp(16px, 5vw, 24px);
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3),
                        0 1px 3px rgba(255, 255, 255, 0.1);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .compact-status-bar {
            display: flex;
            justify-content: space-around;
            align-items: center;
            background: rgba(0, 0, 0, 0.3);
            padding: 8px 12px;
            border-radius: 10px;
            margin-bottom: 15px;
            font-size: 12px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .status-icon {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 4px;
            min-width: 70px;
        }
        
        .status-label {
            color: rgba(255, 255, 255, 0.7);
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .status-value {
            font-size: 13px;
            font-weight: 700;
            padding: 4px 8px;
            border-radius: 6px;
            min-width: 60px;
            text-align: center;
        }
        
        .heater-on { background: rgba(46, 204, 113, 0.2); color: #2ecc71; }
        .heater-off { background: rgba(231, 76, 60, 0.2); color: #e74c3c; }
        .mode-auto { background: rgba(52, 152, 219, 0.2); color: #3498db; }
        .mode-manual { background: rgba(243, 156, 18, 0.2); color: #f39c12; }
        .mode-off { background: rgba(149, 165, 166, 0.2); color: #95a5a6; }
        .wifi-ap { background: rgba(155, 89, 182, 0.2); color: #9b59b6; }
        .wifi-sta { background: rgba(26, 188, 156, 0.2); color: #1abc9c; }
        
        .header {
            text-align: center;
            margin-bottom: clamp(15px, 5vw, 20px);
            padding-bottom: clamp(10px, 3vw, 12px);
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .header h1 {
            margin: 0;
            font-size: clamp(18px, 6vw, 22px);
            font-weight: 700;
            color: #ffffff;
            text-shadow: 0 2px 4px rgba(0,0,0,0.3);
            letter-spacing: 0.5px;
        }
        
        .header-subtitle {
            margin-top: 5px;
            font-size: clamp(12px, 3.5vw, 13px);
            color: rgba(255, 255, 255, 0.6);
        }
        
        .sensor-error {
            background: linear-gradient(135deg, #ff416c 0%, #ff4b2b 100%);
            color: white;
            padding: clamp(10px, 3vw, 12px);
            border-radius: clamp(8px, 3vw, 10px);
            margin: clamp(10px, 3vw, 12px) 0;
            text-align: center;
            font-size: clamp(12px, 3.5vw, 14px);
            display: none;
            animation: pulse 2s infinite;
        }
        
        .data-display {
            background: rgba(0, 0, 0, 0.3);
            padding: clamp(15px, 5vw, 20px);
            margin: clamp(15px, 5vw, 20px) 0;
            border-radius: clamp(12px, 4vw, 16px);
            text-align: center;
            border: 2px solid rgba(255, 255, 255, 0.1);
            position: relative;
            overflow: hidden;
        }
        
        .data-display::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 3px;
            background: linear-gradient(90deg, #3498db, #2ecc71);
        }
        
        .data-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: clamp(15px, 4vw, 20px);
        }
        
        .data-item {
            position: relative;
        }
        
        .data-value {
            font-size: clamp(36px, 10vw, 60px);
            font-weight: 800;
            color: #ffffff;
            text-shadow: 0 4px 8px rgba(0,0,0,0.4);
            line-height: 1;
            margin: 5px 0;
            font-variant-numeric: tabular-nums;
        }
        
        .data-label {
            font-size: clamp(12px, 3.5vw, 13px);
            color: rgba(255, 255, 255, 0.7);
            margin-top: clamp(5px, 2vw, 6px);
            display: block;
            text-transform: uppercase;
            letter-spacing: 1px;
            font-weight: 600;
        }
        
        .control-buttons {
            display: grid;
            grid-template-columns: 1fr;
            gap: clamp(12px, 4vw, 15px);
            margin: clamp(15px, 5vw, 20px) 0;
        }
        
        .control-btn {
            padding: clamp(16px, 5vw, 20px);
            border: none;
            border-radius: clamp(12px, 4vw, 16px);
            font-size: clamp(15px, 4vw, 16px);
            font-weight: 700;
            cursor: pointer;
            transition: background 0.15s ease, box-shadow 0.15s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            min-height: 55px;
            position: relative;
            overflow: hidden;
            user-select: none;
            -webkit-tap-highlight-color: transparent;
            touch-action: manipulation;
        }
        
        .control-btn:active {
            transform: scale(0.96);
            transition: transform 0.05s ease;
        }
        
        .control-btn:focus {
            outline: none;
        }
        
        .control-btn:disabled {
            opacity: 0.5;
            cursor: not-allowed;
            transform: none !important;
        }
        
        .heater-btn {
            background: linear-gradient(135deg, #2ecc71 0%, #27ae60 100%);
            color: white;
            box-shadow: 0 8px 20px rgba(46, 204, 113, 0.3);
        }
        
        .heater-btn.on {
            background: linear-gradient(135deg, #e74c3c 0%, #c0392b 100%);
            box-shadow: 0 8px 20px rgba(231, 76, 60, 0.3);
        }
        
        /* Стили для кнопки в автоматическом режиме */
        .control-btn.auto-btn.mode-auto {
            background: linear-gradient(135deg, #3498db 0%, #2980b9 100%) !important;
            color: white !important;
            box-shadow: 0 8px 20px rgba(52, 152, 219, 0.3) !important;
        }
        
        /* Стили для кнопки в ручном режиме */
        .control-btn.auto-btn.mode-manual {
            background: linear-gradient(135deg, #f39c12 0%, #e67e22 100%) !important;
            color: white !important;
            box-shadow: 0 8px 20px rgba(243, 156, 18, 0.3) !important;
        }
        
        .auto-btn {
            background: linear-gradient(135deg, #3498db 0%, #2980b9 100%);
            color: white;
            box-shadow: 0 8px 20px rgba(52, 152, 219, 0.3);
        }
        
        .toggle-section {
            background: rgba(255, 255, 255, 0.05);
            color: rgba(255, 255, 255, 0.9);
            padding: clamp(15px, 4vw, 18px);
            border-radius: clamp(10px, 3vw, 14px);
            text-align: center;
            cursor: pointer;
            margin: clamp(10px, 3vw, 15px) 0;
            font-size: clamp(14px, 4vw, 15px);
            font-weight: 600;
            border: 1px solid rgba(255, 255, 255, 0.1);
            transition: all 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            min-height: 45px;
        }
        
        .toggle-section:active {
            background: rgba(255, 255, 255, 0.1);
            transform: scale(0.98);
        }
        
        .section {
            background: rgba(0, 0, 0, 0.3);
            padding: clamp(15px, 4vw, 18px);
            border-radius: clamp(10px, 3vw, 14px);
            margin: clamp(10px, 3vw, 15px) 0;
            border: 1px solid rgba(255, 255, 255, 0.1);
            display: none;
        }
        
        .section h2 {
            margin: 0 0 clamp(12px, 4vw, 15px) 0;
            font-size: clamp(16px, 4vw, 18px);
            color: #ffffff;
            text-align: center;
            font-weight: 700;
        }
        
        .section h3 {
            margin: 20px 0 12px 0;
            font-size: clamp(15px, 4vw, 16px);
            color: #3498db;
            font-weight: 600;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
            padding-bottom: 8px;
        }
        
        .form-group {
            margin-bottom: clamp(12px, 4vw, 15px);
        }
        
        .form-label {
            display: block;
            margin-bottom: 6px;
            color: rgba(255, 255, 255, 0.8);
            font-size: clamp(13px, 4vw, 14px);
            font-weight: 600;
        }
        
        .form-input {
            width: 100%;
            padding: clamp(12px, 4vw, 15px);
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: clamp(8px, 3vw, 12px);
            background: rgba(0, 0, 0, 0.3);
            color: #ffffff;
            font-size: clamp(14px, 4vw, 15px);
            transition: all 0.3s ease;
        }
        
        .form-input:focus {
            outline: none;
            border-color: #3498db;
            box-shadow: 0 0 0 3px rgba(52, 152, 219, 0.3);
        }
        
        .help-text {
            font-size: clamp(12px, 3vw, 13px);
            color: rgba(255, 255, 255, 0.5);
            margin-top: 4px;
            display: block;
            font-style: italic;
        }
        
        /* Стили для чекбокса */
        .checkbox-group {
            display: flex;
            align-items: center;
            margin-bottom: 15px;
            padding: 12px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 8px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .checkbox-label {
            margin-left: 10px;
            color: rgba(255, 255, 255, 0.9);
            font-size: 14px;
            flex-grow: 1;
        }
        
        .checkbox-custom {
            width: 20px;
            height: 20px;
            border-radius: 4px;
            background: rgba(0, 0, 0, 0.3);
            border: 2px solid rgba(255, 255, 255, 0.3);
            position: relative;
            cursor: pointer;
            transition: all 0.2s ease;
        }
        
        .checkbox-custom.checked {
            background: #3498db;
            border-color: #3498db;
        }
        
        .checkbox-custom.checked::after {
            content: '✓';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            color: white;
            font-weight: bold;
            font-size: 14px;
        }
        
        .checkbox-input {
            display: none;
        }
        
        .select-network {
            margin-bottom: 10px;
            padding: 10px;
            border-radius: 8px;
            background: rgba(255, 255, 255, 0.1);
            border: 1px solid rgba(255, 255, 255, 0.2);
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .select-network:hover {
            background: rgba(52, 152, 219, 0.2);
            border-color: #3498db;
        }
        
        .network-name {
            font-weight: 600;
            color: #ffffff;
            margin-bottom: 4px;
        }
        
        .network-info {
            display: flex;
            justify-content: space-between;
            font-size: 12px;
            color: rgba(255, 255, 255, 0.7);
        }
        
        .scan-btn {
            width: 100%;
            padding: 12px;
            background: linear-gradient(135deg, #9b59b6 0%, #8e44ad 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: 600;
            cursor: pointer;
            margin-bottom: 15px;
            transition: all 0.3s ease;
        }
        
        .scan-btn:active {
            transform: scale(0.98);
        }
        
        .scan-btn.scanning {
            background: linear-gradient(135deg, #3498db 0%, #2980b9 100%);
        }
        
        .scanning-text {
            text-align: center;
            color: #3498db;
            margin: 10px 0;
            font-size: 14px;
        }
        
        .btn-group {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: clamp(8px, 3vw, 12px);
            margin-top: clamp(15px, 5vw, 20px);
        }
        
        .btn {
            padding: clamp(12px, 4vw, 15px);
            border: none;
            border-radius: clamp(8px, 3vw, 12px);
            font-size: clamp(14px, 4vw, 15px);
            font-weight: 700;
            cursor: pointer;
            transition: all 0.2s ease;
            min-height: 45px;
        }
        
        .btn:active {
            transform: scale(0.95);
        }
        
        .btn-save {
            background: linear-gradient(135deg, #2ecc71 0%, #27ae60 100%);
            color: white;
        }
        
        .btn-reset {
            background: linear-gradient(135deg, #e74c3c 0%, #c0392b 100%);
            color: white;
        }
        
        .btn-wifi {
            background: linear-gradient(135deg, #3498db 0%, #2980b9 100%);
            color: white;
        }
        
        .connection-status {
            position: fixed;
            top: 10px;
            right: 10px;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background: #2ecc71;
            box-shadow: 0 0 10px #2ecc71;
            z-index: 1000;
            transition: all 0.3s ease;
        }
        
        .connection-status.offline {
            background: #e74c3c;
            box-shadow: 0 0 10px #e74c3c;
            animation: blink 1.5s infinite;
        }
        
        .connection-status.wifi-sta {
            background: #9b59b6;
            box-shadow: 0 0 10px #9b59b6;
        }
        
        .connection-info {
            background: rgba(52, 152, 219, 0.1);
            border: 1px solid rgba(52, 152, 219, 0.3);
            padding: clamp(10px, 3vw, 12px);
            border-radius: clamp(8px, 3vw, 10px);
            margin: clamp(12px, 4vw, 15px) 0;
            text-align: center;
            transition: all 0.3s ease;
        }
        
        .connection-info.hidden {
            display: none;
        }
        
        .connection-info h3 {
            margin: 0 0 6px 0;
            font-size: clamp(14px, 4vw, 15px);
            color: #3498db;
        }
        
        .connection-info p {
            margin: 4px 0;
            font-size: clamp(12px, 3.5vw, 13px);
            color: rgba(255, 255, 255, 0.7);
            word-break: break-all;
        }
        
        /* Новые стили для отображения пароля */
        .password-container {
            position: relative;
            display: flex;
            align-items: center;
        }
        
        .password-toggle {
            position: absolute;
            right: 10px;
            background: none;
            border: none;
            color: rgba(255, 255, 255, 0.8);
            cursor: pointer;
            font-size: 18px;
            padding: 5px;
            z-index: 10;
            width: 30px;
            height: 30px;
            display: flex;
            align-items: center;
            justify-content: center;
            border-radius: 4px;
            transition: all 0.2s ease;
        }
        
        .password-toggle:hover {
            background: rgba(255, 255, 255, 0.1);
        }
        
        /* Стили для уведомлений */
        .notification {
            position: fixed;
            top: 20px;
            left: 50%;
            transform: translateX(-50%);
            background: linear-gradient(135deg, #e74c3c 0%, #c0392b 100%);
            color: white;
            padding: 15px 20px;
            border-radius: 10px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.3);
            z-index: 1001;
            display: flex;
            align-items: center;
            gap: 10px;
            max-width: 90%;
        }
        
        .notification.success {
            background: linear-gradient(135deg, #2ecc71 0%, #27ae60 100%);
        }
        
        .notification.warning {
            background: linear-gradient(135deg, #f39c12 0%, #d35400 100%);
        }
        
        .notification.info {
            background: linear-gradient(135deg, #3498db 0%, #2980b9 100%);
        }
        
        .notification-icon {
            font-size: 20px;
        }
        
        .notification-close {
            background: none;
            border: none;
            color: white;
            font-size: 20px;
            cursor: pointer;
            margin-left: 10px;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.7; }
            100% { opacity: 1; }
        }
        
        @keyframes blink {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.3; }
        }
        
        @keyframes valueChange {
            0% { transform: scale(1); color: #ffffff; }
            50% { transform: scale(1.05); color: #3498db; }
            100% { transform: scale(1); color: #ffffff; }
        }
        
        .value-changing {
            animation: valueChange 0.5s ease;
        }
        
        .hidden {
            display: none !important;
        }
        
        @media (min-width: 768px) {
            .compact-status-bar {
                font-size: 13px;
                padding: 10px 15px;
            }
            
            .status-icon {
                min-width: 80px;
            }
            
            .data-grid {
                grid-template-columns: repeat(2, 1fr);
            }
            
            .control-buttons {
                grid-template-columns: repeat(2, 1fr);
            }
        }
    </style>
</head>
<body>
    <div class="connection-status" id="connectionStatus"></div>
    
    <div class="container">
        <div class="compact-status-bar">
            <div class="status-icon">
                <div class="status-label">Обогрев</div>
                <div class="status-value" id="heaterStatusText">Выкл</div>
            </div>
            <div class="status-icon">
                <div class="status-label">Режим</div>
                <div class="status-value" id="modeStatusText">Авто</div>
            </div>
            <div class="status-icon">
                <div class="status-label">WiFi</div>
                <div class="status-value" id="wifiModeText">AP</div>
            </div>
        </div>
        
        <div class="header">
            <h1>Обогрев камеры ассистентов</h1>
            <div class="header-subtitle" id="wifiStatus" style="display: none;"></div>
        </div>
        
        <div class="sensor-error" id="sensorError">
            ❌ Ошибка датчика температуры/влажности
        </div>
        
        <div id="connectionInfo" class="connection-info">
            <h3>📶 Подключено к сети WiFi</h3>
            <p id="staNetwork"></p>
            <p id="staIP"></p>
        </div>
        
        <div class="data-display">
            <div class="data-grid">
                <div class="data-item">
                    <div class="data-value" id="temp">0.0</div>
                    <span class="data-label">Температура °C</span>
                </div>
                
                <div class="data-item">
                    <div class="data-value" id="hum">0.0</div>
                    <span class="data-label">Влажность %</span>
                </div>
            </div>
        </div>
        
        <div class="control-buttons">
            <button id="heaterButton" class="control-btn heater-btn">
                <span id="heaterButtonText">Включить обогрев</span>
            </button>
            
            <button id="autoButton" class="control-btn auto-btn">
                <span id="autoButtonText">Автоматический режим</span>
            </button>
            
            <button id="offButton" class="control-btn off-btn">
                <span id="offButtonText">Выключить</span>
            </button>
        </div>
        
        <div class="toggle-section" id="toggleCalibration">
            <span>⚙️ Калибровка</span>
        </div>
        
        <div id="calibrationSection" class="section">
            <h2>Калибровка системы</h2>
            
            <!-- Калибровка датчика температуры/влажности -->
            <h3>📊 Калибровка датчика</h3>
            <div class="form-group">
                <label class="form-label">Коррекция температуры (°C)</label>
                <input type="number" step="0.1" min="-10" max="10" 
                       id="tempOffset" class="form-input" placeholder="0.0">
                <span class="help-text">Добавить или вычесть значение для коррекции температуры</span>
            </div>
            
            <div class="form-group">
                <label class="form-label">Коррекция влажности (%)</label>
                <input type="number" step="0.1" min="-20" max="20" 
                       id="humOffset" class="form-input" placeholder="0.0">
                <span class="help-text">Добавить или вычесть значение для коррекции влажности</span>
            </div>
            
            <div class="btn-group">
                <button type="button" class="btn btn-save" id="applySensorCalibrationBtn">Применить коррекцию</button>
            </div>
            
            <div style="height: 1px; background: rgba(255,255,255,0.1); margin: 20px 0;"></div>
            
            <!-- Калибровка порогов срабатывания -->
            <h3>⚡ Пороги срабатывания обогрева</h3>
            <form id="calibrationForm">
                <div class="form-group">
                    <label class="form-label">Температура включения (≤ °C)</label>
                    <input type="number" step="0.1" min="-30" max="30" 
                           id="tempThreshold" class="form-input" required>
                </div>
                
                <div class="form-group">
                    <label class="form-label">Влажность включения (≥ %)</label>
                    <input type="number" step="0.1" min="0" max="100" 
                           id="humThreshold" class="form-input" required>
                </div>
                
                <div class="btn-group">
                    <button type="submit" class="btn btn-save">Сохрать пороги</button>
                    <button type="button" class="btn btn-reset" id="resetCalibrationBtn">Сброс</button>
                </div>
            </form>
        </div>
        
        <div class="toggle-section" id="toggleWiFi">
            <span>📶 Настройки WiFi</span>
        </div>
        
        <div id="wifiSection" class="section">
            <h2>Настройки WiFi сети</h2>
            
            <div class="form-group">
                <label class="form-label">Режим работы</label>
                <select id="wifiMode" class="form-input">
                    <option value="ap">Точка доступа</option>
                    <option value="sta">Подключение к сети</option>
                </select>
            </div>
            
            <div id="staConfig" class="hidden">
                <div class="form-group">
                    <label class="form-label">Название сети (SSID)</label>
                    <button class="scan-btn" id="scanNetworksBtn">🔍 Сканировать сети</button>
                    <div class="scanning-text hidden" id="scanningText">Сканирование сетей... (15-20 сек)</div>
                    <div id="networksList"></div>
                    <input type="text" id="staSSID" class="form-input" placeholder="Или введите название сети вручную">
                </div>
                
                <div class="form-group">
                    <label class="form-label">Пароль WiFi</label>
                    <div class="password-container">
                        <input type="password" id="staPassword" class="form-input" placeholder="Пароль от WiFi">
                        <button type="button" class="password-toggle" id="togglePasswordBtn">🔵</button>
                    </div>
                </div>
            </div>
            
            <div id="apConfig">
                <div class="form-group">
                    <label class="form-label">Название точки доступа</label>
                    <input type="text" id="apSSID" class="form-input">
                </div>
                
                <div class="form-group">
                    <label class="form-label">Пароль точки доступа</label>
                    <div class="password-container">
                        <input type="password" id="apPassword" class="form-input" value="12345678">
                        <button type="button" class="password-toggle" id="toggleApPasswordBtn">🔵</button>
                    </div>
                </div>
            </div>
            
            <!-- ЧЕКБОКС ДЛЯ СКРЫТИЯ ИНФОРМАЦИИ -->
            <div class="checkbox-group">
                <input type="checkbox" id="hideConnectionInfo" class="checkbox-input">
                <div class="checkbox-custom" id="hideConnectionInfoCheckbox"></div>
                <label for="hideConnectionInfo" class="checkbox-label">
                    Скрыть информацию
                </label>
            </div>
            
            <!-- ОБНОВЛЕННАЯ ГРУППА КНОПОК -->
            <div class="btn-group">
                <button type="button" class="btn btn-save" id="saveWiFiBtn">Подключить</button>
                <button type="button" class="btn btn-wifi" id="restartWiFiBtn">Перезапустить</button>
                <button type="button" class="btn btn-reset" id="resetWiFiBtn">Сбросить</button>
            </div>
        </div>
    </div>

    <script>
        let heaterEnabled = false;
        let isAutoMode = true;
        let heaterOffMode = false; // Режим полного выключения обогрева
        let lastUpdate = Date.now();
        let connectionValid = true;
        let wifiMode = 'ap';
        let staConnected = false;
        let staIP = '';
        let staNetwork = '';
        let currentSSID = '';
        let scanCheckInterval = null;
        let hideConnectionInfo = false;
        let showPassword = false;
        let wifiModeManuallyChanged = false; // Флаг ручного изменения режима WiFi
        let wifiModeChangeTime = 0; // Время последнего ручного изменения
        const WIFI_MODE_LOCK_TIME = 30000; // Блокировка автоматического обновления на 30 секунд
        let staSSIDManuallySet = false; // Флаг ручного ввода/выбора SSID
        let selectedSSID = ''; // Сохраненный выбранный SSID
        
        document.addEventListener('DOMContentLoaded', function() {
            console.log("Документ загружен, инициализация...");
            loadInitialData();
            setupEventListeners();
            startDataUpdates();
            startConnectionCheck();
            autoOpenPortal();
        });
        
        function loadInitialData() {
            console.log("Загрузка начальных данных...");
            fetch('/data')
                .then(response => {
                    if (!response.ok) throw new Error(`HTTP ${response.status}`);
                    return response.json();
                })
                .then(data => {
                    console.log("Данные получены:", data);
                    updateAllData(data);
                    loadCalibrationParams(data.calibrationParams);
                    updateWifiInfo(data.wifiMode, data.staIP, data.staNetwork, data.currentSSID);
                    // Загружаем настройку скрытия информации
                    if (data.hideConnectionInfo !== undefined) {
                        hideConnectionInfo = data.hideConnectionInfo;
                        updateHideConnectionInfoUI();
                    }
                })
                .catch(error => {
                    console.error('Ошибка загрузки:', error);
                    updateConnectionStatus(false);
                });
        }
        
        function setupEventListeners() {
            console.log("Настройка обработчиков событий...");
            
            const heaterBtn = document.getElementById('heaterButton');
            const autoBtn = document.getElementById('autoButton');
            const offBtn = document.getElementById('offButton');
            
            if (heaterBtn) {
                // Добавляем обработчики для лучшего отклика на мобильных и десктопах
                heaterBtn.addEventListener('click', toggleHeater, { passive: true });
                heaterBtn.addEventListener('touchstart', function(e) {
                    e.preventDefault();
                    this.style.transform = 'scale(0.96)';
                }, { passive: false });
                heaterBtn.addEventListener('touchend', function(e) {
                    e.preventDefault();
                    this.style.transform = '';
                    toggleHeater();
                }, { passive: false });
                console.log("Кнопка обогрева добавлена");
            } else {
                console.error("Кнопка обогрева не найдена!");
            }
            
            if (autoBtn) {
                // Добавляем обработчики для лучшего отклика на мобильных и десктопах
                autoBtn.addEventListener('click', setAutoMode, { passive: true });
                autoBtn.addEventListener('touchstart', function(e) {
                    e.preventDefault();
                    this.style.transform = 'scale(0.96)';
                }, { passive: false });
                autoBtn.addEventListener('touchend', function(e) {
                    e.preventDefault();
                    this.style.transform = '';
                    setAutoMode();
                }, { passive: false });
                console.log("Кнопка авторежима добавлена");
            } else {
                console.error("Кнопка авторежима не найдена!");
            }
            
            if (offBtn) {
                // Добавляем обработчики для кнопки выключения
                offBtn.addEventListener('click', setOffMode, { passive: true });
                offBtn.addEventListener('touchstart', function(e) {
                    e.preventDefault();
                    this.style.transform = 'scale(0.96)';
                }, { passive: false });
                offBtn.addEventListener('touchend', function(e) {
                    e.preventDefault();
                    this.style.transform = '';
                    setOffMode();
                }, { passive: false });
                console.log("Кнопка выключения режимов добавлена");
            } else {
                console.error("Кнопка выключения режимов не найдена!");
            }
            
            document.getElementById('toggleCalibration').addEventListener('click', () => 
                toggleSection('calibrationSection'));
            document.getElementById('toggleWiFi').addEventListener('click', () => 
                toggleSection('wifiSection'));
            
            document.getElementById('calibrationForm').addEventListener('submit', saveCalibration);
            document.getElementById('applySensorCalibrationBtn').addEventListener('click', applySensorCalibration);
            document.getElementById('resetCalibrationBtn').addEventListener('click', resetCalibration);
            document.getElementById('wifiMode').addEventListener('change', handleWifiModeChange);
            document.getElementById('saveWiFiBtn').addEventListener('click', saveWiFiSettings);
            document.getElementById('restartWiFiBtn').addEventListener('click', restartWiFi);
            document.getElementById('resetWiFiBtn').addEventListener('click', resetWiFiConnection);
            document.getElementById('scanNetworksBtn').addEventListener('click', startScan);
            
            // Отслеживаем ручной ввод SSID
            const staSSIDInput = document.getElementById('staSSID');
            if (staSSIDInput) {
                staSSIDInput.addEventListener('input', function() {
                    if (this.value.length > 0) {
                        selectedSSID = this.value;
                        staSSIDManuallySet = true;
                        console.log('SSID введен вручную: ' + this.value);
                    }
                });
            }
            
            // Обработчик для чекбокса скрытия информации
            const hideCheckbox = document.getElementById('hideConnectionInfo');
            const hideCheckboxCustom = document.getElementById('hideConnectionInfoCheckbox');
            
            hideCheckbox.addEventListener('change', function() {
                hideConnectionInfo = this.checked;
                hideCheckboxCustom.classList.toggle('checked', this.checked);
                // Сохраняем настройку на сервере
                saveHideConnectionInfoSetting();
            });
            
            hideCheckboxCustom.addEventListener('click', function() {
                hideCheckbox.checked = !hideCheckbox.checked;
                hideCheckbox.dispatchEvent(new Event('change'));
            });
            
            // Обработчик для кнопки показа пароля WiFi сети
            const togglePasswordBtn = document.getElementById('togglePasswordBtn');
            const staPasswordInput = document.getElementById('staPassword');
            
            togglePasswordBtn.addEventListener('click', function() {
                showPassword = !showPassword;
                staPasswordInput.type = showPassword ? 'text' : 'password';
                this.textContent = showPassword ? '⚪' : '🔵';
            });
            
            // Обработчик для кнопки показа пароля точки доступа
            const toggleApPasswordBtn = document.getElementById('toggleApPasswordBtn');
            const apPasswordInput = document.getElementById('apPassword');
            
            toggleApPasswordBtn.addEventListener('click', function() {
                const type = apPasswordInput.type === 'password' ? 'text' : 'password';
                apPasswordInput.type = type;
                this.textContent = type === 'text' ? '⚪' : '🔵';
            });
        }
        
        function startDataUpdates() {
            console.log("Запуск обновления данных...");
            // Оптимизировано: один интервал вместо двух, обновление каждые 1.5 секунды для лучшей отзывчивости
            setInterval(updateData, 1500);
        }
        
        function startConnectionCheck() {
            setInterval(() => {
                if (Date.now() - lastUpdate > 10000) {
                    updateConnectionStatus(false);
                }
            }, 5000);
        }
        
        function updateData() {
            // Оптимизировано: используем кэш браузера и быстрый запрос
            fetch('/data', {
                cache: 'no-cache',
                headers: {
                    'Cache-Control': 'no-cache'
                }
            })
                .then(response => {
                    if (!response.ok) throw new Error(`HTTP ${response.status}`);
                    return response.json();
                })
                .then(data => {
                    updateAllData(data);
                    updateWifiInfo(data.wifiMode, data.staIP, data.staNetwork, data.currentSSID);
                    updateConnectionStatus(true);
                    lastUpdate = Date.now();
                })
                .catch(error => {
                    console.log('Ошибка подключения:', error.message);
                    updateConnectionStatus(false);
                });
        }
        
        function updateAllData(data) {
            updateValueWithAnimation('temp', data.temperature.toFixed(1));
            updateValueWithAnimation('hum', data.humidity.toFixed(1));
            
            document.getElementById('sensorError').style.display = 
                data.sensorConnected ? 'none' : 'block';
            
            heaterEnabled = data.heaterState;
            isAutoMode = data.autoMode;
            heaterOffMode = data.heaterOffMode || false;
            
            updateStatusDisplay();
        }
        
        function updateValueWithAnimation(elementId, newValue) {
            const element = document.getElementById(elementId);
            if (!element) return;
            
            const currentValue = element.textContent;
            
            if (currentValue !== newValue) {
                element.classList.add('value-changing');
                element.textContent = newValue;
                
                setTimeout(() => {
                    element.classList.remove('value-changing');
                }, 500);
            }
        }
        
        function updateConnectionStatus(isConnected) {
            const statusElement = document.getElementById('connectionStatus');
            if (!statusElement) return;
            
            if (isConnected) {
                statusElement.classList.remove('offline');
                if (wifiMode === 'sta' && staConnected) {
                    statusElement.classList.add('wifi-sta');
                } else {
                    statusElement.classList.remove('wifi-sta');
                }
                connectionValid = true;
            } else {
                statusElement.classList.add('offline');
                connectionValid = false;
            }
        }
        
        function updateWifiInfo(mode, ip, network, currentSSIDValue) {
            wifiMode = mode || 'ap';
            staConnected = !!ip;
            staIP = ip || '';
            staNetwork = network || '';
            currentSSID = currentSSIDValue || '';
            
            const wifiStatus = document.getElementById('wifiStatus');
            const connectionInfo = document.getElementById('connectionInfo');
            const wifiModeText = document.getElementById('wifiModeText');
            const staNetworkEl = document.getElementById('staNetwork');
            const staIPEl = document.getElementById('staIP');
            
            // ОБНОВЛЯЕМ ПОЛЯ НАСТРОЕК ТЕКУЩИМИ ДАННЫМИ
            const wifiModeSelect = document.getElementById('wifiMode');
            const staSSIDInput = document.getElementById('staSSID');
            const apSSIDInput = document.getElementById('apSSID');
            
            // НЕ обновляем выпадающий список, если он был изменен вручную недавно
            // Это предотвращает самопроизвольное переключение режима
            if (wifiModeSelect) {
                const timeSinceManualChange = Date.now() - wifiModeChangeTime;
                if (!wifiModeManuallyChanged || timeSinceManualChange > WIFI_MODE_LOCK_TIME) {
                    // Обновляем только если не было ручного изменения или прошло достаточно времени
                    wifiModeSelect.value = wifiMode;
                    // Сбрасываем флаг после обновления
                    if (wifiModeManuallyChanged && timeSinceManualChange > WIFI_MODE_LOCK_TIME) {
                        wifiModeManuallyChanged = false;
                    }
                } else {
                    // Если режим был изменен вручную недавно, не обновляем выпадающий список
                    console.log('Режим WiFi заблокирован от автоматического обновления (изменен вручную ' + 
                                Math.round(timeSinceManualChange / 1000) + ' сек назад)');
                }
            }
            if (apSSIDInput) apSSIDInput.value = currentSSID;
            
            if (wifiMode === 'sta' && staConnected) {
                if (wifiStatus) {
                    wifiStatus.textContent = '';
                    wifiStatus.style.display = 'none';
                }
                if (wifiModeText) {
                    wifiModeText.textContent = 'STA';
                    wifiModeText.className = 'status-value wifi-sta';
                }
                // После успешного подключения обновляем поле SSID и сбрасываем флаг
                if (staSSIDInput) {
                    staSSIDInput.value = network;
                    staSSIDManuallySet = false; // Сбрасываем флаг после успешного подключения
                    selectedSSID = '';
                }
                
                if (connectionInfo) {
                    if (hideConnectionInfo) {
                        connectionInfo.classList.add('hidden');
                    } else {
                        connectionInfo.classList.remove('hidden');
                    }
                }
                if (staNetworkEl) staNetworkEl.textContent = `Сеть: ${network}`;
                if (staIPEl) staIPEl.textContent = `IP: ${ip}`;
            } else {
                if (wifiStatus) {
                    wifiStatus.textContent = '';
                    wifiStatus.style.display = 'none';
                }
                if (wifiModeText) {
                    wifiModeText.textContent = 'AP';
                    wifiModeText.className = 'status-value wifi-ap';
                }
                // НЕ очищаем поле SSID, если оно было установлено вручную или выбрано из списка
                // Очищаем только если не подключены и SSID не был установлен вручную
                if (staSSIDInput) {
                    if (!staSSIDManuallySet) {
                        // Если SSID не был установлен вручную, очищаем поле
                        staSSIDInput.value = '';
                    } else if (selectedSSID) {
                        // Восстанавливаем выбранный SSID, если он был установлен
                        staSSIDInput.value = selectedSSID;
                    }
                }
                if (connectionInfo) connectionInfo.classList.add('hidden');
            }
            
            updateConnectionStatus(true);
        }
        
        function updateHideConnectionInfoUI() {
            const hideCheckbox = document.getElementById('hideConnectionInfo');
            const hideCheckboxCustom = document.getElementById('hideConnectionInfoCheckbox');
            
            if (hideCheckbox && hideCheckboxCustom) {
                hideCheckbox.checked = hideConnectionInfo;
                hideCheckboxCustom.classList.toggle('checked', hideConnectionInfo);
            }
            
            // Обновляем отображение блока с информацией
            const connectionInfo = document.getElementById('connectionInfo');
            if (connectionInfo && wifiMode === 'sta' && staConnected) {
                if (hideConnectionInfo) {
                    connectionInfo.classList.add('hidden');
                } else {
                    connectionInfo.classList.remove('hidden');
                }
            }
        }
        
        function updateStatusDisplay() {
            const heaterStatus = document.getElementById('heaterStatusText');
            const heaterButton = document.getElementById('heaterButton');
            const heaterButtonText = document.getElementById('heaterButtonText');
            const modeStatus = document.getElementById('modeStatusText');
            const autoButton = document.getElementById('autoButton');
            const autoButtonText = document.getElementById('autoButtonText');
            const offButton = document.getElementById('offButton');
            const offButtonText = document.getElementById('offButtonText');
            
            if (heaterStatus) {
                if (heaterEnabled) {
                    heaterStatus.textContent = 'Вкл';
                    heaterStatus.className = 'status-value heater-on';
                } else {
                    heaterStatus.textContent = 'Выкл';
                    heaterStatus.className = 'status-value heater-off';
                }
            }
            
            if (heaterButton && heaterButtonText) {
                if (heaterOffMode) {
                    heaterButton.className = 'control-btn heater-btn';
                    heaterButtonText.textContent = 'Включить обогрев';
                    heaterButton.disabled = false;
                } else if (heaterEnabled) {
                    heaterButton.className = 'control-btn heater-btn on';
                    heaterButtonText.textContent = 'Выключить обогрев';
                    heaterButton.disabled = false;
                } else {
                    heaterButton.className = 'control-btn heater-btn';
                    heaterButtonText.textContent = 'Включить обогрев';
                    heaterButton.disabled = false;
                }
            }
            
            if (modeStatus) {
                if (heaterOffMode) {
                    modeStatus.textContent = 'Выкл';
                    modeStatus.className = 'status-value mode-off';
                } else if (isAutoMode) {
                    modeStatus.textContent = 'Авто';
                    modeStatus.className = 'status-value mode-auto';
                } else {
                    modeStatus.textContent = 'Ручной';
                    modeStatus.className = 'status-value mode-manual';
                }
            }
            
            // Обновляем кнопку режима - показывает режим, на который можно переключиться
            if (autoButton && autoButtonText) {
                if (heaterOffMode) {
                    // Режим выключения - кнопка неактивна, не позволяет переключиться
                    autoButton.className = 'control-btn auto-btn mode-auto disabled';
                    autoButtonText.textContent = 'Автоматический режим';
                    autoButton.style.cssText = 'background: linear-gradient(135deg, #7f8c8d 0%, #95a5a6 100%); color: rgba(255, 255, 255, 0.5); box-shadow: none; cursor: not-allowed; opacity: 0.6;';
                    autoButton.disabled = true;
                } else if (isAutoMode) {
                    // Автоматический режим включен - кнопка оранжевая, предлагает переключиться на ручной
                    autoButton.className = 'control-btn auto-btn mode-manual';
                    autoButtonText.textContent = 'Ручной режим';
                    autoButton.style.cssText = 'background: linear-gradient(135deg, #f39c12 0%, #e67e22 100%); color: white; box-shadow: 0 8px 20px rgba(243, 156, 18, 0.3);';
                    autoButton.disabled = false;
                } else {
                    // Ручной режим включен - кнопка синяя, предлагает переключиться на автоматический
                    autoButton.className = 'control-btn auto-btn mode-auto';
                    autoButtonText.textContent = 'Автоматический режим';
                    autoButton.style.cssText = 'background: linear-gradient(135deg, #3498db 0%, #2980b9 100%); color: white; box-shadow: 0 8px 20px rgba(52, 152, 219, 0.3);';
                    autoButton.disabled = false;
                }
            }
            
            // Обновляем кнопку выключения
            if (offButton && offButtonText) {
                if (heaterOffMode) {
                    offButton.className = 'control-btn off-btn active';
                    offButtonText.textContent = 'Включить';
                    offButton.style.cssText = 'background: linear-gradient(135deg, #95a5a6 0%, #7f8c8d 100%); color: white; box-shadow: 0 8px 20px rgba(149, 165, 166, 0.3);';
                } else {
                    offButton.className = 'control-btn off-btn';
                    offButtonText.textContent = 'Выключить';
                    offButton.style.cssText = 'background: linear-gradient(135deg, #95a5a6 0%, #7f8c8d 100%); color: white; box-shadow: 0 8px 20px rgba(149, 165, 166, 0.2);';
                }
            }
        }
        
        function toggleHeater() {
            // Если режим выключения активен, не позволяем переключать обогрев
            if (heaterOffMode) {
                console.log("Режим выключения активен, переключение обогрева заблокировано");
                showNotification('Сначала включите режимы, нажав кнопку "Включить"', 'warning');
                return;
            }
            
            console.log("Кнопка обогрева нажата");
            
            fetch('/control?action=toggle')
                .then(response => {
                    if (!response.ok) throw new Error(`HTTP ${response.status}`);
                    return response.json();
                })
                .then(data => {
                    console.log("Ответ на toggle:", data);
                    if (data.success) {
                        heaterEnabled = data.heaterState;
                        isAutoMode = data.autoMode;
                        heaterOffMode = data.heaterOffMode || false;
                        updateStatusDisplay();
                    } else if (data.error) {
                        showNotification(data.error, 'warning');
                        heaterOffMode = data.heaterOffMode || false;
                        updateStatusDisplay();
                    }
                })
                .catch(error => {
                    console.error('Ошибка:', error);
                    showNotification('Ошибка связи с устройством: ' + error.message, 'error');
                });
        }
        
        function setAutoMode() {
            // Если режим выключения активен, не позволяем переключать режимы
            if (heaterOffMode) {
                console.log("Режим выключения активен, переключение режимов заблокировано");
                showNotification('Сначала включите режимы, нажав кнопку "Включить"', 'warning');
                return;
            }
            
            console.log("Кнопка авторежима нажата");
            
            fetch('/control?action=auto')
                .then(response => {
                    if (!response.ok) throw new Error(`HTTP ${response.status}`);
                    return response.json();
                })
                .then(data => {
                    console.log("Ответ на auto:", data);
                    if (data.success) {
                        heaterEnabled = data.heaterState;
                        isAutoMode = data.autoMode;
                        heaterOffMode = data.heaterOffMode || false;
                        updateStatusDisplay();
                    } else if (data.error) {
                        showNotification(data.error, 'warning');
                        heaterOffMode = data.heaterOffMode || false;
                        updateStatusDisplay();
                    }
                })
                .catch(error => {
                    console.error('Ошибка:', error);
                    showNotification('Ошибка связи с устройством: ' + error.message, 'error');
                });
        }
        
        function setOffMode() {
            console.log("Кнопка выключения режимов нажата");
            
            fetch('/control?action=off')
                .then(response => {
                    if (!response.ok) throw new Error(`HTTP ${response.status}`);
                    return response.json();
                })
                .then(data => {
                    console.log("Ответ на off:", data);
                    if (data.success) {
                        heaterEnabled = data.heaterState;
                        isAutoMode = data.autoMode;
                        heaterOffMode = data.heaterOffMode || false;
                        updateStatusDisplay();
                        if (heaterOffMode) {
                            showNotification('Все режимы обогрева выключены. Датчики продолжают работать.', 'info');
                        } else {
                            showNotification('Режимы включены. Автоматический режим активен.', 'success');
                        }
                    }
                })
                .catch(error => {
                    console.error('Ошибка:', error);
                    showNotification('Ошибка связи с устройством: ' + error.message, 'error');
                });
        }
        
        function loadCalibrationParams(params) {
            if (params) {
                const tempInput = document.getElementById('tempThreshold');
                const humInput = document.getElementById('humThreshold');
                const tempOffsetInput = document.getElementById('tempOffset');
                const humOffsetInput = document.getElementById('humOffset');
                
                if (tempInput) tempInput.value = params.tempThreshold.toFixed(1);
                if (humInput) humInput.value = params.humThreshold.toFixed(1);
                if (tempOffsetInput) tempOffsetInput.value = params.tempOffset.toFixed(1);
                if (humOffsetInput) humOffsetInput.value = params.humOffset.toFixed(1);
            }
        }
        
        function applySensorCalibration() {
            const tempOffset = document.getElementById('tempOffset').value;
            const humOffset = document.getElementById('humOffset').value;
            
            if (!tempOffset || !humOffset) {
                showNotification('Заполните все поля коррекции', 'warning');
                return;
            }
            
            if (!confirm('Применить коррекцию датчика? Изменения вступят в силу немедленно.')) {
                return;
            }
            
            const formData = new URLSearchParams();
            formData.append('tempOffset', tempOffset);
            formData.append('humOffset', humOffset);
            
            fetch('/sensorcalibrate', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    showNotification('Коррекция датчика применена!', 'success');
                    // Обновляем данные через 1 секунду
                    setTimeout(() => {
                        loadInitialData();
                    }, 1000);
                } else {
                    showNotification('Ошибка применения коррекции', 'error');
                }
            })
            .catch(error => {
                showNotification('Ошибка связи с устройством', 'error');
            });
        }
        
        function saveCalibration(event) {
            event.preventDefault();
            
            const tempThreshold = document.getElementById('tempThreshold').value;
            const humThreshold = document.getElementById('humThreshold').value;
            
            if (!tempThreshold || !humThreshold) {
                showNotification('Заполните все поля', 'warning');
                return;
            }
            
            if (!confirm('Сохранить пороги срабатывания? Устройство перезагрузится.')) {
                return;
            }
            
            const formData = new URLSearchParams();
            formData.append('tempThreshold', tempThreshold);
            formData.append('humThreshold', humThreshold);
            
            fetch('/calibrate', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    showNotification('Пороги сохранены! Устройство перезагрузится.', 'success');
                    setTimeout(() => {
                        window.location.reload();
                    }, 2000);
                } else {
                    showNotification('Ошибка сохранения порогов', 'error');
                }
            })
            .catch(error => {
                showNotification('Ошибка связи с устройством', 'error');
            });
        }
        
        function resetCalibration() {
            if (!confirm('Сбросить ВСЕ калибровочные параметры к заводским? Устройство перезагрузится.')) {
                return;
            }
            
            fetch('/resetcalibration')
                .then(response => {
                    if (response.ok) {
                        showNotification('Вся калибровка сброшена! Устройство перезагрузится.', 'success');
                        setTimeout(() => {
                            window.location.reload();
                        }, 2000);
                    } else {
                        showNotification('Ошибка сброса калибровки', 'error');
                    }
                })
                .catch(error => {
                    showNotification('Ошибка связи с устройством', 'error');
                });
        }
        
        function handleWifiModeChange() {
            const mode = document.getElementById('wifiMode').value;
            const staConfig = document.getElementById('staConfig');
            const apConfig = document.getElementById('apConfig');
            
            // Устанавливаем флаг ручного изменения режима
            wifiModeManuallyChanged = true;
            wifiModeChangeTime = Date.now();
            console.log('Режим WiFi изменен вручную на: ' + mode);
            
            if (mode === 'sta') {
                staConfig.classList.remove('hidden');
                apConfig.classList.add('hidden');
            } else {
                staConfig.classList.add('hidden');
                apConfig.classList.remove('hidden');
            }
        }
        
        function startScan() {
            const scanBtn = document.getElementById('scanNetworksBtn');
            const scanningText = document.getElementById('scanningText');
            const networksList = document.getElementById('networksList');
            
            // Очищаем предыдущие результаты
            networksList.innerHTML = '';
            
            // Показываем сообщение о сканировании
            scanBtn.classList.add('scanning');
            scanBtn.disabled = true;
            scanBtn.textContent = 'Сканирование...';
            scanningText.classList.remove('hidden');
            
            // Запускаем сканирование на сервере
            fetch('/startscan')
                .then(response => {
                    if (!response.ok) throw new Error('Ошибка запуска сканирования');
                    return response.json();
                })
                .then(data => {
                    if (data.success) {
                        console.log('Сканирование запущено');
                        // Начинаем опрашивать статус сканирования
                        checkScanStatus();
                    }
                })
                .catch(error => {
                    console.error('Ошибка запуска сканирования:', error);
                    resetScanButton();
                    networksList.innerHTML = '<div style="color: #e74c3c; text-align: center; padding: 10px;">Ошибка запуска сканирования</div>';
                });
        }
        
        function checkScanStatus() {
            if (scanCheckInterval) {
                clearInterval(scanCheckInterval);
            }
            
            let attempts = 0;
            const maxAttempts = 40; // 20 секунд (40 * 500ms)
            
            scanCheckInterval = setInterval(() => {
                attempts++;
                
                fetch('/scanstatus')
                    .then(response => response.json())
                    .then(data => {
                        if (data.scanning) {
                            // Сканирование еще продолжается
                            if (attempts >= maxAttempts) {
                                clearInterval(scanCheckInterval);
                                scanTimeout();
                            }
                        } else if (data.success && data.networks) {
                            // Сканирование завершено успешно
                            clearInterval(scanCheckInterval);
                            displayNetworks(data.networks);
                        } else if (data.error) {
                            // Ошибка сканирования
                            clearInterval(scanCheckInterval);
                            displayScanError(data.error);
                        }
                    })
                    .catch(error => {
                        console.error('Ошибка проверки статуса:', error);
                        if (attempts >= maxAttempts) {
                            clearInterval(scanCheckInterval);
                            scanTimeout();
                        }
                    });
            }, 500); // Проверяем каждые 500ms
        }
        
        function displayNetworks(networks) {
            const scanBtn = document.getElementById('scanNetworksBtn');
            const scanningText = document.getElementById('scanningText');
            const networksList = document.getElementById('networksList');
            
            resetScanButton();
            
            if (networks.length > 0) {
                networksList.innerHTML = '<div class="form-label">Доступные сети:</div>';
                networks.forEach(network => {
                    const networkDiv = document.createElement('div');
                    networkDiv.className = 'select-network';
                    
                    // Формируем строку с уровнем сигнала
                    let signalStrength = '📶';
                    if (network.rssi >= -50) signalStrength = '📶 Отличный';
                    else if (network.rssi >= -60) signalStrength = '📶 Хороший';
                    else if (network.rssi >= -70) signalStrength = '📶 Средний';
                    else signalStrength = '📶 Слабый';
                    
                    networkDiv.innerHTML = `
                        <div class="network-name">${network.ssid}</div>
                        <div class="network-info">
                            <span>${signalStrength} (${network.rssi} dBm)</span>
                            <span>${network.encryption}</span>
                        </div>
                    `;
                    networkDiv.addEventListener('click', () => {
                        const staSSIDInput = document.getElementById('staSSID');
                        if (staSSIDInput) {
                            staSSIDInput.value = network.ssid;
                            selectedSSID = network.ssid; // Сохраняем выбранный SSID
                            staSSIDManuallySet = true; // Устанавливаем флаг ручного выбора
                            console.log('Выбрана сеть: ' + network.ssid);
                        }
                        document.getElementById('staPassword').focus();
                    });
                    networksList.appendChild(networkDiv);
                });
            } else {
                networksList.innerHTML = '<div style="color: rgba(255,255,255,0.5); text-align: center; padding: 10px;">Сети не найдены</div>';
            }
        }
        
        function displayScanError(error) {
            const scanBtn = document.getElementById('scanNetworksBtn');
            const scanningText = document.getElementById('scanningText');
            const networksList = document.getElementById('networksList');
            
            resetScanButton();
            networksList.innerHTML = `<div style="color: #e74c3c; text-align: center; padding: 10px;">Ошибка: ${error}</div>`;
        }
        
        function scanTimeout() {
            const scanBtn = document.getElementById('scanNetworksBtn');
            const scanningText = document.getElementById('scanningText');
            const networksList = document.getElementById('networksList');
            
            resetScanButton();
            networksList.innerHTML = '<div style="color: #f39c12; text-align: center; padding: 10px;">Таймаут сканирования</div>';
        }
        
        function resetScanButton() {
            const scanBtn = document.getElementById('scanNetworksBtn');
            const scanningText = document.getElementById('scanningText');
            
            scanBtn.classList.remove('scanning');
            scanBtn.disabled = false;
            scanBtn.textContent = '🔍 Сканировать сети';
            scanningText.classList.add('hidden');
        }
        
        function saveWiFiSettings() {
            const mode = document.getElementById('wifiMode').value;
            const staSSID = document.getElementById('staSSID').value;
            const staPassword = document.getElementById('staPassword').value;
            const apSSID = document.getElementById('apSSID').value;
            const apPassword = document.getElementById('apPassword').value;
            
            if (mode === 'sta' && (!staSSID || !staPassword)) {
                showNotification('Заполните все поля для подключения к WiFi сети', 'warning');
                return;
            }
            
            if (!apSSID || !apPassword) {
                showNotification('Заполните все поля для точки доступа', 'warning');
                return;
            }
            
            if (!confirm('Подключиться к новой сети WiFi? Устройство перезагрузится через 10 секунд.')) {
                return;
            }
            
            // Сбрасываем флаг ручного изменения после сохранения настроек
            wifiModeManuallyChanged = false;
            wifiModeChangeTime = 0;
            
            const formData = new URLSearchParams();
            formData.append('mode', mode);
            formData.append('staSSID', staSSID);
            formData.append('staPassword', staPassword);
            formData.append('apSSID', apSSID);
            formData.append('apPassword', apPassword);
            formData.append('hideConnectionInfo', hideConnectionInfo ? '1' : '0');
            
            fetch('/updatewifi', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    return response.text();
                } else {
                    throw new Error('Ошибка подключения');
                }
            })
            .then(responseText => {
                if (responseText.includes('wrong_password')) {
                    showNotification('Неправильный пароль WiFi сети', 'error');
                } else if (responseText.includes('auth_failed')) {
                    showNotification('Ошибка авторизации в WiFi сети', 'error');
                } else if (responseText.includes('not_found')) {
                    showNotification('WiFi сеть не найдена', 'error');
                } else if (responseText.includes('connected') || responseText.includes('ap_mode')) {
                    showNotification('Настройки WiFi сохранены! Устройство перезагрузится через 10 секунд.', 'success');
                    // Показываем сообщение и даем время на перезагрузку
                    setTimeout(() => {
                        showNotification('Перезагружаемся...', 'info');
                    }, 5000);
                    
                    setTimeout(() => {
                        // Пробуем переподключиться к новому IP
                        reconnectToNewIP();
                    }, 10000);
                } else {
                    showNotification('Настройки WiFi сохранены!', 'success');
                    setTimeout(() => {
                        window.location.reload();
                    }, 2000);
                }
            })
            .catch(error => {
                console.error('Ошибка подключения:', error);
                showNotification('Настройки сохранены. Устройство перезагрузится. Подключитесь к точке доступа для проверки.', 'info');
                setTimeout(() => {
                    window.location.href = 'http://192.168.4.1';
                }, 5000);
            });
        }
        
        function reconnectToNewIP() {
            // Сначала пробуем обновить страницу
            window.location.reload();
            
            // Если не получилось, через 5 секунд пробуем перейти на точку доступа
            setTimeout(() => {
                window.location.href = 'http://192.168.4.1';
            }, 5000);
        }
        
        function resetWiFiConnection() {
            if (!confirm('Сбросить текущее WiFi подключение и переключиться на точку доступа?')) {
                return;
            }
            
            fetch('/resetwifi')
                .then(response => {
                    if (response.ok) {
                        showNotification('WiFi подключение сброшено! Устройство переключится на точку доступа.', 'success');
                        setTimeout(() => {
                            window.location.reload();
                        }, 3000);
                    } else {
                        showNotification('Ошибка сброса WiFi', 'error');
                    }
                })
                .catch(error => {
                    showNotification('Ошибка связи с устройством', 'error');
                });
        }
        
        function saveHideConnectionInfoSetting() {
            const formData = new URLSearchParams();
            formData.append('hideConnectionInfo', hideConnectionInfo ? '1' : '0');
            
            fetch('/savehideinfo', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    console.log('Настройка скрытия информации сохранена');
                    // Обновляем отображение блока с информацией
                    updateWifiInfo(wifiMode, staIP, staNetwork, currentSSID);
                }
            })
            .catch(error => {
                console.error('Ошибка сохранения настройки:', error);
            });
        }
        
        function restartWiFi() {
            if (confirm('Перезапустить WiFi? Устройство будет недоступно 10-20 секунд.')) {
                fetch('/restartwifi')
                    .then(response => {
                        if (response.ok) {
                            showNotification('WiFi перезапускается...', 'info');
                        }
                    })
                    .catch(error => {
                        showNotification('Ошибка перезапуска WiFi', 'error');
                    });
            }
        }
        
        function toggleSection(sectionId) {
            const section = document.getElementById(sectionId);
            if (!section) return;
            
            // Проверяем видимость через computed style (учитывает и CSS, и inline стили)
            const computedStyle = window.getComputedStyle(section);
            const isHidden = computedStyle.display === 'none';
            
            // Скрываем все секции
            document.querySelectorAll('.section').forEach(s => {
                s.style.display = 'none';
            });
            
            // Показываем выбранную секцию, если она была скрыта
            section.style.display = isHidden ? 'block' : 'none';
        }
        
        function autoOpenPortal() {
            ['/generate_204', '/hotspot-detect.html', '/connecttest.txt', '/fwlink'].forEach(url => {
                fetch(url).catch(() => {});
            });
        }
        
        // Функция для показа уведомлений
        function showNotification(message, type = 'info') {
            // Удаляем существующие уведомления
            const existingNotifications = document.querySelectorAll('.notification');
            existingNotifications.forEach(notification => {
                notification.remove();
            });
            
            // Создаем новое уведомление
            const notification = document.createElement('div');
            notification.className = `notification ${type}`;
            
            let icon = 'ℹ️';
            if (type === 'error') icon = '❌';
            if (type === 'success') icon = '✅';
            if (type === 'warning') icon = '⚠️';
            
            notification.innerHTML = `
                <span class="notification-icon">${icon}</span>
                <span>${message}</span>
                <button class="notification-close">×</button>
            `;
            
            document.body.appendChild(notification);
            
            // Обработчик закрытия уведомления
            const closeBtn = notification.querySelector('.notification-close');
            closeBtn.addEventListener('click', () => {
                notification.remove();
            });
            
            // Автоматическое закрытие через 5 секунд
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.remove();
                }
            }, 5000);
        }
    </script>
</body>
</html>
)rawliteral";

// Функция чтения датчика с коррекцией
bool readSensorData() {
  // Проверяем доступность датчика
  Wire.beginTransmission(0x40);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    sensorConnected = false;
    return false;
  }

  // Чтение температуры (команда 0xF3 - Hold Master Mode)
  Wire.beginTransmission(0x40);
  Wire.write(0xF3); // Команда измерения температуры без удержания шины
  error = Wire.endTransmission();
  
  if (error != 0) {
    sensorConnected = false;
    return false;
  }
  
  // Для SHT21 нужно больше времени на измерение (85мс для температуры)
  delay(85);  // Увеличено для SHT21
  
  // Читаем 3 байта: 2 байта данных + 1 байт CRC
  Wire.requestFrom(0x40, 3);
  
  if (Wire.available() < 3) {
    sensorConnected = false;
    return false;
  }
  
  uint16_t tempRaw = (Wire.read() << 8) | Wire.read();
  byte crc = Wire.read(); // CRC (пока не проверяем)
  
  // Чтение влажности (команда 0xF5 - Hold Master Mode)
  Wire.beginTransmission(0x40);
  Wire.write(0xF5); // Команда измерения влажности без удержания шины
  error = Wire.endTransmission();
  
  if (error != 0) {
    sensorConnected = false;
    return false;
  }
  
  // Для SHT21 нужно больше времени на измерение (29мс для влажности)
  delay(29);  // Увеличено для SHT21
  
  // Читаем 3 байта: 2 байта данных + 1 байт CRC
  Wire.requestFrom(0x40, 3);
  
  if (Wire.available() < 3) {
    sensorConnected = false;
    return false;
  }
  
  uint16_t humRaw = (Wire.read() << 8) | Wire.read();
  crc = Wire.read(); // CRC (пока не проверяем)
  
  // Проверка валидности данных (старшие биты должны быть 0 для SHT21)
  tempRaw &= 0xFFFC; // Очищаем младшие 2 бита (статусные биты)
  humRaw &= 0xFFFC;  // Очищаем младшие 2 бита (статусные биты)
  
  // Расчёт базовых значений по формуле SHT21
  temperature = -46.85 + 175.72 * (tempRaw / 65536.0);
  humidity = -6.0 + 125.0 * (humRaw / 65536.0);
  
  // Применение калибровочных коррекций
  temperature += calParams.tempOffset;
  humidity += calParams.humOffset;
  
  // Ограничение значений
  if (humidity > 100) humidity = 100;
  if (humidity < 0) humidity = 0;
  
  // Проверка на разумные значения
  if (temperature < -40 || temperature > 125) {
    sensorConnected = false;
    return false;
  }
  
  sensorConnected = true;
  return true;
}

// Функция определения типа WiFi сети
String getWiFiBand(int channel) {
  // Определяем полосу частот по каналу
  if (channel >= 1 && channel <= 14) {
    return "2.4 ГГц";
  } else if (channel >= 36 && channel <= 165) {
    return "5 ГГц";
  }
  return "Неизвестно";
}

// Функция определения стандарта WiFi
String getWiFiStandard(int channel, wifi_auth_mode_t encryption) {
  // Определяем стандарт WiFi на основе канала и других параметров
  if (channel >= 36 && channel <= 165) {
    // 5 ГГц диапазон
    // WiFi 6 (802.11ax) может работать на 5 ГГц
    // WiFi 5 (802.11ac) только на 5 ГГц
    // Более новые стандарты шифрования (WPA3) часто указывают на WiFi 6
    if (encryption == WIFI_AUTH_WPA3_PSK || encryption == WIFI_AUTH_WPA2_WPA3_PSK) {
      return "WiFi 6 (802.11ax)";
    }
    // Для ESP32-C3 на 5 ГГц обычно WiFi 5 (802.11ac) или WiFi 6
    return "WiFi 5/6 (802.11ac/ax)";
  } else if (channel >= 1 && channel <= 14) {
    // 2.4 ГГц диапазон
    // WiFi 6 (802.11ax) может работать и на 2.4 ГГц
    // WiFi 4 (802.11n) на 2.4 ГГц
    if (encryption == WIFI_AUTH_WPA3_PSK || encryption == WIFI_AUTH_WPA2_WPA3_PSK) {
      return "WiFi 6 (802.11ax)";
    }
    // Для ESP32-C3 на 2.4 ГГц обычно WiFi 4 (802.11n) или WiFi 6
    return "WiFi 4/6 (802.11n/ax)";
  }
  return "Неизвестно";
}

// Функция получения полной информации о типе сети
String getNetworkType(int channel, wifi_auth_mode_t encryption) {
  String band = getWiFiBand(channel);
  String standard = getWiFiStandard(channel, encryption);
  return band + " • " + standard;
}

// Асинхронное сканирование WiFi сетей
void startScanWiFiNetworks() {
  Serial.println("📡 Запуск асинхронного сканирования WiFi сетей...");
  
  // Если уже идет сканирование, отменяем его
  if (isScanning) {
    int currentStatus = WiFi.scanComplete();
    if (currentStatus == WIFI_SCAN_RUNNING) {
      Serial.println("⚠️ Отмена предыдущего сканирования...");
      WiFi.scanDelete(); // Отменяем текущее сканирование
      delay(500); // Даем время на отмену
    }
  }
  
  isScanning = true;
  scanCompleted = false;
  scanStartTime = millis();
  
  // Сбрасываем предыдущие результаты
  scanResult.clear();
  
  // Очищаем предыдущие результаты сканирования
  WiFi.scanDelete();
  delay(200); // Небольшая задержка для стабильности
  
  // Запускаем сканирование в фоне
  int scanResult = WiFi.scanNetworks(true, true); // async, show_hidden
  
  if (scanResult == WIFI_SCAN_FAILED) {
    Serial.println("❌ Ошибка запуска сканирования");
    isScanning = false;
  } else {
  Serial.println("✅ Сканирование запущено в фоновом режиме");
  }
}

// Проверка статуса сканирования
String getScanStatus() {
  if (!isScanning) {
    return "{\"scanning\":false}";
  }
  
  // Проверяем таймаут
  unsigned long elapsed = millis() - scanStartTime;
  if (elapsed > SCAN_TIMEOUT) {
    isScanning = false;
    Serial.print("❌ Таймаут сканирования (прошло ");
    Serial.print(elapsed / 1000);
    Serial.println(" секунд)");
    WiFi.scanDelete(); // Очищаем результаты при таймауте
    return "{\"error\":\"Таймаут сканирования\",\"scanning\":false}";
  }
  
  // Проверяем статус сканирования
  int scanStatus = WiFi.scanComplete();
  
  // Обработка ошибок сканирования
  if (scanStatus == WIFI_SCAN_FAILED) {
    isScanning = false;
    Serial.println("❌ Ошибка сканирования WiFi");
    WiFi.scanDelete();
    return "{\"error\":\"Ошибка сканирования\",\"scanning\":false}";
  }
  
  if (scanStatus == WIFI_SCAN_RUNNING) {
    // Показываем прогресс (сколько секунд прошло)
    unsigned long seconds = elapsed / 1000;
    return "{\"scanning\":true,\"elapsed\":" + String(seconds) + "}";
  }
  
  if (scanStatus >= 0) {
    Serial.print("✅ Сканирование завершено. Найдено сетей: ");
    Serial.println(scanStatus);
    
    // Формируем JSON с результатами
    scanResult.clear();
    JsonArray networks = scanResult.createNestedArray("networks");
    
    // Структура для объединения сетей с одинаковым SSID
    struct NetworkInfo {
      String ssid;
      int bestRssi;
      String encryption;
      String bands; // Объединенные полосы частот (например: "2.4 ГГц, 5 ГГц")
      String standard;
      int channels[2]; // Каналы для каждой полосы
      bool has24GHz;
      bool has5GHz;
    };
    
    // Собираем информацию о всех сетях
    NetworkInfo networkMap[20]; // Максимум 20 уникальных сетей
    int networkCount = 0;
    
    for (int i = 0; i < scanStatus && networkCount < 20; i++) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      int channel = WiFi.channel(i);
      String frequencyBand = getWiFiBand(channel);
      wifi_auth_mode_t encryptionType = WiFi.encryptionType(i);
      
      // Определяем тип шифрования
      String encryption = "Открытая";
      switch (encryptionType) {
        case WIFI_AUTH_OPEN:
          encryption = "Открытая";
          break;
        case WIFI_AUTH_WEP:
          encryption = "WEP";
          break;
        case WIFI_AUTH_WPA_PSK:
          encryption = "WPA";
          break;
        case WIFI_AUTH_WPA2_PSK:
          encryption = "WPA2";
          break;
        case WIFI_AUTH_WPA_WPA2_PSK:
          encryption = "WPA/WPA2";
          break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
          encryption = "WPA2 Enterprise";
          break;
        case WIFI_AUTH_WPA3_PSK:
          encryption = "WPA3";
          break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
          encryption = "WPA2/WPA3";
          break;
        default:
          encryption = "Неизвестно";
      }
      
      // Ищем, есть ли уже такая сеть в списке
      int foundIndex = -1;
      for (int j = 0; j < networkCount; j++) {
        if (networkMap[j].ssid == ssid) {
          foundIndex = j;
          break;
        }
      }
      
      if (foundIndex >= 0) {
        // Объединяем с существующей сетью
        NetworkInfo* net = &networkMap[foundIndex];
        if (rssi > net->bestRssi) {
          net->bestRssi = rssi; // Используем лучший RSSI
        }
        
        // Добавляем полосу частот, если её еще нет
        if (frequencyBand == "2.4 ГГц" && !net->has24GHz) {
          net->has24GHz = true;
          net->channels[0] = channel;
          if (net->bands.length() > 0) {
            net->bands += ", 2.4 ГГц";
          } else {
            net->bands = "2.4 ГГц";
          }
        } else if (frequencyBand == "5 ГГц" && !net->has5GHz) {
          net->has5GHz = true;
          net->channels[1] = channel;
          if (net->bands.length() > 0) {
            net->bands += ", 5 ГГц";
          } else {
            net->bands = "5 ГГц";
          }
        }
      } else {
        // Новая сеть
        NetworkInfo* net = &networkMap[networkCount];
        net->ssid = ssid;
        net->bestRssi = rssi;
        net->encryption = encryption;
        net->standard = getWiFiStandard(channel, encryptionType);
        net->has24GHz = (frequencyBand == "2.4 ГГц");
        net->has5GHz = (frequencyBand == "5 ГГц");
        net->bands = frequencyBand;
        net->channels[0] = net->has24GHz ? channel : 0;
        net->channels[1] = net->has5GHz ? channel : 0;
        networkCount++;
      }
    }
    
    // Формируем JSON из объединенных сетей
    for (int i = 0; i < networkCount; i++) {
      NetworkInfo* net = &networkMap[i];
      JsonObject network = networks.createNestedObject();
      network["ssid"] = net->ssid;
      network["rssi"] = net->bestRssi;
      network["encryption"] = net->encryption;
      network["band"] = net->bands; // Объединенные полосы
      network["standard"] = net->standard;
      
      // Если сеть на обеих частотах - показываем как WiFi 6 (802.11ax)
      if (net->has24GHz && net->has5GHz) {
        network["standard"] = "WiFi 6 (802.11ax)";
        network["type"] = net->bands + " • WiFi 6 (802.11ax)";
      } else {
        network["type"] = net->bands + " • " + net->standard;
      }
      
      // Показываем каналы, если есть
      if (net->has24GHz && net->has5GHz) {
        network["channels"] = String(net->channels[0]) + ", " + String(net->channels[1]);
      } else if (net->has24GHz) {
        network["channels"] = String(net->channels[0]);
      } else if (net->has5GHz) {
        network["channels"] = String(net->channels[1]);
      }
      
      Serial.print("  ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(net->ssid);
      Serial.print(" (");
      Serial.print(net->bestRssi);
      Serial.print(" dBm, ");
      Serial.print(net->bands);
      Serial.print(", ");
      Serial.print(net->standard);
      Serial.print(") ");
      Serial.println(net->encryption);
    }
    
    // Очищаем результаты сканирования
    WiFi.scanDelete();
    
    isScanning = false;
    scanCompleted = true;
    
    // Добавляем статус успеха
    scanResult["success"] = true;
    
    // Оптимизировано: прямое возвращение String без промежуточных копий
    String jsonString;
    if (!jsonString.reserve(2048)) {
      Serial.println("⚠️ Не удалось зарезервировать память для JSON");
      isScanning = false;
      return "{\"error\":\"Недостаточно памяти\"}";
    }
    
    // Проверяем успешность сериализации
    size_t bytesWritten = serializeJson(scanResult, jsonString);
    if (bytesWritten == 0) {
      Serial.println("⚠️ Ошибка сериализации JSON");
      isScanning = false;
      return "{\"error\":\"Ошибка формирования ответа\"}";
    }
    
    return jsonString;  // Прямое возвращение без копирования
  }
  
  // Если scanStatus < 0, но не WIFI_SCAN_FAILED и не WIFI_SCAN_RUNNING - неизвестная ошибка
  if (scanStatus < 0) {
    isScanning = false;
    Serial.print("❌ Неизвестная ошибка сканирования. Статус: ");
    Serial.println(scanStatus);
    WiFi.scanDelete();
    return "{\"error\":\"Неизвестная ошибка сканирования\",\"scanning\":false}";
  }
  
  // Если дошли сюда, значит что-то пошло не так
  isScanning = false;
  Serial.println("⚠️ Неожиданное состояние сканирования");
  return "{\"error\":\"Неожиданное состояние\",\"scanning\":false}";
}

// Проверка критических условий для камеры
bool isCameraAtRisk() {
    return (temperature <= 10.0 && humidity >= 75.0) ||
           (temperature >= 12.0 && humidity >= 80.0) ||
           humidity >= 85.0 ||
           temperature <= 5.0;
}

// Загрузка настроек
void loadSettings() {
  // Используем значения по умолчанию, если Preferences не открывается
  if (!preferences.begin("calibration", true)) {
    Serial.println("⚠️ Ошибка открытия Preferences для калибровки, используем заводские значения");
    calParams = factoryCalibration;
    return;
  }
  
  // Загружаем значения с валидацией
  float tempThreshold = preferences.getFloat("tempThreshold", factoryCalibration.tempThreshold);
  float humThreshold = preferences.getFloat("humThreshold", factoryCalibration.humThreshold);
  float tempOffset = preferences.getFloat("tempOffset", factoryCalibration.tempOffset);
  float humOffset = preferences.getFloat("humOffset", factoryCalibration.humOffset);
  
  // Валидация загруженных данных
  bool dataValid = true;
  
  // Проверка температуры включения: должна быть в разумных пределах (-50°C до 50°C)
  if (tempThreshold < -50.0 || tempThreshold > 50.0) {
    Serial.println("⚠️ Некорректное значение tempThreshold (" + String(tempThreshold) + "°C), используем заводское");
    tempThreshold = factoryCalibration.tempThreshold;
    dataValid = false;
  }
  
  // Проверка влажности включения: должна быть от 0% до 100%
  if (humThreshold < 0.0 || humThreshold > 100.0) {
    Serial.println("⚠️ Некорректное значение humThreshold (" + String(humThreshold) + "%), используем заводское");
    humThreshold = factoryCalibration.humThreshold;
    dataValid = false;
  }
  
  // Проверка коррекции температуры: должна быть в пределах ±20°C
  if (abs(tempOffset) > 20.0) {
    Serial.println("⚠️ Некорректное значение tempOffset (" + String(tempOffset) + "°C), используем заводское");
    tempOffset = factoryCalibration.tempOffset;
    dataValid = false;
  }
  
  // Проверка коррекции влажности: должна быть в пределах ±50%
  if (abs(humOffset) > 50.0) {
    Serial.println("⚠️ Некорректное значение humOffset (" + String(humOffset) + "%), используем заводское");
    humOffset = factoryCalibration.humOffset;
    dataValid = false;
  }
  
  // Применяем валидированные значения
  calParams.tempThreshold = tempThreshold;
  calParams.humThreshold = humThreshold;
  calParams.tempOffset = tempOffset;
  calParams.humOffset = humOffset;
  
  if (dataValid) {
    Serial.println("📊 Загружены калибровочные параметры:");
  } else {
    Serial.println("📊 Загружены калибровочные параметры (некоторые исправлены):");
  }
  Serial.println("   Температура включения: " + String(calParams.tempThreshold) + "°C");
  Serial.println("   Влажность включения: " + String(calParams.humThreshold) + "%");
  Serial.println("   Коррекция температуры: " + String(calParams.tempOffset) + "°C");
  Serial.println("   Коррекция влажности: " + String(calParams.humOffset) + "%");
  
  preferences.end();
  
  // Загружаем настройку скрытия информации о подключении
  if (!preferences.begin("display", true)) {
    Serial.println("⚠️ Ошибка открытия Preferences для отображения, используем значение по умолчанию");
    showConnectionInfo = true; // Значение по умолчанию
    preferences.end(); // Закрываем, если было открыто
    return;
  }
  
  showConnectionInfo = preferences.getBool("showConnectionInfo", true);
  Serial.println("📱 Настройка отображения: " + String(showConnectionInfo ? "Показывать" : "Скрывать") + " информацию о подключении");
  
  preferences.end();
}

// Сохранение настроек
void saveSettings() {
  if (!preferences.begin("calibration", false)) {
    Serial.println("❌ Ошибка открытия Preferences для сохранения");
    return;
  }
  
  // Проверяем успешность записи каждого параметра
  size_t written;
  bool allWritten = true;
  
  written = preferences.putFloat("tempThreshold", calParams.tempThreshold);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи tempThreshold в Preferences");
    allWritten = false;
  }
  
  written = preferences.putFloat("humThreshold", calParams.humThreshold);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи humThreshold в Preferences");
    allWritten = false;
  }
  
  written = preferences.putFloat("tempOffset", calParams.tempOffset);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи tempOffset в Preferences");
    allWritten = false;
  }
  
  written = preferences.putFloat("humOffset", calParams.humOffset);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи humOffset в Preferences");
    allWritten = false;
  }
  
  preferences.end();
  
  if (allWritten) {
    Serial.println("💾 Калибровочные параметры сохранены:");
    Serial.println("   Температура включения: " + String(calParams.tempThreshold) + "°C");
    Serial.println("   Влажность включения: " + String(calParams.humThreshold) + "%");
    Serial.println("   Коррекция температуры: " + String(calParams.tempOffset) + "°C");
    Serial.println("   Коррекция влажности: " + String(calParams.humOffset) + "%");
  } else {
    Serial.println("⚠️ Некоторые параметры не были сохранены из-за ошибок записи");
  }
}

// Сохранение настройки отображения
void saveDisplaySettings() {
  if (!preferences.begin("display", false)) {
    Serial.println("❌ Ошибка открытия Preferences для отображения");
    return;
  }
  
  size_t written = preferences.putBool("showConnectionInfo", showConnectionInfo);
  preferences.end();
  
  if (written > 0) {
    Serial.println("📱 Настройка отображения сохранена: " + String(showConnectionInfo ? "Показывать" : "Скрывать") + " информацию о подключении");
  } else {
    Serial.println("⚠️ Ошибка записи настройки отображения в Preferences");
  }
}

// Сохранение настроек WiFi в Preferences
void saveWiFiSettingsToPreferences() {
  if (!preferences.begin("wifi-config", false)) {
    Serial.println("❌ Ошибка открытия Preferences WiFi");
    return;
  }
  
  // Проверяем и обрезаем строки до допустимой длины перед сохранением
  String safeStaSSID = staSSID;
  String safeStaPassword = staPassword;
  String safeApSSID = apSSID;
  String safeApPassword = apPassword;
  
  if (safeStaSSID.length() > 32) {
    Serial.println("⚠️ SSID слишком длинный, обрезаем до 32 символов");
    safeStaSSID = safeStaSSID.substring(0, 32);
  }
  if (safeStaPassword.length() > 63) {
    Serial.println("⚠️ Пароль WiFi слишком длинный, обрезаем до 63 символов");
    safeStaPassword = safeStaPassword.substring(0, 63);
  }
  if (safeApSSID.length() > 32) {
    Serial.println("⚠️ AP SSID слишком длинный, обрезаем до 32 символов");
    safeApSSID = safeApSSID.substring(0, 32);
  }
  if (safeApPassword.length() > 63) {
    Serial.println("⚠️ AP пароль слишком длинный, обрезаем до 63 символов");
    safeApPassword = safeApPassword.substring(0, 63);
  }
  
  // Проверяем успешность записи каждого параметра
  size_t written;
  bool allWritten = true;
  
  written = preferences.putBool("staEnabled", wifiSTAEnabled);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи staEnabled в Preferences");
    allWritten = false;
  }
  
  written = preferences.putString("staSSID", safeStaSSID);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи staSSID в Preferences");
    allWritten = false;
  }
  
  written = preferences.putString("staPassword", safeStaPassword);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи staPassword в Preferences");
    allWritten = false;
  }
  
  written = preferences.putString("apSSID", safeApSSID);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи apSSID в Preferences");
    allWritten = false;
  }
  
  written = preferences.putString("apPassword", safeApPassword);
  if (written == 0) {
    Serial.println("⚠️ Ошибка записи apPassword в Preferences");
    allWritten = false;
  }
  
  preferences.end();
  
  if (allWritten) {
    Serial.println("💾 Настройки WiFi сохранены");
  } else {
    Serial.println("⚠️ Некоторые настройки WiFi не были сохранены из-за ошибок записи");
  }
}

// Сброс калибровки
void resetCalibrationToFactory() {
  if (!preferences.begin("calibration", false)) {
    Serial.println("❌ Ошибка открытия Preferences для сброса");
    return;
  }
  
  preferences.clear();
  preferences.end();
  
  calParams = factoryCalibration;
  Serial.println("🔄 Вся калибровка сброшена к заводским настройкам");
}

// Сканирование I2C шины для диагностики
void scanI2C() {
  Serial.println("🔍 Сканирование I2C шины...");
  byte devicesFound = 0;
  
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("   Найден I2C устройство по адресу 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.print(" (");
      Serial.print(address);
      Serial.println(")");
      devicesFound++;
    }
  }
  
  if (devicesFound == 0) {
    Serial.println("   ❌ I2C устройства не найдены!");
    Serial.println("   Проверьте подключение SDA (GPIO" + String(I2C_SDA) + ") и SCL (GPIO" + String(I2C_SCL) + ")");
  } else {
    Serial.print("   ✅ Найдено устройств: ");
    Serial.println(devicesFound);
  }
}

// Инициализация датчика
bool initSensor() {
  Serial.println("🔧 Инициализация датчика GY-21 STJF (SHT21)...");
  
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(200); // Увеличена задержка для стабилизации I2C
  
  // Сканируем I2C шину для диагностики
  scanI2C();
  
  // Проверяем наличие датчика по адресу 0x40
  Wire.beginTransmission(0x40);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("✅ Датчик SHT21 обнаружен по адресу 0x40");
    
    // Пробуем прочитать данные несколько раз
    int attempts = 0;
    const int maxAttempts = 3;
    while (attempts < maxAttempts) {
      if (readSensorData()) {
        Serial.println("✅ Датчик работает корректно");
        Serial.print("   Температура: ");
        Serial.print(temperature, 1);
        Serial.print("°C, Влажность: ");
        Serial.print(humidity, 1);
        Serial.println("%");
        return true;
      }
      attempts++;
      delay(100);
      Serial.println("   ⚠️ Попытка " + String(attempts) + " из " + String(maxAttempts) + " неудачна, повторяем...");
    }
    
    Serial.println("❌ Не удалось прочитать данные с датчика после " + String(maxAttempts) + " попыток");
  } else {
    Serial.print("❌ Датчик не обнаружен по адресу 0x40. Код ошибки I2C: ");
    Serial.println(error);
    Serial.println("   Возможные причины:");
    Serial.println("   - Неправильное подключение SDA/SCL");
    Serial.println("   - Отсутствие подтягивающих резисторов (4.7 кОм)");
    Serial.println("   - Проблема с питанием датчика (3.3V)");
    Serial.println("   - GPIO8 подключен к LED (может мешать работе I2C)");
  }
  
  // Используем значения по умолчанию
  temperature = 20.0 + calParams.tempOffset;
  humidity = 50.0 + calParams.humOffset;
  sensorConnected = false;
  return false;
}

// Мониторинг изменений состояния
void monitorConditionChanges() {
    bool currentCameraRisk = isCameraAtRisk();
    
    if (currentCameraRisk != lastCameraRiskState) {
        lastCameraRiskState = currentCameraRisk;
        conditionChangeTime = millis();
        
        if (currentCameraRisk) {
            Serial.println("⚠️ Обнаружено ухудшение условий для камеры");
        } else {
            Serial.println("✅ Условия для камеры улучшились");
        }
    }
}

// Проверка необходимости возврата к автоматике
// Ручной режим работает 5 минут независимо от условий (кроме критического перегрева)
bool shouldReturnToAutoMode() {
    if (!manualMode) return false;
    
    // Проверяем, что таймер был инициализирован
    if (manualModeStartTime == 0) {
        Serial.println("⚠️ ВНИМАНИЕ: manualModeStartTime не инициализирован!");
        manualModeStartTime = millis(); // Инициализируем сейчас
        return false;
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsedTime = 0;
    
    // Проверяем переполнение millis() (происходит примерно раз в 50 дней)
    if (currentTime < manualModeStartTime) {
        // Переполнение произошло, сбрасываем таймер
        Serial.println("⚠️ Переполнение millis(), сброс таймера ручного режима");
        manualModeStartTime = currentTime;
        return false;
    }
    
    elapsedTime = currentTime - manualModeStartTime;
    
    // Если прошло 5 минут с момента ручного управления - возвращаемся к автоматике
    if (elapsedTime >= MANUAL_MODE_TIMEOUT) {
        Serial.print("⏰ Время в ручном режиме: ");
        Serial.print(elapsedTime / 1000);
        Serial.println(" секунд (>= 300 секунд)");
        return true;
    }
    
    // Логируем оставшееся время раз в минуту
    static unsigned long lastLogTime = 0;
    if (currentTime - lastLogTime > 60000) { // Раз в минуту
        unsigned long remainingSeconds = (MANUAL_MODE_TIMEOUT - elapsedTime) / 1000;
        Serial.print("⏳ Ручной режим активен, осталось: ");
        Serial.print(remainingSeconds);
        Serial.println(" секунд до автоматического возврата");
        lastLogTime = currentTime;
    }
    
    return false;
}

// Логика управления обогревом
void controlHeaterLogic() {
  monitorConditionChanges();
  
  // Если включен режим полного выключения - обогрев всегда выключен, но датчики работают
  if (heaterOffMode) {
    if (heaterState) {
      heaterState = false;
      digitalWrite(HEATER_PIN, heaterState);
      cachedData.isValid = false;
      Serial.println("🔕 Режим выключения: Обогрев выключен, датчики работают");
    }
    return; // Выходим - обогрев не управляется, только мониторинг
  }
  
  // Проверяем, нужно ли вернуться к автоматическому режиму (только по таймеру 5 минут)
  if (shouldReturnToAutoMode()) {
    manualMode = false;
    cachedData.isValid = false; // Инвалидируем кэш при изменении режима
    Serial.println("🔄 Возврат к автоматическому режиму: прошло 5 минут");
    Serial.print("   Время в ручном режиме: ");
    Serial.print((millis() - manualModeStartTime) / 1000);
    Serial.println(" секунд");
  }
  
  // КРИТИЧЕСКАЯ БЕЗОПАСНОСТЬ: только при перегреве >= 15.0 или ошибке датчика
  // В этих случаях принудительно выключаем обогрев и ручной режим
  bool hardwareCritical = temperature >= 15.0 || !sensorConnected;
  if (hardwareCritical) {
    heaterState = false;
    manualMode = false;
    heaterOffMode = false; // Сбрасываем режим выключения при критической ситуации
    digitalWrite(HEATER_PIN, heaterState);
    
    if (temperature >= 15.0) {
      Serial.println("🚨 ПЕРЕГРЕВ: Выключение обогрева для безопасности");
    } else {
      Serial.println("🚨 ОШИБКА ДАТЧИКА: Выключение обогрева");
    }
    return;
  }
  
  // Если включен ручной режим - работаем в ручном режиме независимо от других условий
  // Ручной режим работает 5 минут, после чего автоматически переключается обратно
  if (manualMode) {
    // ВАЖНО: В ручном режиме обогрев НЕ управляется автоматически!
    // Пользователь сам включает/выключает обогрев через кнопку "Включить/Выключить обогрев"
    // Обогрев остается в том состоянии, которое установил пользователь (или было до переключения)
    // Режим автоматически переключится через 5 минут (проверяется в shouldReturnToAutoMode)
    // Обогрев при этом НЕ меняется - остается в текущем состоянии
    return;
  }
  
  // АВТОМАТИЧЕСКИЙ РЕЖИМ: работаем по условиям
  bool cameraRisk = isCameraAtRisk();
  if (cameraRisk) {
    bool stateChanged = (heaterState != true);
    heaterState = true;
    digitalWrite(HEATER_PIN, heaterState);
    if (stateChanged) cachedData.isValid = false; // Инвалидируем кэш при изменении состояния
    Serial.println("🔒 Защита камеры: Включение обогрева (автоматический режим)");
    return;
  }
  
  // Автоматическое управление по порогам
  bool newHeaterState = false;
  
  if (temperature <= calParams.tempThreshold || humidity >= calParams.humThreshold) {
    newHeaterState = true;
  }
  
  if (newHeaterState != heaterState) {
    heaterState = newHeaterState;
    digitalWrite(HEATER_PIN, heaterState);
    cachedData.isValid = false; // Инвалидируем кэш при изменении состояния обогрева
    
    if (heaterState) {
      Serial.println("🔧 Автоматика: Включение обогрева");
    } else {
      Serial.println("🔧 Автоматика: Выключение обогрева");
    }
  }
}

// Функция мониторинга памяти
void printMemoryInfo() {
  size_t freeHeap = ESP.getFreeHeap();
  size_t totalHeap = ESP.getHeapSize();
  size_t minFreeHeap = ESP.getMinFreeHeap();
  
  // Оптимизировано: выводим информацию только при критическом уровне или раз в 5 минут
  static unsigned long lastFullReport = 0;
  bool isCritical = (freeHeap < MIN_FREE_HEAP);
  bool shouldReport = isCritical || (millis() - lastFullReport > 300000); // Полный отчет раз в 5 минут
  
  if (shouldReport) {
    Serial.print("💾 Память: свободно=");
    Serial.print(freeHeap);
    Serial.print(" байт, минимум=");
    Serial.print(minFreeHeap);
    Serial.print(" байт, всего=");
    Serial.print(totalHeap);
    Serial.print(" байт (");
    Serial.print((freeHeap * 100) / totalHeap);
    Serial.println("%)");
    lastFullReport = millis();
  }
  
  // Предупреждение при низкой памяти
  if (isCritical) {
    Serial.println("⚠️ ВНИМАНИЕ: Низкий уровень свободной памяти!");
    cleanupMemory();
  }
}

// Функция очистки памяти
void cleanupMemory() {
  Serial.println("🧹 Очистка памяти...");
  
  // Очистка JSON документа сканирования
  scanResult.clear();
  
  // Очистка результатов сканирования WiFi (если не идет сканирование)
  if (!isScanning) {
    WiFi.scanDelete();
  }
  
  // Принудительная сборка мусора для ESP32-C3
  // ESP32-C3 не поддерживает heap_caps_malloc_trim, используем альтернативный метод
  #ifdef CONFIG_IDF_TARGET_ESP32C3
    // Для ESP32-C3 просто очищаем буферы, сборка мусора происходит автоматически
    // Можно принудительно вызвать сборку через создание временного объекта
    {
      String temp = "";
      temp.reserve(1);
      temp = "";
    }
  #endif
  
  // Вывод информации о памяти после очистки
  Serial.print("💾 Память после очистки: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" байт");
}

// Функция проверки и восстановления WiFi соединения
void checkAndRestoreWiFi() {
  // Простая проверка без мьютекса (ESP32-C3 одноядерный)
  if (!wifiSTAEnabled || staSSID.length() == 0) {
    return; // Если режим STA не настроен
  }
  
  unsigned long currentTime = millis();
  if (currentTime - lastWifiCheck > WIFI_CHECK_INTERVAL) {
    lastWifiCheck = currentTime;
    
    wl_status_t status = WiFi.status();
    
    if (status == WL_CONNECTED) {
      wifiDisconnectCount = 0;
      wifiConnected = true;
      // DNS-сервер только в режиме точки доступа
      if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
      }
      return;
    }
    
    wifiConnected = false;
    wifiDisconnectCount++;
    Serial.print("⚠️ WiFi статус: ");
    switch (status) {
      case WL_IDLE_STATUS:
        Serial.print("IDLE");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.print("НЕТ СЕТИ");
        break;
      case WL_SCAN_COMPLETED:
        Serial.print("СКАНИРОВАНИЕ");
        break;
      case WL_CONNECT_FAILED:
        Serial.print("ОШИБКА ПОДКЛЮЧЕНИЯ");
        break;
      case WL_CONNECTION_LOST:
        Serial.print("ПОТЕРЯНО СОЕДИНЕНИЕ");
        break;
      case WL_DISCONNECTED:
        Serial.print("ОТКЛЮЧЕНО");
        break;
      default:
        Serial.print(status);
    }
    Serial.print(", счетчик: ");
    Serial.println(wifiDisconnectCount);
    
    if (wifiDisconnectCount >= MAX_DISCONNECT_COUNT) {
      Serial.println("🔄 Переключение на точку доступа");
      // Устанавливаем флаги (без мьютекса - ESP32-C3 одноядерный)
      forceAPMode = true;
      wifiSTAEnabled = false;
      saveWiFiSettingsToPreferences();
      restartWiFi();
      wifiDisconnectCount = 0; // Сброс счетчика после переключения
    }
  }
}

// УПРОЩЕННАЯ ОБРАБОТКА КНОПКИ TTP-223 (A=1, B=0)
void handleButton() {
  static bool wasPressed = false;
  static unsigned long pressStart = 0;
  
  // Для TTP-223 с A=1/B=0: LOW = нажата, HIGH = отпущена
  bool isPressed = (digitalRead(BUTTON_PIN) == LOW);
  
  // Кнопка только что нажата
  if (isPressed && !wasPressed) {
    pressStart = millis();
    wasPressed = true;
    Serial.println("🔘 Кнопка нажата (TTP-223)");
  }
  
  // Кнопка только что отпущена
  if (!isPressed && wasPressed) {
    wasPressed = false;
    unsigned long duration = millis() - pressStart;
    
    Serial.print("🔘 Отпущена, длительность: ");
    Serial.print(duration);
    Serial.println(" мс");
    
    // Фильтр дребезга (игнорируем <30мс)
    if (duration < 30) {
      Serial.println("🔘 Слишком коротко (дребезг) - игнорируем");
      return;
    }
    
    // КОРОТКОЕ НАЖАТИЕ (30мс - 2 секунды)
    if (duration >= 30 && duration < 2000) {
      manualMode = true;
      manualModeStartTime = millis();
      manualModeStartRisk = isCameraAtRisk();
      
      heaterState = !heaterState;
      digitalWrite(HEATER_PIN, heaterState);
      cachedData.isValid = false; // Инвалидируем кэш при изменении состояния кнопкой
      
      Serial.print("🔘 Короткое нажатие: Обогрев ");
      Serial.println(heaterState ? "ВКЛ" : "ВЫКЛ");
    }
    // ДЛИННОЕ НАЖАТИЕ (2 - 5 секунд)
    else if (duration >= 2000 && duration < 5000) {
      Serial.println("🔘 Длинное нажатие: Перезапуск WiFi");
      restartWiFi();
    }
    // ПРОДОЛЖИТЕЛЬНОЕ НАЖАТИЕ (5+ секунд) - Сброс настроек WiFi
    else if (duration >= 5000) {
      Serial.println("🔘 Продолжительное нажатие (5+ сек): Сброс настроек WiFi");
      resetWiFiSettings();
    }
  }
  
  // ОТДЕЛЬНАЯ ПРОВЕРКА ПРОДОЛЖИТЕЛЬНОГО УДЕРЖАНИЯ (без отпускания)
  static bool longPressTriggered = false;
  static unsigned long lastWarningTime = 0;
  if (isPressed) {
    unsigned long currentDuration = millis() - pressStart;
    
    // Предупреждение за 1 секунду до сброса (на 4 секунде)
    if (currentDuration >= 4000 && currentDuration < 5000 && (millis() - lastWarningTime) > 500) {
      Serial.println("⚠️ Еще 1 секунда до сброса настроек WiFi...");
      lastWarningTime = millis();
    }
    
    // Если удерживаем 5+ секунд - сброс настроек WiFi
    if (currentDuration >= 5000 && !longPressTriggered) {
      longPressTriggered = true;
      Serial.println("🔘 Продолжительное удержание (5+ сек): Сброс настроек WiFi!");
      resetWiFiSettings();
    }
  } else {
    // Сброс флагов при отпускании
    longPressTriggered = false;
    lastWarningTime = 0;
  }
}

// Функция сброса WiFi подключения и перехода на точку доступа
void resetWiFiConnection() {
  Serial.println("🔄 Сброс WiFi подключения, переход на точку доступа");
  
  // Устанавливаем флаги (без мьютекса - ESP32-C3 одноядерный)
  forceAPMode = true;
  wifiSTAEnabled = false;
  wifiConnected = false;
  
  saveWiFiSettingsToPreferences();
  
  WiFi.disconnect(true);
  // Оптимизировано: короткая задержка необходима для стабильности WiFi, но можно уменьшить
  delay(500); // Уменьшено с 1000 до 500мс для улучшения отзывчивости
  
  startAP();
  
  Serial.println("✅ Переключено на точку доступа");
}

// Настройка параметров WiFi для надежного подключения
void configureWiFiForConnection() {
  // Отключаем энергосбережение WiFi для более стабильного подключения
  WiFi.setSleep(false);
  
  // Увеличиваем мощность передатчика для лучшего сигнала
  // ESP32-C3: используем константу для максимальной мощности
  // Пробуем установить максимальную доступную мощность
  #if defined(WIFI_POWER_19_5dBm)
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
  #elif defined(WIFI_POWER_19dBm)
    WiFi.setTxPower(WIFI_POWER_19dBm);
  #elif defined(WIFI_POWER_18_5dBm)
    WiFi.setTxPower(WIFI_POWER_18_5dBm);
  #elif defined(WIFI_POWER_17dBm)
    WiFi.setTxPower(WIFI_POWER_17dBm);
  #endif
  // Если константы не определены, мощность останется по умолчанию (не критично)
  
  // Включаем автоматическое переподключение
  WiFi.setAutoReconnect(true);
  
  Serial.println("⚙️ WiFi настроен для надежного подключения");
}

// Проверка доступности WiFi сети перед подключением
bool isNetworkAvailable(String ssid) {
  Serial.println("🔍 Проверка доступности сети: " + ssid);
  
  // Быстрое сканирование для проверки доступности сети
  int n = WiFi.scanNetworks(false, false); // Синхронное, без скрытых сетей
  
  if (n == WIFI_SCAN_FAILED) {
    Serial.println("⚠️ Ошибка сканирования, продолжаем попытку подключения");
    return true; // Если сканирование не удалось, все равно пытаемся подключиться
  }
  
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == ssid) {
      int rssi = WiFi.RSSI(i);
      Serial.print("✅ Сеть найдена! RSSI: ");
      Serial.print(rssi);
      Serial.println(" dBm");
      
      // Проверяем силу сигнала (слишком слабый сигнал может не подключиться)
      if (rssi < -90) {
        Serial.println("⚠️ Слишком слабый сигнал, но попробуем подключиться");
      }
      
      WiFi.scanDelete(); // Очищаем результаты сканирования
      return true;
    }
  }
  
  Serial.println("❌ Сеть не найдена при сканировании");
  WiFi.scanDelete(); // Очищаем результаты сканирования
  return false;
}

// Улучшенная функция подключения к WiFi с повторными попытками
bool connectToWiFiWithRetry(String ssid, String password, int maxRetries = 8) {
  configureWiFiForConnection();
  
  // Проверяем доступность сети перед первой попыткой (более агрессивная проверка)
  bool networkFound = false;
  for (int preCheck = 0; preCheck < 3; preCheck++) {
    if (isNetworkAvailable(ssid)) {
      networkFound = true;
      break;
    }
    if (preCheck < 2) {
      Serial.println("   ⏳ Повторная проверка через 1.5 секунды...");
      delay(1500);
    }
  }
  
  if (!networkFound) {
    Serial.println("⚠️ Сеть не найдена при предварительной проверке, но продолжаем попытки подключения");
    // Не возвращаем false сразу - возможно сеть скрыта или временно недоступна
  }
  
  for (int retry = 1; retry <= maxRetries; retry++) {
    Serial.print("📡 Попытка подключения " + String(retry) + "/" + String(maxRetries) + " к сети: " + ssid);
    
    // Отключаем AP перед каждой попыткой
  WiFi.softAPdisconnect(true);
    delay(2000); // Увеличена задержка для полного отключения AP
    
    // Полный сброс WiFi перед подключением
    WiFi.mode(WIFI_OFF);
    delay(1500); // Увеличена задержка для полного выключения
    WiFi.mode(WIFI_STA);
    delay(1500); // Увеличена задержка для инициализации
    
    WiFi.disconnect();
    delay(2000); // Увеличена задержка для полного отключения
    
    // Проверяем, что WiFi готов к подключению (более тщательная проверка)
    unsigned long initStart = millis();
    int initAttempts = 0;
    while (initAttempts < 30) {
      wl_status_t status = WiFi.status();
      if (status == WL_DISCONNECTED || status == WL_IDLE_STATUS) {
        break; // WiFi готов
      }
      delay(100);
      initAttempts++;
      if (millis() - initStart > 5000) break; // Максимум 5 секунд на инициализацию
    }
    
    // Дополнительная пауза для стабилизации
    delay(500);
    
    // Начинаем подключение
    unsigned long startTime = millis();
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Увеличиваем время ожидания до 70 секунд для каждой попытки
    int attempts = 0;
    const int maxAttempts = 140; // 140 * 500ms = 70 секунд
    const unsigned long timeout = 70000; // 70 секунд
    
    bool connected = false;
    int consecutiveFailures = 0;
    wl_status_t lastStatus = WL_IDLE_STATUS;
    
    while (attempts < maxAttempts) {
      wl_status_t status = WiFi.status();
      
      if (status == WL_CONNECTED) {
        // Дополнительная проверка: убеждаемся, что IP получен
        delay(1000); // Увеличена задержка для получения IP
        if (WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
          // Дополнительная проверка: ping gateway для подтверждения подключения
          connected = true;
          break;
        } else {
          // IP не получен, продолжаем ожидание
          Serial.print("⏳");
        }
      }
      
      // Отслеживаем повторяющиеся ошибки
      if (status == lastStatus && status != WL_CONNECTED && status != WL_DISCONNECTED) {
        consecutiveFailures++;
        if (consecutiveFailures > 20) {
          // Если статус не меняется долгое время, прерываем попытку
          Serial.println("\n⚠️ Статус не меняется, прерываем попытку");
          break;
        }
      } else {
        consecutiveFailures = 0;
      }
      lastStatus = status;
      
      // Проверяем критические ошибки, при которых повтор не поможет
      if (status == WL_NO_SSID_AVAIL) {
        // Не прерываем сразу - возможно временная проблема
        if (attempts > 40) { // Только после 20 секунд ожидания
          Serial.println("\n⚠️ Сеть не найдена, но продолжаем попытки");
          break; // Выходим из цикла ожидания, но продолжаем попытки
        }
      }
      
      delay(500);
      if (attempts % 10 == 0) {
      Serial.print(".");
      }
      attempts++;
      
      if (millis() - startTime > timeout) {
        Serial.println("\n⏱️ Таймаут подключения (попытка " + String(retry) + ")");
        break;
      }
    }
    
    if (connected) {
      Serial.println("\n✅ Подключено к WiFi на попытке " + String(retry) + "!");
      Serial.print("   📡 IP адрес: ");
      Serial.println(WiFi.localIP());
      Serial.print("   📶 Сила сигнала (RSSI): ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      
      return true;
    } else {
      wl_status_t status = WiFi.status();
      Serial.print("\n❌ Попытка " + String(retry) + " не удалась. Статус: ");
      switch (status) {
        case WL_NO_SSID_AVAIL:
          Serial.println("Сеть не найдена");
          // Проверяем еще раз перед возвратом false
          if (retry < maxRetries) {
            delay(2000);
            if (isNetworkAvailable(ssid)) {
              Serial.println("   🔄 Сеть найдена, продолжаем попытки");
              break; // Продолжаем попытки
            }
          }
          return false;
        case WL_CONNECT_FAILED:
          Serial.println("Ошибка аутентификации");
          break;
        case WL_DISCONNECTED:
          Serial.println("Отключено");
          break;
        default:
          Serial.println("Неизвестная ошибка (" + String(status) + ")");
      }
      
      // Если это не последняя попытка, делаем паузу перед следующей
      if (retry < maxRetries) {
        // Уменьшаем паузы для более быстрого подключения: 2, 2.5, 3, 3.5 секунд
        int pauseTime = 2000 + (retry * 500);
        Serial.print("   ⏳ Пауза ");
        Serial.print(pauseTime / 1000.0, 1);
        Serial.println(" секунд перед следующей попыткой...");
        delay(pauseTime);
        
        // Перед следующей попыткой быстро проверяем доступность сети
        if (retry < maxRetries - 1) {
          Serial.println("   🔍 Быстрая проверка доступности сети...");
          if (!isNetworkAvailable(ssid)) {
            Serial.println("   ⚠️ Сеть временно недоступна, дополнительная пауза 1.5 сек");
            delay(1500);
          } else {
            Serial.println("   ✅ Сеть доступна, продолжаем подключение");
          }
        }
      }
    }
  }
  
  Serial.println("\n❌ Не удалось подключиться после всех попыток");
  return false;
}

// Функция перезапуска WiFi (без перезагрузки устройства)
void restartWiFi() {
  Serial.println("🔄 Перезапуск WiFi...");
  
  if (wifiSTAEnabled && staSSID.length() > 0 && !forceAPMode) {
    // Используем улучшенную функцию подключения с повторными попытками (8 попыток)
    if (connectToWiFiWithRetry(staSSID, staPassword, 8)) {
      // Устанавливаем переменные (без мьютекса - ESP32-C3 одноядерный)
      wifiConnected = true;
      wifiDisconnectCount = 0;
      forceAPMode = false;
      
      // Переключаемся в режим AP+STA с задержкой для стабильности
      delay(1000);
      WiFi.mode(WIFI_AP_STA);
      delay(1000);
      startAP();
    } else {
      wifiConnected = false;
      Serial.println("\n❌ Не удалось подключиться к WiFi после всех попыток");
      // НЕ устанавливаем forceAPMode сразу - даем возможность повторить попытку
      Serial.println("   📡 Запускаем точку доступа...");
      
      // Возвращаемся к точке доступа
      WiFi.mode(WIFI_AP);
      startAP();
    }
  } else {
    Serial.println("📡 Запуск точки доступа (WiFi не настроен)");
    WiFi.mode(WIFI_AP);
    startAP();
  }
}

// Функция подключения к WiFi сети
bool connectToWiFiNetwork(String ssid, String password) {
  // Используем улучшенную функцию подключения с повторными попытками (8 попыток)
  if (connectToWiFiWithRetry(ssid, password, 8)) {
    // Устанавливаем переменные (без мьютекса - ESP32-C3 одноядерный)
    wifiConnected = true;
    wifiSTAEnabled = true;
    staSSID = ssid;
    staPassword = password;
    wifiDisconnectCount = 0;
    forceAPMode = false;
    
    // Переключаемся в режим AP+STA с задержкой для стабильности
    delay(1000);
    WiFi.mode(WIFI_AP_STA);
    delay(1000);
    startAP();
    
    saveWiFiSettingsToPreferences();
    
    return true;
  } else {
    wifiConnected = false;
    Serial.println("\n❌ Не удалось подключиться к WiFi после всех попыток");
    
    // Определяем причину ошибки
    wl_status_t status = WiFi.status();
    String errorMsg = "";
    switch (status) {
      case WL_NO_SSID_AVAIL:
        errorMsg = "not_found";  // Сеть не найдена
        break;
      case WL_CONNECT_FAILED:
        // WL_CONNECT_FAILED может означать как неправильный пароль, так и другие ошибки аутентификации
        errorMsg = "auth_failed";  // Ошибка аутентификации (включая неправильный пароль)
        break;
      case WL_DISCONNECTED:
        errorMsg = "disconnected";  // Отключено
        break;
      default:
        errorMsg = "unknown_error";
    }
    
    Serial.println("Причина ошибки: " + errorMsg);
    
    // Устанавливаем флаг (без мьютекса - ESP32-C3 одноядерный)
    forceAPMode = true;
    startAP();
    
    return false;
  }
}

// Функция сброса настроек WiFi
void resetWiFiSettings() {
  Serial.println("🔄 Сброс настроек WiFi к заводским...");
  
  if (!preferences.begin("wifi-config", false)) {
    Serial.println("❌ Ошибка открытия Preferences WiFi");
    return;
  }
  
  preferences.clear();
  
  // Проверяем успешность записи заводских настроек
  size_t written1 = preferences.putString("apSSID", "Camera_Heater_AP");
  size_t written2 = preferences.putString("apPassword", "12345678");
  size_t written3 = preferences.putBool("staEnabled", false);
  
  preferences.end();
  
  if (written1 == 0 || written2 == 0 || written3 == 0) {
    Serial.println("⚠️ Ошибка записи заводских настроек WiFi в Preferences");
  }
  
  Serial.println("✅ Настройки WiFi сброшены");
  
  // Обновляем переменные в памяти
  apSSID = "Camera_Heater_AP";
  apPassword = "12345678";
  wifiSTAEnabled = false;
  forceAPMode = true;
  wifiConnected = false;
  
  // Отключаем WiFi и перезапускаем в режиме AP
  WiFi.disconnect(true);
  delay(500);
  
  // Перезапускаем WiFi в режиме AP
  startWiFi();
  
  Serial.println("✅ WiFi перезапущен в режиме точки доступа");
}

// Запуск WiFi
void startWiFi() {
  Serial.println("\n🔄 Инициализация WiFi...");
  
  // Используем значения по умолчанию, если Preferences не открывается
  if (!preferences.begin("wifi-config", true)) {
    Serial.println("⚠️ Ошибка открытия Preferences WiFi, используем значения по умолчанию");
    wifiSTAEnabled = false;
    staSSID = "";
    staPassword = "";
    apSSID = "Camera_Heater_AP";
    apPassword = "12345678";
    
    // Запускаем точку доступа с заводскими настройками
    Serial.println("📡 Запуск точки доступа с заводскими настройками...");
    delay(500);
    WiFi.mode(WIFI_AP);
    delay(1000);
    startAP();
    return;
  }
  
  wifiSTAEnabled = preferences.getBool("staEnabled", false);
  staSSID = preferences.getString("staSSID", "");
  staPassword = preferences.getString("staPassword", "");
  apSSID = preferences.getString("apSSID", "Camera_Heater_AP");
  apPassword = preferences.getString("apPassword", "12345678");
  
  // Валидация загруженных строк WiFi
  if (staSSID.length() > 32) {
    Serial.println("⚠️ Загруженный staSSID слишком длинный, обрезаем");
    staSSID = staSSID.substring(0, 32);
  }
  if (staPassword.length() > 63) {
    Serial.println("⚠️ Загруженный staPassword слишком длинный, обрезаем");
    staPassword = staPassword.substring(0, 63);
  }
  if (apSSID.length() > 32) {
    Serial.println("⚠️ Загруженный apSSID слишком длинный, используем заводской");
    apSSID = "Camera_Heater_AP";
  }
  if (apPassword.length() > 63) {
    Serial.println("⚠️ Загруженный apPassword слишком длинный, используем заводской");
    apPassword = "12345678";
  }
  
  preferences.end();
  
  // ИСПРАВЛЕНИЕ: Сбрасываем forceAPMode при старте, если есть сохраненные настройки WiFi
  // Это позволяет устройству пытаться подключиться к сохраненной сети после перезагрузки
  if (wifiSTAEnabled && staSSID.length() > 0) {
    forceAPMode = false; // Сбрасываем флаг принудительного режима AP при старте
    Serial.println("🔄 Сброс forceAPMode при старте - будет попытка подключения к сохраненной сети");
  }
  
  // ИСПРАВЛЕНИЕ: Сначала пытаемся подключиться к WiFi, затем запускаем AP
  // Если настроено подключение к WiFi сети - пытаемся подключиться
  if (wifiSTAEnabled && staSSID.length() > 0 && !forceAPMode) {
    Serial.println("📡 Автоматическое подключение к сохраненной сети: " + staSSID);
    
    // Используем улучшенную функцию подключения с повторными попытками (8 попыток)
    if (connectToWiFiWithRetry(staSSID, staPassword, 8)) {
      // Устанавливаем переменные (без мьютекса - ESP32-C3 одноядерный)
      wifiConnected = true;
      wifiDisconnectCount = 0;
      forceAPMode = false;
      
      Serial.println("\n✅ Автоматически подключено к WiFi!");
      Serial.print("   📡 IP адрес: ");
      Serial.println(WiFi.localIP());
      Serial.print("   📶 SSID: ");
      Serial.println(staSSID);
      
      // Переключаемся в режим AP+STA с задержкой для стабильности
      delay(1000);
      WiFi.mode(WIFI_AP_STA);
      delay(1000);
      startAP();
    } else {
      wifiConnected = false;
      wl_status_t status = WiFi.status();
      Serial.println("\n❌ Не удалось автоматически подключиться к WiFi после всех попыток");
      Serial.print("   Статус: ");
      Serial.println(status);
      
      // НЕ устанавливаем forceAPMode сразу - даем возможность повторить попытку
      // forceAPMode будет установлен только после нескольких неудачных попыток
      Serial.println("   📡 Запускаем точку доступа для настройки...");
      
      // Запускаем только точку доступа с задержкой для стабильности
      delay(1000);
      WiFi.mode(WIFI_AP);
      delay(1000);
      startAP();
    }
  } else {
    // WiFi не настроен или принудительный режим AP
    Serial.println("📡 Режим точки доступа (WiFi не настроен)");
    delay(500);
    WiFi.mode(WIFI_AP);
    delay(1000);
    startAP();
  }
}

// Запуск точки доступа
void startAP() {
  Serial.println("\n📡 Запуск точки доступа...");
  
  // ВАЖНО: Убеждаемся, что apSSID установлен правильно
  if (apSSID.length() == 0) {
    apSSID = "Camera_Heater_AP";
    Serial.println("⚠️ apSSID пуст, используем значение по умолчанию");
  }
  if (apPassword.length() == 0) {
    apPassword = "12345678";
    Serial.println("⚠️ apPassword пуст, используем значение по умолчанию");
  }
  
  // Отключаем AP перед переключением режима
  WiFi.softAPdisconnect(true);
  delay(1000);
  
  // Определяем режим: если подключены к WiFi, используем AP+STA, иначе только AP
  bool shouldUseAPSTA = (wifiSTAEnabled && staSSID.length() > 0 && !forceAPMode && wifiConnected && WiFi.status() == WL_CONNECTED);
  
  // Получаем текущий режим WiFi
  WiFiMode_t currentMode = WiFi.getMode();
  
  // Переключаем режим только если нужно
  if (shouldUseAPSTA && currentMode != WIFI_AP_STA) {
    // Если уже подключены к WiFi, переключаемся в режим AP+STA
    WiFi.mode(WIFI_AP_STA);
    delay(1500); // Увеличена задержка для стабильности
    Serial.println("   📱 Режим: AP+STA (Точка доступа + Подключение к сети)");
  } else if (!shouldUseAPSTA && currentMode != WIFI_AP) {
    // Если не подключены к WiFi, используем только AP
    WiFi.mode(WIFI_AP);
    delay(1500); // Увеличена задержка для стабильности
    Serial.println("   📱 Режим: AP (Только точка доступа)");
  } else {
    // Режим уже правильный, просто выводим информацию
    if (shouldUseAPSTA) {
      Serial.println("   📱 Режим: AP+STA (уже установлен)");
    } else {
      Serial.println("   📱 Режим: AP (уже установлен)");
    }
  }
  
  // Дополнительная задержка после переключения режима для стабилизации
  delay(500);
  
  // КРИТИЧНО: Явно устанавливаем имя AP после переключения режима
  // Это предотвращает изменение имени на стандартное ESP32 имя
  if (!WiFi.softAP(apSSID.c_str(), apPassword.c_str())) {
    Serial.println("⚠️ Ошибка запуска AP, повторная попытка...");
    delay(500);
    WiFi.softAPdisconnect(true);
    delay(500);
    if (!WiFi.softAP(apSSID.c_str(), apPassword.c_str())) {
      Serial.println("❌ Критическая ошибка: не удалось запустить AP");
      return;
    }
  }
  
  // Дополнительная задержка для инициализации AP
  delay(500);
  
  // Проверяем, что имя AP установлено правильно
  String currentAPSSID = WiFi.softAPSSID();
  if (currentAPSSID != apSSID && currentAPSSID.length() > 0) {
    Serial.print("⚠️ Имя AP не соответствует настройкам: ");
    Serial.print(currentAPSSID);
    Serial.print(" -> ");
    Serial.println(apSSID);
    Serial.println("   🔄 Переустанавливаем правильное имя...");
    
    // Переустанавливаем правильное имя AP
    WiFi.softAPdisconnect(true);
    delay(500);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    delay(500);
    
    // Проверяем результат
    currentAPSSID = WiFi.softAPSSID();
    if (currentAPSSID == apSSID) {
      Serial.println("   ✅ Имя AP успешно восстановлено");
    } else {
      Serial.print("   ⚠️ Имя AP все еще: ");
      Serial.println(currentAPSSID);
    }
  }
  
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  if (!WiFi.softAPConfig(local_ip, gateway, subnet)) {
    Serial.println("⚠️ Предупреждение: не удалось настроить AP конфигурацию");
  }
  
  // ВАЖНО: Явно устанавливаем имя и пароль AP после переключения режима
  if (!WiFi.softAP(apSSID.c_str(), apPassword.c_str())) {
    Serial.println("❌ Ошибка запуска AP, повторная попытка...");
    delay(1000);
    // Повторная попытка с явным отключением
    WiFi.softAPdisconnect(true);
    delay(500);
    if (!WiFi.softAP(apSSID.c_str(), apPassword.c_str())) {
      Serial.println("❌ Критическая ошибка запуска AP");
    return;
    }
  }
  
  delay(1000); // Увеличена задержка для полной инициализации AP
  
  // Проверяем, что имя AP установлено правильно (используем уже объявленную переменную)
  currentAPSSID = WiFi.softAPSSID();
  if (currentAPSSID != apSSID && currentAPSSID.length() > 0) {
    Serial.print("⚠️ Имя AP изменилось на: ");
    Serial.println(currentAPSSID);
    Serial.println("   🔄 Переустанавливаем правильное имя...");
    WiFi.softAPdisconnect(true);
    delay(500);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    delay(500);
  }
  
  Serial.print("   📡 AP IP адрес: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("   📶 SSID: ");
  Serial.println(WiFi.softAPSSID().length() > 0 ? WiFi.softAPSSID() : apSSID);
  Serial.println("   🔑 Пароль: " + apPassword);
  
  // Запуск DNS-сервера для переадресации на портал авторизации (captive portal)
  // DNS-сервер перехватывает все DNS-запросы и перенаправляет на наш веб-сервер
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("   ✅ DNS-сервер запущен для переадресации на портал");
  
  if (shouldUseAPSTA && WiFi.status() == WL_CONNECTED) {
    Serial.print("   🌐 STA IP адрес: ");
    Serial.println(WiFi.localIP());
  }
}

// Вывод информации о подключении
void printConnectionInfo() {
  Serial.println("\n📱 ИНСТРУКЦИЯ ПОДКЛЮЧЕНИЯ:");
  Serial.println("   1. Откройте настройки WiFi на телефоне");
  Serial.println("   2. Найдите сеть: " + apSSID);
  Serial.println("   3. Пароль: " + apPassword);
  Serial.println("   4. Подключитесь к сети");
  Serial.println("   5. Должно открыться окно авторизации");
  Serial.println("   6. Если нет - откройте браузер:");
  Serial.println("      http://192.168.4.1");
  Serial.println("==================================================\n");
}

// Обработчик корневой страницы (Пункт 6: оптимизация памяти)
void handleRoot() {
  Serial.println("📄 Запрос главной страницы");
  // Отправляем HTML напрямую из PROGMEM для экономии RAM
  server.send_P(200, "text/html; charset=utf-8", htmlPage);
}

// Обработчик данных - оптимизирован: использует кэширование и char[] буфер
void handleData() {
  // Оптимизация: используем кэш, если данные свежие (меньше 500мс)
  unsigned long currentTime = millis();
  if (cachedData.isValid && (currentTime - cachedData.lastUpdate) < CACHE_VALID_TIME) {
    server.send(200, "application/json", cachedData.jsonBuffer);
    return; // Быстрый ответ из кэша
  }
  
  // Статический буфер для JSON ответа (переиспользуется)
  static char jsonBuffer[1024]; // Увеличено для передачи паролей WiFi
  
  // Определяем текущий режим WiFi
  const char* currentWiFiMode;
  const char* currentSSID;
  IPAddress staIP = wifiConnected ? WiFi.localIP() : IPAddress(0, 0, 0, 0);
  char ipStr[16] = "";
  
  // Экранируем пароли для JSON (заменяем кавычки и обратные слеши)
  String safeStaPassword = staPassword;
  String safeApPassword = apPassword;
  safeStaPassword.replace("\\", "\\\\");
  safeStaPassword.replace("\"", "\\\"");
  safeApPassword.replace("\\", "\\\\");
  safeApPassword.replace("\"", "\\\"");
  
  if (forceAPMode || !wifiSTAEnabled || !wifiConnected) {
    currentWiFiMode = "ap";
    currentSSID = apSSID.c_str();
  } else {
    currentWiFiMode = "sta";
    currentSSID = staSSID.c_str();
    if (wifiConnected) {
      snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", staIP[0], staIP[1], staIP[2], staIP[3]);
    }
  }
  
  // Формируем JSON через snprintf для экономии памяти
  // Добавляем пароли WiFi для автозаполнения в приложении (только если устройство подключено)
  int len = snprintf(jsonBuffer, sizeof(jsonBuffer),
    "{"
    "\"temperature\":%.1f,"
    "\"humidity\":%.1f,"
    "\"heaterState\":%s,"
    "\"sensorConnected\":%s,"
    "\"autoMode\":%s,"
    "\"heaterOffMode\":%s,"
    "\"wifiMode\":\"%s\","
    "\"currentSSID\":\"%s\","
    "\"staIP\":\"%s\","
    "\"staNetwork\":\"%s\","
    "\"staPassword\":\"%s\","
    "\"apPassword\":\"%s\","
    "\"hideConnectionInfo\":%s,"
    "\"calibrationParams\":{"
    "\"tempThreshold\":%.1f,"
    "\"humThreshold\":%.1f,"
    "\"tempOffset\":%.1f,"
    "\"humOffset\":%.1f"
    "}}",
    temperature,
    humidity,
    heaterState ? "true" : "false",
    sensorConnected ? "true" : "false",
    (!manualMode && !heaterOffMode) ? "true" : "false",
    heaterOffMode ? "true" : "false",
    currentWiFiMode,
    currentSSID,
    ipStr,
    staSSID.c_str(),
    safeStaPassword.c_str(),
    safeApPassword.c_str(),
    showConnectionInfo ? "false" : "true",
    calParams.tempThreshold,
    calParams.humThreshold,
    calParams.tempOffset,
    calParams.humOffset
  );
  
  // Сохраняем в кэш для быстрого ответа на следующие запросы
  if (len > 0 && len < sizeof(cachedData.jsonBuffer)) {
    strncpy(cachedData.jsonBuffer, jsonBuffer, sizeof(cachedData.jsonBuffer) - 1);
    cachedData.jsonBuffer[sizeof(cachedData.jsonBuffer) - 1] = '\0';
    cachedData.lastUpdate = millis();
    cachedData.isValid = true;
  }
  
  server.send(200, "application/json", jsonBuffer);
}

// Функция для отправки данных по UDP (широковещательная рассылка)
void sendUdpBroadcast() {
  // Проверяем, что WiFi инициализирован
  if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
    return;
  }
  
  // Используем тот же JSON буфер, что и для HTTP
  static char jsonBuffer[1024];
  
  // Определяем текущий режим WiFi
  const char* currentWiFiMode;
  const char* currentSSID;
  IPAddress staIP = wifiConnected ? WiFi.localIP() : IPAddress(0, 0, 0, 0);
  char ipStr[16] = "";
  
  // Экранируем пароли для JSON
  String safeStaPassword = staPassword;
  String safeApPassword = apPassword;
  safeStaPassword.replace("\\", "\\\\");
  safeStaPassword.replace("\"", "\\\"");
  safeApPassword.replace("\\", "\\\\");
  safeApPassword.replace("\"", "\\\"");
  
  if (forceAPMode || !wifiSTAEnabled || !wifiConnected) {
    currentWiFiMode = "ap";
    currentSSID = apSSID.c_str();
  } else {
    currentWiFiMode = "sta";
    currentSSID = staSSID.c_str();
    if (wifiConnected) {
      snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", staIP[0], staIP[1], staIP[2], staIP[3]);
    }
  }
  
  // Формируем JSON
  int len = snprintf(jsonBuffer, sizeof(jsonBuffer),
    "{"
    "\"temperature\":%.1f,"
    "\"humidity\":%.1f,"
    "\"heaterState\":%s,"
    "\"sensorConnected\":%s,"
    "\"autoMode\":%s,"
    "\"heaterOffMode\":%s,"
    "\"wifiMode\":\"%s\","
    "\"currentSSID\":\"%s\","
    "\"staIP\":\"%s\","
    "\"staNetwork\":\"%s\","
    "\"staPassword\":\"%s\","
    "\"apPassword\":\"%s\","
    "\"hideConnectionInfo\":%s,"
    "\"calibrationParams\":{"
    "\"tempThreshold\":%.1f,"
    "\"humThreshold\":%.1f,"
    "\"tempOffset\":%.1f,"
    "\"humOffset\":%.1f"
    "}}",
    temperature,
    humidity,
    heaterState ? "true" : "false",
    sensorConnected ? "true" : "false",
    (!manualMode && !heaterOffMode) ? "true" : "false",
    heaterOffMode ? "true" : "false",
    currentWiFiMode,
    currentSSID,
    ipStr,
    staSSID.c_str(),
    safeStaPassword.c_str(),
    safeApPassword.c_str(),
    showConnectionInfo ? "false" : "true",
    calParams.tempThreshold,
    calParams.humThreshold,
    calParams.tempOffset,
    calParams.humOffset
  );
  
  if (len > 0 && len < sizeof(jsonBuffer)) {
    // Определяем broadcast адрес в зависимости от режима WiFi
    IPAddress broadcastIP;
    if (wifiConnected && !forceAPMode) {
      // В режиме STA используем broadcast адрес текущей сети
      IPAddress localIP = WiFi.localIP();
      IPAddress subnet = WiFi.subnetMask();
      broadcastIP = IPAddress(
        (localIP[0] & subnet[0]) | (~subnet[0] & 0xFF),
        (localIP[1] & subnet[1]) | (~subnet[1] & 0xFF),
        (localIP[2] & subnet[2]) | (~subnet[2] & 0xFF),
        (localIP[3] & subnet[3]) | (~subnet[3] & 0xFF)
      );
    } else {
      // В режиме AP используем broadcast адрес AP сети (192.168.4.255)
      broadcastIP = IPAddress(192, 168, 4, 255);
    }
    
    // Отправляем UDP пакет
    udp.beginPacket(broadcastIP, UDP_PORT);
    udp.write((uint8_t*)jsonBuffer, len);
    udp.endPacket();
  }
}

// Обработчик запуска сканирования
void handleStartScan() {
  Serial.println("📡 Запрос запуска сканирования сетей");
  startScanWiFiNetworks();
  server.send(200, "application/json", "{\"success\":true}");
}

// Обработчик статуса сканирования
void handleScanStatus() {
  String status = getScanStatus();
  status.reserve(2048); // Предварительное выделение памяти
  server.send(200, "application/json", status);
  status = ""; // Освобождение памяти после отправки
}

// Обработчик сохранения настройки скрытия информации
void handleSaveHideInfo() {
  if (server.method() == HTTP_POST) {
    String hideValue = server.arg("hideConnectionInfo");
    showConnectionInfo = (hideValue != "1");
    saveDisplaySettings();
    server.send(200, "text/plain", "OK");
    Serial.println("📱 Настройка скрытия информации сохранена: " + String(showConnectionInfo ? "Показывать" : "Скрывать"));
  }
}

// Обработчик калибровки датчика
void handleSensorCalibrate() {
  Serial.println("📊 Запрос калибровки датчика");
  
  if (server.method() == HTTP_POST) {
    float tempOffset = server.arg("tempOffset").toFloat();
    float humOffset = server.arg("humOffset").toFloat();
    
    Serial.print("Новые коррекции датчика: tempOffset=");
    Serial.print(tempOffset);
    Serial.print("°C, humOffset=");
    Serial.print(humOffset);
    Serial.println("%");
    
    // Проверка допустимых значений
    if (abs(tempOffset) > 10.0) {
      server.send(400, "text/plain; charset=utf-8", "Ошибка: коррекция температуры должна быть в пределах ±10°C");
      return;
    }
    
    if (abs(humOffset) > 20.0) {
      server.send(400, "text/plain; charset=utf-8", "Ошибка: коррекция влажности должна быть в пределах ±20%");
      return;
    }
    
    // Сохраняем коррекции
    calParams.tempOffset = tempOffset;
    calParams.humOffset = humOffset;
    saveSettings();
    
    // Немедленно применяем коррекцию
    readSensorData();
    
    server.send(200, "text/plain", "OK");
    Serial.println("✅ Коррекция датчика применена и сохранена");
  }
}

// Обработчик сброса WiFi подключения
void handleResetWiFi() {
  Serial.println("🔄 Запрос сброса настроек WiFi до заводских");
  
  resetWiFiSettings();
  
  String page = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="ru">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>WiFi сброшен</title>
      <style>
          body { 
              font-family: Arial, sans-serif; 
              background: #1a1a1a; 
              color: white; 
              text-align: center; 
              padding: 20px;
              display: flex;
              justify-content: center;
              align-items: center;
              min-height: 100vh;
              margin: 0;
          }
          .container { 
              max-width: 300px; 
              background: #2c3e50; 
              padding: 25px; 
              border-radius: 12px; 
              box-shadow: 0 4px 20px rgba(0,0,0,0.4);
              border: 1px solid #34495e;
          }
          .success { 
              color: #2ecc71; 
              font-size: 20px; 
              margin-bottom: 15px;
          }
      </style>
  </head>
  <body>
      <div class="container">
          <div class="success">✅ WiFi подключение сброшено!</div>
          <p>Устройство переключилось на точку доступа.</p>
          <p>Через 3 секунды вы будете перенаправлены...</p>
      </div>
      <script>
          setTimeout(function() {
              window.location.href = 'http://192.168.4.1';
          }, 3000);
      </script>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html; charset=utf-8", page);
}

// Обработчик управления
void handleControl() {
  Serial.println("🎛️ Запрос управления");
  String action = server.arg("action");
  Serial.print("Действие: ");
  Serial.println(action);
  
  if (action == "toggle") {
    // Если режим выключения активен, не позволяем переключать обогрев
    if (heaterOffMode) {
      Serial.println("🔧 Веб-управление: Попытка переключения обогрева при выключенном состоянии - заблокировано");
      char jsonBuffer[256];
      snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{\"success\":false,\"error\":\"Режимы выключены. Сначала включите режимы.\",\"heaterState\":%s,\"autoMode\":%s,\"heaterOffMode\":%s}",
        heaterState ? "true" : "false",
        (!manualMode && !heaterOffMode) ? "true" : "false",
        heaterOffMode ? "true" : "false"
      );
      server.send(200, "application/json", jsonBuffer);
      return;
    }
    
    manualMode = true;
    manualModeStartTime = millis();
    manualModeStartRisk = isCameraAtRisk();
    
    heaterState = !heaterState;
    digitalWrite(HEATER_PIN, heaterState);
    cachedData.isValid = false; // Инвалидируем кэш при изменении состояния
    
    Serial.print("🔧 Веб-управление: Ручной режим - Обогрев ");
    Serial.println(heaterState ? "ВКЛ" : "ВЫКЛ");
  } 
  else if (action == "auto") {
    // Переключаем режим: автоматический <-> ручной
    // ВАЖНО: При переключении режима состояние обогрева НЕ меняется!
    // Если режим выключения активен, не позволяем переключать режимы
    if (heaterOffMode) {
      Serial.println("🔧 Веб-управление: Попытка переключения режима при выключенном состоянии - заблокировано");
      char jsonBuffer[256];
      snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{\"success\":false,\"error\":\"Режимы выключены. Сначала включите режимы.\",\"heaterState\":%s,\"autoMode\":%s,\"heaterOffMode\":%s}",
        heaterState ? "true" : "false",
        (!manualMode && !heaterOffMode) ? "true" : "false",
        heaterOffMode ? "true" : "false"
      );
      server.send(200, "application/json", jsonBuffer);
      return;
    } else if (manualMode) {
      // Переключаем с ручного на автоматический
      manualMode = false;
      manualModeStartTime = 0; // Сбрасываем таймер при выходе из ручного режима
      cachedData.isValid = false; // Инвалидируем кэш при изменении режима
      Serial.println("🔧 Веб-управление: Переключение на автоматический режим");
      Serial.print("   Обогрев остается: ");
      Serial.println(heaterState ? "ВКЛ" : "ВЫКЛ");
    } else {
      // Переключаем с автоматического на ручной
      // ВАЖНО: heaterState НЕ меняется - обогрев остается в текущем состоянии
      manualMode = true;
      manualModeStartTime = millis();
      manualModeStartRisk = isCameraAtRisk();
      cachedData.isValid = false; // Инвалидируем кэш при изменении режима
      Serial.println("🔧 Веб-управление: Переключение на ручной режим (5 минут)");
      Serial.print("   Время начала ручного режима: ");
      Serial.print(manualModeStartTime);
      Serial.print(" мс (");
      Serial.print(manualModeStartTime / 1000);
      Serial.println(" секунд с запуска)");
      Serial.print("   Обогрев остается: ");
      Serial.println(heaterState ? "ВКЛ" : "ВЫКЛ");
      Serial.println("   Пользователь может управлять обогревом вручную");
      Serial.println("   Автоматический возврат через 5 минут (300 секунд)");
    }
  }
  else if (action == "off") {
    // Переключаем режим выключения: если включен - выключаем, если выключен - включаем
    if (heaterOffMode) {
      // Выходим из режима выключения в автоматический
      heaterOffMode = false;
      manualMode = false;
      cachedData.isValid = false;
      Serial.println("🔧 Веб-управление: Выход из режима выключения в автоматический");
    } else {
      // Включаем режим полного выключения обогрева (для мониторинга датчиков)
      heaterOffMode = true;
      manualMode = false;
      heaterState = false;
      digitalWrite(HEATER_PIN, heaterState);
      cachedData.isValid = false;
      Serial.println("🔕 Веб-управление: Режим полного выключения обогрева");
      Serial.println("   Обогрев выключен, датчики продолжают работать");
    }
  } else {
    Serial.println("❌ Неизвестное действие: " + action);
  }
  
  // Оптимизировано: используем char[] буфер вместо String
  char jsonBuffer[192];
  snprintf(jsonBuffer, sizeof(jsonBuffer),
    "{\"success\":true,\"heaterState\":%s,\"autoMode\":%s,\"heaterOffMode\":%s}",
    heaterState ? "true" : "false",
    (!manualMode && !heaterOffMode) ? "true" : "false",
    heaterOffMode ? "true" : "false"
  );
  server.send(200, "application/json", jsonBuffer);
}

// Обработчик обновления WiFi
void handleUpdateWifi() {
  Serial.println("📶 Запрос обновления WiFi");
  
  if (server.method() == HTTP_POST) {
    String mode = server.arg("mode");
    String newStaSSID = server.arg("staSSID");
    String newStaPassword = server.arg("staPassword");
    String newApSSID = server.arg("apSSID");
    String newApPassword = server.arg("apPassword");
    String hideValue = server.arg("hideConnectionInfo");
    
    Serial.println("Получены настройки WiFi:");
    Serial.println("Mode: " + mode);
    Serial.println("STA SSID: " + newStaSSID);
    Serial.println("AP SSID: " + newApSSID);
    Serial.println("Hide connection info: " + hideValue);
    
    // Сохраняем настройку скрытия информации
    if (hideValue.length() > 0) {
      showConnectionInfo = (hideValue != "1");
      saveDisplaySettings();
    }
    
    // Сохраняем настройки точки доступа
    if (newApSSID.length() > 0 && newApPassword.length() > 0) {
      apSSID = newApSSID;
      apPassword = newApPassword;
      
      if (!preferences.begin("wifi-config", false)) {
        server.send(500, "text/plain", "Ошибка сохранения");
        return;
      }
      
      // Проверяем и обрезаем строки перед сохранением
      String safeApSSID = apSSID;
      String safeApPassword = apPassword;
      if (safeApSSID.length() > 32) safeApSSID = safeApSSID.substring(0, 32);
      if (safeApPassword.length() > 63) safeApPassword = safeApPassword.substring(0, 63);
      
      size_t written1 = preferences.putString("apSSID", safeApSSID);
      size_t written2 = preferences.putString("apPassword", safeApPassword);
      preferences.end();
      
      if (written1 == 0 || written2 == 0) {
        Serial.println("⚠️ Ошибка записи настроек AP в Preferences");
      }
    }
    
    bool result = false;
    const char* responseMsg; // Оптимизировано: используем const char* вместо String
    
    if (mode == "sta") {
      // Пытаемся подключиться к новой сети
      if (connectToWiFiNetwork(newStaSSID, newStaPassword)) {
        result = true;
        responseMsg = "connected";
      } else {
        // Определяем причину ошибки
        wl_status_t status = WiFi.status();
        if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED) {
          responseMsg = "wrong_password";
        } else if (status == WL_DISCONNECTED) {
          responseMsg = "not_found";
        } else {
          responseMsg = "auth_failed";
        }
      }
    } else {
      // Режим точки доступа
      // Устанавливаем флаги (без мьютекса - ESP32-C3 одноядерный)
      forceAPMode = true;
      wifiSTAEnabled = false;
      wifiConnected = false;
      
      if (!preferences.begin("wifi-config", false)) {
        server.send(500, "text/plain", "Ошибка сохранения");
        return;
      }
      
      size_t written = preferences.putBool("staEnabled", false);
      preferences.end();
      
      if (written == 0) {
        Serial.println("⚠️ Ошибка записи staEnabled в Preferences");
      }
      
      // Перезапускаем точку доступа
      restartWiFi();
      
      result = true;
      responseMsg = "ap_mode";
    }
    
    // Сохраняем режим перед очисткой
    bool shouldRestart = (result && mode == "sta");
    
    // Отправляем простой ответ и ЗАКРЫВАЕМ соединение
    server.send(200, "text/plain", responseMsg);
    
    // Очистка памяти после обработки запроса
    mode = "";
    newStaSSID = "";
    newStaPassword = "";
    newApSSID = "";
    newApPassword = "";
    hideValue = "";
    // responseMsg - const char*, не требует очистки
    
    // Оптимизировано: убрали delay(100) - server.send() уже отправил ответ
    // Если подключение успешно - перезагружаем ESP
    if (shouldRestart) {
      Serial.println("✅ Подключение успешно, перезагружаем ESP...");
      // Минимальная задержка для гарантии отправки ответа клиенту
      delay(500);  // Оптимизировано: уменьшено с 1000мс до 500мс
      ESP.restart();
    }
  }
}

// Обработчик калибровки порогов срабатывания
void handleCalibrate() {
  Serial.println("⚙️ Запрос калибровки порогов");
  
  if (server.method() == HTTP_POST) {
    calParams.tempThreshold = server.arg("tempThreshold").toFloat();
    calParams.humThreshold = server.arg("humThreshold").toFloat();
    
    Serial.print("Новые пороги срабатывания: temp=");
    Serial.print(calParams.tempThreshold);
    Serial.print("°C, hum=");
    Serial.println(calParams.humThreshold);
    
    if (calParams.tempThreshold >= 15.0) {
      server.send(400, "text/plain; charset=utf-8", "Ошибка: температура включения должна быть ниже 15°C");
      return;
    }
    
    saveSettings();
    
    String page = R"rawliteral(
    <!DOCTYPE HTML>
<html lang="ru">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Пороги сохранены</title>
        <style>
            body { 
                font-family: Arial, sans-serif; 
                background: #1a1a1a; 
                color: white; 
                text-align: center; 
                padding: 20px;
                display: flex;
                justify-content: center;
                align-items: center;
                min-height: 100vh;
                margin: 0;
            }
            .container { 
                max-width: 300px; 
                background: #2c3e50; 
                padding: 25px; 
                border-radius: 12px; 
                box-shadow: 0 4px 20px rgba(0,0,0,0.4);
                border: 1px solid #34495e;
            }
            .success { 
                color: #2ecc71; 
                font-size: 20px; 
                margin-bottom: 15px;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <div class="success">✅ Пороги сохранены!</div>
            <p>Устройство перезагрузится через 3 секунды...</p>
        </div>
        <script>
            setTimeout(function() {
                window.location.href = 'http://192.168.4.1';
            }, 3000);
        </script>
    </body>
    </html>
    )rawliteral";
    
    server.send(200, "text/html; charset=utf-8", page);
    
    delay(3000);
    ESP.restart();
  }
}

// Обработчик сброса калибровки
void handleResetCalibration() {
  Serial.println("🔄 Запрос сброса всей калибровки");
  
  resetCalibrationToFactory();
  
  String page = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="ru">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Калибровка сброшена</title>
      <style>
          body { 
              font-family: Arial, sans-serif; 
              background: #1a1a1a; 
              color: white; 
              text-align: center; 
              padding: 20px;
              display: flex;
              justify-content: center;
              align-items: center;
              min-height: 100vh;
              margin: 0;
          }
          .container { 
              max-width: 300px; 
              background: #2c3e50; 
              padding: 25px; 
              border-radius: 12px; 
              box-shadow: 0 4px 20px rgba(0,0,0,0.4);
              border: 1px solid #34495e;
          }
          .success { 
              color: #2ecc71; 
              font-size: 20px; 
              margin-bottom: 15px;
          }
      </style>
  </head>
  <body>
      <div class="container">
          <div class="success">✅ Вся калибровка сброшена!</div>
          <p>Устройство перезагрузится через 3 секунды...</p>
      </div>
      <script>
          setTimeout(function() {
              window.location.href = 'http://192.168.4.1';
          }, 3000);
      </script>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html; charset=utf-8", page);
  
  // Минимальная задержка только для гарантии отправки ответа клиенту
  // ESP.restart() выполнится после отправки ответа
  delay(100);
  ESP.restart();
}

// Обработчик перезапуска WiFi
void handleRestartWiFi() {
  Serial.println("🔄 Запрос перезапуска WiFi");
  
  // Отправляем ответ клиенту перед выполнением операции
  server.send(200, "text/plain; charset=utf-8", "WiFi перезапускается");
  // Убрали delay() - ответ уже отправлен, операция выполняется без блокировки
  restartWiFi();
}

// Настройка
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n🚀 Запуск системы обогрева камеры ассистентов");
  Serial.println("Версия: 7.11 - Оптимизация производительности и скорости отклика");
  Serial.println("Кнопка TTP-223: A=1, B=0 (при нажатии = LOW)");
  Serial.println("===================================================");
  
  // КРИТИЧНО для ESP32-C3: Даем время FreeRTOS полностью инициализироваться
  delay(1000);
  
  // Вывод информации о памяти при запуске
  Serial.print("💾 Свободная память при запуске: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" байт / ");
  Serial.print(ESP.getHeapSize());
  Serial.println(" байт");
  
  // КРИТИЧНО: Инициализация NVS (Non-Volatile Storage) для Preferences
  // Это должно быть сделано перед первым использованием Preferences
  if (!preferences.begin("init", false)) {
    Serial.println("⚠️ Предупреждение: не удалось инициализировать NVS, но продолжаем работу");
  } else {
    preferences.end(); // Закрываем тестовое открытие
    Serial.println("✅ NVS инициализирован");
  }
  
  // Мьютекс убран - на ESP32-C3 не требуется (одно ядро, последовательное выполнение)
  
  // Оптимизация String объектов - предварительное выделение памяти
  apSSID.reserve(32);
  apPassword.reserve(32);
  staSSID.reserve(32);
  staPassword.reserve(32);
  
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(HEATER_PIN, LOW);
  
  // Тест кнопки при запуске
  Serial.print("Состояние кнопки при запуске: ");
  Serial.println(digitalRead(BUTTON_PIN) == HIGH ? "Не нажата (HIGH)" : "Нажата (LOW)");
  
  // КРИТИЧНО: Добавляем задержку для полной инициализации FreeRTOS на ESP32-C3
  delay(1000);
  
  loadSettings();
  delay(500); // Задержка после Preferences
  
  initSensor();
  delay(500); // Задержка после I2C
  
  startWiFi();
  delay(2000); // Увеличена задержка после WiFi для полной инициализации
  
  // Инициализация UDP для широковещательной рассылки данных
  udp.begin(UDP_PORT);
  Serial.println("✅ UDP сервер инициализирован на порту " + String(UDP_PORT));
  
  // Регистрация обработчиков для captive portal (переадресация на портал авторизации)
  delay(500);
  server.on("/", HTTP_GET, handleRoot);
  
  // Обработчики для различных устройств и систем, которые проверяют наличие интернета
  // Эти URL используются для автоматического открытия портала авторизации (captive portal)
  server.on("/generate_204", HTTP_GET, handleRoot);  // Android - проверка подключения
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);  // iOS - обнаружение точки доступа
  server.on("/connecttest.txt", HTTP_GET, handleRoot);  // Windows - тест подключения
  server.on("/fwlink", HTTP_GET, handleRoot);  // Windows - перенаправление
  server.on("/success.txt", HTTP_GET, []() {
    server.send(200, "text/plain", "success");
  });  // Некоторые устройства
  server.on("/ncsi.txt", HTTP_GET, []() {
    server.send(200, "text/plain", "Microsoft NCSI");
  });  // Windows Network Connectivity Status Indicator
  server.on("/redirect", HTTP_GET, handleRoot);  // Общий редирект
  server.on("/library/test/success.html", HTTP_GET, handleRoot);  // iOS альтернативный
  server.on("/kindle-wifi/wifiredirect.html", HTTP_GET, handleRoot);  // Amazon Kindle
  
  server.on("/data", HTTP_GET, handleData);
  server.on("/startscan", HTTP_GET, handleStartScan);
  server.on("/scanstatus", HTTP_GET, handleScanStatus);
  server.on("/savehideinfo", HTTP_POST, handleSaveHideInfo);
  server.on("/sensorcalibrate", HTTP_POST, handleSensorCalibrate);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/updatewifi", HTTP_POST, handleUpdateWifi);
  server.on("/restartwifi", HTTP_GET, handleRestartWiFi);
  server.on("/resetwifi", HTTP_GET, handleResetWiFi);
  server.on("/calibrate", HTTP_POST, handleCalibrate);
  server.on("/resetcalibration", HTTP_GET, handleResetCalibration);
  // Обработчик фонового изображения - заглушка (изображение слишком большое для ESP32)
  server.on("/background.png", HTTP_GET, []() {
    server.send(404, "text/plain", "Background image not available");
  });
  
  // Обработчик для всех необработанных запросов - переадресация на главную страницу
  // Это важно для работы captive portal (переадресации)
  server.onNotFound([]() {
    Serial.println("📡 Запрос на несуществующую страницу: " + server.uri() + " - переадресация на главную");
    handleRoot();
  });
  
  // Запуск веб-сервера с дополнительной задержкой для ESP32-C3
  delay(1500); // Увеличена задержка перед запуском сервера
  Serial.println("🔄 Запуск веб-сервера...");
  
  // КРИТИЧНО: Проверяем, что WiFi инициализирован перед запуском сервера
  if (WiFi.getMode() == WIFI_OFF) {
    Serial.println("⚠️ WiFi не инициализирован, запускаем точку доступа...");
    delay(500);
    WiFi.mode(WIFI_AP);
    delay(1000);
    startAP();
    delay(1000);
  }
  
  server.begin();
  delay(1500); // Увеличена задержка после запуска сервера для инициализации очередей FreeRTOS
  Serial.println("✅ Веб-сервер запущен");
  Serial.println("📡 Ожидание запросов...");
  
  printConnectionInfo();
}

// Основной цикл
void loop() {
  server.handleClient();
  dnsServer.processNextRequest(); // Обработка DNS-запросов для переадресации на портал
  handleButton();
  
  // Мониторинг памяти
  if (millis() - lastMemoryCheck > MEMORY_CHECK_INTERVAL) {
    lastMemoryCheck = millis();
    printMemoryInfo();
  }
  
  // Проверка WiFi соединения
  checkAndRestoreWiFi();
  
  // Периодическая проверка имени точки доступа
  if (millis() - lastAPNameCheck > AP_NAME_CHECK_INTERVAL) {
    lastAPNameCheck = millis();
    
    // Проверяем только если AP активен
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
      String currentAPSSID = WiFi.softAPSSID();
      
      // Если имя AP изменилось на стандартное ESP32 имя или не соответствует настройкам
      if (currentAPSSID.length() > 0 && currentAPSSID != apSSID && 
          (currentAPSSID.startsWith("ESP_") || currentAPSSID != "Camera_Heater_AP")) {
        Serial.print("⚠️ Обнаружено изменение имени AP: ");
        Serial.print(currentAPSSID);
        Serial.print(" -> ");
        Serial.println(apSSID);
        Serial.println("   🔄 Переустанавливаем правильное имя...");
        
        // Переустанавливаем правильное имя AP
        WiFi.softAPdisconnect(true);
        delay(500);
        WiFi.softAP(apSSID.c_str(), apPassword.c_str());
        delay(500);
        
        // Проверяем результат
        String newAPSSID = WiFi.softAPSSID();
        if (newAPSSID == apSSID) {
          Serial.println("   ✅ Имя AP успешно восстановлено");
        } else {
          Serial.print("   ⚠️ Имя AP все еще: ");
          Serial.println(newAPSSID);
        }
      }
    }
  }
  
  // Неблокирующее переподключение WiFi (оптимизировано: без delay())
  if (wifiSTAEnabled && !wifiConnected && !forceAPMode) {
    if (!wifiReconnectState.isReconnecting) {
      // Начинаем переподключение
      if (millis() - wifiReconnectTimer > WIFI_RECONNECT_INTERVAL) {
        Serial.println("🔄 Попытка переподключения к WiFi...");
        wifiReconnectState.isReconnecting = true;
        wifiReconnectState.reconnectStartTime = millis();
        wifiReconnectState.reconnectAttempts = 0;
        wifiReconnectState.lastCheckTime = millis();
        WiFi.mode(WIFI_STA);
        WiFi.begin(staSSID.c_str(), staPassword.c_str());
      }
    } else {
      // Продолжаем переподключение (неблокирующе)
      unsigned long currentTime = millis();
      
      // Проверяем статус каждые 500мс
      if (currentTime - wifiReconnectState.lastCheckTime >= wifiReconnectState.checkInterval) {
        wifiReconnectState.lastCheckTime = currentTime;
        wifiReconnectState.reconnectAttempts++;
        
        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true;
          wifiDisconnectCount = 0;
          wifiReconnectState.isReconnecting = false;
          Serial.println("\n✅ WiFi соединение восстановлено");
          WiFi.mode(WIFI_AP_STA);
          delay(500);
          startAP(); // ВАЖНО: Перезапускаем AP с правильным именем
          wifiReconnectTimer = millis();
        } else if (wifiReconnectState.reconnectAttempts >= wifiReconnectState.maxAttempts || 
                   (currentTime - wifiReconnectState.reconnectStartTime) > wifiReconnectState.timeout) {
          wifiConnected = false;
          wifiReconnectState.isReconnecting = false;
          Serial.println("\n❌ Не удалось восстановить WiFi соединение");
          wifiReconnectTimer = millis();
        } else {
          Serial.print(".");
        }
      }
    }
  } else {
    // Сбрасываем состояние переподключения, если режим изменился
    if (wifiReconnectState.isReconnecting) {
      wifiReconnectState.isReconnecting = false;
    }
  }
  
  // Оптимизировано: читаем датчик каждые 800мс вместо 1000мс для лучшей отзывчивости
  if (millis() - lastSensorRead > 800) {
    if (!readSensorData()) {
      // Выводим ошибку только раз в 5 секунд, чтобы не засорять Serial
      static unsigned long lastErrorPrint = 0;
      if (millis() - lastErrorPrint > 5000) {
        Serial.println("⚠️ Ошибка чтения датчика");
        lastErrorPrint = millis();
      }
    }
    
    controlHeaterLogic();
    
    // Оптимизировано: выводим информацию только раз в 5 секунд для снижения нагрузки на Serial
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 5000) {
      Serial.print("🌡️ Температура: ");
      Serial.print(temperature, 1);
      Serial.print("°C (коррекция: ");
      Serial.print(calParams.tempOffset >= 0 ? "+" : "");
      Serial.print(calParams.tempOffset, 1);
      Serial.print("°C), 💧 Влажность: ");
      Serial.print(humidity, 1);
      Serial.print("% (коррекция: ");
      Serial.print(calParams.humOffset >= 0 ? "+" : "");
      Serial.print(calParams.humOffset, 1);
      Serial.print("%), 🔧 Обогрев: ");
      Serial.print(heaterState ? "ВКЛ" : "ВЫКЛ");
      Serial.print(", Режим: ");
      Serial.print(manualMode ? "Ручной" : "Авто");
      Serial.print(", WiFi: ");
      if (forceAPMode) {
        Serial.println("AP (принудительно)");
      } else if (wifiConnected) {
        Serial.println("STA (" + staSSID + ")");
      } else {
        Serial.println("AP");
      }
      lastStatusPrint = millis();
    }
    
    // Инвалидируем кэш при изменении данных
    cachedData.isValid = false;
    
    lastSensorRead = millis();
  }
  
  // Отправка данных по UDP каждые UDP_BROADCAST_INTERVAL миллисекунд
  if (millis() - lastUdpBroadcast >= UDP_BROADCAST_INTERVAL) {
    sendUdpBroadcast();
    lastUdpBroadcast = millis();
  }
}