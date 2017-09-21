//
//  Particles.cpp
//  Vortex
//

#include "Particles.h"

#include <random>

namespace Vortex2D { namespace Fluid {

Particles::Particles(const Renderer::Device& device,
                     const glm::ivec2& size,
                     Renderer::Buffer& particles,
                     const Renderer::DispatchParams& params)
    : Renderer::RenderTexture(device, size.x, size.y, vk::Format::eR32Sint)
    , mNewParticles(device, vk::BufferUsageFlagBits::eStorageBuffer, false, 8*size.x*size.y*sizeof(Particle))
    , mCount(device, vk::BufferUsageFlagBits::eStorageBuffer, false, size.x*size.y*sizeof(int))
    , mIndex(device, vk::BufferUsageFlagBits::eStorageBuffer, false, size.x*size.y*sizeof(int))
    , mSeeds(device, vk::BufferUsageFlagBits::eStorageBuffer, false, 4*sizeof(glm::ivec2))
    , mLocalSeeds(device, vk::BufferUsageFlagBits::eStorageBuffer, true, 4*sizeof(glm::ivec2))
    , mDispatchParams(device, vk::BufferUsageFlagBits::eStorageBuffer, false, sizeof(params))
    , mNewDispatchParams(device, vk::BufferUsageFlagBits::eStorageBuffer, false, sizeof(params))
    , mParticleCountWork(device, Renderer::ComputeSize::Default1D(), "../Vortex2D/ParticleCount.comp.spv",
                         {vk::DescriptorType::eStorageBuffer,
                          vk::DescriptorType::eStorageBuffer,
                          vk::DescriptorType::eStorageImage})
    , mParticleCountBound(mParticleCountWork.Bind({particles, mDispatchParams, *this}))
    , mPrefixScan(device, size)
    , mPrefixScanBound(mPrefixScan.Bind(mCount, mIndex, mNewDispatchParams))
    , mParticleBucketWork(device, Renderer::ComputeSize::Default1D(), "../Vortex2D/ParticleBucket.comp.spv",
                          {vk::DescriptorType::eStorageBuffer,
                           vk::DescriptorType::eStorageBuffer,
                           vk::DescriptorType::eStorageBuffer,
                           vk::DescriptorType::eStorageBuffer,
                           vk::DescriptorType::eStorageBuffer})
    , mParticleBucketBound(mParticleBucketWork.Bind(size, {particles,
                                                           mNewParticles,
                                                           mIndex,
                                                           mCount,
                                                           mDispatchParams}))
    , mParticleSpawnWork(device, size, "../Vortex2D/ParticleSpawn.comp.spv",
                        {vk::DescriptorType::eStorageBuffer,
                         vk::DescriptorType::eStorageBuffer,
                         vk::DescriptorType::eStorageBuffer,
                         vk::DescriptorType::eStorageBuffer})
    , mParticleSpawnBound(mParticleSpawnWork.Bind({mNewParticles, mIndex, mCount, mSeeds}))
    , mCountWork(device, false)
    , mScanWork(device, false)
{
    Renderer::Buffer localDispathParams(device, vk::BufferUsageFlagBits::eStorageBuffer, true, sizeof(params));
    localDispathParams.CopyFrom(params);
    Renderer::ExecuteCommand(device, [&](vk::CommandBuffer commandBuffer)
    {
        mDispatchParams.CopyFrom(commandBuffer, localDispathParams);
    });

    mCountWork.Record([&](vk::CommandBuffer commandBuffer)
    {
        mParticleCountBound.RecordIndirect(commandBuffer, mDispatchParams);
        Barrier(commandBuffer,
                vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderWrite,
                vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderRead);
    });

    mScanWork.Record([&](vk::CommandBuffer commandBuffer)
    {
        mCount.CopyFrom(commandBuffer, *this);
        mPrefixScanBound.Record(commandBuffer);
        mIndex.Barrier(commandBuffer, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
        mParticleBucketBound.RecordIndirect(commandBuffer, mDispatchParams);
        mNewParticles.Barrier(commandBuffer, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
        mSeeds.CopyFrom(commandBuffer, mLocalSeeds);
        mParticleSpawnBound.Record(commandBuffer);
        mNewParticles.Barrier(commandBuffer, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
        particles.CopyFrom(commandBuffer, mNewParticles);
        mDispatchParams.CopyFrom(commandBuffer, mNewDispatchParams);
    });
}

void Particles::Count()
{
    mCountWork.Submit();
}

void Particles::Scan()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);

    glm::ivec2 seeds[] = {{dis(gen), dis(gen)},
                          {dis(gen), dis(gen)},
                          {dis(gen), dis(gen)},
                          {dis(gen), dis(gen)}};

    mLocalSeeds.CopyFrom(seeds);
    mScanWork.Submit();
}


}}
