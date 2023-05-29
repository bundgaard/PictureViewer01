#pragma once
#include "Application.h"
#include "BaseWindow.h"

#include <memory>
#include <dwrite.h>
#include <commdlg.h>
#include "GraphicFactory.h"

struct ZipFile;
class GraphicsManager;
class ZipManager;
class AnimatedImage;
class Viewer final : public BaseWindow<Viewer>
{

public:
	Viewer(Viewer&) = delete;
	Viewer(Viewer&&) = delete;
	Viewer& operator=(Viewer&&) = delete;
	Viewer& operator=(Viewer&) = delete;

	explicit Viewer();
	~Viewer() override;

	[[nodiscard]] HRESULT Initialize(HINSTANCE hInst) override;
	[[nodiscard]] HRESULT LoadFile(std::wstring const& path) const;
	[[nodiscard]] HRESULT LoadImage() const;
	[[nodiscard]] HRESULT OpenArchive();

	void OnSize(UINT width, UINT height) noexcept override;
	LRESULT OnPaint(HWND hwnd) noexcept override;
	void OnKeyDown(UINT32 virtualKey) noexcept override;
	void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept override;
	void OnLButtonDown(float x, float y) noexcept override;
	void OnLButtonUp(float x, float y) noexcept override;
	void OnMouseScrollWheel(short delta) noexcept override;
	void OnChar(wchar_t keyCode, short repeatCount) noexcept override;
	static void Start() noexcept;
protected:
	void UpdateTitle();
	void ResetTitle() const;
	static void ArchiveWorker(Viewer* viewer, std::wstring const& Filename);
private:
	GraphicFactory mGraphicFactory;
	std::unique_ptr<GraphicsManager> mGraphicManager;
	std::unique_ptr<ZipManager>  m_ZipManager;
	std::unique_ptr<AnimatedImage> mAnimImage;
	

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

	std::wstring m_OriginalTitle;
	int mCurrentPage;

};

