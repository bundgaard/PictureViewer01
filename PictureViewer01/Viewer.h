#pragma once
#include "Application.h"
#include "SafeRelease.h"
#include "BaseWindow.h"

#include <memory>

#include <dwrite.h>
#include <commdlg.h>

struct ZipFile;
class GraphicsManager;
class ZipManager;

class Viewer final : public BaseWindow<Viewer>
{

public:
	Viewer(Viewer&) = delete;

	explicit Viewer();
	~Viewer() override;

	HRESULT Initialize(HINSTANCE hInst) override;
	HRESULT LoadFile(std::wstring const& path) const;
	HRESULT LoadImage(int delta) const;
	HRESULT OpenArchive();

	void OnSize(UINT width, UINT height) noexcept override;
	LRESULT OnPaint(HWND hwnd) noexcept override;
	void OnKeyDown(UINT32 virtualKey) noexcept override;
	void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept override;
	void OnLButtonDown(float x, float y) noexcept override;
	void OnLButtonUp(float x, float y) noexcept override;
	void OnMouseScrollWheel(short delta) noexcept override;
	void OnChar(wchar_t keyCode, short repeatCount) noexcept override;
	static void Start() noexcept;

private:
	std::unique_ptr<GraphicsManager> mGraphicManager;
	std::unique_ptr<ZipManager>  m_ZipManager;

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

};

