# include "EffectViewer.hpp"

EffectViewerScene::EffectViewerScene(const InitData& init)
	: IScene{ init }
{

}

void EffectViewerScene::update()
{
    if (KeyEscape.down())
    {
        changeScene(State::Lobby);
    }
}

void EffectViewerScene::draw() const
{
	Scene::SetBackground(ColorF{ 0.24, 0.2, 0.2 });
	FontAsset(U"TitleFont")(U"効果確認").drawAt(72, Vec2{ 400, 140 });
	FontAsset(U"Bold")(U"（ダミー）Escでポーズに戻る").drawAt(24, Vec2{ 400, 300 });
}


