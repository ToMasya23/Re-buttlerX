# pragma once
# include "../Common.hpp"

class SettingsScene : public App::Scene
{
public:

	SettingsScene(const InitData& init);

	void update() override;

	void draw() const override;
};


