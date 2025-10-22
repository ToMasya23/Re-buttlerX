# include "Battle.hpp"

Battle::Battle(const InitData& init)
	: IScene{ init }
{

}

void Battle::update()
{
	// ボタンの更新
	m_backTransition.update(m_backButton.mouseOver());

	if (m_backButton.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	// ボタンのクリック処理
	if (m_backButton.leftClicked())
	{
		changeScene(State::Menu);
	}
}

void Battle::draw() const
{
	Scene::SetBackground(ColorF{ 0.6, 0.3, 0.3 });

	// タイトル描画
	FontAsset(U"TitleFont")(U"戦闘画面")
		.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.4, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 100, Vec2{ 400, 150 });

	// ボタン描画
	m_backButton.draw(ColorF{ 1.0, m_backTransition.value() }).drawFrame(2);

	const Font& boldFont = FontAsset(U"Bold");
	boldFont(U"メニュー画面に戻る").drawAt(32, m_backButton.center(), ColorF{ 0.1 });
}


