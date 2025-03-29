    // Color evaluate(const Point2 &uv) const override {
    //     // UV coordinates
    //     float u = uv.x();
    //     float v = uv.y();

    //     // Compute resolution
    //     const int width = m_image->resolution().x();
    //     const int height = m_image->resolution().y();

    //     // Convert UV coordinates to image space
    //     float x = u * (width);
    //     float y = v * (height);

    //     // Integer pixel coordinates and fractional offsets
    //     int x0 = int(std::floor(x)); // Top-left pixel
    //     int y0 = int(std::floor(y)); \
    //     float dx = x - x0;                        // Fractional X
    //     float dy = y - y0;                        // Fractional Y

    //     // Border Mode: Clamp
    //     if (m_border == BorderMode::Clamp) {
    //         x0 = std::clamp(x0, 0, width - 1);
    //         y0 = std::clamp(y0, 0, height - 1);
    //     }
    //     // Border Mode: Repeat
    //     else if (m_border == BorderMode::Repeat) {
    //         // x0 = (x0 % width + width) % width; // Wrap around in X
    //         // y0 = (y0 % height + height) % height; // Wrap around in Y
    //         x0 = x0 > width ? x0 % width - 1: x0; // Wrap around in X
    //         y0 = y0 > width ? y0 % width -1 : y0; // Wrap around in Y
    //     }

    //     // Filtering Mode: Nearest Neighbor
    //     if (m_filter == FilterMode::Nearest) {
    //         // Nearest neighbor simply rounds to the closest pixel
    //         int nearestX = x0; //int(std::round(x)) % width;
    //         int nearestY = y0; //int(std::round(y)) % height;
    //         return m_image->get(Point2i(nearestX, nearestY)) * m_exposure;
    //     }

    //     // Filtering Mode: Bilinear
    //     else if (m_filter == FilterMode::Bilinear) {
    //         // Calculate the neighboring pixel coordinates
    //         int x1 = x0 + 1;
    //         int y1 = y0 + 1;

    //         // Wrap for Repeat mode
    //         if (m_border == BorderMode::Repeat) {
    //             // x1 = (x1 % width + width) % width;
    //             // y1 = (y1 % height + height) % height;
    //             x1 = x1 > width  ? x1 % width - 1: x1;
    //             y1 = y1 > height ? y1 % height - 1: y1;
    //         } 
    //         // Clamp for Clamp mode
    //         else if (m_border == BorderMode::Clamp) {
    //             x1 = std::clamp(x1, 0, width - 1);
    //             y1 = std::clamp(y1, 0, height - 1);
    //         }

    //         // Fetch the four neighboring pixels
    //         Color c00 = m_image->get(Point2i(x0, y0));
    //         Color c10 = m_image->get(Point2i(x1, y0));
    //         Color c01 = m_image->get(Point2i(x0, y1));
    //         Color c11 = m_image->get(Point2i(x1, y1));

    //         // Bilinear interpolation
    //         Color result = (1 - dx) * (1 - dy) * c00 +
    //                     dx * (1 - dy) * c10 +
    //                     (1 - dx) * dy * c01 +
    //                     dx * dy * c11;

    //         return result * m_exposure;
    //     }

    //     // Return black if unhandled cases occur
    //     return Color(0);
    // }