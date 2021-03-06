//
//  WindmillWaterExample.h
//  Vortex2D
//

#include <Vortex2D/Vortex2D.h>

#include "Runner.h"
#include "Rigidbody.h"

#include <functional>
#include <vector>
#include <memory>
#include <cmath>
#include <glm/gtc/constants.hpp>

extern glm::vec4 red;
extern glm::vec4 blue;

class Watermill
{
public:
    Watermill(const Vortex2D::Renderer::Device& device,
              b2World& rWorld,
              Vortex2D::Fluid::World& world)
        : mWindmillTexture(device, 150.0f, 150.0f, vk::Format::eR32Sfloat)
        , mWindmill(device, mWindmillTexture)
        , mRigidbody(world.CreateRigidbody(Vortex2D::Fluid::RigidBody::Type::eStrong, 0.0f, 0.0f, mWindmill, {75.0f, 75.0f}))
    {
        glm::vec2 centre(75.0f, 75.0f);
        mRigidbody->Anchor = centre;

        { // create centre body
            b2BodyDef def;
            def.type = b2_staticBody;

            mCentre = rWorld.CreateBody(&def);

            b2CircleShape circleShape;
            circleShape.m_radius = 1.0f;

            b2FixtureDef fixture;
            fixture.density = 1.0f;
            fixture.shape = &circleShape;

            mCentre->CreateFixture(&fixture);
        }

        { // create wings body and joint
            b2BodyDef def;
            def.type =  b2_dynamicBody;
            def.angularDamping = 3.0f;
            mWings = rWorld.CreateBody(&def);

            b2RevoluteJointDef joint;
            joint.bodyA = mCentre;
            joint.bodyB = mWings;
            joint.collideConnected = false;

            rWorld.CreateJoint(&joint);
        }

        // build wings
        mWindmillTexture.Record({Vortex2D::Fluid::BoundariesClear}).Submit().Wait();

        int n = 6;
        float dist = 30.0f;
        glm::vec2 wingSize(75.0f, 8.0f);

        for (int i = 0; i < n; i++)
        {
            float angle = i * 2.0f * glm::pi<float>() / n;
            float x = dist * std::cos(angle);
            float y = dist * std::sin(angle);

            b2PolygonShape wing;
            wing.SetAsBox(wingSize.x / 2.0f, wingSize.y / 2.0f,
                          b2Vec2(x, y),
                          angle);

            b2FixtureDef fixtureDef;
            fixtureDef.shape = &wing;
            fixtureDef.density = 1.0f;

            mWings->CreateFixture(&fixtureDef);

            Vortex2D::Fluid::Rectangle fluidWing(device, wingSize);
            fluidWing.Anchor = wingSize / glm::vec2(2.0f);
            fluidWing.Position = glm::vec2(x, y) + centre;
            fluidWing.Rotation = glm::degrees(angle);
            mWindmillTexture.Record({fluidWing}, Vortex2D::Fluid::UnionBlend).Submit().Wait();
        }
    }

    void SetTransform(const glm::vec2& pos, float angle)
    {
        mRigidbody->Position = pos;
        mRigidbody->Rotation = angle;
        mCentre->SetTransform({pos.x, pos.y}, angle);
        mWings->SetTransform({pos.x, pos.y}, angle);
    }

    void Update()
    {
        auto pos = mWings->GetPosition();
        mRigidbody->Position = {pos.x, pos.y};
        mRigidbody->Rotation = glm::degrees(mWings->GetAngle());
        mRigidbody->UpdatePosition();

        glm::vec2 vel = {mWings->GetLinearVelocity().x, mWings->GetLinearVelocity().y};
        float angularVelocity = mWings->GetAngularVelocity();
        mRigidbody->SetVelocities(vel, angularVelocity);

        auto force = mRigidbody->GetForces();
        b2Vec2 b2Force = {force.velocity.x, force.velocity.y};
        mWings->ApplyForceToCenter(b2Force, true);
        mWings->ApplyTorque(force.angular_velocity, true);
    }

private:
    Vortex2D::Renderer::RenderTexture mWindmillTexture;
    Vortex2D::Renderer::Sprite mWindmill;
    Vortex2D::Fluid::RigidBody* mRigidbody;
    b2Body* mCentre;
    b2Body* mWings;
};

class WatermillExample : public Runner
{
    const float gravityForce = 100.0f;

public:
    WatermillExample(const Vortex2D::Renderer::Device& device,
                         const glm::ivec2& size,
                         float dt)
        : delta(dt)
        , waterSource(device, {25.0, 25.0f})
        , waterForce(device, {25.0f, 25.0f})
        , gravity(device, glm::vec2(256.0f, 256.0f))
        , world(device, size, dt)
        , solidPhi(world.SolidDistanceField())
        , liquidPhi(world.LiquidDistanceField())
        , rWorld(b2Vec2(0.0f, gravityForce))
        , left(device, rWorld, b2_staticBody, world, Vortex2D::Fluid::RigidBody::Type::eStatic, {50.0f, 5.0f})
        , bottom(device, rWorld, b2_staticBody, world, Vortex2D::Fluid::RigidBody::Type::eStatic, {250.0f, 5.0f})
        , watermill(device, rWorld, world)
    {
        gravity.Colour = glm::vec4(0.0f, dt * gravityForce, 0.0f, 0.0f);

        solidPhi.Colour = red;
        liquidPhi.Colour = blue;
    }

    void Init(const Vortex2D::Renderer::Device& device,
              Vortex2D::Renderer::RenderTarget& renderTarget) override
    {

        // Add particles
        waterSource.Position = {15.0f, 25.0f};
        waterSource.Colour = glm::vec4(4);

        // Add force
        waterForce.Position = {5.0f, 25.0f};
        waterForce.Colour = glm::vec4(20.0f, 0.0f, 0.0f, 0.0f);

        sourceRender = world.RecordParticleCount({waterSource});

        // Draw boundaries
        left.SetTransform({50.0f, 100.0f}, 60.0f);
        left.Update();

        bottom.SetTransform({5.0f, 250.0f}, 0.0f);
        bottom.Update();

        watermill.SetTransform({150.0f, 180.0f}, 25.0f);

        // Set gravity
        velocityRender = world.RecordVelocity({gravity, waterForce});

        Vortex2D::Renderer::ColorBlendState blendState;
        blendState.ColorBlend
                .setBlendEnable(true)
                .setAlphaBlendOp(vk::BlendOp::eAdd)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero);

        windowRender = renderTarget.Record({liquidPhi, solidPhi}, blendState);
    }

    void Step() override
    {
        watermill.Update();

        sourceRender.Submit();
        world.SubmitVelocity(velocityRender);
        world.Step();

        const int velocityStep = 8;
        const int positionStep = 3;
        rWorld.Step(delta, velocityStep, positionStep);

        windowRender.Submit();
    }

private:
    float delta;
    Vortex2D::Renderer::IntRectangle waterSource;
    Vortex2D::Renderer::Rectangle waterForce;
    Vortex2D::Renderer::Rectangle gravity;
    Vortex2D::Fluid::WaterWorld world;
    Vortex2D::Fluid::DistanceField solidPhi, liquidPhi;
    Vortex2D::Renderer::RenderCommand sourceRender, velocityRender, windowRender;

    b2World rWorld;
    BoxRigidbody left, bottom;
    Watermill watermill;
};
