# include "Title.hpp"

Title::Title(const InitData& init)
	: IScene{ init }
{

}

void Title::update()
{
	// Enter または Space キーでロビーに遷移
	if (KeyEnter.down() || KeySpace.down())
	{
		changeScene(State::Lobby);
	}
}

void Title::draw() const
{
	Scene::SetBackground(ColorF{ 0.2, 0.8, 0.4 });

	// タイトル描画
	FontAsset(U"TitleFont")(U"GAME")
		.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.2, 0.6, 0.2 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 100, Vec2{ 400, 100 });

	// 操作説明
	const Font& boldFont = FontAsset(U"Bold");
	boldFont(U"ENTER / SPACE を押してスタート").drawAt(24, Vec2{ 400, 300 }, ColorF{ 0.9 });
}


