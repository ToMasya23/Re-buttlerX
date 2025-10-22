# pragma once
# include "../Common.hpp"

// 待機（マッチング）シーン
class Matching : public App::Scene
{
public:

	Matching(const InitData& init);

	void update() override;

	void draw() const override;

private:

	RoundRect m_startButton{ Arg::center(400, 360), 300, 60, 8 };
	RoundRect m_backButton{ Arg::center(400, 440), 300, 60, 8 };

	Transition m_startTr{ 0.4s, 0.2s };
	Transition m_backTr{ 0.4s, 0.2s };
};


