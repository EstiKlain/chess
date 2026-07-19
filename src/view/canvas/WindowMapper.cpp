#include "WindowMapper.hpp"

#include <algorithm>

namespace WindowMapper
{
    double scaleFactor(int windowWidth, int windowHeight,
                        int imageWidth, int imageHeight)
    {
        if (windowWidth <= 0 || windowHeight <= 0 || imageWidth <= 0 || imageHeight <= 0)
            return 0.0;

        const double scaleX = static_cast<double>(windowWidth) / imageWidth;
        const double scaleY = static_cast<double>(windowHeight) / imageHeight;
        return std::min(scaleX, scaleY);
    }

    std::optional<ImagePixel> windowToImage(int windowX, int windowY,
                                             int windowWidth, int windowHeight,
                                             int imageWidth, int imageHeight)
    {
        const double scale = scaleFactor(windowWidth, windowHeight, imageWidth, imageHeight);
        if (scale <= 0.0)
            return std::nullopt;

        const double scaledWidth = imageWidth * scale;
        const double scaledHeight = imageHeight * scale;
        const double offsetX = (windowWidth - scaledWidth) / 2.0;
        const double offsetY = (windowHeight - scaledHeight) / 2.0;

        const double imageX = (windowX - offsetX) / scale;
        const double imageY = (windowY - offsetY) / scale;

        if (imageX < 0.0 || imageY < 0.0 || imageX >= imageWidth || imageY >= imageHeight)
            return std::nullopt; // click landed in the letterbox padding

        return ImagePixel{static_cast<int>(imageX), static_cast<int>(imageY)};
    }
}