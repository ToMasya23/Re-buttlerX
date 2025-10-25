# pragma once
# include "../Common.hpp"

class HowToPlayScene : public App::Scene
{
public:

	HowToPlayScene(const InitData& init);

	void update() override;

	void draw() const override;
};


