# pragma once
#include "AuraRenderer.hpp"

// シンプルな円形オーラの実装
class SimpleAuraRenderer : public IAuraRenderer
{
public:
    void draw(const RectF& characterRect, int32 attributeId) const override
    {
        // デフォルト属性（0）の場合はオーラを描画しない
        if (attributeId == Attribute::Default)
        {
            return;
        }
        
        const ColorF color = Attribute::GetColor(attributeId);
        const Vec2 center = characterRect.center();
        
        // キャラクターサイズに基づいてオーラサイズを計算
        const double baseRadius = Max(characterRect.w, characterRect.h) * 0.17;
        
        // 脈動エフェクト
        const double pulse = 1.0 + 0.05 * Sin(Scene::Time() * 3.0);
        const double radius = baseRadius * pulse;
        
        // グラデーションオーラを描画（中心が濃く、外側が薄い）
        for (int i = 5; i >= 0; --i)
        {
            const double r = radius * (1.0 + i * 0.1);
            const double alpha = color.a * (1.0 - i * 0.15);
            Circle{ center, r }.draw(ColorF{ color.r, color.g, color.b, alpha });
        }
    }
};
