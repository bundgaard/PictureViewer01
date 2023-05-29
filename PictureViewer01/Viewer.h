#pragma once
#include "Application.h"
#include "AnimatedImage.h"
#include "BaseWindow.h"

#include "GraphicFactory.h"
#include "GraphicsManager.h"
#include "ZipManager.h"

#include <memory>
#include <dwrite.h>
#include <commdlg.h>


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
	[[nodiscard]] HRESULT LoadFile(std::wstring const& path);
	[[nodiscard]] HRESULT LoadImage();
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
	GraphicsManager mGraphicManager;
	ZipManager  mZipManager;
	AnimatedImage mAnimImage;

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

	std::wstring m_OriginalTitle;
	int mCurrentPage;

};

