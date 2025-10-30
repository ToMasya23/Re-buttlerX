# pragma once
#include <Siv3D.hpp>
#include "BattleTypes.hpp"

// プレイヤー状態を管理する構造体
struct PlayerState
{
	// 基本パラメータ
	int32 hp = BattleConstants::MaxHP;
	double costValue = 100.0;
	int32 crazyGauge = 0;

	// 防御状態
	bool defending = false;
	Stopwatch defendTimer{ StartImmediately::No };

	// 顔テクスチャ（ローカル表示用）
	s3d::Texture texSmile;
	s3d::Texture texMagao;
	s3d::Texture texCloudy;
	s3d::Texture texCrying;

	// コスト値を整数として取得
	int32 getCost() const
	{
		return Clamp<int32>(static_cast<int32>(Round(costValue)), 0, 100);
	}

	// コスト回復
	void regenCost(double dt, double regenRate = BattleConstants::CostRegenPerSec)
	{
		costValue = Min(costValue + regenRate * dt, 100.0);
	}

	// コスト消費
	bool trySpendCost(int32 amount)
	{
		if (getCost() >= amount)
		{
			costValue -= amount;
			return true;
		}
		return false;
	}

	// 防御タイマー更新
	void updateDefenseTimer(double maxDuration = BattleConstants::DefendDurationSec)
	{
		if (defending && defendTimer.sF() >= maxDuration)
		{
			defending = false;
		}
	}

	// 顔テクスチャ選択（クレイジーゲージに応じて）
	const s3d::Texture& getCurrentFaceTexture() const
	{
		const int32 crazyPercent = crazyGauge;
		if (crazyPercent < 40) return texSmile;
		else if (crazyPercent < 60) return texMagao;
		else if (crazyPercent < 80) return texCloudy;
		else return texCrying;
	}
};
