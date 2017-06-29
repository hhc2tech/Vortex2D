//
//  CommandBuffer.h
//  Vortex2D
//

#ifndef CommandBuffer_h
#define CommandBuffer_h

#include <Vortex2D/Renderer/Common.h>
#include <Vortex2D/Renderer/Device.h>

#include <vector>
#include <functional>

namespace Vortex2D { namespace Renderer {

class CommandBuffer
{
public:
    using CommandFn = std::function<void(vk::CommandBuffer)>;

    CommandBuffer(const Device& device);
    ~CommandBuffer();

    void Record(CommandFn commandFn);
    void Wait();
    void Submit();

private:
    const Device& mDevice;
    vk::CommandBuffer mCommandBuffer;
    vk::UniqueFence mFence;
};

}}

#endif
