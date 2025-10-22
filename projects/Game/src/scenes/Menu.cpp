# include "Menu.hpp"

Menu::Menu(const InitData& init)
	: IScene{ init }
{

}

void Menu::update()
{
	// ボタンの更新
	m_battleTransition.update(m_battleButton.mouseOver());

	if (m_battleButton.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	// ボタンのクリック処理
	if (m_battleButton.leftClicked())
	{
		changeScene(State::Battle);
	}
}

void Menu::draw() const
{
	Scene::SetBackground(ColorF{ 0.3, 0.4, 0.6 });

	// タイトル描画
	FontAsset(U"TitleFont")(U"メニュー")
		.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.2, 0.4 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 100, Vec2{ 400, 150 });

	// ボタン描画
	m_battleButton.draw(ColorF{ 1.0, m_battleTransition.value() }).drawFrame(2);

	const Font& boldFont = FontAsset(U"Bold");
	boldFont(U"戦闘画面に進む").drawAt(32, m_battleButton.center(), ColorF{ 0.1 });
}


