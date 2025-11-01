#pragma once
#include <Siv3D.hpp>
#include "Singleton.hpp"

class AudioManager : public Singleton<AudioManager> {
	friend class Singleton<AudioManager>;
	AudioManager() = default;

public:
	// ------------------------------------------------------------
	// playBGM:
	// - If 'path' is the same as current, keep playing (only adjust volume).
	// - If different, stop current and load/play the new one.
	// - If the file does not exist, do nothing (keep current BGM).
	// ------------------------------------------------------------
	void playBGM(const s3d::FilePath& path, double volume = 0.5, bool loop = true)
	{
		// ファイルが存在しない場合は何もしない（現在の再生を維持）
		if (path.isEmpty() || !s3d::FileSystem::Exists(path)) {
			return;
		}

		// 同じパスなら、再生を維持して音量だけ調整
		if (m_currentPath == path && m_bgm) {
			if (!m_bgm.isPlaying()) {
				m_bgm.play(); // 何らかの理由で停止していた場合は再開
			}
			m_bgm.setVolume(volume);
			return;
		}

		// 別の曲に切り替え（古いものは停止→置き換え）
		if (m_bgm) {
			m_bgm.stop();
		}
		m_bgm = s3d::Audio{ path, loop ? s3d::Loop::Yes : s3d::Loop::No };
		m_bgm.setVolume(volume).play();
		m_currentPath = path;
	}

	// 旧 API 互換：startBGM -> playBGM
	// Backward compatible alias
	void startBGM(const s3d::FilePath& path, double volume = 0.5)
	{
		playBGM(path, volume, true);
	}

	// 明示停止（「同じ曲の場合は止めない」ルールを無視して止めたい時用）
	// Force stop current BGM
	void stopBGM()
	{
		if (m_bgm) {
			m_bgm.stop();
		}
		m_currentPath.clear();
	}

	// 便利関数：音量のみ変更（曲は変えない）
	// Convenience: adjust volume without changing the track
	void setBGMVolume(double volume)
	{
		if (m_bgm) {
			m_bgm.setVolume(volume);
		}
	}

	// 現在の曲が path と同じか？
	// Is the current track the same as 'path'?
	bool isCurrentBGM(const s3d::FilePath& path) const
	{
		return (m_currentPath == path) && static_cast<bool>(m_bgm);
	}

	// 再生中か？
	bool isPlaying() const
	{
		return m_bgm && m_bgm.isPlaying();
	}

	// 現在の曲パス
	const s3d::FilePath& currentPath() const
	{
		return m_currentPath;
	}

private:
	s3d::Audio     m_bgm;          // Current BGM audio
	s3d::FilePath  m_currentPath;  // Track the currently loaded file path
};
