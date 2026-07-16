#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <functional>

#include "ICanvas.hpp"

// The concrete OpenCV-backed canvas. This is (together with img.hpp/.cpp
// itself) the only place in the whole project that is allowed to mention
// cv:: types by name -- everyone else, including BoardRenderer later, talks
// to ICanvas only.
//
// Why ImgCanvas doesn't just wrap Img: Img (see img.hpp) is a sprite
// loader/compositor -- read() loads a file, draw_on() composites one loaded
// image onto another, and show() is a one-shot debug viewer that blocks
// forever on cv::waitKey(0) and then destroys the window. None of that
// fits a live render loop, and Img has no shape-drawing primitives
// (fillRect, etc.) and no mutable Mat accessor. So ImgCanvas owns its own
// frame buffer (a plain cv::Mat) and drives cv::imshow / cv::waitKey(1)
// itself every frame. Img is left completely untouched; it comes back in
// UI-Iteration B when SpriteLoader uses Img::read() to load piece frames,
// which ImgCanvas will then composite onto this same frame buffer.
class ImgCanvas : public ICanvas
{
public:
    ImgCanvas(int width, int height, const std::string &windowTitle);
    ~ImgCanvas() override;

    void clear(const ColorRGB &color) override;
    void fillRect(const Rect &rect, const ColorRGB &color) override;
    void drawImage(const Img &sprite, int x, int y) override;

    void present() override;
    bool shouldClose() const override;

    void setOnMouseClick(std::function<void(int x, int y)> callback) override;

    void drawText(const std::string &text, int x, int y, const ColorRGB &color) override;

    int width() const override { return width_; }
    int height() const override { return height_; }

    // Exposed (not part of ICanvas) so that a future BoardRenderer/
    // SpriteLoader adapter in view/render can composite Img frames onto
    // us via Img::draw_on()-style pixel copies. Nothing in Iteration A
    // calls this.
    cv::Mat &mat() { return frame_; }

private:
    static void mouseCallbackThunk(int event, int x, int y, int flags, void *userdata);

    int width_;
    int height_;
    std::string windowTitle_;
    cv::Mat frame_;
    bool closed_ = false;
    std::function<void(int, int)> onClick_;
};
