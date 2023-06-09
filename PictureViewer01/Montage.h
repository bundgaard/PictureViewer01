#pragma once


struct ID2D1HwndRenderTarget;
class GraphicFactory;

class Montage
{

	GraphicFactory& mGraphicsFactory;
	bool mIsActive;
public:
	Montage() = delete;
	Montage(Montage&) = delete;
	Montage(Montage&&) = delete;
	Montage& operator=(Montage&) = delete;
	Montage& operator=(Montage&&) = delete;
	explicit Montage(GraphicFactory& graphicsFactory);
	~Montage();
	void Render(ID2D1HwndRenderTarget* renderTarget) noexcept;



};

