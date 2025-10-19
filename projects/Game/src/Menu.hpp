# pragma once
# include "Common.hpp"

// メニューシーン
class Menu : public App::Scene
{
public:

	Menu(const InitData& init);

	void update() override;

	void draw() const override;

private:

	RoundRect m_battleButton{ Arg::center(400, 300), 300, 60, 8 };
	RoundRect m_exitButton{ Arg::center(400, 400), 300, 60, 8 };

	Transition m_battleTransition{ 0.4s, 0.2s };
	Transition m_exitTransition{ 0.4s, 0.2s };
};
