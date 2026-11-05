import SwiftUI

struct StatusBar: View {
    @ObservedObject var viewModel: DeviceViewModel
    
    var body: some View {
        HStack(spacing: 0) {
            // Обогрев
            CompactStatusItem(
                label: "Обогрев",
                value: viewModel.deviceData?.heaterState == true ? "Вкл" : "Выкл",
                valueColor: viewModel.deviceData?.heaterState == true ? .green : .red
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
                valueColor: viewModel.deviceData?.wifiMode == "sta" ? .blue : .purple
            )
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(Color.black.opacity(0.3))
        .cornerRadius(10)
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .stroke(Color.white.opacity(0.1), lineWidth: 1)
        )
        .padding(.horizontal)
    }
    
    private var modeText: String {
        if viewModel.deviceData?.heaterOffMode == true {
            return "Выкл"
        } else if viewModel.deviceData?.autoMode == true {
            return "Авто"
        } else {
            return "Ручной"
        }
    }
    
    private var modeColor: Color {
        if viewModel.deviceData?.heaterOffMode == true {
            return .gray
        } else if viewModel.deviceData?.autoMode == true {
            return .blue
        } else {
            return .orange
        }
    }
    
    private var wifiText: String {
        guard let data = viewModel.deviceData else { return "AP" }
        
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
}

struct CompactStatusItem: View {
    let label: String
    let value: String
    let valueColor: Color
    
    var body: some View {
        VStack(spacing: 2) {
            Text(label)
                .font(.system(size: 10))
                .foregroundColor(.white.opacity(0.7))
            Text(value)
                .font(.system(size: 12, weight: .semibold))
                .foregroundColor(valueColor)
        }
        .frame(minWidth: 60)
        .frame(maxWidth: .infinity)
    }
}
