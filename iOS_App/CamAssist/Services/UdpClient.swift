//
//  UdpClient.swift
//  CamAssist
//
//  UDP клиент для получения данных от устройства ESP32
//

import Foundation
import Network

class UdpClient: ObservableObject {
    private let port: UInt16 = 8888
    private var listener: NWListener?
    private var isListening = false
    
    @Published var lastReceivedData: DeviceData?
    
    func startListening() {
        guard !isListening else { return }
        
        do {
            let params = NWParameters.udp
            params.allowLocalEndpointReuse = true
            params.allowFastOpen = true
            
            listener = try NWListener(using: params, on: NWEndpoint.Port(rawValue: port)!)
            
            listener?.newConnectionHandler = { [weak self] connection in
                self?.handleConnection(connection)
            }
            
            listener?.stateUpdateHandler = { state in
                switch state {
                case .ready:
                    print("UDP listener ready on port \(self.port)")
                case .failed(let error):
                    print("UDP listener failed: \(error)")
                default:
                    break
                }
            }
            
            listener?.start(queue: .main)
            isListening = true
        } catch {
            print("Failed to start UDP listener: \(error)")
        }
    }
    
    private func handleConnection(_ connection: NWConnection) {
        connection.start(queue: .main)
        
        connection.receive(minimumIncompleteLength: 1, maximumLength: 2048) { [weak self] data, _, isComplete, error in
            if let error = error {
                print("UDP receive error: \(error)")
                return
            }
            
            if let data = data, !data.isEmpty {
                self?.processData(data)
            }
            
            if !isComplete {
                self?.handleConnection(connection)
            }
        }
    }
    
    private func processData(_ data: Data) {
        do {
            let decoder = JSONDecoder()
            let deviceData = try decoder.decode(DeviceData.self, from: data)
            
            DispatchQueue.main.async {
                self.lastReceivedData = deviceData
            }
        } catch {
            print("Failed to decode UDP data: \(error)")
        }
    }
    
    func stop() {
        listener?.cancel()
        listener = nil
        isListening = false
    }
}
