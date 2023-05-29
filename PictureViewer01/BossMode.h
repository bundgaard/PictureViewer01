#pragma once
#include <string>
struct IDWriteTextFormat;
struct ID2D1HwndRenderTarget;
class GraphicFactory;
class BossMode
{
	GraphicFactory& mGraphicsFactory;
	IDWriteTextFormat* mTextFormat;
	bool mIsActive;


	void DrawCenteredText(std::wstring const& aText);
public:
	
	BossMode(GraphicFactory& graphicsFactory);
	BossMode() = delete;
	BossMode(BossMode&) = delete;
	BossMode(BossMode&&) = delete;
	BossMode& operator=(BossMode&) = delete;
	BossMode& operator=(BossMode&&) = delete;

	~BossMode();
	void Render(ID2D1HwndRenderTarget* renderTarget) noexcept;

	void SetActive(bool aValue) noexcept;
	[[nodiscard]] bool IsActive();

	
};

