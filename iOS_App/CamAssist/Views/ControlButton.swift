//
//  ControlButton.swift
//  CamAssist
//
//  Кнопка управления с анимацией
//

import SwiftUI

struct ControlButton: View {
    let text: String
    let action: () -> Void
    let backgroundColor: Color
    var enabled: Bool = true
    
    @State private var isPressed = false
    
    var body: some View {
        Button(action: {
            withAnimation(.easeInOut(duration: 0.1)) {
                isPressed = true
            }
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                withAnimation(.easeInOut(duration: 0.2)) {
                    isPressed = false
                }
                action()
            }
        }) {
            Text(text)
                .font(.system(size: 16, weight: .semibold))
                .foregroundColor(.white)
                .frame(maxWidth: .infinity)
                .frame(height: 60)
                .background(
                    LinearGradient(
                        colors: [
                            backgroundColor.opacity(0.8),
                            backgroundColor.opacity(0.6)
                        ],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .cornerRadius(12)
                .shadow(color: backgroundColor.opacity(0.3), radius: 8, x: 0, y: 4)
                .scaleEffect(isPressed ? 0.94 : 1.0)
        }
        .disabled(!enabled)
        .opacity(enabled ? 1.0 : 0.5)
    }
}
