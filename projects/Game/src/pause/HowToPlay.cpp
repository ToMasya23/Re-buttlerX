# include "HowToPlay.hpp"

HowToPlayScene::HowToPlayScene(const InitData& init)
	: IScene{ init }
{

}

void HowToPlayScene::update()
{
	if (KeyEscape.down())
	{
		changeScene(State::PauseOverlay);
	}
}

void HowToPlayScene::draw() const
{
	Scene::SetBackground(ColorF{ 0.2, 0.25, 0.2 });
	FontAsset(U"TitleFont")(U"ゲーム説明").drawAt(72, Vec2{ 400, 140 });
	FontAsset(U"Bold")(U"（ダミー）Escでポーズに戻る").drawAt(24, Vec2{ 400, 300 });
}


