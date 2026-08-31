#include <LWS/Bitmap.hpp>
#include <LWS/source/internal/BitmapValidation.hpp>

#include <LLUtils/Exception.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace
{
    uint8_t Premultiply(uint8_t channel, uint8_t alpha)
    {
        return static_cast<uint8_t>((static_cast<uint16_t>(channel) * alpha + 127U) / 255U);
    }
}  // namespace

namespace LWS
{
    class Bitmap::Impl
    {
      public:

        explicit Impl(const BitmapBuffer& source)
        {
            const auto sourceLayout = internal::validateBitmapBuffer(source);
            if (!sourceLayout.has_value() || source.width > std::numeric_limits<uint32_t>::max() / 4U ||
                source.height > std::numeric_limits<size_t>::max() / 4U / source.width)
                LL_EXCEPTION(LLUtils::Exception::ErrorCode::BadParameters, "Invalid bitmap layout");

            fWidth = source.width;
            fHeight = source.height;
            fPixels.resize(static_cast<size_t>(fWidth) * fHeight * 4U);
            if (source.format == BitmapPixelFormat::Bgra8Premultiplied)
            {
                const size_t rowSize = static_cast<size_t>(fWidth) * 4U;
                for (uint32_t y = 0; y < fHeight; ++y)
                {
                    const uint32_t sourceY = source.rowOrder == BitmapRowOrder::TopDown ? y : fHeight - y - 1;
                    std::memcpy(fPixels.data() + static_cast<size_t>(y) * rowSize,
                                source.pixels.data() + static_cast<size_t>(sourceY) * sourceLayout->rowPitch, rowSize);
                }
                return;
            }

            for (uint32_t y = 0; y < fHeight; ++y)
            {
                const uint32_t sourceY = source.rowOrder == BitmapRowOrder::TopDown ? y : fHeight - y - 1;
                const auto* sourceRow = reinterpret_cast<const uint8_t*>(source.pixels.data()) +
                                        static_cast<size_t>(sourceY) * sourceLayout->rowPitch;
                auto* targetRow = reinterpret_cast<uint8_t*>(fPixels.data()) + static_cast<size_t>(y) * fWidth * 4U;
                for (uint32_t x = 0; x < fWidth; ++x)
                {
                    const auto* sourcePixel = sourceRow + static_cast<size_t>(x) * sourceLayout->bytesPerPixel;
                    auto* targetPixel = targetRow + static_cast<size_t>(x) * 4U;
                    const uint8_t alpha = sourceLayout->bytesPerPixel == 4 ? sourcePixel[3] : 255U;
                    targetPixel[0] = source.format == BitmapPixelFormat::Bgra8 ? Premultiply(sourcePixel[0], alpha)
                                                                               : sourcePixel[0];
                    targetPixel[1] = source.format == BitmapPixelFormat::Bgra8 ? Premultiply(sourcePixel[1], alpha)
                                                                               : sourcePixel[1];
                    targetPixel[2] = source.format == BitmapPixelFormat::Bgra8 ? Premultiply(sourcePixel[2], alpha)
                                                                               : sourcePixel[2];
                    targetPixel[3] = alpha;
                }
            }
        }

        Impl(uint32_t width, uint32_t height, std::vector<std::byte> pixels)
            : fWidth(width), fHeight(height), fPixels(std::move(pixels))
        {
        }

        std::unique_ptr<Impl> Resize(int width, int height, LLUtils::Color background) const
        {
            if (width <= 0 || height <= 0 || static_cast<uint32_t>(width) > std::numeric_limits<uint32_t>::max() / 4U)
                LL_EXCEPTION(LLUtils::Exception::ErrorCode::BadParameters, "Invalid bitmap size");
            if (static_cast<size_t>(width) > std::numeric_limits<size_t>::max() / 4U / static_cast<size_t>(height))
            {
                LL_EXCEPTION(LLUtils::Exception::ErrorCode::BadParameters, "Bitmap is too large");
            }

            std::vector<std::byte> pixels(static_cast<size_t>(width) * height * 4U);
            const uint8_t alpha = background.A();
            const std::array backgroundPixel{Premultiply(background.B(), alpha), Premultiply(background.G(), alpha),
                                             Premultiply(background.R(), alpha), alpha};
            for (size_t offset = 0; offset < pixels.size(); offset += 4U)
                std::memcpy(pixels.data() + offset, backgroundPixel.data(), backgroundPixel.size());

            const double scale = std::min(
                {1.0, static_cast<double>(width) / fWidth, static_cast<double>(height) / fHeight});
            const int scaledWidth = std::max(1, static_cast<int>(std::lround(fWidth * scale)));
            const int scaledHeight = std::max(1, static_cast<int>(std::lround(fHeight * scale)));
            const int offsetX = (width - scaledWidth) / 2;
            const int offsetY = (height - scaledHeight) / 2;
            for (int y = 0; y < scaledHeight; ++y)
            {
                const double sourceY = std::clamp((y + 0.5) / scale - 0.5, 0.0, static_cast<double>(fHeight - 1));
                const uint32_t y0 = static_cast<uint32_t>(sourceY);
                const uint32_t y1 = std::min(y0 + 1, fHeight - 1);
                const double yWeight = sourceY - y0;
                for (int x = 0; x < scaledWidth; ++x)
                {
                    const double sourceX = std::clamp((x + 0.5) / scale - 0.5, 0.0, static_cast<double>(fWidth - 1));
                    const uint32_t x0 = static_cast<uint32_t>(sourceX);
                    const uint32_t x1 = std::min(x0 + 1, fWidth - 1);
                    const double xWeight = sourceX - x0;
                    const size_t targetOffset = (static_cast<size_t>(y + offsetY) * width +
                                                 static_cast<size_t>(x + offsetX)) *
                                                4U;
                    auto* target = reinterpret_cast<uint8_t*>(pixels.data() + targetOffset);
                    std::array<uint8_t, 4> sourcePixel{};
                    for (size_t channel = 0; channel < 4; ++channel)
                    {
                        const auto sample = [&](uint32_t sampleX, uint32_t sampleY)
                        {
                            return static_cast<double>(reinterpret_cast<const uint8_t*>(
                                fPixels.data())[(static_cast<size_t>(sampleY) * fWidth + sampleX) * 4U + channel]);
                        };
                        const double top = std::lerp(sample(x0, y0), sample(x1, y0), xWeight);
                        const double bottom = std::lerp(sample(x0, y1), sample(x1, y1), xWeight);
                        sourcePixel[channel] = static_cast<uint8_t>(std::lround(std::lerp(top, bottom, yWeight)));
                    }
                    const uint8_t inverseAlpha = 255U - sourcePixel[3];
                    for (size_t channel = 0; channel < 3; ++channel)
                    {
                        target[channel] = static_cast<uint8_t>(sourcePixel[channel] +
                                                               (backgroundPixel[channel] * inverseAlpha + 127U) / 255U);
                    }
                    target[3] = static_cast<uint8_t>(sourcePixel[3] +
                                                     (backgroundPixel[3] * inverseAlpha + 127U) / 255U);
                }
            }

            return std::make_unique<Impl>(static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                                          std::move(pixels));
        }

        BitmapBuffer GetBuffer() const
        {
            return {
                .pixels = fPixels,
                .format = BitmapPixelFormat::Bgra8Premultiplied,
                .rowOrder = BitmapRowOrder::TopDown,
                .width = fWidth,
                .height = fHeight,
                .rowPitch = fWidth * 4U,
            };
        }

      private:

        uint32_t fWidth{};
        uint32_t fHeight{};
        std::vector<std::byte> fPixels;
    };

    Bitmap::Bitmap(const BitmapBuffer& bitmapBuffer) : impl_(std::make_unique<Impl>(bitmapBuffer)) {}
    Bitmap::Bitmap(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
    Bitmap::~Bitmap() = default;
    BitmapSharedPtr Bitmap::resize(int width, int height, LLUtils::Color background) const
    {
        return BitmapSharedPtr(new Bitmap(impl_->Resize(width, height, background)));
    }
    BitmapBuffer Bitmap::GetBuffer() const
    {
        return impl_->GetBuffer();
    }
}  // namespace LWS
