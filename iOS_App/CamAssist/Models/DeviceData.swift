import Foundation

// MARK: - DeviceData
struct DeviceData: Codable {
    let temperature: Float
    let humidity: Float
    let heaterState: Bool
    let sensorConnected: Bool
    let autoMode: Bool
    let heaterOffMode: Bool?
    let wifiMode: String // "ap" or "sta"
    let currentSSID: String
    let staIP: String
    let staNetwork: String
    let staPassword: String?
    let apPassword: String?
    let hideConnectionInfo: Bool
    let calibrationParams: CalibrationParams
}

// MARK: - CalibrationParams
struct CalibrationParams: Codable {
    let tempThreshold: Float
    let humThreshold: Float
    let tempOffset: Float
    let humOffset: Float
}

// MARK: - ControlResponse
struct ControlResponse: Codable {
    let success: Bool
    let heaterState: Bool?
    let autoMode: Bool?
    let heaterOffMode: Bool?
    let error: String?
}

// MARK: - ScanNetwork
struct ScanNetwork: Codable {
    let ssid: String
    let rssi: Int
    let encryption: String
}

// MARK: - ScanStatus
struct ScanStatus: Codable {
    let status: String // "scanning", "completed", "failed"
    let networks: [ScanNetwork]?
    let success: Bool?
    let error: String?
}
