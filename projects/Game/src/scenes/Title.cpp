# include "Title.hpp"

Title::Title(const InitData& init)
	: IScene{ init }
{
    // 画像の読み込み
    m_titleTexture = Texture{ U"assets/ui/title/title_screen.png" };
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
    const Size sceneSize = Scene::Size();

    if ((not m_sceneRT) || (m_sceneRT.size() != sceneSize))
    {
        const_cast<Title*>(this)->m_sceneRT = RenderTexture{ sceneSize };
    }
    if ((not m_blurInternal) || (m_blurInternal.size() != sceneSize))
    {
        const_cast<Title*>(this)->m_blurInternal = RenderTexture{ sceneSize };
    }
    if ((not m_blurTarget) || (m_blurTarget.size() != sceneSize))
    {
        const_cast<Title*>(this)->m_blurTarget = RenderTexture{ sceneSize };
    }

    {
        const ScopedRenderTarget2D rt{ m_sceneRT };
        m_sceneRT.clear(ColorF{ 0, 0, 0 });  // 背景色は黒にする

        // 画像が読み込めている場合は描画
        if (m_titleTexture)
        {
            // 画面サイズに合わせてスケーリング
            const double scale = Min(
                static_cast<double>(sceneSize.x) / m_titleTexture.width(),
                static_cast<double>(sceneSize.y) / m_titleTexture.height()
            );
            
            // 中央に描画
            m_titleTexture.scaled(scale).drawAt(Scene::Center());
        }

        // 操作説明（既存のコード）
        const Font& boldFont = FontAsset(U"Bold");
        boldFont(U"ENTER / SPACE を押してスタート").drawAt(24, Vec2{ 400, 450 }, ColorF{ 0.9 });
    }
	m_sceneRT.draw();
}


