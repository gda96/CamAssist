import SwiftUI

struct DataCard: View {
    let title: String
    let value: Float
    let unit: String
    let icon: String
    
    var body: some View {
        VStack(spacing: 8) {
            Text(icon)
                .font(.system(size: 32))
            
            Text(String(format: "%.1f", value))
                .font(.system(size: 24, weight: .bold))
                .foregroundColor(.white)
            
            HStack(spacing: 4) {
                Text(unit)
                    .font(.system(size: 14))
                    .foregroundColor(.white.opacity(0.7))
            }
            
            Text(title)
                .font(.system(size: 12))
                .foregroundColor(.white.opacity(0.7))
        }
        .frame(maxWidth: .infinity)
        .frame(height: 140)
        .padding()
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.white.opacity(0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 12)
                        .stroke(Color.white.opacity(0.2), lineWidth: 1)
                )
        )
    }
}
