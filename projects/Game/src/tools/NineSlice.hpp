#pragma once
# include <Siv3D.hpp>

// ------------------------------------------------------------
// ヘッダオンリー NineSliceSkin（Siv3D v0.6.14+）
// - 枠のエッジ厚を「画面ピクセル単位」で維持
// - 中央領域の描画は任意（伸張）。既定は「描画しない」
// - サンプラ：ClampNearest（ドット絵/ピクセルパーフェクト）
// - FilePath から生成した場合は「初回 draw 時に遅延ロード」
// ------------------------------------------------------------
class NineSliceSkin {
public:
	// 公開メンバ（既存コードとの互換用）：atlas がロード済みかを直接確認できる
	mutable s3d::Texture atlas;

	// 四辺のマージン（ピクセル。＝枠の厚み）
	int left = 0;
	int right = 0;
	int top = 0;
	int bottom = 0;

	// 中央領域を描画するか（既定は false：描画しない）
	bool drawCenter = false;

	// 構築：ファイルパスから（コンストラクタではロードせず、初回 draw で読み込む）
	NineSliceSkin(const s3d::FilePath& path,
				  int l, int r, int t, int b,
				  bool drawCenter_ = false,
				  s3d::TextureDesc desc = s3d::TextureDesc::Unmipped)
		: left(l), right(r), top(t), bottom(b),
		drawCenter(drawCenter_), m_path(path), m_desc(desc) {
	}

	// 構築：既存テクスチャから（即時利用可能）
	NineSliceSkin(const s3d::Texture& tex,
				  int l, int r, int t, int b,
				  bool drawCenter_ = false)
		: atlas(tex), left(l), right(r), top(t), bottom(b),
		drawCenter(drawCenter_), m_desc(s3d::TextureDesc::Unmipped) {
	}

	// デフォルト構築：後から setPath()/setAtlas() で設定
	NineSliceSkin() = default;

	// 画像パスと各種パラメータを設定（次回 draw 時にロード）
	void setPath(const s3d::FilePath& path,
				 int l, int r, int t, int b,
				 bool drawCenter_ = false,
				 s3d::TextureDesc desc = s3d::TextureDesc::Unmipped) {
		left = l; right = r; top = t; bottom = b;
		drawCenter = drawCenter_;
		m_path = path; m_desc = desc;
		atlas.release(); // パスを変えたので旧テクスチャを解放。初回 draw で再ロード
	}

	// 既存テクスチャを設定（即時有効）
	void setAtlas(const s3d::Texture& tex,
				  int l, int r, int t, int b,
				  bool drawCenter_ = false) {
		atlas = tex;
		left = l; right = r; top = t; bottom = b;
		drawCenter = drawCenter_;
		m_path.clear();
	}

	// ロード済み判定（内部で ensureLoaded() を呼ぶ）
	bool ready() const { ensureLoaded(); return static_cast<bool>(atlas); }

	// 指定の矩形 dst に 9-slice 描画
	// snapToPixel: 分割境界を整数ピクセルへスナップして半ピクセルの滲みを防ぐ
	void draw(const s3d::RectF& dst,
			  const s3d::ColorF& color = s3d::ColorF{ 1,1,1,1 },
			  bool snapToPixel = true,
			  const s3d::SamplerState& sampler = s3d::SamplerState::ClampNearest) const
	{
		ensureLoaded();
		if (!atlas) return;

		const int W = atlas.width();
		const int H = atlas.height();
		if (W <= 0 || H <= 0) return;

		// 元画像側のマージン（範囲内にクランプ）
		const int Ls = s3d::Clamp(left, 0, W);
		const int Rs = s3d::Clamp(right, 0, W - Ls);
		const int Ts = s3d::Clamp(top, 0, H);
		const int Bs = s3d::Clamp(bottom, 0, H - Ts);
		const int Cw = s3d::Max(0, W - Ls - Rs);
		const int Ch = s3d::Max(0, H - Ts - Bs);

		if (dst.w <= 0.0 || dst.h <= 0.0) return;

		// 描画先側のマージン（元画像のピクセル厚を維持。小さすぎる場合は自動で縮める）
		double l = s3d::Min<double>(Ls, dst.w);
		double r = s3d::Min<double>(Rs, s3d::Max(0.0, dst.w - l));
		double t = s3d::Min<double>(Ts, dst.h);
		double b = s3d::Min<double>(Bs, s3d::Max(0.0, dst.h - t));
		double cw = s3d::Max(0.0, dst.w - l - r);
		double ch = s3d::Max(0.0, dst.h - t - b);

		auto snap = [&](double v) { return snapToPixel ? std::round(v) : v; };

		// 境界を整数ピクセルへスナップ（x0|x1|x2|x3、y0|y1|y2|y3）
		const double x0 = snap(dst.x);
		const double x1 = snap(dst.x + l);
		const double x2 = snap(dst.x + l + cw);
		const double x3 = snap(dst.x + dst.w);

		const double y0 = snap(dst.y);
		const double y1 = snap(dst.y + t);
		const double y2 = snap(dst.y + t + ch);
		const double y3 = snap(dst.y + dst.h);

		const s3d::RectF dTL{ x0, y0, x1 - x0, y1 - y0 };
		const s3d::RectF dT{ x1, y0, x2 - x1, y1 - y0 };
		const s3d::RectF dTR{ x2, y0, x3 - x2, y1 - y0 };

		const s3d::RectF dL{ x0, y1, x1 - x0, y2 - y1 };
		const s3d::RectF dC{ x1, y1, x2 - x1, y2 - y1 };
		const s3d::RectF dR{ x2, y1, x3 - x2, y2 - y1 };

		const s3d::RectF dBL{ x0, y2, x1 - x0, y3 - y2 };
		const s3d::RectF dB{ x1, y2, x2 - x1, y3 - y2 };
		const s3d::RectF dBR{ x2, y2, x3 - x2, y3 - y2 };

		// 元画像側の切り出し矩形
		const s3d::Rect sTL{ 0,           0,            Ls, Ts };
		const s3d::Rect sT{ Ls,          0,            Cw, Ts };
		const s3d::Rect sTR{ W - Rs,      0,            Rs, Ts };

		const s3d::Rect sL{ 0,           Ts,           Ls, Ch };
		const s3d::Rect sC{ Ls,          Ts,           Cw, Ch };
		const s3d::Rect sR{ W - Rs,      Ts,           Rs, Ch };

		const s3d::Rect sBL{ 0,           H - Bs,       Ls, Bs };
		const s3d::Rect sB{ Ls,          H - Bs,       Cw, Bs };
		const s3d::Rect sBR{ W - Rs,      H - Bs,       Rs, Bs };

		const s3d::ScopedRenderStates2D _nearest{ sampler };

		// 小ユーティリティ：元矩形 s（Rect）を先矩形 d（RectF）に描画
		auto drawRegion = [&](const s3d::Rect& s, const s3d::RectF& d)
			{
				if (d.w <= 0 || d.h <= 0 || s.w <= 0 || s.h <= 0) return;

				const s3d::TextureRegion reg = atlas(s);
				const s3d::Vec2 scale{ d.w / s.w, d.h / s.h }; // エッジの厚みを維持するため片軸伸張
				reg.scaled(scale).draw(d.pos, color);          // 左上基準で描画
			};

		// 角：固定サイズ
		drawRegion(sTL, dTL);
		drawRegion(sTR, dTR);
		drawRegion(sBL, dBL);
		drawRegion(sBR, dBR);

		// エッジ：片軸伸張
		drawRegion(sT, dT);
		drawRegion(sB, dB);
		drawRegion(sL, dL);
		drawRegion(sR, dR);

		// 中央：必要な場合のみ描画
		if (drawCenter)
			drawRegion(sC, dC);
	}

private:
	// 初回描画時に遅延ロード（FilePath 指定時のみ）
	void ensureLoaded() const {
		if (!atlas && !m_path.isEmpty()) {
			atlas = s3d::Texture{ m_path, m_desc };
		}
	}

	// パスから構築した場合のみ保持
	s3d::FilePath    m_path;
	s3d::TextureDesc m_desc = s3d::TextureDesc::Unmipped;
};
