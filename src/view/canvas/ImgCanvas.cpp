#include "ImgCanvas.hpp"

ImgCanvas::ImgCanvas(int width, int height, const std::string &windowTitle)
    : width_(width), height_(height), windowTitle_(windowTitle)
{
    frame_ = cv::Mat(height_, width_, CV_8UC3);
    cv::namedWindow(windowTitle_, cv::WINDOW_AUTOSIZE);
}

ImgCanvas::~ImgCanvas()
{
    cv::destroyWindow(windowTitle_);
}

void ImgCanvas::clear(const ColorRGB &color)
{
    // OpenCV Mats are BGR internally, our public API is RGB -- convert once,
    // here, so nothing above this file ever has to think about channel order.
    frame_.setTo(cv::Scalar(color.b, color.g, color.r));
}

void ImgCanvas::fillRect(const Rect &rect, const ColorRGB &color)
{
    cv::rectangle(frame_,
                  cv::Point(rect.x, rect.y),
                  cv::Point(rect.x + rect.w, rect.y + rect.h),
                  cv::Scalar(color.b, color.g, color.r),
                  cv::FILLED);
}

void ImgCanvas::present()
{
    cv::imshow(windowTitle_, frame_);

    int key = cv::waitKey(1);
    if (key == 27 || key == 'q' || key == 'Q')
        closed_ = true;

    if (cv::getWindowProperty(windowTitle_, cv::WND_PROP_VISIBLE) < 1)
        closed_ = true;
}

bool ImgCanvas::shouldClose() const
{
    return closed_;
}
