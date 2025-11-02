# pragma once
# include "../Common.hpp"

// クレイジー割合に応じた顔テクスチャ選択
struct FaceTextures
{
    s3d::Texture smile;
    s3d::Texture magao;
    s3d::Texture cloudy;
    s3d::Texture crying;

    void load()
    {
        smile = s3d::Texture{ U"assets/ui/faces/笑顔CG一.png" };
        magao = s3d::Texture{ U"assets/ui/faces/真顔CG二.png" };
        cloudy = s3d::Texture{ U"assets/ui/faces/怪しめCG三.png" };
        crying = s3d::Texture{ U"assets/ui/faces/泣きCG四.png" };
    }

    const s3d::Texture& select(int crazyPercent) const
    {
        if (crazyPercent < 40) return smile;
        else if (crazyPercent < 60) return magao;
        else if (crazyPercent < 80) return cloudy;
        else return crying;
    }
};


