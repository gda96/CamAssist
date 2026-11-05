//
//  ContentView.swift
//  CamAssist
//
//  Главный экран приложения
//

import SwiftUI

struct ContentView: View {
    @StateObject private var viewModel = DeviceViewModel()
    
    var body: some View {
        DeviceView(viewModel: viewModel)
            .task {
                await viewModel.loadDeviceData()
            }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
