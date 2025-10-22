# pragma once
# include "../Common.hpp"

class EffectViewerScene : public App::Scene
{
public:

	EffectViewerScene(const InitData& init);

	void update() override;

	void draw() const override;
};


