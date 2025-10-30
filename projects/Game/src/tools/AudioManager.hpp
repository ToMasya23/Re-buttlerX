#pragma once
#include <Siv3D.hpp>
#include "Singleton.hpp"

class AudioManager : public Singleton<AudioManager> {
	friend class Singleton<AudioManager>;
	AudioManager() = default;

public:
	//（可选）BGM：只在存在该文件时启用，避免资源缺失
	void startBGM(const s3d::FilePath& path, double volume = 0.5) {
		if (s3d::FileSystem::Exists(path)) {
			m_bgm = s3d::Audio{ path, s3d::Loop::Yes };
			m_bgm.setVolume(volume).play();
		}
	}
	void stopBGM() {
		if (m_bgm) m_bgm.stop();
	}

private:
	s3d::Audio m_bgm; // 文件存在时才用
};
