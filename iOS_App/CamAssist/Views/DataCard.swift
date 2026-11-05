//
//  DataCard.swift
//  CamAssist
//
//  Карточка отображения данных датчика
//

import SwiftUI

struct DataCard: View {
    let title: String
    let value: String
    let unit: String
    let icon: String
    
    var body: some View {
        VStack(spacing: 8) {
            Text(icon)
                .font(.system(size: 32))
            
            Text(value)
                .font(.system(size: 28, weight: .bold))
                .foregroundColor(.white)
            
            HStack(spacing: 4) {
                Text(unit)
                    .font(.system(size: 16, weight: .medium))
                Text(title)
                    .font(.system(size: 14, weight: .regular))
            }
            .foregroundColor(.white.opacity(0.8))
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
