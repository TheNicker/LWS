#pragma once

#ifdef LWS_PLATFORM_WAYLAND

    #include <cstdint>
    #include <span>
    #include <string_view>

namespace LWS::internal
{
    void renderCaptionTitle(std::span<uint32_t> pixels, int32_t width, std::string_view title, int32_t rightEdge);
}

#endif
