//
//  Advection.cpp
//  Vortex
//

#include "Advection.h"

#include <Vortex2D/Renderer/Pipeline.h>
#include <Vortex2D/Engine/Density.h>

#include "vortex2d_generated_spirv.h"

namespace Vortex2D { namespace Fluid {


Advection::Advection(const Renderer::Device& device, const glm::ivec2& size, float dt, Velocity& velocity)
    : mDt(dt)
    , mSize(size)
    , mVelocity(velocity)
    , mVelocityAdvect(device, size, AdvectVelocity_comp)
    , mVelocityAdvectBound(mVelocityAdvect.Bind({velocity.Input(), velocity.Output()}))
    , mAdvect(device, size, Advect_comp)
    , mAdvectParticles(device, Renderer::ComputeSize::Default1D(), AdvectParticles_comp)
    , mAdvectVelocityCmd(device, false)
    , mAdvectCmd(device, false)
    , mAdvectParticlesCmd(device, false)
{
    mAdvectVelocityCmd.Record([&](vk::CommandBuffer commandBuffer)
    {
        mVelocityAdvectBound.PushConstant(commandBuffer, 8, dt);
        mVelocityAdvectBound.Record(commandBuffer);
        velocity.CopyBack(commandBuffer);
    });
}

void Advection::AdvectVelocity()
{
    mAdvectVelocityCmd.Submit();
}

void Advection::AdvectInit(Density& density)
{
    mAdvectBound = mAdvect.Bind({mVelocity.Input(), density, density.mFieldBack});
    mAdvectCmd.Record([&](vk::CommandBuffer commandBuffer)
    {
        mAdvectBound.PushConstant(commandBuffer, 8, mDt);
        mAdvectBound.Record(commandBuffer);
        density.mFieldBack.Barrier(commandBuffer,
                      vk::ImageLayout::eGeneral,
                      vk::AccessFlagBits::eShaderWrite,
                      vk::ImageLayout::eGeneral,
                      vk::AccessFlagBits::eShaderRead);
        density.CopyFrom(commandBuffer, density.mFieldBack);
    });
}

void Advection::Advect()
{
    mAdvectCmd.Submit();
}

void Advection::AdvectParticleInit(Renderer::GenericBuffer& particles,
                                   Renderer::Texture& levelSet,
                                   Renderer::GenericBuffer& dispatchParams)
{
    mAdvectParticlesBound = mAdvectParticles.Bind(mSize, {particles, dispatchParams, mVelocity.Input(), levelSet});
    mAdvectParticlesCmd.Record([&](vk::CommandBuffer commandBuffer)
    {
        commandBuffer.debugMarkerBeginEXT({"Particle advect", {{ 0.09f, 0.17f, 0.36f, 1.0f}}});
        mAdvectParticlesBound.PushConstant(commandBuffer, 8, mDt);
        mAdvectParticlesBound.RecordIndirect(commandBuffer, dispatchParams);
        particles.Barrier(commandBuffer, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
        commandBuffer.debugMarkerEndEXT();
    });
}

void Advection::AdvectParticles()
{
    mAdvectParticlesCmd.Submit();
}

}}
