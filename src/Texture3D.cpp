/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */
#include <GL/glew.h>
#include <Texture3D.h>
#include <Utils.h>

using namespace osp;

Texture3D::Texture3D(std::vector<unsigned int> _dim) : Texture(_dim) {}

Texture3D * Texture3D::New(std::vector<unsigned int> _dim) {
  if (_dim.size() != 3) {
    ERROR("Texture3D needs a dimension vector of size 3, defaulting to 1x1x1");
    _dim = std::vector<unsigned int>(3, 1);
  }
  return new Texture3D(_dim);
}

bool Texture3D::Init(float *_data) {
  INFO("Initializing Texture3D");
  glGetError();

  if (initialized_) {
    WARNING("Texture3D already initialized, doing nothing");
    return true;
  }

  unsigned int maxTextureSize;
  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, (int*)&maxTextureSize);
  if (dim_[0] > maxTextureSize || dim_[1] > maxTextureSize ||
      dim_[2] > maxTextureSize) {
    ERROR("Dimension larger than texture size");
    ERROR("Max texture size: " << maxTextureSize);
    ERROR("Dims: " << dim_[0] << " " << dim_[1] << " " << dim_[2]);
    return false;
  }
 
  //glEnable(GL_TEXTURE_3D);
  if (CheckGLError("Texture3D::Init(): enabling 3D texture") != GL_NO_ERROR) {
    return false;
  }
  glGenTextures(1, &handle_);
  glBindTexture(GL_TEXTURE_3D, handle_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, 
               dim_[0], dim_[1], dim_[2],
               0, GL_RED, GL_FLOAT, static_cast<GLvoid*>(_data));
  glBindTexture(GL_TEXTURE_3D, 0);
  
  initialized_ = true;

  return (CheckGLError("Texture3D::Init()") == GL_NO_ERROR);
}


bool Texture3D::UpdateSubRegion(unsigned int _xOffset,
                                unsigned int _yOffset,
                                unsigned int _zOffset,
                                unsigned int _xSize,
                                unsigned int _ySize,
                                unsigned int _zSize,
                                float *_data) {

  glGetError();
  glBindTexture(GL_TEXTURE_3D, handle_);
  glTexSubImage3D(GL_TEXTURE_3D,
                  0,
                  _xOffset,
                  _yOffset,
                  _zOffset,
                  _xSize,
                  _ySize,
                  _zSize,
                  GL_RED,
                  GL_FLOAT,
                  _data);
  glBindTexture(GL_TEXTURE_3D, 0);
  
  return (CheckGLError("Texture3D::UpdateSubRegion") == GL_NO_ERROR);
}

