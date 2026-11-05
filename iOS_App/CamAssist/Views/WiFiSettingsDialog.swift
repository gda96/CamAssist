import SwiftUI

struct WiFiSettingsDialog: View {
    @ObservedObject var viewModel: DeviceViewModel
    @Environment(\.dismiss) var dismiss
    
    @State private var mode: String = "ap"
    @State private var staSSID: String = ""
    @State private var staPassword: String = ""
    @State private var apSSID: String = "Camera_Heater_AP"
    @State private var apPassword: String = "12345678"
    @State private var hideConnectionInfo: Bool = false
    
    var body: some View {
        NavigationView {
            Form {
                Section("Режим WiFi") {
                    Picker("Режим", selection: $mode) {
                        Text("Точка доступа (AP)").tag("ap")
                        Text("Домашняя сеть (STA)").tag("sta")
                    }
                }
                
                if mode == "sta" {
                    Section("Настройки домашней сети") {
                        TextField("SSID", text: $staSSID)
                        SecureField("Пароль", text: $staPassword)
                    }
                }
                
                Section("Настройки точки доступа") {
                    TextField("SSID", text: $apSSID)
                    SecureField("Пароль", text: $apPassword)
                }
                
                Section("Отображение") {
                    Toggle("Скрыть информацию о подключении", isOn: $hideConnectionInfo)
                }
                
                Section {
                    Button("Перезагрузить", role: .destructive) {
                        Task {
                            await viewModel.restartWifi()
                        }
                    }
                    
                    Button("Сбросить", role: .destructive) {
                        Task {
                            await viewModel.resetWifi()
                        }
                    }
                }
            }
            .navigationTitle("Настройки WiFi")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Закрыть") {
                        dismiss()
                    }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Сохранить") {
                        Task {
                            await viewModel.updateWifi(
                                mode: mode,
                                staSSID: mode == "sta" ? staSSID : nil,
                                staPassword: mode == "sta" ? staPassword : nil,
                                apSSID: apSSID,
                                apPassword: apPassword,
                                hideConnectionInfo: hideConnectionInfo
                            )
                            dismiss()
                        }
                    }
                }
            }
        }
        .onAppear {
            if let data = viewModel.deviceData {
                mode = data.wifiMode
                staSSID = data.staNetwork
                staPassword = data.staPassword ?? ""
                apSSID = data.currentSSID
                apPassword = data.apPassword ?? ""
                hideConnectionInfo = data.hideConnectionInfo
            }
        }
    }
}
