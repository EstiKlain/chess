#include "ImgCanvas.hpp"
#include "img.hpp"

ImgCanvas::ImgCanvas(int width, int height, const std::string &windowTitle)
    : width_(width), height_(height), windowTitle_(windowTitle)
{
    frame_ = cv::Mat(height_, width_, CV_8UC3);
    cv::namedWindow(windowTitle_, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowTitle_, width_, height_);
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

void ImgCanvas::drawImage(const Img &sprite, int x, int y)
{
    const cv::Mat &src = sprite.get_mat();
    if (src.empty())
        return; // nothing loaded — draw nothing rather than crash
 
    const int w = src.cols;
    const int h = src.rows;
 
    // Clip silently instead of throwing (Img::draw_on throws on overflow;
    // we'd rather a piece drawn one pixel past the board edge doesn't take
    // the whole render loop down).
    if (x < 0 || y < 0 || x + w > frame_.cols || y + h > frame_.rows)
        return;
 
    cv::Mat roi = frame_(cv::Rect(x, y, w, h));
 
    if (src.channels() == 4)
    {
        // Proper per-pixel alpha blend: split into B/G/R/A planes, then for
        // each color channel mix source and existing background weighted
        // by alpha (0 = fully transparent, 255 = fully opaque).
        std::vector<cv::Mat> srcChannels;
        cv::split(src, srcChannels);
 
        cv::Mat alpha;
        srcChannels[3].convertTo(alpha, CV_32F, 1.0 / 255.0);
        cv::Mat invAlpha = cv::Mat::ones(alpha.size(), CV_32F) - alpha;
 
        std::vector<cv::Mat> roiChannels;
        cv::split(roi, roiChannels);
 
        for (int c = 0; c < 3; ++c)
        {
            cv::Mat srcF, roiF, blendedF, blended8;
            srcChannels[c].convertTo(srcF, CV_32F);
            roiChannels[c].convertTo(roiF, CV_32F);
            blendedF = alpha.mul(srcF) + invAlpha.mul(roiF);
            blendedF.convertTo(blended8, CV_8U);
            roiChannels[c] = blended8;
        }
 
        cv::merge(roiChannels, roi);
    }
    else
    {
        // No alpha channel — opaque copy, same as Img::draw_on's fallback.
        src.copyTo(roi);
    }
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

void ImgCanvas::setOnMouseClick(std::function<void(int, int)> callback)
{
    onClick_ = std::move(callback);
    cv::setMouseCallback(windowTitle_, &ImgCanvas::mouseCallbackThunk, this);
}

void ImgCanvas::setOnRightMouseClick(std::function<void(int, int)> callback)
{
    onRightClick_ = std::move(callback);
    cv::setMouseCallback(windowTitle_, &ImgCanvas::mouseCallbackThunk, this);
}

void ImgCanvas::drawText(const std::string &text, int x, int y, const ColorRGB &color)
{
    cv::putText(frame_, text, cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 1.0,
                cv::Scalar(color.b, color.g, color.r), 2);
}

void ImgCanvas::mouseCallbackThunk(int event, int x, int y, int /*flags*/, void *userdata)
{
    auto *self = static_cast<ImgCanvas *>(userdata);
    if (!self)
        return;

    if (event == cv::EVENT_LBUTTONDOWN)
    {
        if (self->onClick_)
            self->onClick_(x, y);
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        if (self->onRightClick_)
            self->onRightClick_(x, y);
    }
}