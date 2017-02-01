//
//  Writer.h
//  Vortex2D
//

#ifndef Vortex2D_Writer_h
#define Vortex2D_Writer_h

#include "Common.h"
#include "Texture.h"
#include "Buffer.h"
#include <vector>

namespace Vortex2D { namespace Renderer {

/**
 * @brief Helper class to write data to a texture
 */
class Writer
{
public:
    Writer(Texture& texture);
    Writer(Buffer& buffer);

    void Write(const uint8_t* data);
    void Write(const std::vector<glm::vec4>& data);
    void Write(const std::vector<glm::vec2>& data);
    void Write(const std::vector<float>& data);
    void Write(const float* data);

private:
    void Write(const void* data);

    Texture& mTexture;
};

}}

#endif