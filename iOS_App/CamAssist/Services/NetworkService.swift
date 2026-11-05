import Foundation
import Combine

class NetworkService {
    static let shared = NetworkService()
    
    private let baseURL: String
    private let session: URLSession
    private let decoder: JSONDecoder
    
    private var _baseURL: String
    
    init(baseURL: String = "http://192.168.4.1") {
        self._baseURL = baseURL
        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 5.0
        config.timeoutIntervalForResource = 10.0
        self.session = URLSession(configuration: config)
        
        self.decoder = JSONDecoder()
        decoder.keyDecodingStrategy = .convertFromSnakeCase
    }
    
    var baseURL: String {
        get { _baseURL }
        set { _baseURL = newValue }
    }
    
    func updateBaseURL(_ newURL: String) {
        _baseURL = newURL
    }
    
    // MARK: - Device Data
    func getDeviceData() async throws -> DeviceData {
        guard let url = URL(string: "\(baseURL)/data") else {
            throw NetworkError.invalidURL
        }
        
        let (data, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
        
        return try decoder.decode(DeviceData.self, from: data)
    }
    
    // MARK: - Control
    func controlDevice(action: String) async throws -> ControlResponse {
        guard let url = URL(string: "\(baseURL)/control?action=\(action)") else {
            throw NetworkError.invalidURL
        }
        
        let (data, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
        
        return try decoder.decode(ControlResponse.self, from: data)
    }
    
    // MARK: - WiFi
    func resetWifi() async throws {
        guard let url = URL(string: "\(baseURL)/resetwifi") else {
            throw NetworkError.invalidURL
        }
        
        let (_, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
    }
    
    func restartWifi() async throws {
        guard let url = URL(string: "\(baseURL)/restartwifi") else {
            throw NetworkError.invalidURL
        }
        
        let (_, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
    }
    
    func updateWifi(
        mode: String,
        staSSID: String? = nil,
        staPassword: String? = nil,
        apSSID: String? = nil,
        apPassword: String? = nil,
        hideConnectionInfo: Bool? = nil,
        staIP: String? = nil
    ) async throws {
        guard let url = URL(string: "\(baseURL)/updatewifi") else {
            throw NetworkError.invalidURL
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
        if let ip = staIP { queryItems.append(URLQueryItem(name: "staIP", value: ip)) }
        
        components.queryItems = queryItems
        request.httpBody = components.query?.data(using: .utf8)
        
        let (_, response) = try await session.data(for: request)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
    }
    
    // MARK: - Calibration
    func calibrateSensor(tempOffset: Float, humOffset: Float) async throws {
        guard let url = URL(string: "\(baseURL)/sensorcalibrate") else {
            throw NetworkError.invalidURL
        }
        
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
        
        let body = "tempOffset=\(tempOffset)&humOffset=\(humOffset)"
        request.httpBody = body.data(using: .utf8)
        
        let (_, response) = try await session.data(for: request)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
    }
    
    func calibrateThresholds(tempThreshold: Float, humThreshold: Float) async throws {
        guard let url = URL(string: "\(baseURL)/calibrate") else {
            throw NetworkError.invalidURL
        }
        
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
        
        let body = "tempThreshold=\(tempThreshold)&humThreshold=\(humThreshold)"
        request.httpBody = body.data(using: .utf8)
        
        let (_, response) = try await session.data(for: request)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
    }
    
    // MARK: - Scan
    func startScan() async throws {
        guard let url = URL(string: "\(baseURL)/startscan") else {
            throw NetworkError.invalidURL
        }
        
        let (_, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
    }
    
    func getScanStatus() async throws -> ScanStatus {
        guard let url = URL(string: "\(baseURL)/scanstatus") else {
            throw NetworkError.invalidURL
        }
        
        let (data, response) = try await session.data(from: url)
        
        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw NetworkError.invalidResponse
        }
        
        return try decoder.decode(ScanStatus.self, from: data)
    }
}

// MARK: - NetworkError
enum NetworkError: Error {
    case invalidURL
    case invalidResponse
    case decodingError
    case networkError(Error)
}
