# include <Siv3D.hpp> // Siv3D v0.6.16
# include "Common.hpp"
# include "Title.hpp"
# include "Menu.hpp"
# include "Battle.hpp"
# include "scenes/Game.hpp"
// 新規シーン
# include "scenes/Lobby.hpp"
# include "scenes/Matching.hpp"
# include "scenes/Result.hpp"
# include "pause/Settings.hpp"
# include "pause/HowToPlay.hpp"
# include "pause/EffectViewer.hpp"

void Main()
{
	// ESC でアプリ終了しないようにする（ポーズメニューで使うため）
	System::SetTerminationTriggers(UserAction::CloseButtonClicked);

	FontAsset::Register(U"TitleFont", FontMethod::MSDF, 48, U"example/font/RocknRoll/RocknRollOne-Regular.ttf");
	FontAsset(U"TitleFont").setBufferThickness(4);
	FontAsset::Register(U"Bold", FontMethod::MSDF, 48, Typeface::Bold);

	App manager;
    manager.add<Title>(State::Title);
    manager.add<Menu>(State::Menu);
    manager.add<Battle>(State::Battle);
    manager.add<Game>(State::Game);
    manager.add<Lobby>(State::Lobby);
    manager.add<Matching>(State::Matching);
    manager.add<ResultScene>(State::Result);
    manager.add<SettingsScene>(State::Settings);
    manager.add<HowToPlayScene>(State::HowToPlay);
    manager.add<EffectViewerScene>(State::EffectViewer);

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
