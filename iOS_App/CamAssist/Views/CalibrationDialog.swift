import SwiftUI

struct CalibrationDialog: View {
    @ObservedObject var viewModel: DeviceViewModel
    @Environment(\.dismiss) var dismiss
    
    @State private var tempOffset: Float = 0
    @State private var humOffset: Float = 0
    @State private var tempThreshold: Float = 10
    @State private var humThreshold: Float = 70
    
    var body: some View {
        NavigationView {
            Form {
                Section("Коррекция датчика") {
                    VStack(alignment: .leading) {
                        Text("Температура: \(String(format: "%.1f", tempOffset))°C")
                        Slider(value: $tempOffset, in: -10...10, step: 0.1)
                    }
                    
                    VStack(alignment: .leading) {
                        Text("Влажность: \(String(format: "%.1f", humOffset))%")
                        Slider(value: $humOffset, in: -10...10, step: 0.1)
                    }
                }
                
                Section("Пороги") {
                    VStack(alignment: .leading) {
                        Text("Температура: \(String(format: "%.1f", tempThreshold))°C")
                        Slider(value: $tempThreshold, in: 0...30, step: 0.1)
                    }
                    
                    VStack(alignment: .leading) {
                        Text("Влажность: \(String(format: "%.1f", humThreshold))%")
                        Slider(value: $humThreshold, in: 0...100, step: 1)
                    }
                }
            }
            .navigationTitle("Калибровка")
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
                            await viewModel.calibrateSensor(tempOffset: tempOffset, humOffset: humOffset)
                            await viewModel.calibrateThresholds(tempThreshold: tempThreshold, humThreshold: humThreshold)
                            dismiss()
                        }
                    }
                }
            }
        }
        .onAppear {
            if let data = viewModel.deviceData {
                tempOffset = data.calibrationParams.tempOffset
                humOffset = data.calibrationParams.humOffset
                tempThreshold = data.calibrationParams.tempThreshold
                humThreshold = data.calibrationParams.humThreshold
            }
        }
    }
}
