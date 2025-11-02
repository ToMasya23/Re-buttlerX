# include "Settings.hpp"

SettingsScene::SettingsScene(const InitData& init)
	: IScene{ init }
{

}

void SettingsScene::update()
{
    if (KeyEscape.down())
    {
        changeScene(getData().pauseReturnState);
    }
}

void SettingsScene::draw() const
{
	Scene::SetBackground(ColorF{ 0.22, 0.22, 0.28 });
	FontAsset(U"TitleFont")(U"設定").drawAt(72, Vec2{ 400, 140 });
	FontAsset(U"Bold")(U"（ダミー画面）Escでポーズに戻る").drawAt(24, Vec2{ 400, 300 });
}


