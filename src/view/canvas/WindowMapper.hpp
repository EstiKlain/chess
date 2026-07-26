#pragma once

#include <optional>

// A single point in image-pixel space -- coordinates inside the fixed board
// image ImgCanvas draws into -- as opposed to window-pixel space, which is
// what OpenCV reports from an OS window the user may have resized. Kept as
// its own tiny struct so callers never accidentally mix the two coordinate
// spaces (e.g. pass a raw window pixel straight into
// BoardMapper::pixelToCell, which expects image-pixel space).
struct ImagePixel
{
    int x;
    int y;
};

// Pure, stateless mapping from window-pixel coordinates to image-pixel
namespace WindowMapper
{
    // The uniform scale factor applied to the image so it fits entirely
    // inside a window of the given size while preserving its aspect ratio
    // (i.e. the smaller of the two per-axis ratios). Exposed as its own
    // function -- not just inlined inside windowToImage -- so tests and
    // future callers (e.g. scaling drawn line widths or font size to match
    // the current zoom level) can read it directly. Returns 0.0 for
    // degenerate (zero or negative) window/image sizes.
    double scaleFactor(int windowWidth, int windowHeight,
                        int imageWidth, int imageHeight);

    // Converts a click reported in window-pixel space (windowX, windowY)
    // into image-pixel space, given the current OS window size
    // (windowWidth x windowHeight) and the fixed original image size
    // (imageWidth x imageHeight) ImgCanvas always renders into. The image
    // is scaled uniformly (via scaleFactor) and centered inside the
    // window, so any leftover space becomes letterbox padding on the sides
    // or top/bottom rather than stretching the board and pieces. Returns
    // std::nullopt when the click landed in that letterbox padding, i.e.
    // outside the actual rendered image, or when the sizes are degenerate.
    std::optional<ImagePixel> windowToImage(int windowX, int windowY,
                                             int windowWidth, int windowHeight,
                                             int imageWidth, int imageHeight);
}