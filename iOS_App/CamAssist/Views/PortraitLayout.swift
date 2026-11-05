//
//  PortraitLayout.swift
//  CamAssist
//
//  Вертикальная раскладка для портретной ориентации
//

import SwiftUI

struct PortraitLayout: View {
    @ObservedObject var viewModel: DeviceViewModel
    @Binding var showCalibration: Bool
    @Binding var showWiFiSettings: Bool
    
    var body: some View {
        ScrollView {
            VStack(spacing: 12) {
                Spacer()
                    .frame(height: 8)
                
                // Заголовок
                Text("Обогрев камеры ассистентов")
                    .font(.system(size: 24, weight: .bold))
                    .foregroundColor(.white)
                    .padding(.bottom, 8)
                
                // Статус бар
                StatusBar(deviceData: viewModel.deviceData)
                    .padding(.horizontal, 16)
                
                Spacer()
                    .frame(height: 8)
                
                // Карточки данных
                HStack(spacing: 12) {
                    DataCard(
                        title: "Температура",
                        value: String(format: "%.1f", viewModel.deviceData?.temperature ?? 0.0),
                        unit: "°C",
                        icon: "🌡️"
                    )
                    
                    DataCard(
                        title: "Влажность",
                        value: String(format: "%.1f", viewModel.deviceData?.humidity ?? 0.0),
                        unit: "%",
                        icon: "💧"
                    )
                }
                .padding(.horizontal, 16)
                
                // Кнопки управления
                VStack(spacing: 12) {
                    ControlButton(
                        text: viewModel.deviceData?.heaterState == true ? "Выключить обогрев" : "Включить обогрев",
                        action: {
                            Task {
                                await viewModel.toggleHeater()
                            }
                        },
                        backgroundColor: viewModel.deviceData?.heaterState == true ? .red : .green,
                        enabled: viewModel.deviceData?.heaterOffMode != true
                    )
                    
                    ControlButton(
                        text: viewModel.deviceData?.autoMode == true ? "Автоматический режим" : "Ручной режим",
                        action: {
                            Task {
                                await viewModel.setAutoMode()
                            }
                        },
                        backgroundColor: viewModel.deviceData?.autoMode == true ? .blue : .orange,
                        enabled: viewModel.deviceData?.heaterOffMode != true
                    )
                    
                    ControlButton(
                        text: viewModel.deviceData?.heaterOffMode == true ? "Включить" : "Выключить",
                        action: {
                            Task {
                                await viewModel.setOffMode()
                            }
                        },
                        backgroundColor: .gray
                    )
                    
                    ControlButton(
                        text: "Калибровка",
                        action: { showCalibration = true },
                        backgroundColor: .blue
                    )
                    
                    ControlButton(
                        text: "Настройки WiFi",
                        action: { showWiFiSettings = true },
                        backgroundColor: .purple
                    )
                }
                .padding(.horizontal, 16)
                
                Spacer()
                    .frame(height: 12)
            }
        }
    }
}
