# pragma once
# include "../Common.hpp"

// カード定義
struct CardSpec
{
    String id;
    String name;
    int32 cost = 0;       // 使用時コスト
    double delaySec = 0;  // 詠唱時間（秒）
    double weight = 1.0;  // 抽選重み
};


