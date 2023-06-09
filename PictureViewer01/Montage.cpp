#include "Montage.h"

Montage::Montage(GraphicFactory& graphicsFactory) 
	: mGraphicsFactory(graphicsFactory)
	, mIsActive(false)
{
}

Montage::~Montage()
{
}

void Montage::Render(ID2D1HwndRenderTarget* renderTarget) noexcept
{

}
