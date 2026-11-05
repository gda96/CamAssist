//
//  CalibrationView.swift
//  CamAssist
//
//  Экран калибровки датчиков
//

import SwiftUI

struct CalibrationView: View {
    @ObservedObject var viewModel: DeviceViewModel
    @Environment(\.dismiss) var dismiss
    
    @State private var tempOffset: Float = 0.0
    @State private var humOffset: Float = 0.0
    @State private var tempThreshold: Float = 10.0
    @State private var humThreshold: Float = 70.0
    
    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Коррекция датчиков")) {
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Коррекция температуры: \(tempOffset, specifier: "%.1f")°C")
                        Slider(value: $tempOffset, in: -10...10, step: 0.1)
                    }
                    
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Коррекция влажности: \(humOffset, specifier: "%.1f")%")
                        Slider(value: $humOffset, in: -20...20, step: 0.1)
                    }
                }
                
                Section(header: Text("Пороги автоматического режима")) {
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Порог температуры: \(tempThreshold, specifier: "%.1f")°C")
                        Slider(value: $tempThreshold, in: 0...30, step: 0.5)
                    }
                    
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Порог влажности: \(humThreshold, specifier: "%.1f")%")
                        Slider(value: $humThreshold, in: 0...100, step: 1)
                    }
                }
                
                Section {
                    Button("Сохранить") {
                        Task {
                            await viewModel.calibrateSensor(tempOffset: tempOffset, humOffset: humOffset)
                            await viewModel.calibrateThresholds(tempThreshold: tempThreshold, humThreshold: humThreshold)
                            dismiss()
                        }
                    }
                    .frame(maxWidth: .infinity)
                }
            }
            .navigationTitle("Калибровка")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Закрыть") {
                        dismiss()
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
