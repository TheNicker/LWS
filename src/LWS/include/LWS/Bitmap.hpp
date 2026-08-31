#pragma once

#include <LWS/interfaces/backends.hpp>
#include <LLUtils/Color.h>

#include <memory>

namespace LWS
{
    class Bitmap;
    using BitmapSharedPtr = std::shared_ptr<Bitmap>;

    class Bitmap
    {
      public:

        explicit Bitmap(const BitmapBuffer& bitmapBuffer);
        ~Bitmap();

        Bitmap(const Bitmap&) = delete;
        Bitmap& operator=(const Bitmap&) = delete;
        Bitmap(Bitmap&&) noexcept = delete;
        Bitmap& operator=(Bitmap&&) noexcept = delete;

        [[nodiscard]] BitmapSharedPtr resize(int width, int height, LLUtils::Color background = {0, 0, 0, 0}) const;
        // The returned pixel view remains valid for this immutable bitmap's lifetime.
        [[nodiscard]] BitmapBuffer GetBuffer() const;

      private:

        class Impl;
        explicit Bitmap(std::unique_ptr<Impl> impl);
        std::unique_ptr<Impl> impl_;
    };
}  // namespace LWS
