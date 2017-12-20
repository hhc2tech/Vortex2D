//
//  Common.h
//  Vortex2D
//

#include <vector>
#include <iostream>
#include <random>

#include <gmock/gmock.h>
#include <glm/vec2.hpp>

#include <Vortex2D/Renderer/Texture.h>
#include <Vortex2D/Renderer/Buffer.h>
#include <Vortex2D/Renderer/CommandBuffer.h>

#include <Vortex2D/Engine/LinearSolver/LinearSolver.h>

#include "fluidsim.h"

//Boundary definition - several circles in a circular domain.

const Vec2f c0(0.5f,0.5f), c1(0.7f,0.5f), c2(0.3f,0.35f), c3(0.5f,0.7f);
const float rad0 = 0.4f,  rad1 = 0.1f,  rad2 = 0.1f,   rad3 = 0.1f;

static float circle_phi(const Vec2f& position, const Vec2f& centre, float radius)
{
    return (dist(position,centre) - radius);
}

static float boundary_phi(const Vec2f& position)
{
    float phi0 = -circle_phi(position, c0, rad0);

    return phi0;
}

static float complex_boundary_phi(const Vec2f& position)
{
    float phi0 = -circle_phi(position, c0, rad0);
    float phi1 = circle_phi(position, c1, rad1);
    float phi2 = circle_phi(position, c2, rad2);
    float phi3 = circle_phi(position, c3, rad3);

    return min(min(phi0,phi1),min(phi2,phi3));
}

static void AddParticles(const glm::vec2& size, FluidSim& sim, float (*phi)(const Vec2f&))
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for(int i = 0; i < 4*sqr(size.x); ++i)
    {
        Vec2f pt(dist(gen), dist(gen));
        if (phi(pt) > 0 && pt[0] > 0.5)
        {
            sim.add_particle(pt);
        }
    }
}

static void SetVelocity(const Vortex2D::Renderer::Device& device,
                        const glm::ivec2& size,
                        Vortex2D::Renderer::Texture& velocity,
                        FluidSim& sim)
{
    Vortex2D::Renderer::Texture input(device, size.x, size.y, vk::Format::eR32G32Sfloat, true);

    std::vector<glm::vec2> velocityData(size.x * size.y, glm::vec2(0.0f));
    for (int i = 0; i < size.x; i++)
    {
        for (int j = 0; j < size.y; j++)
        {
            std::size_t index = i + size.x * j;
            velocityData[index].x = sim.u(i, j);
            velocityData[index].y = sim.v(i, j);
        }
    }

    input.CopyFrom(velocityData);
    ExecuteCommand(device, [&](vk::CommandBuffer commandBuffer)
    {
        velocity.CopyFrom(commandBuffer, input);
    });
}

static void SetSolidPhi(const Vortex2D::Renderer::Device& device,
                        const glm::ivec2& size,
                        Vortex2D::Renderer::Texture& solidPhi,
                        FluidSim& sim,
                        float scale = 1.0f)
{
    Vortex2D::Renderer::Texture input(device, size.x, size.y, vk::Format::eR32Sfloat, true);

    std::vector<float> phi(size.x * size.y, 0.0f);
    for (int i = 0; i < size.x; i++)
    {
        for (int j = 0; j < size.y; j++)
        {
            int width = size.x;
            phi[i + j * width] = scale * sim.nodal_solid_phi(i, j);
        }
    }

    input.CopyFrom(phi);
    ExecuteCommand(device, [&](vk::CommandBuffer commandBuffer)
    {
        solidPhi.CopyFrom(commandBuffer, input);
    });
}

static void SetLiquidPhi(const Vortex2D::Renderer::Device& device,
                         const glm::ivec2& size,
                         Vortex2D::Renderer::Texture& liquidPhi,
                         FluidSim& sim,
                         float scale = 1.0f)
{
    Vortex2D::Renderer::Texture input(device, size.x, size.y, vk::Format::eR32Sfloat, true);

    std::vector<float> phi(size.x * size.y, 0.0f);
    for (int i = 0; i < size.x; i++)
    {
        for (int j = 0; j < size.y; j++)
        {
            int width = size.x;
            phi[i + j * width] = scale * sim.liquid_phi(i, j);
        }
    }

    input.CopyFrom(phi);
    ExecuteCommand(device, [&](vk::CommandBuffer commandBuffer)
    {
        liquidPhi.CopyFrom(commandBuffer, input);
    });
}

static void BuildInputs(const Vortex2D::Renderer::Device& device,
                        const glm::ivec2& size,
                        FluidSim& sim,
                        Vortex2D::Renderer::Texture& velocity,
                        Vortex2D::Renderer::Texture& solidPhi,
                        Vortex2D::Renderer::Texture& liquidPhi)
{
    SetVelocity(device, size, velocity, sim);

    sim.compute_phi();
    sim.compute_pressure_weights();
    sim.solve_pressure(0.01f);

    SetSolidPhi(device, size, solidPhi, sim);
    SetLiquidPhi(device, size, liquidPhi, sim);
}

static void BuildLinearEquation(const glm::ivec2& size,
                                Vortex2D::Renderer::Buffer<float>& d,
                                Vortex2D::Renderer::Buffer<glm::vec2>& l,
                                Vortex2D::Renderer::Buffer<float>& div, FluidSim& sim)
{
    std::vector<float> diagonalData(size.x * size.y);
    std::vector<glm::vec2> lowerData(size.x * size.y);
    std::vector<float> divData(size.x * size.y);

    for (int i = 1; i < size.x - 1; i++)
    {
        for (int j = 1; j < size.y - 1; j++)
        {
            unsigned index = i + size.x * j;
            divData[index] = (float)sim.rhs[index];
            diagonalData[index] = (float)sim.matrix(index, index);
            lowerData[index].x = (float)sim.matrix(index - 1, index);
            lowerData[index].y = (float)sim.matrix(index, index - size.x);
        }
    }

    using Vortex2D::Renderer::CopyFrom;

    CopyFrom(d, diagonalData);
    CopyFrom(l, lowerData);
    CopyFrom(div, divData);
}

static void PrintDiagonal(const glm::ivec2& size, Vortex2D::Renderer::Buffer<float>& buffer)
{
    std::vector<float> pixels(size.x * size.y);
    Vortex2D::Renderer::CopyTo(buffer, pixels);

    for (std::size_t j = 0; j < size.y; j++)
    {
        for (std::size_t i = 0; i < size.x; i++)
        {
            std::size_t index = i + size.x * j;
            std::cout << "(" <<  pixels[index] << ")";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

static void PrintWeights(const glm::ivec2& size, FluidSim& sim)
{
    for (int j = 1; j < size.y - 1; j++)
    {
        for (int i = 1; i < size.x - 1; i++)
        {
            int index = i + size.x * j;
            std::cout << "(" <<  sim.matrix(index + 1, index) << ","
                      << sim.matrix(index - 1, index) << ","
                      << sim.matrix(index, index + size.x) << ","
                      << sim.matrix(index, index - size.x) << ")";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

static void PrintVelocity(const glm::ivec2& size, Vortex2D::Renderer::Texture& buffer)
{
    std::vector<glm::vec2> pixels(size.x * size.y);
    buffer.CopyTo(pixels);

    for (std::size_t j = 0; j < size.y; j++)
    {
        for (std::size_t i = 0; i < size.x; i++)
        {
            std::size_t index = i + size.x * j;
            std::cout << "(" <<  pixels[index].x << "," << pixels[index].y << ")";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

static void PrintVelocity(const glm::ivec2& size, FluidSim& sim)
{
    for (int j = 0; j < size.y; j++)
    {
        for (int i = 0; i < size.x; i++)
        {
            std::cout << "(" << sim.u(i, j) << "," << sim.v(i, j) << ")";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

static void CheckVelocity(const Vortex2D::Renderer::Device& device,
                          const glm::ivec2& size,
                          Vortex2D::Renderer::Texture& velocity,
                          FluidSim& sim,
                          float error = 1e-6)
{
    Vortex2D::Renderer::Texture output(device, size.x, size.y, vk::Format::eR32G32Sfloat, true);
    ExecuteCommand(device, [&](vk::CommandBuffer commandBuffer)
    {
        output.CopyFrom(commandBuffer, velocity);
    });

    std::vector<glm::vec2> pixels(size.x * size.y);
    output.CopyTo(pixels);

    // FIXME need to check the entire velocity buffer
    for (std::size_t i = 1; i < size.x - 1; i++)
    {
        for (std::size_t j = 1; j < size.y - 1; j++)
        {
            auto uv = pixels[i + j * size.x];
            EXPECT_NEAR(sim.u(i, j), uv.x, error) << "Mismatch at " << i << "," << j;
            EXPECT_NEAR(sim.v(i, j), uv.y, error) << "Mismatch at " << i << "," << j;
        }
    }
}

static void CheckValid(const glm::ivec2& size, FluidSim& sim, Vortex2D::Renderer::Buffer<glm::ivec2>& valid)
{
    std::vector<glm::ivec2> validData(size.x*size.y);
    Vortex2D::Renderer::CopyTo(valid, validData);

    for (int i = 0; i < size.x - 1; i++)
    {
        for (int j = 0; j < size.y - 1; j++)
        {
            std::size_t index = i + j * size.x;
            EXPECT_EQ(validData[index].x, sim.u_valid(i, j)) << "Mismatch at " << i << "," << j;
            EXPECT_EQ(validData[index].y, sim.v_valid(i, j)) << "Mismatch at " << i << "," << j;
        }
    }
}
