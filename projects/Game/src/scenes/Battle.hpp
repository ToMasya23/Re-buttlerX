# pragma once
# include "../Common.hpp"

// 戦闘シーン
class Battle : public App::Scene
{
public:

	Battle(const InitData& init);

	void update() override;

	void draw() const override;

private:

	RoundRect m_backButton{ Arg::center(400, 500), 300, 60, 8 };

	Transition m_backTransition{ 0.4s, 0.2s };
};


