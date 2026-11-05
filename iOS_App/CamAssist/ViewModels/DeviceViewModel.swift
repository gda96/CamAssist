import Foundation
import Combine
import SwiftUI

@MainActor
class DeviceViewModel: ObservableObject {
    @Published var deviceData: DeviceData?
    @Published var isLoading = false
    @Published var isConnected = false
    @Published var error: String?
    @Published var scanStatus: ScanStatus?
    @Published var isScanning = false
    
    private let networkService = NetworkService.shared
    private let udpService = UdpService.shared
    private var cancellables = Set<AnyCancellable>()
    private var updateTask: Task<Void, Never>?
    
    init() {
        setupUdpListener()
        startPeriodicUpdate()
    }
    
    private func setupUdpListener() {
        udpService.$receivedData
            .compactMap { $0 }
            .sink { [weak self] data in
                self?.deviceData = data
                self?.isConnected = true
                self?.error = nil
                
                    // Обновляем base URL если устройство в STA режиме
                    if data.wifiMode == "sta" && !data.staIP.isEmpty && data.staIP != "0.0.0.0" {
                        networkService.updateBaseURL("http://\(data.staIP)")
                    }
            }
            .store(in: &cancellables)
    }
    
    private func startPeriodicUpdate() {
        updateTask?.cancel()
        updateTask = Task {
            while !Task.isCancelled {
                await loadDeviceData()
                try? await Task.sleep(nanoseconds: 2_000_000_000) // 2 секунды
            }
        }
    }
    
    func loadDeviceData() async {
        isLoading = true
        error = nil
        
        do {
            let data = try await networkService.getDeviceData()
            self.deviceData = data
            self.isConnected = true
            
            // Обновляем base URL если устройство в STA режиме
            if data.wifiMode == "sta" && !data.staIP.isEmpty && data.staIP != "0.0.0.0" {
                networkService.updateBaseURL("http://\(data.staIP)")
            }
        } catch {
            self.error = error.localizedDescription
            self.isConnected = false
        }
        
        isLoading = false
    }
    
    func toggleHeater() async {
        guard deviceData?.heaterOffMode != true else {
            error = "Сначала включите режимы, нажав кнопку \"Включить\""
            return
        }
        
        do {
            let response = try await networkService.controlDevice(action: "toggle")
            if let errorMessage = response.error {
                error = errorMessage
            } else {
                await loadDeviceData()
            }
        } catch {
            error = error.localizedDescription
        }
    }
    
    func setAutoMode() async {
        guard deviceData?.heaterOffMode != true else {
            error = "Сначала включите режимы, нажав кнопку \"Включить\""
            return
        }
        
        do {
            let response = try await networkService.controlDevice(action: "auto")
            if let errorMessage = response.error {
                error = errorMessage
            } else {
                await loadDeviceData()
            }
        } catch {
            error = error.localizedDescription
        }
    }
    
    func setOffMode() async {
        do {
            let response = try await networkService.controlDevice(action: "off")
            if let errorMessage = response.error {
                error = errorMessage
            } else {
                await loadDeviceData()
            }
        } catch {
            error = error.localizedDescription
        }
    }
    
    func resetWifi() async {
        do {
            try await networkService.resetWifi()
            error = "Настройки WiFi сброшены. Устройство перезагружается..."
            try? await Task.sleep(nanoseconds: 5_000_000_000) // 5 секунд
            await loadDeviceData()
        } catch {
            error = error.localizedDescription
        }
    }
    
    func restartWifi() async {
        do {
            try await networkService.restartWifi()
            error = "WiFi модуль перезагружается..."
            try? await Task.sleep(nanoseconds: 3_000_000_000) // 3 секунды
            await loadDeviceData()
        } catch {
            error = error.localizedDescription
        }
    }
    
    func calibrateSensor(tempOffset: Float, humOffset: Float) async {
        do {
            try await networkService.calibrateSensor(tempOffset: tempOffset, humOffset: humOffset)
            await loadDeviceData()
        } catch {
            error = error.localizedDescription
        }
    }
    
    func calibrateThresholds(tempThreshold: Float, humThreshold: Float) async {
        do {
            try await networkService.calibrateThresholds(tempThreshold: tempThreshold, humThreshold: humThreshold)
            await loadDeviceData()
        } catch {
            error = error.localizedDescription
        }
    }
    
    func startScan() async {
        isScanning = true
        do {
            try await networkService.startScan()
            await checkScanStatus()
        } catch {
            error = error.localizedDescription
            isScanning = false
        }
    }
    
    private func checkScanStatus() async {
        while isScanning {
            do {
                let status = try await networkService.getScanStatus()
                scanStatus = status
                
                if status.status == "completed" || status.status == "failed" {
                    isScanning = false
                } else {
                    try? await Task.sleep(nanoseconds: 1_000_000_000) // 1 секунда
                }
            } catch {
                try? await Task.sleep(nanoseconds: 1_000_000_000)
            }
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
    ) async {
        do {
            try await networkService.updateWifi(
                mode: mode,
                staSSID: staSSID,
                staPassword: staPassword,
                apSSID: apSSID,
                apPassword: apPassword,
                hideConnectionInfo: hideConnectionInfo,
                staIP: staIP
            )
            await loadDeviceData()
        } catch {
            error = error.localizedDescription
        }
    }
    
    deinit {
        updateTask?.cancel()
    }
}
