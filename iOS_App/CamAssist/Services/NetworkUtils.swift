//
//  NetworkUtils.swift
//  CamAssist
//
//  Утилиты для работы с сетью
//

import Foundation
import Network

class NetworkUtils {
    static let shared = NetworkUtils()
    
    private let apModeIP = "192.168.4.1"
    private let apModeSSID = "Camera_Heater_AP"
    
    /// Определяет базовый URL устройства ESP32
    func getDeviceBaseUrl(staIP: String? = nil) -> String {
        // Если передан IP из STA режима, используем его
        if let staIP = staIP, !staIP.isEmpty, staIP != "0.0.0.0" {
            return "http://\(staIP)/"
        }
        
        // Проверяем, подключены ли мы к AP сети устройства
        // В iOS это сложнее, поэтому используем AP режим по умолчанию
        return "http://\(apModeIP)/"
    }
    
    /// Проверяет доступность устройства
    func isDeviceReachable(url: String, completion: @escaping (Bool) -> Void) {
        guard let url = URL(string: url) else {
            completion(false)
            return
        }
        
        var request = URLRequest(url: url)
        request.timeoutInterval = 3.0
        request.httpMethod = "GET"
        
        URLSession.shared.dataTask(with: request) { _, response, _ in
            if let httpResponse = response as? HTTPURLResponse {
                completion(httpResponse.statusCode == 200)
            } else {
                completion(false)
            }
        }.resume()
    }
}
