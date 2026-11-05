import Foundation
import Network
import Combine

class UdpService: ObservableObject {
    static let shared = UdpService()
    
    private let port: UInt16 = 8888
    private var listener: NWListener?
    private var connections: [NWConnection] = []
    private var cancellables = Set<AnyCancellable>()
    
    @Published var receivedData: DeviceData?
    
    private init() {}
    
    func startListening() {
        let parameters = NWParameters.udp
        parameters.allowLocalEndpointReuse = true
        
        // Включаем multicast для приема broadcast пакетов
        if #available(iOS 14.0, *) {
            parameters.includePeerToPeer = true
        }
        
        do {
            guard let port = NWEndpoint.Port(rawValue: port) else {
                print("Invalid port: \(self.port)")
                return
            }
            
            listener = try NWListener(using: parameters, on: port)
            
            listener?.newConnectionHandler = { [weak self] connection in
                self?.setupConnection(connection)
            }
            
            listener?.stateUpdateHandler = { [weak self] state in
                switch state {
                case .ready:
                    print("UDP listener ready on port \(self?.port ?? 0)")
                case .failed(let error):
                    print("UDP listener failed: \(error)")
                case .waiting(let error):
                    print("UDP listener waiting: \(error)")
                default:
                    break
                }
            }
            
            listener?.start(queue: .global())
        } catch {
            print("Failed to start UDP listener: \(error)")
        }
    }
    
    private func setupConnection(_ connection: NWConnection) {
        connections.append(connection)
        
        connection.stateUpdateHandler = { [weak self] state in
            switch state {
            case .ready:
                self?.receiveData(on: connection)
            case .failed(let error):
                print("UDP connection failed: \(error)")
            default:
                break
            }
        }
        
        connection.start(queue: .global())
    }
    
    private func receiveData(on connection: NWConnection) {
        connection.receiveMessage { [weak self] data, context, isComplete, error in
            if let error = error {
                print("UDP receive error: \(error)")
                return
            }
            
            guard let data = data else { return }
            
            do {
                let decoder = JSONDecoder()
                decoder.keyDecodingStrategy = .convertFromSnakeCase
                let deviceData = try decoder.decode(DeviceData.self, from: data)
                
                DispatchQueue.main.async {
                    self?.receivedData = deviceData
                }
            } catch {
                print("Failed to decode UDP data: \(error)")
            }
            
            // Продолжаем слушать
            self?.receiveData(on: connection)
        }
    }
    
    func stop() {
        listener?.cancel()
        connections.forEach { $0.cancel() }
        connections.removeAll()
    }
}
