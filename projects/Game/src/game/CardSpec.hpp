# pragma once
# include "../Common.hpp"

// カード定義
struct CardSpec
{
    String id;
    String name;
    int32 cost = 0;        // 使用時コスト
    double delaySec = 0;   // 詠唱時間（秒）
    double weight = 1.0;   // 抽選重み
    int32 attributeId = 0; // 属性ID（0=デフォルト, 1=量, 2=質, 3=反撃）
    int32 damageHP = 0;    // HPへのダメージ量（effects[type=damageHP]）
    int32 addCrazy = 0;    // クレイジーゲージ増加量（effects[type=addCrazy]）
};


