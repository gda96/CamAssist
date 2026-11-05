//
//  StatusBar.swift
//  CamAssist
//
//  Компактный статус бар
//

import SwiftUI

struct StatusBar: View {
    let deviceData: DeviceData?
    
    var body: some View {
        HStack(spacing: 0) {
            // Обогрев
            CompactStatusItem(
                label: "Обогрев",
                value: deviceData?.heaterState == true ? "Вкл" : "Выкл",
                valueColor: deviceData?.heaterState == true ? .green : .red
            )
            
            // Режим
            CompactStatusItem(
                label: "Режим",
                value: modeText,
                valueColor: modeColor
            )
            
            // WiFi
            CompactStatusItem(
                label: "WiFi",
                value: wifiText,
                valueColor: wifiColor
            )
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(Color.black.opacity(0.3))
                .overlay(
                    RoundedRectangle(cornerRadius: 10)
                        .stroke(Color.white.opacity(0.1), lineWidth: 1)
                )
        )
    }
    
    private var modeText: String {
        if deviceData?.heaterOffMode == true {
            return "Выкл"
        } else if deviceData?.autoMode == true {
            return "Авто"
        } else {
            return "Ручной"
        }
    }
    
    private var modeColor: Color {
        if deviceData?.heaterOffMode == true {
            return .gray
        } else if deviceData?.autoMode == true {
            return .blue
        } else {
            return .orange
        }
    }
    
    private var wifiText: String {
        guard let data = deviceData else { return "AP" }
        
        if data.wifiMode == "sta" && !data.staNetwork.isEmpty {
            // Показываем SSID (обрезаем если слишком длинный)
            let ssid = data.staNetwork
            return ssid.count > 12 ? String(ssid.prefix(12)) + "..." : ssid
        } else if data.wifiMode == "sta" {
            return "STA"
        } else {
            return "AP"
        }
    }
    
    private var wifiColor: Color {
        deviceData?.wifiMode == "sta" ? .green : .purple
    }
}

struct CompactStatusItem: View {
    let label: String
    let value: String
    let valueColor: Color
    
    var body: some View {
        VStack(spacing: 2) {
            Text(label)
                .font(.system(size: 10, weight: .medium))
                .foregroundColor(.white.opacity(0.7))
            
            Text(value)
                .font(.system(size: 12, weight: .semibold))
                .foregroundColor(valueColor)
        }
        .frame(minWidth: 60)
        .frame(maxWidth: .infinity)
    }
}
