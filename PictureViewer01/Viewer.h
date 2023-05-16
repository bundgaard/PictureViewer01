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

class Viewer : public BaseWindow<Viewer>
{
public:
	explicit Viewer(GraphicsManager& graphicManager, ZipManager& zipManager);
	~Viewer();

	HRESULT Initialize(HINSTANCE hInst);
	HRESULT LoadFile(std::wstring const& Path);
	HRESULT LoadImage(int delta);
	HRESULT OpenArchive();

	virtual void OnSize(UINT Width, UINT Height) noexcept override;
	virtual LRESULT OnPaint(HWND hwnd) noexcept override;
	virtual void OnKeyDown(UINT32 VirtualKey) noexcept override;
	virtual void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept override;
	virtual void OnLButtonDown(float x, float y) noexcept override;
	virtual void OnLButtonUp(float x, float y) noexcept override;
	virtual void OnMouseScrollWheel(short delta) noexcept override;
	virtual void OnChar(wchar_t KeyCode, short RepeatCount) noexcept override;
	void Start();
protected:
	std::vector<std::unique_ptr<ZipFile>> ReadZip(std::wstring const& Filename);
private:
	GraphicsManager& mGraphicManager;
	ZipManager& m_ZipManager;

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

	

	

};

