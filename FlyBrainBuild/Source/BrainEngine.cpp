#include "BrainEngine.h"
#include <cmath>

BrainEngine::BrainEngine()
{
    nodeCount = juce::jmin<int>(maxNodes, static_cast<int>(flybrain::generated::nodeCount));
    outgoing.resize(static_cast<std::size_t>(nodeCount));
    visualNodes.reserve(static_cast<std::size_t>(nodeCount));
    visualEdges.reserve(flybrain::generated::edgeCount);

    for (int i = 0; i < nodeCount; ++i)
    {
        const auto& n = flybrain::generated::nodes[static_cast<std::size_t>(i)];
        visualNodes.push_back({ n.x, n.y, n.transmitter });
        activity[static_cast<std::size_t>(i)].store(0.0f);
    }

    for (const auto& e : flybrain::generated::edges)
    {
        if (e.source >= nodeCount || e.target >= nodeCount) continue;
        outgoing[e.source].push_back({ static_cast<int>(e.target), e.weight, e.delayMs });
        if (visualEdges.size() < 1500)
            visualEdges.push_back({ static_cast<int>(e.source), static_cast<int>(e.target) });
    }

    pending.reserve(32768);
    reset();
}

void BrainEngine::reset()
{
    pending.clear();
    membrane.fill(0.0f);
    refractoryMs.fill(0.0f);
    for (int i = 0; i < nodeCount; ++i)
        activity[static_cast<std::size_t>(i)].store(0.0f, std::memory_order_relaxed);
}

void BrainEngine::stimulate(int node, float amount)
{
    if (node < 0 || node >= nodeCount) return;
    membrane[static_cast<std::size_t>(node)] += amount;
}

float BrainEngine::getActivity(int i) const noexcept
{
    if (i < 0 || i >= nodeCount) return 0.0f;
    return activity[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
}

void BrainEngine::step(float dtMs, const Parameters& p, std::vector<int>& firedNodes)
{
    firedNodes.clear();
    if (nodeCount == 0) return;

    const float scaledDt = juce::jmax(0.0001f, dtMs * p.timeScale);
    const float leak = std::exp(-scaledDt / juce::jmax(1.0f, p.decayMs));

    for (int i = 0; i < nodeCount; ++i)
    {
        membrane[static_cast<std::size_t>(i)] *= leak;
        refractoryMs[static_cast<std::size_t>(i)] = juce::jmax(0.0f, refractoryMs[static_cast<std::size_t>(i)] - scaledDt);
        const auto old = activity[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        activity[static_cast<std::size_t>(i)].store(old * 0.90f, std::memory_order_relaxed);
    }

    for (std::size_t i = 0; i < pending.size(); )
    {
        auto& event = pending[i];
        event.remainingMs -= scaledDt * juce::jmax(0.05f, p.propagationSpeed);
        if (event.remainingMs <= 0.0f)
        {
            if (event.target >= 0 && event.target < nodeCount)
                membrane[static_cast<std::size_t>(event.target)] += event.amount;
            pending[i] = pending.back();
            pending.pop_back();
        }
        else ++i;
    }

    for (int i = 0; i < nodeCount; ++i)
    {
        if (refractoryMs[static_cast<std::size_t>(i)] <= 0.0f && membrane[static_cast<std::size_t>(i)] >= p.threshold)
        {
            const float overshoot = membrane[static_cast<std::size_t>(i)] / juce::jmax(0.01f, p.threshold);
            membrane[static_cast<std::size_t>(i)] *= 0.12f;
            refractoryMs[static_cast<std::size_t>(i)] = 4.0f;
            activity[static_cast<std::size_t>(i)].store(juce::jlimit(0.0f, 1.0f, 0.55f + overshoot * 0.18f), std::memory_order_relaxed);
            firedNodes.push_back(i);

            for (const auto& edge : outgoing[static_cast<std::size_t>(i)])
            {
                if (pending.size() >= pending.capacity()) break;
                const float amount = edge.weight * p.coupling * juce::jlimit(0.3f, 1.6f, overshoot);
                pending.push_back({ edge.target, amount, edge.delayMs });
            }
        }
    }
}
