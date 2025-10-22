# pragma once
# include "../Common.hpp"

// ロビーシーン
class Lobby : public App::Scene
{
public:

	Lobby(const InitData& init);

	void update() override;

	void draw() const override;

private:

	RoundRect m_pvpButton{ Arg::center(400, 260), 300, 60, 8 };
	RoundRect m_pveButton{ Arg::center(400, 340), 300, 60, 8 };
	RoundRect m_exitButton{ Arg::center(400, 420), 300, 60, 8 };

	Transition m_pvpTr{ 0.4s, 0.2s };
	Transition m_pveTr{ 0.4s, 0.2s };
	Transition m_exitTr{ 0.4s, 0.2s };
};


