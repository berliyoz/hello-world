#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>
#include "GeneratedNetwork.h"

class BrainEngine
{
public:
    static constexpr int maxNodes = 2048;

    struct Parameters
    {
        float threshold = 0.65f;
        float decayMs = 180.0f;
        float timeScale = 1.0f;
        float propagationSpeed = 1.0f;
        float coupling = 0.8f;
    };

    struct VisualNode { float x = 0.5f, y = 0.5f; std::uint8_t transmitter = 0; };
    struct VisualEdge { int source = 0, target = 0; };

    BrainEngine();
    void reset();
    void stimulate(int node, float amount);
    void step(float dtMs, const Parameters& params, std::vector<int>& firedNodes);

    int getNodeCount() const noexcept { return nodeCount; }
    float getActivity(int i) const noexcept;
    const std::vector<VisualNode>& getVisualNodes() const noexcept { return visualNodes; }
    const std::vector<VisualEdge>& getVisualEdges() const noexcept { return visualEdges; }

private:
    struct Edge { int target = 0; float weight = 0.0f; float delayMs = 0.0f; };
    struct Pending { int target = 0; float amount = 0.0f; float remainingMs = 0.0f; };

    int nodeCount = 0;
    std::vector<std::vector<Edge>> outgoing;
    std::vector<Pending> pending;
    std::array<float, maxNodes> membrane{};
    std::array<float, maxNodes> refractoryMs{};
    std::array<std::atomic<float>, maxNodes> activity{};
    std::vector<VisualNode> visualNodes;
    std::vector<VisualEdge> visualEdges;
};
