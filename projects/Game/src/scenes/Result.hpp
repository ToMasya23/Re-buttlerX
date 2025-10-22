# pragma once
# include "../Common.hpp"

// リザルトシーン
class ResultScene : public App::Scene
{
public:

	ResultScene(const InitData& init);

	void update() override;

	void draw() const override;

private:

	RoundRect m_rematchButton{ Arg::center(400, 360), 300, 60, 8 };
	RoundRect m_lobbyButton{ Arg::center(400, 440), 300, 60, 8 };

	Transition m_rematchTr{ 0.4s, 0.2s };
	Transition m_lobbyTr{ 0.4s, 0.2s };
};


