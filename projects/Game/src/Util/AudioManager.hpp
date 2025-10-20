#pragma once
#include <Siv3D.hpp>
#include "Singleton.hpp"

class AudioManager : public Singleton<AudioManager> {
	friend class Singleton<AudioManager>;
	AudioManager() = default;

public:
	// 通用点击音
	void playClick(double volume = 0.8) {
		m_click.playOneShot(volume);
	}

	// 打到砖块时
	void playBrickHit(double volume = 0.7) {
		m_brick.playOneShot(volume);
	}

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
	s3d::Audio m_click{ s3d::GMInstrument::Woodblock, s3d::PianoKey::C6, 0.06s, 0.02s };
	s3d::Audio m_brick{ s3d::GMInstrument::SynthBrass1, s3d::PianoKey::C5, 0.10s, 0.00s };
	s3d::Audio m_bgm; // 文件存在时才用
};
