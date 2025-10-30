#pragma once
# include <Siv3D.hpp>

// ------------------------------------------------------------
// Header-only NineSliceSkin for Siv3D v0.6.14+
// - Keep edge thickness in screen pixels
// - Center drawing optional (stretched); default: not drawn
// - Sampler: ClampNearest (pixel-perfect)
// - Deferred texture load when constructed from FilePath
// ------------------------------------------------------------
class NineSliceSkin {
public:
	// 公共成员（与既有代码兼容）：可直接检查 atlas 是否已加载
	mutable s3d::Texture atlas;

	// 四向边距（像素，等于边框厚度）
	int left = 0;
	int right = 0;
	int top = 0;
	int bottom = 0;

	// 是否绘制中心（默认为 false，不画中心）
	bool drawCenter = false;

	// 构造：从路径（不会在构造时立刻加载，避免 E200）
	NineSliceSkin(const s3d::FilePath& path,
				  int l, int r, int t, int b,
				  bool drawCenter_ = false,
				  s3d::TextureDesc desc = s3d::TextureDesc::Unmipped)
		: left(l), right(r), top(t), bottom(b),
		drawCenter(drawCenter_), m_path(path), m_desc(desc) {
	}

	// 构造：从已存在的纹理（立即可用）
	NineSliceSkin(const s3d::Texture& tex,
				  int l, int r, int t, int b,
				  bool drawCenter_ = false)
		: atlas(tex), left(l), right(r), top(t), bottom(b),
		drawCenter(drawCenter_), m_desc(s3d::TextureDesc::Unmipped) {
	}

	// 默认构造：稍后 setPath() / setAtlas()
	NineSliceSkin() = default;

	void setPath(const s3d::FilePath& path,
				 int l, int r, int t, int b,
				 bool drawCenter_ = false,
				 s3d::TextureDesc desc = s3d::TextureDesc::Unmipped) {
		left = l; right = r; top = t; bottom = b;
		drawCenter = drawCenter_;
		m_path = path; m_desc = desc;
		atlas.release(); // 重新指定路径后，清空旧纹理；首次 draw 时再加载
	}

	void setAtlas(const s3d::Texture& tex,
				  int l, int r, int t, int b,
				  bool drawCenter_ = false) {
		atlas = tex;
		left = l; right = r; top = t; bottom = b;
		drawCenter = drawCenter_;
		m_path.clear();
	}

	bool ready() const { ensureLoaded(); return static_cast<bool>(atlas); }

	// 在目标矩形 dst 上绘制九宫格
	// snapToPixel: 将分割边界对齐到整数像素，避免半像素发灰
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

		// 源边距（夹在合法范围）
		const int Ls = s3d::Clamp(left, 0, W);
		const int Rs = s3d::Clamp(right, 0, W - Ls);
		const int Ts = s3d::Clamp(top, 0, H);
		const int Bs = s3d::Clamp(bottom, 0, H - Ts);
		const int Cw = s3d::Max(0, W - Ls - Rs);
		const int Ch = s3d::Max(0, H - Ts - Bs);

		if (dst.w <= 0.0 || dst.h <= 0.0) return;

		// 目标边距（保持与源边距同样的像素厚度；若太小则自动压缩）
		double l = s3d::Min<double>(Ls, dst.w);
		double r = s3d::Min<double>(Rs, s3d::Max(0.0, dst.w - l));
		double t = s3d::Min<double>(Ts, dst.h);
		double b = s3d::Min<double>(Bs, s3d::Max(0.0, dst.h - t));
		double cw = s3d::Max(0.0, dst.w - l - r);
		double ch = s3d::Max(0.0, dst.h - t - b);

		auto snap = [&](double v) { return snapToPixel ? std::round(v) : v; };

		// 像素边界对齐：按 x0|x1|x2|x3、y0|y1|y2|y3 计算
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

		// 源区域
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

		// 小工具：把一个源区域 s（Rect）画到目标矩形 d（RectF）
		auto drawRegion = [&](const s3d::Rect& s, const s3d::RectF& d)
			{
				if (d.w <= 0 || d.h <= 0 || s.w <= 0 || s.h <= 0) return;

				const s3d::TextureRegion reg = atlas(s);
				const s3d::Vec2 scale{ d.w / s.w, d.h / s.h };   // 非等比缩放（保持像素边厚度）
				reg.scaled(scale).draw(d.pos, color);            // 在左上角位置绘制
			};

		// 角：固定大小
		drawRegion(sTL, dTL);
		drawRegion(sTR, dTR);
		drawRegion(sBL, dBL);
		drawRegion(sBR, dBR);

		// 边：单轴拉伸
		drawRegion(sT, dT);
		drawRegion(sB, dB);
		drawRegion(sL, dL);
		drawRegion(sR, dR);

		// 中心（可选）
		if (drawCenter)
			drawRegion(sC, dC);
	}

private:
	// 从路径延迟加载（首次 draw 时）
	void ensureLoaded() const {
		if (!atlas && !m_path.isEmpty()) {
			atlas = s3d::Texture{ m_path, m_desc };
		}
	}

	// 仅当从路径构造时使用
	s3d::FilePath   m_path;
	s3d::TextureDesc m_desc = s3d::TextureDesc::Unmipped;
};
