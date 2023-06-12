#include "BossMode.h"
#include <d2d1.h>
#include "GraphicFactory.h"

namespace
{
	constexpr wchar_t NEWLINE[] = L"\r\n";
	std::wstring logo = L"Microsoft Windows";

	std::wstring text = L"An error has occurred. To continue:\n"
		L"Press Enter to return to Windows, or\n"
		L"Press CTRL+ALT+DEL to restart your computer. If you do this you will lose any unsaved information in all open applications.";


}
void BossMode::DrawCenteredText(std::wstring const& aText)
{
}

BossMode::BossMode(GraphicFactory& graphicsFactory) : mGraphicsFactory(graphicsFactory), mIsActive(false), mTextFormat(nullptr)
{
	HRESULT hr = S_OK;
	
	if (SUCCEEDED(hr))
	{
		hr = mGraphicsFactory.GetWriteFactory()->CreateTextFormat(
			L"yoster island", //L"Fredericka The Great",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			20.0f,
			L"",
			&mTextFormat);
	}
	if (FAILED(hr))
	{
		hr = mGraphicsFactory.GetWriteFactory()->CreateTextFormat(
			L"arial",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			20.0f,
			L"",
			&mTextFormat
		);
	}
}

BossMode::~BossMode()
{
	if (mTextFormat)
	{
		mTextFormat->Release();
		mTextFormat = nullptr;
	}
}

void BossMode::Render(ID2D1HwndRenderTarget* renderTarget) noexcept
{
	const auto [width, height] = renderTarget->GetSize();
	ID2D1SolidColorBrush* brush = nullptr;
	UINT32 BlueDeathScreenColor = (8 << 16) | (39 << 8) | (249);
	HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(BlueDeathScreenColor), &brush);

	if (SUCCEEDED(hr))
	{
		renderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, width, height), brush);
		IDWriteTextLayout* layout = nullptr;
		hr = mGraphicsFactory.GetWriteFactory()->CreateTextLayout(
			text.c_str(),
			static_cast<UINT32>(text.size()),
			mTextFormat,
			width,
			height,
			&layout
		);
		if (SUCCEEDED(hr))
		{
			brush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));
			DWRITE_TEXT_METRICS tm{};
			layout->GetMetrics(&tm);

			renderTarget->DrawTextLayout(D2D1::Point2F((width - tm.widthIncludingTrailingWhitespace) / 2.0f, (height - tm.height) / 2.0f), layout, brush);
			layout->Release();
			layout = nullptr;
		}
		brush->Release();
	}

}

void BossMode::SetActive(bool aValue) noexcept
{
	if (aValue != mIsActive)
	{
		mIsActive = aValue;
	}
}

bool BossMode::IsActive()
{
	return mIsActive;
}
