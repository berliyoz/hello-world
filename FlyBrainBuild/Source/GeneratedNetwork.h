#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace flybrain::generated
{
struct Node { float x, y; std::uint8_t transmitter; };
struct Edge { std::uint16_t source, target; float weight, delayMs; };

constexpr std::size_t nodeCount = 256;
constexpr std::size_t edgeCount = 896;

constexpr std::array<Node, nodeCount> makeNodes()
{
    std::array<Node, nodeCount> result{};
    for (std::size_t i = 0; i < nodeCount; ++i)
    {
        const auto col = i % 16;
        const auto row = i / 16;
        const float jitterX = static_cast<float>((row * 7 + col * 3) % 5) * 0.0035f;
        const float jitterY = static_cast<float>((row * 5 + col * 11) % 5) * 0.0035f;
        result[i] = Node{
            0.08f + static_cast<float>(col) / 18.0f + jitterX,
            0.08f + static_cast<float>(row) / 18.0f + jitterY,
            static_cast<std::uint8_t>(i % 6)
        };
    }
    return result;
}

constexpr std::array<Edge, edgeCount> makeEdges()
{
    std::array<Edge, edgeCount> result{};
    std::size_t e = 0;
    for (std::size_t i = 0; i < nodeCount; ++i)
    {
        const auto source = static_cast<std::uint16_t>(i);
        const std::uint16_t targets[3] = {
            static_cast<std::uint16_t>((i + 1) % nodeCount),
            static_cast<std::uint16_t>((i + 17) % nodeCount),
            static_cast<std::uint16_t>((i * 37 + 23) % nodeCount)
        };
        for (int k = 0; k < 3; ++k)
        {
            const float weight = 0.22f + static_cast<float>((i * 13 + static_cast<std::size_t>(k) * 17) % 70) / 100.0f;
            const float delay = 2.0f + static_cast<float>((i * 11 + static_cast<std::size_t>(k) * 29) % 90);
            result[e++] = Edge{ source, targets[k], weight, delay };
        }
        if (i < 128)
        {
            const auto target = static_cast<std::uint16_t>((i * 73 + 41) % nodeCount);
            const float weight = 0.18f + static_cast<float>((i * 19) % 75) / 100.0f;
            const float delay = 4.0f + static_cast<float>((i * 23) % 110);
            result[e++] = Edge{ source, target, weight, delay };
        }
    }
    return result;
}

inline constexpr auto nodes = makeNodes();
inline constexpr auto edges = makeEdges();
}
