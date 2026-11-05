//
//  DeviceService.swift
//  CamAssist
//
//  Сервис для работы с API устройства ESP32
//

import Foundation

class DeviceService {
    private var baseURL: String
    private let session: URLSession
    
    init(baseURL: String = "http://192.168.4.1/") {
        self.baseURL = baseURL
        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 5.0
        config.timeoutIntervalForResource = 5.0
        self.session = URLSession(configuration: config)
    }
    
    func updateBaseURL(_ newBaseURL: String) {
        self.baseURL = newBaseURL
    }
    
    // MARK: - Device Data
    
    func getDeviceData() async throws -> DeviceData {
        guard let url = URL(string: "\(baseURL)data") else {
            throw DeviceError.invalidURL
        }
        
        let (data, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
        
        let decoder = JSONDecoder()
        return try decoder.decode(DeviceData.self, from: data)
    }
    
    // MARK: - Control
    
    func controlDevice(action: String) async throws -> ControlResponse {
        guard let url = URL(string: "\(baseURL)control?action=\(action)") else {
            throw DeviceError.invalidURL
        }
        
        let (data, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
        
        let decoder = JSONDecoder()
        return try decoder.decode(ControlResponse.self, from: data)
    }
    
    func toggleHeater() async throws -> ControlResponse {
        return try await controlDevice(action: "toggle")
    }
    
    func setAutoMode() async throws -> ControlResponse {
        return try await controlDevice(action: "auto")
    }
    
    func setOffMode() async throws -> ControlResponse {
        return try await controlDevice(action: "off")
    }
    
    // MARK: - Calibration
    
    func calibrateSensor(tempOffset: Float, humOffset: Float) async throws {
        guard let url = URL(string: "\(baseURL)sensorcalibrate") else {
            throw DeviceError.invalidURL
        }
        
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
        
        let body = "tempOffset=\(tempOffset)&humOffset=\(humOffset)"
        request.httpBody = body.data(using: .utf8)
        
        let (_, response) = try await session.data(for: request)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
    }
    
    func calibrateThresholds(tempThreshold: Float, humThreshold: Float) async throws {
        guard let url = URL(string: "\(baseURL)calibrate") else {
            throw DeviceError.invalidURL
        }
        
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
        
        let body = "tempThreshold=\(tempThreshold)&humThreshold=\(humThreshold)"
        request.httpBody = body.data(using: .utf8)
        
        let (_, response) = try await session.data(for: request)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
    }
    
    // MARK: - WiFi
    
    func updateWiFi(
        mode: String,
        staSSID: String? = nil,
        staPassword: String? = nil,
        apSSID: String? = nil,
        apPassword: String? = nil,
        hideConnectionInfo: Bool? = nil,
        staIP: String? = nil
    ) async throws {
        guard let url = URL(string: "\(baseURL)updatewifi") else {
            throw DeviceError.invalidURL
        }
        
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
        
        var components = URLComponents()
        var queryItems: [URLQueryItem] = [URLQueryItem(name: "mode", value: mode)]
        
        if let staSSID = staSSID { queryItems.append(URLQueryItem(name: "staSSID", value: staSSID)) }
        if let staPassword = staPassword { queryItems.append(URLQueryItem(name: "staPassword", value: staPassword)) }
        if let apSSID = apSSID { queryItems.append(URLQueryItem(name: "apSSID", value: apSSID)) }
        if let apPassword = apPassword { queryItems.append(URLQueryItem(name: "apPassword", value: apPassword)) }
        if let hide = hideConnectionInfo { queryItems.append(URLQueryItem(name: "hideConnectionInfo", value: hide ? "1" : "0")) }
        if let staIP = staIP { queryItems.append(URLQueryItem(name: "staIP", value: staIP)) }
        
        components.queryItems = queryItems
        request.httpBody = components.query?.data(using: .utf8)
        
        let (_, response) = try await session.data(for: request)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
    }
    
    func resetWiFi() async throws {
        guard let url = URL(string: "\(baseURL)resetwifi") else {
            throw DeviceError.invalidURL
        }
        
        let (_, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
    }
    
    func restartWiFi() async throws {
        guard let url = URL(string: "\(baseURL)restartwifi") else {
            throw DeviceError.invalidURL
        }
        
        let (_, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
    }
    
    // MARK: - Scan
    
    func startScan() async throws {
        guard let url = URL(string: "\(baseURL)startscan") else {
            throw DeviceError.invalidURL
        }
        
        let (_, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
    }
    
    func getScanStatus() async throws -> ScanStatus {
        guard let url = URL(string: "\(baseURL)scanstatus") else {
            throw DeviceError.invalidURL
        }
        
        let (data, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw DeviceError.serverError
        }
        
        let decoder = JSONDecoder()
        return try decoder.decode(ScanStatus.self, from: data)
    }
}

enum DeviceError: Error {
    case invalidURL
    case serverError
    case decodingError
    case networkError
    
    var localizedDescription: String {
        switch self {
        case .invalidURL:
            return "Неверный URL"
        case .serverError:
            return "Ошибка сервера"
        case .decodingError:
            return "Ошибка декодирования данных"
        case .networkError:
            return "Ошибка сети"
        }
    }
}
