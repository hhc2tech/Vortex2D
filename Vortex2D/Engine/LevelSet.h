//
//  LevelSet.h
//  Vortex2D
//

#ifndef LevelSet_h
#define LevelSet_h

#include <Vortex2D/Renderer/RenderTexture.h>
#include <Vortex2D/Renderer/Work.h>
#include <Vortex2D/Renderer/CommandBuffer.h>

namespace Vortex2D { namespace Fluid {

/**
 * @brief A signed distance field, which can be re-initialized. In other words, a level set.
 */
class LevelSet : public Renderer::RenderTexture
{
public:
    VORTEX2D_API LevelSet(const Renderer::Device& device, const glm::ivec2& size, int reinitializeIterations = 100);

    /**
     * @brief Bind a solid level set, which will be used to extrapolate into this level set
     * @param solidPhi
     */
    VORTEX2D_API void ExtrapolateBind(Renderer::Texture& solidPhi);

    /**
     * @brief Reinitialise the level set, i.e. ensure it is a correct signed distance field.
     */
    VORTEX2D_API void Reinitialise();

    /**
     * @brief Extrapolate this level set into the solid level set it was attached to.
     * This only performs a single cell extrapolation.
     */
    VORTEX2D_API void Extrapolate();

    /**
     * @brief Same as \ref Extrapolate but to be recorded as part of other commands
     * @param commandBuffer
     */
    void ExtrapolateRecord(vk::CommandBuffer commandBuffer);

private:
    Renderer::Texture mLevelSet0;
    Renderer::Texture mLevelSetBack;

    vk::UniqueSampler mSampler;

    Renderer::Work mExtrapolate;
    Renderer::Work::Bound mExtrapolateBound;
    Renderer::Work mRedistance;
    Renderer::Work::Bound mRedistanceFront;
    Renderer::Work::Bound mRedistanceBack;

    Renderer::CommandBuffer mExtrapolateCmd;
    Renderer::CommandBuffer mReinitialiseCmd;
};

}}

#endif
