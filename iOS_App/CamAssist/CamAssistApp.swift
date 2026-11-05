import SwiftUI

@main
struct CamAssistApp: App {
    var body: some Scene {
        WindowGroup {
            DeviceView()
                .preferredColorScheme(.dark)
        }
    }
}
