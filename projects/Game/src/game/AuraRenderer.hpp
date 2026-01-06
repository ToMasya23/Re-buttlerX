# pragma once
#include <Siv3D.hpp>

// 属性ID定義
namespace Attribute
{
    constexpr int32 Default = 0;   // デフォルト（灰色/非表示）
    constexpr int32 Quantity = 1;  // 量（赤）
    constexpr int32 Quality = 2;   // 質（青）
    constexpr int32 Counter = 3;   // 反撃（黄色）
    
    inline ColorF GetColor(int32 attributeId)
    {
        switch (attributeId)
        {
        case Quantity: return ColorF{ 1.0, 0.2, 0.2, 0.6 };  // 赤
        case Quality:  return ColorF{ 0.2, 0.4, 1.0, 0.6 };  // 青
        case Counter:  return ColorF{ 1.0, 0.9, 0.2, 0.6 };  // 黄色
        default:       return ColorF{ 0.5, 0.5, 0.5, 0.3 };  // 灰色
        }
    }
}

// オーラ描画の抽象インターフェース
class IAuraRenderer
{
public:
    virtual ~IAuraRenderer() = default;
    
    // キャラクター背後にオーラを描画
    virtual void draw(const RectF& characterRect, int32 attributeId) const = 0;
};
