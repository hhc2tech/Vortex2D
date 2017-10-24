//
//  LinearSolverTests.cpp
//  Vortex2D
//

#include "Verify.h"
#include "VariationalHelpers.h"
#include <Vortex2D/Engine/LinearSolver/Reduce.h>
#include <Vortex2D/Engine/LinearSolver/GaussSeidel.h>
#include <Vortex2D/Engine/LinearSolver/Transfer.h>
#include <Vortex2D/Engine/LinearSolver/Multigrid.h>
#include <Vortex2D/Engine/LinearSolver/ConjugateGradient.h>
#include <Vortex2D/Engine/LinearSolver/Diagonal.h>
#include <Vortex2D/Engine/LinearSolver/IncompletePoisson.h>
#include <Vortex2D/Engine/Pressure.h>

#include <algorithm>
#include <chrono>
#include <numeric>

using namespace Vortex2D::Renderer;
using namespace Vortex2D::Fluid;

extern Device* device;

TEST(LinearSolverTests, ReduceSum)
{
    glm::ivec2 size(10, 15);
    int n = size.x * size.y;
    Buffer<float> input(*device, n, true);
    Buffer<float> output(*device, 1, true);

    ReduceSum reduce(*device, size);
    auto reduceBound = reduce.Bind(input, output);

    std::vector<float> inputData(n);

    {
        float n = 1.0f;
        std::generate(inputData.begin(), inputData.end(), [&n]{ return n++; });
    }

    CopyFrom(input, inputData);

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
       reduceBound.Record(commandBuffer);
    });

    std::vector<float> outputData(1, 0.0f);
    CopyTo(output, outputData);

    ASSERT_EQ(0.5f * n * (n + 1), outputData[0]);
}

TEST(LinearSolverTests, ReduceBigSum)
{
    glm::ivec2 size(500);
    int n = size.x * size.y; // 1 million

    Buffer<float> input(*device, n, true);
    Buffer<float> output(*device, 1, true);

    ReduceSum reduce(*device, size);
    auto reduceBound = reduce.Bind(input, output);

    std::vector<float> inputData(n, 1.0f);

    {
        float n = 1.0f;
        std::generate(inputData.begin(), inputData.end(), [&n]{ return n++; });
    }

    CopyFrom(input, inputData);

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
       reduceBound.Record(commandBuffer);
    });

    std::vector<float> outputData(1, 0.0f);
    CopyTo(output, outputData);

    ASSERT_EQ(0.5f * n * (n + 1), outputData[0]);
}

TEST(LinearSolverTests, ReduceMax)
{
    glm::ivec2 size(10, 15);
    int n = size.x * size.y;

    Buffer<float> input(*device, n, true);
    Buffer<float> output(*device, 1, true);

    ReduceMax reduce(*device, size);
    auto reduceBound = reduce.Bind(input, output);

    std::vector<float> inputData(10*15);

    {
        float n = -1.0f;
        std::generate(inputData.begin(), inputData.end(), [&n]{ return n--; });
    }

    CopyFrom(input, inputData);

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
       reduceBound.Record(commandBuffer);
    });

    std::vector<float> outputData(1, 0.0f);
    CopyTo(output, outputData);

    ASSERT_EQ(150.0f, outputData[0]);
}

TEST(LinearSolverTests, Transfer_Prolongate)
{

    glm::ivec2 coarseSize(3);
    glm::ivec2 fineSize(4);

    Transfer t(*device);

    Buffer<float> fineDiagonal(*device, fineSize.x*fineSize.y, true);
    std::vector<float> fineDiagonalData(fineSize.x*fineSize.y, {1.0f});
    CopyFrom(fineDiagonal, fineDiagonalData);

    Buffer<float> coarseDiagonal(*device, coarseSize.x*coarseSize.y, true);
    std::vector<float> coarseDiagonalData(coarseSize.x*coarseSize.y, {1.0f});
    CopyFrom(coarseDiagonal, coarseDiagonalData);

    Buffer<float> input(*device, coarseSize.x*coarseSize.y, true);
    Buffer<float> output(*device, fineSize.x*fineSize.y, true);

    std::vector<float> data(coarseSize.x * coarseSize.y, 0.0f);
    std::iota(data.begin(), data.end(), 1.0f);
    CopyFrom(input, data);

    t.InitProlongate(0, fineSize, output, fineDiagonal, input, coarseDiagonal);
    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
        t.Prolongate(commandBuffer, 0);
    });

    std::vector<float> outputData(fineSize.x*fineSize.y, 0.0f);
    CopyTo(output, outputData);

    float total;
    total = (9*5 + 3*2 + 3*4 + 1*1) / 16.0f;
    EXPECT_FLOAT_EQ(total, outputData[1 + fineSize.x * 1]);

    total = (9*5 + 3*2 + 3*6 + 1*3) / 16.0f;
    EXPECT_FLOAT_EQ(total, outputData[2 + fineSize.x * 1]);

    total = (9*5 + 3*4 + 3*8 + 1*7) / 16.0f;
    EXPECT_FLOAT_EQ(total, outputData[1 + fineSize.x * 2]);

    total = (9*5 + 3*6 + 3*8 + 1*9) / 16.0f;
    EXPECT_FLOAT_EQ(total, outputData[2 + fineSize.x * 2]);

}

TEST(LinearSolverTests, Transfer_Restrict)
{
    glm::ivec2 coarseSize(3);
    glm::ivec2 fineSize(4);

    Transfer t(*device);

    Buffer<float> fineDiagonal(*device, fineSize.x*fineSize.y, true);
    std::vector<float> fineDiagonalData(fineSize.x*fineSize.y, {1.0f});
    CopyFrom(fineDiagonal, fineDiagonalData);

    Buffer<float> coarseDiagonal(*device, coarseSize.x*coarseSize.y, true);
    std::vector<float> coarseDiagonalData(coarseSize.x*coarseSize.y, {1.0f});
    CopyFrom(coarseDiagonal, coarseDiagonalData);

    Buffer<float> input(*device, fineSize.x*fineSize.y, true);
    Buffer<float> output(*device, coarseSize.x*coarseSize.y, true);

    std::vector<float> data(fineSize.x * fineSize.y, 1.0f);
    std::iota(data.begin(), data.end(), 1.0f);
    CopyFrom(input, data);

    t.InitRestrict(0, fineSize, input, fineDiagonal, output, coarseDiagonal);
    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
        t.Restrict(commandBuffer, 0);
    });

    float total = (1*1 + 3*2 + 3*3 + 1*4 +
                   3*5 + 9*6 + 9*7 + 3*8 +
                   3*9 + 9*10 + 9*11 + 3*12 +
                   1*13 + 3*14 + 3*15 + 1*16) / 64.0f;

    std::vector<float> outputData(coarseSize.x*coarseSize.y, 0.0f);
    CopyTo(output, outputData);

    EXPECT_FLOAT_EQ(total, outputData[1 + coarseSize.x * 1]);
}

void CheckPressure(const glm::ivec2& size, const std::vector<double>& pressure, Buffer<float>& bufferPressure, float error)
{
    std::vector<float> bufferPressureData(size.x * size.y);
    CopyTo(bufferPressure, bufferPressureData);

    for (int i = 0; i < size.x; i++)
    {
        for (int j = 0; j < size.y; j++)
        {
            std::size_t index = i + j * size.x;
            float value = (float)pressure[index];
            EXPECT_NEAR(value, bufferPressureData[index], error) << "Mismatch at " << i << ", " << j << "\n";
        }
    }
}

TEST(LinearSolverTests, Simple_SOR)
{
    glm::ivec2 size(50);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(boundary_phi);

    AddParticles(size, sim, boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    LinearSolver::Data data(*device, size, true);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    LinearSolver::Parameters params(1000, 1e-4f);
    GaussSeidel solver(*device, size);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.Solve(params);

    device->Queue().waitIdle();

    CheckPressure(size, sim.pressure, data.X, 1e-4f);

    std::cout << "Solved with number of iterations: " << params.OutIterations << std::endl;
}

TEST(LinearSolverTests, Complex_SOR)
{
    glm::ivec2 size(50);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(complex_boundary_phi);

    AddParticles(size, sim, complex_boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    LinearSolver::Data data(*device, size, true);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    LinearSolver::Parameters params(1000, 1e-4f);
    GaussSeidel solver(*device, size);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.Solve(params);

    device->Queue().waitIdle();

    CheckPressure(size, sim.pressure, data.X, 1e-4f);

    std::cout << "Solved with number of iterations: " << params.OutIterations << std::endl;
}

TEST(LinearSolverTests, Simple_CG)
{
    glm::ivec2 size(50);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(boundary_phi);

    AddParticles(size, sim, boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    LinearSolver::Data data(*device, size, true);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    Diagonal preconditioner(*device, size);

    LinearSolver::Parameters params(1000, 1e-5f);
    ConjugateGradient solver(*device, size, preconditioner);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.NormalSolve(params);

    device->Queue().waitIdle();

    CheckPressure(size, sim.pressure, data.X, 1e-3f); // TODO somehow error is bigger than 1e-5

    std::cout << "Solved with number of iterations: " << params.OutIterations << std::endl;
}

TEST(LinearSolverTests, Diagonal_Simple_PCG)
{
    glm::ivec2 size(50);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(boundary_phi);

    AddParticles(size, sim, boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    LinearSolver::Data data(*device, size, true);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    Diagonal preconditioner(*device, size);

    LinearSolver::Parameters params(1000, 1e-5f);
    ConjugateGradient solver(*device, size, preconditioner);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.Solve(params);

    device->Queue().waitIdle();

    CheckPressure(size, sim.pressure, data.X, 1e-5f);

    std::cout << "Solved with number of iterations: " << params.OutIterations << std::endl;
}

TEST(LinearSolverTests, GaussSeidel_Simple_PCG)
{
    glm::ivec2 size(50);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(boundary_phi);

    AddParticles(size, sim, boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    LinearSolver::Data data(*device, size, true);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    GaussSeidel preconditioner(*device, size);
    preconditioner.SetW(1.0f);
    preconditioner.SetPreconditionerIterations(8);

    LinearSolver::Parameters params(1000, 1e-5f);
    ConjugateGradient solver(*device, size, preconditioner);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.Solve(params);

    device->Queue().waitIdle();

    CheckPressure(size, sim.pressure, data.X, 1e-4f);

    std::cout << "Solved with number of iterations: " << params.OutIterations << std::endl;
}

TEST(LinearSolverTests, IncompletePoisson_Simple_PCG)
{
    glm::ivec2 size(50);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(boundary_phi);

    AddParticles(size, sim, boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    LinearSolver::Data data(*device, size, true);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    IncompletePoisson preconditioner(*device, size);

    LinearSolver::Parameters params(1000, 1e-5f);
    ConjugateGradient solver(*device, size, preconditioner);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.Solve(params);

    device->Queue().waitIdle();

    CheckPressure(size, sim.pressure, data.X, 1e-5f);

    std::cout << "Solved with number of iterations: " << params.OutIterations << std::endl;
}

TEST(LinearSolverTests, Zero_CG)
{
    glm::vec2 size(50);

    LinearSolver::Data data(*device, size, true);

    Diagonal preconditioner(*device, size);

    LinearSolver::Parameters params(1000, 1e-5f);
    ConjugateGradient solver(*device, size, preconditioner);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.NormalSolve(params);

    device->Queue().waitIdle();

    ASSERT_EQ(0, params.OutIterations);

    std::vector<float> pressureData(size.x*size.y, 1.0f);
    CopyTo(data.X, pressureData);

    for (int i = 0; i < size.x; i++)
    {
        for (int j = 0; j < size.y; j++)
        {
            EXPECT_FLOAT_EQ(0.0f, pressureData[i + size.x * j]);
        }
    }
}

TEST(LinearSolverTests, Zero_PCG)
{
    glm::vec2 size(50);

    LinearSolver::Data data(*device, size, true);

    Diagonal preconditioner(*device, size);

    LinearSolver::Parameters params(1000, 1e-5f);
    ConjugateGradient solver(*device, size, preconditioner);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);
    solver.Solve(params);

    device->Queue().waitIdle();

    ASSERT_EQ(0, params.OutIterations);

    std::vector<float> pressureData(size.x*size.y, 1.0f);
    CopyTo(data.X, pressureData);

    for (int i = 0; i < size.x; i++)
    {
        for (int j = 0; j < size.y; j++)
        {
            EXPECT_FLOAT_EQ(0.0f, pressureData[i + size.x * j]);
        }
    }
}

TEST(LinearSolverTests, Simple_Multigrid)
{
    glm::ivec2 size(32);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(boundary_phi);

    AddParticles(size, sim, boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    // solution from FluidSim
    PrintData(size.x, size.y, sim.pressure);

    LinearSolver::Data data(*device, size, true);

    Texture velocity(*device, size.x, size.y, vk::Format::eR32G32Sfloat, false);
    Texture solidPhi(*device, size.x, size.y, vk::Format::eR32Sfloat, false);
    Texture liquidPhi(*device, size.x, size.y, vk::Format::eR32Sfloat, false);
    Texture solidVelocity(*device, size.x, size.y, vk::Format::eR32G32Sfloat, false);
    Buffer<glm::ivec2> valid(*device, size.x*size.y, true);

    SetSolidPhi(*device, size, solidPhi, sim, size.x);
    SetLiquidPhi(*device, size, liquidPhi, sim, size.x);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    Pressure pressure(*device, 0.01f, size, data, velocity, solidPhi, liquidPhi, solidVelocity, valid);

    Multigrid multigrid(*device, size, 0.01f);
    multigrid.BuildHierarchiesInit(pressure, solidPhi, liquidPhi);
    multigrid.Init(data.Diagonal, data.Lower, data.B, data.X);

    multigrid.BuildHierarchies();

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
        multigrid.Record(commandBuffer);
    });

    device->Queue().waitIdle();

    Texture localLiquidPhi(*device, 32, 32, vk::Format::eR32Sfloat, true);
    Texture localLiquidPhi2(*device, 16, 16, vk::Format::eR32Sfloat, true);

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
        localLiquidPhi.CopyFrom(commandBuffer, liquidPhi);
        localLiquidPhi2.CopyFrom(commandBuffer, multigrid.mLiquidPhis[0]);
    });

    PrintTexture<float>(localLiquidPhi);
    PrintTexture<float>(localLiquidPhi2);

    PrintBuffer<float>(size, data.X);
}

TEST(LinearSolverTests, Multigrid_Simple_PCG)
{
    glm::ivec2 size(64);

    FluidSim sim;
    sim.initialize(1.0f, size.x, size.y);
    sim.set_boundary(boundary_phi);

    AddParticles(size, sim, boundary_phi);

    sim.add_force(0.01f);
    sim.project(0.01f);

    LinearSolver::Data data(*device, size, true);

    Texture velocity(*device, size.x, size.y, vk::Format::eR32G32Sfloat, false);
    Texture liquidPhi(*device, size.x, size.y, vk::Format::eR32Sfloat, false);
    Texture solidPhi(*device, size.x, size.y, vk::Format::eR32Sfloat, false);
    Texture solidVelocity(*device, size.x, size.y, vk::Format::eR32G32Sfloat, false);
    Buffer<glm::ivec2> valid(*device, size.x*size.y, true);

    SetSolidPhi(*device, size, solidPhi, sim, size.x);
    SetLiquidPhi(*device, size, liquidPhi, sim, size.x);

    BuildLinearEquation(size, data.Diagonal, data.Lower, data.B, sim);

    Pressure pressure(*device, 0.01f, size, data, velocity, solidPhi, liquidPhi, solidVelocity, valid);

    Multigrid preconditioner(*device, size, 0.01f);
    preconditioner.BuildHierarchiesInit(pressure, solidPhi, liquidPhi);

    LinearSolver::Parameters params(1000, 1e-5f);
    ConjugateGradient solver(*device, size, preconditioner);

    solver.Init(data.Diagonal, data.Lower, data.B, data.X);

    preconditioner.BuildHierarchies();
    solver.Solve(params);

    device->Queue().waitIdle();

    CheckPressure(size, sim.pressure, data.X, 1e-5f);

    std::cout << "Solved with number of iterations: " << params.OutIterations << std::endl;
}
