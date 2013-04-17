#ifndef CL_HANDLER_H
#define CL_HANDLER_H

#include <string>
#include <map>
#include <CL/cl.hpp>

namespace osp {

class Texture2D;

class CLHandler {
public:
  static CLHandler * New();
  bool Init();
  bool CreateContext();
	bool AddGLTexture(unsigned int _argNr, Texture2D * _texture);

	bool CreateProgram(std::string _filename);
	bool BuildProgram();
	bool CreateKernel();
	bool CreateCommandQueue();
	bool RunRaycaster();
private:
  CLHandler();
	char * ReadSource(std::string _filename, int &_numChars) const;
  std::string GetErrorString(cl_int _error);
  cl_int error_;
  cl_uint numPlatforms_;
  cl_platform_id platforms_[32];
  cl_uint numDevices_;
  cl_device_id devices_[32];
  char deviceName_[1024];
  cl_context context_;
  cl_command_queue commandQueue_;
  cl_mem cubeFront_;
	cl_mem cubeBack_;
  cl_mem output_;
	cl_program program_;
	cl_kernel kernel_;
  static unsigned int instances_;

	// Stores textures together with their kernel argument number
	std::map<cl_uint, cl_mem> GLTextures_;


};

}

#endif
