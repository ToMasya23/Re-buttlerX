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
        smile = s3d::Texture{ U"assets/ui/faces/smile.png" };
        magao = s3d::Texture{ U"assets/ui/faces/magao.png" };
        cloudy = s3d::Texture{ U"assets/ui/faces/cloudy.png" };
        crying = s3d::Texture{ U"assets/ui/faces/crying.png" };
    }

    const s3d::Texture& select(int crazyPercent) const
    {
        if (crazyPercent < 40) return smile;
        else if (crazyPercent < 60) return magao;
        else if (crazyPercent < 80) return cloudy;
        else return crying;
    }
};


