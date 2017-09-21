//
//  Particles.h
//  Vortex
//

#ifndef Vortex2d_Particles_h
#define Vortex2d_Particles_h

#include <Vortex2D/Renderer/Buffer.h>
#include <Vortex2D/Renderer/RenderTexture.h>
#include <Vortex2D/Engine/PrefixScan.h>

namespace Vortex2D { namespace Fluid {

struct Particle
{
  alignas(8) glm::vec2 Position;
};

class Particles : public Renderer::RenderTexture
{
public:
    Particles(const Renderer::Device& device,
              const glm::ivec2& size,
              Renderer::Buffer& particles,
              const Renderer::DispatchParams& params = {0});

    void Count();
    void Scan();

    int GetCount();

private:
    Renderer::Buffer mNewParticles;
    Renderer::Buffer mCount;
    Renderer::Buffer mIndex;
    Renderer::Buffer mSeeds, mLocalSeeds;

    Renderer::Buffer mDispatchParams, mNewDispatchParams, mLocalDispatchParams;

    Renderer::Work mParticleCountWork;
    Renderer::Work::Bound mParticleCountBound;
    PrefixScan mPrefixScan;
    PrefixScan::Bound mPrefixScanBound;
    Renderer::Work mParticleBucketWork;
    Renderer::Work::Bound mParticleBucketBound;
    Renderer::Work mParticleSpawnWork;
    Renderer::Work::Bound mParticleSpawnBound;

    Renderer::CommandBuffer mCountWork;
    Renderer::CommandBuffer mScanWork;
    Renderer::CommandBuffer mDispatchCountWork;
};

}}

#endif
