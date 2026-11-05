import SwiftUI

struct DeviceView: View {
    @StateObject private var viewModel = DeviceViewModel()
    @State private var showCalibrationDialog = false
    @State private var showWiFiDialog = false
    
    var body: some View {
        GeometryReader { geometry in
            let isLandscape = geometry.size.width > geometry.size.height
            
            ZStack {
                // Фоновое изображение
                if let backgroundImage = UIImage(named: "background") {
                    Image(uiImage: backgroundImage)
                        .resizable()
                        .scaledToFill()
                        .frame(width: geometry.size.width, height: geometry.size.height)
                        .opacity(0.9)
                        .clipped()
                } else {
                    // Fallback градиент
                    LinearGradient(
                        colors: [
                            Color(red: 0.1, green: 0.1, blue: 0.18),
                            Color(red: 0.09, green: 0.13, blue: 0.24)
                        ],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                }
                
                // Затемнение для читаемости
                Color.black.opacity(0.3)
                
                if isLandscape {
                    LandscapeLayout(viewModel: viewModel, showCalibrationDialog: $showCalibrationDialog, showWiFiDialog: $showWiFiDialog)
                } else {
                    PortraitLayout(viewModel: viewModel, showCalibrationDialog: $showCalibrationDialog, showWiFiDialog: $showWiFiDialog)
                }
                
                // Индикатор загрузки
                if viewModel.isLoading {
                    ProgressView()
                        .scaleEffect(1.5)
                        .tint(.white)
                }
                
                // Ошибки
                if let error = viewModel.error {
                    VStack {
                        Spacer()
                        Text(error)
                            .foregroundColor(.white)
                            .padding()
                            .background(Color.red.opacity(0.8))
                            .cornerRadius(10)
                            .padding()
                    }
                }
            }
        }
        .onAppear {
            Task {
                await viewModel.loadDeviceData()
            }
            UdpService.shared.startListening()
        }
        .sheet(isPresented: $showCalibrationDialog) {
            CalibrationDialog(viewModel: viewModel)
        }
        .sheet(isPresented: $showWiFiDialog) {
            WiFiSettingsDialog(viewModel: viewModel)
        }
    }
}

// MARK: - Portrait Layout
struct PortraitLayout: View {
    @ObservedObject var viewModel: DeviceViewModel
    @Binding var showCalibrationDialog: Bool
    @Binding var showWiFiDialog: Bool
    
    var body: some View {
        ScrollView {
            VStack(spacing: 16) {
                Spacer().frame(height: 20)
                
                Text("Обогрев камеры ассистентов")
                    .font(.title)
                    .foregroundColor(.white)
                    .padding(.bottom, 8)
                
                StatusBar(viewModel: viewModel)
                
                // Карточки данных
                HStack(spacing: 12) {
                    DataCard(title: "Температура", value: viewModel.deviceData?.temperature ?? 0, unit: "°C", icon: "🌡️")
                    DataCard(title: "Влажность", value: viewModel.deviceData?.humidity ?? 0, unit: "%", icon: "💧")
                }
                .padding(.horizontal)
                
                // Кнопки управления
                VStack(spacing: 12) {
                    ControlButton(
                        text: viewModel.deviceData?.heaterState == true ? "Выключить обогрев" : "Включить обогрев",
                        action: { Task { await viewModel.toggleHeater() } },
                        backgroundColor: viewModel.deviceData?.heaterState == true ? .red : .green,
                        enabled: viewModel.deviceData?.heaterOffMode != true
                    )
                    
                    ControlButton(
                        text: viewModel.deviceData?.autoMode == true ? "Автоматический режим" : "Ручной режим",
                        action: { Task { await viewModel.setAutoMode() } },
                        backgroundColor: viewModel.deviceData?.autoMode == true ? .blue : .orange,
                        enabled: viewModel.deviceData?.heaterOffMode != true
                    )
                    
                    ControlButton(
                        text: viewModel.deviceData?.heaterOffMode == true ? "Включить" : "Выключить",
                        action: { Task { await viewModel.setOffMode() } },
                        backgroundColor: .gray
                    )
                    
                    ControlButton(
                        text: "Калибровка",
                        action: { showCalibrationDialog = true },
                        backgroundColor: .blue
                    )
                    
                    ControlButton(
                        text: "Настройки WiFi",
                        action: { showWiFiDialog = true },
                        backgroundColor: .purple
                    )
                }
                .padding(.horizontal)
                
                Spacer().frame(height: 20)
            }
        }
    }
}

// MARK: - Landscape Layout
struct LandscapeLayout: View {
    @ObservedObject var viewModel: DeviceViewModel
    @Binding var showCalibrationDialog: Bool
    @Binding var showWiFiDialog: Bool
    
    var body: some View {
        HStack(spacing: 12) {
            // Левая колонка - данные
            VStack(spacing: 12) {
                HStack(spacing: 12) {
                    DataCard(title: "Температура", value: viewModel.deviceData?.temperature ?? 0, unit: "°C", icon: "🌡️")
                    DataCard(title: "Влажность", value: viewModel.deviceData?.humidity ?? 0, unit: "%", icon: "💧")
                }
                
                Spacer()
                
                ControlButton(
                    text: "Калибровка",
                    action: { showCalibrationDialog = true },
                    backgroundColor: .blue
                )
            }
            .frame(maxWidth: .infinity)
            
            // Правая колонка - управление
            VStack(spacing: 12) {
                ControlButton(
                    text: viewModel.deviceData?.heaterState == true ? "Выключить обогрев" : "Включить обогрев",
                    action: { Task { await viewModel.toggleHeater() } },
                    backgroundColor: viewModel.deviceData?.heaterState == true ? .red : .green,
                    enabled: viewModel.deviceData?.heaterOffMode != true
                )
                
                ControlButton(
                    text: viewModel.deviceData?.autoMode == true ? "Автоматический режим" : "Ручной режим",
                    action: { Task { await viewModel.setAutoMode() } },
                    backgroundColor: viewModel.deviceData?.autoMode == true ? .blue : .orange,
                    enabled: viewModel.deviceData?.heaterOffMode != true
                )
                
                Spacer()
                
                ControlButton(
                    text: "Настройки WiFi",
                    action: { showWiFiDialog = true },
                    backgroundColor: .purple
                )
            }
            .frame(maxWidth: .infinity)
        }
        .padding()
        
        // Кнопка выключения внизу
        VStack {
            Spacer()
            ControlButton(
                text: viewModel.deviceData?.heaterOffMode == true ? "Включить" : "Выключить",
                action: { Task { await viewModel.setOffMode() } },
                backgroundColor: .gray
            )
            .padding(.horizontal)
        }
    }
}
