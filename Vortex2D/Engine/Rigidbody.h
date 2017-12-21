//
//  Rigidbody.h
//  Vortex2D
//

#ifndef Vortex2d_Rigidbody_h
#define Vortex2d_Rigidbody_h

#include <Vortex2D/Renderer/Buffer.h>
#include <Vortex2D/Renderer/Drawable.h>
#include <Vortex2D/Renderer/Transformable.h>
#include <Vortex2D/Renderer/Pipeline.h>
#include <Vortex2D/Renderer/RenderTexture.h>
#include <Vortex2D/Renderer/Work.h>

#include <Vortex2D/Engine/Size.h>

namespace Vortex2D { namespace Fluid {

class RigidBody : public Renderer::Transformable
{
public:
    struct Velocity
    {
        alignas(8) glm::vec2 velocity;
        alignas(4) float angular_velocity;
    };

    RigidBody(const Renderer::Device& device,
              const Dimensions& dimensions,
              Renderer::Drawable& drawable,
              const glm::vec2& centre);

    void SetVelocities(const glm::vec2& velocity, float angularVelocity);
    Velocity GetVelocities() const;

    void UpdatePosition();

    Renderer::RenderCommand RecordPhi(Renderer::RenderTexture& phi);

    Renderer::Work::Bound BindDiv(Renderer::GenericBuffer& div);
    Renderer::Work::Bound BindVelocityConstrain(Renderer::GenericBuffer& velocity);

    // TODO:
    // BindPressure
    // BindA

private:
    const Renderer::Device& mDevice;
    Renderer::RenderTexture mPhi;
    Renderer::Drawable& mDrawable;
    glm::vec2 mCentre;
    glm::mat4 mView;
    Renderer::UniformBuffer<Velocity> mVelocity;
    Renderer::UniformBuffer<glm::mat4> mMVBuffer;
};

}}

#endif
