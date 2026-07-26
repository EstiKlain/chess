#pragma once

#include <cstdint>
#include <functional>
#include <string>

class Img;

// Plain-old-data color/rect types so ICanvas has zero dependency on any
// graphics library, including OpenCV. BoardRenderer, HUD classes, etc.
// (Iteration B onward) talk only in terms of these + ICanvas.
struct ColorRGB
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

struct Rect
{
    int x;
    int y;
    int w;
    int h;
};

// Abstract drawing surface. Iteration A only needs clear/fillRect/present/
// shouldClose; drawImage() is added in Iteration B once SpriteLoader exists
// and BoardRenderer needs to composite piece sprites onto the canvas.
class ICanvas
{
public:
    virtual ~ICanvas() = default;

    virtual void clear(const ColorRGB &color) = 0;
    virtual void fillRect(const Rect &rect, const ColorRGB &color) = 0;

    virtual void drawImage(const Img &sprite, int x, int y) = 0;

    // Pushes the current frame to the screen and pumps the window's event
    // loop for one tick (this is where cv::waitKey(1) lives, hidden inside
    // ImgCanvas). Must be called once per frame.
    virtual void present() = 0;

    // True once the user closed the window or pressed the quit key.
    virtual bool shouldClose() const = 0;
    
    virtual void setOnMouseClick(std::function<void(int x, int y)> callback) = 0;

    // Right-click channel, symmetric to setOnMouseClick above. Kept as a
    // separate method (not a button-flag parameter) so ICanvas exposes two
    // distinct, self-documenting input channels rather than one channel
    // with a hidden branch. ICanvas still knows nothing about *why* a
    // right-click matters (that's Controller::handleJumpClick's job) -
    // only that a right mouse button went down, and where. Same rule as
    // setOnMouseClick: only ImgCanvas is allowed to turn this into a real
    // OpenCV call. Breaking change to ICanvas - every implementation
    // (including test fakes) needs a new override.
    virtual void setOnRightMouseClick(std::function<void(int x, int y)> callback) = 0;
    virtual void drawText(const std::string &text, int x, int y, const ColorRGB &color) = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;
};
