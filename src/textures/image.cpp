#include <lightwave.hpp>

namespace lightwave 

{
    ImageTexture::ImageTexture(const Properties &properties) {
        if (properties.has("filename")) {
            m_image = std::make_shared<Image>(properties);
        } else {
            m_image = properties.getChild<Image>();
        }
        m_exposure = properties.get<float>("exposure", 1);

        // clang-format off
        m_border = properties.getEnum<BorderMode>("border", BorderMode::Repeat, {
            { "clamp", BorderMode::Clamp },
            { "repeat", BorderMode::Repeat },
        });

        m_filter = properties.getEnum<FilterMode>("filter", FilterMode::Bilinear, {
            { "nearest", FilterMode::Nearest },
            { "bilinear", FilterMode::Bilinear },
        });
        // clang-format on
        width = m_image->resolution().x();
        height = m_image->resolution().y();
    }

    Color ImageTexture::filterNearest(const Point2 &uv) const {
        // Convert UV to floating-point pixel coordinates
        float x = uv.x() * width;
        float y = uv.y() * height;

        // Calculate integer pixel indices
        int px = int(floor(x));
        int py = int(floor(y));

        // Handle border modes on integer coordinates
        if (m_border == BorderMode::Clamp) {
            px = clamp(px, 0, width - 1);
            py = clamp(py, 0, height - 1);
        } else if (m_border == BorderMode::Repeat) {
            px = (px % width + width) % width; // Wrap X
            py = (py % height + height) % height; // Wrap Y
        }

        return m_image->get(Point2i(px, py));
        }    
    

    Color ImageTexture::filterBilinear(const Point2 &uv) const {
        // Convert UV to floating-point pixel coordinates
        float x = uv.x() * width  - 0.5f;
        float y = uv.y() * height - 0.5f;

        // Integer and fractional parts
        int x0 = int(floor(x));
        int y0 = int(floor(y));
        float fx = x - x0;
        float fy = y - y0;

        // Neighboring pixels
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        // Apply border handling to integer coordinates
        if (m_border == BorderMode::Clamp) {
            x0 = clamp(x0, 0, width - 1);
            y0 = clamp(y0, 0, height - 1);
            x1 = clamp(x1, 0, width - 1);
            y1 = clamp(y1, 0, height - 1);
        } else if (m_border == BorderMode::Repeat) {
            x0 = (x0 % width + width) % width;
            y0 = (y0 % height + height) % height;
            x1 = (x1 % width + width) % width;
            y1 = (y1 % height + height) % height;
        }
        
        // Fetch neighboring texels
        Color c00 = m_image->get(Point2i(x0, y0));
        Color c10 = m_image->get(Point2i(x1, y0));
        Color c01 = m_image->get(Point2i(x0, y1));
        Color c11 = m_image->get(Point2i(x1, y1));

        // Perform bilinear interpolation
        return (1 - fx) * (1 - fy) * c00 +
            fx * (1 - fy) * c10 +
            (1 - fx) * fy * c01 +
            fx * fy * c11;
    }

    Color ImageTexture::evaluate(const Point2 &uv) const{
        if (!m_image) {
            return Color(0); // Return black if no image is loaded
        }
        Point2 new_uv(uv.x(), 1- uv.y());
        if (m_filter == FilterMode::Nearest) {
            return filterNearest(new_uv) * m_exposure;
        } else if (m_filter == FilterMode::Bilinear) {
            return filterBilinear(new_uv) * m_exposure;
        }

        return Color(0); // Fallback
    }

} // namespace lightwave

REGISTER_TEXTURE(ImageTexture, "image")

