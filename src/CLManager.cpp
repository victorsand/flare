/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <GL/glew.h>
#include <GL/glx.h>
#include <CLManager.h>
#include <CLProgram.h>
#include <TransferFunction.h>
#include <Texture.h>
#include <Utils.h>
#include <sstream>

using namespace osp;

CLManager::CLManager() {
}

CLManager * CLManager::New() {
  return new CLManager();
}

CLManager::~CLManager() {
  for (auto it=clPrograms_.begin(); it!=clPrograms_.end(); ++it) {
    delete it->second;
  }
  for (unsigned int i=0; i<NUM_QUEUE_INDICES; ++i) {
    clReleaseCommandQueue(commandQueues_[i]);
  }
  clReleaseContext(context_);
}

bool CLManager::InitPlatform() {
  error_ = clGetPlatformIDs(MAX_PLATFORMS, platforms_, &numPlatforms_);
  if (error_ == CL_SUCCESS) {
    INFO("Number of CL platforms: " << numPlatforms_);
  } else {
    CheckSuccess(error_, "InitPlatform()");
    return false;
  }
  // TODO support more platforms?
  if (numPlatforms_ > 1) {
    WARNING("More than one platform found, this is unsupported");
  }
  return true;
}


bool CLManager::InitDevices() {
  if (numPlatforms_ < 1) {
    ERROR("Number of platforms is < 1");
    return false;
  }

  // Find devices
  error_ = clGetDeviceIDs(platforms_[0], CL_DEVICE_TYPE_ALL,
                          sizeof(devices_), devices_, &numDevices_);
  if (CheckSuccess(error_, "InitDevices() getting IDs")) {
    INFO("Number of CL devices: " << numDevices_);
  } else {
    return false;
  }

  // Loop over found devices and print info
  for (unsigned int i=0; i<numDevices_; i++) {
    error_ = clGetDeviceInfo(devices_[i], CL_DEVICE_NAME, 
                             sizeof(deviceName_), deviceName_, NULL);
    if (CheckSuccess(error_, "InitDevices() printing info")) {
      INFO("Device " << i << " name: " << deviceName_);
    } else {
      return false;
    }
  }

  // Get maximum allocatable size for each device
  for (unsigned int i=0; i<numDevices_; i++) {
    error_ = clGetDeviceInfo(devices_[i], CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                             sizeof(maxMemAllocSize_[i]), 
                             &(maxMemAllocSize_[i]), NULL);
    if (!CheckSuccess(error_, "InitDevices() finding maxMemAllocSize")) {
      return false;
    }
  }

  return true;
}


bool CLManager::CreateContext() {
  if (numPlatforms_ < 1) {
    ERROR("Number of platforms < 1, can't create context");
    return false;
  }

  if (numDevices_ < 1) {
    ERROR("Number of devices < 1, can't create context");
    return false;
  }

  // Create an OpenCL context with a reference to an OpenGL context
  cl_context_properties contextProperties[] = {
    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
    CL_CONTEXT_PLATFORM, (cl_context_properties)platforms_[0],
    0};
 
  // TODO Support more than one device?
  context_ = clCreateContext(contextProperties, 1, &devices_[0], NULL,
                             NULL, &error_);

  return CheckSuccess(error_, "CreateContext()");
}


bool CLManager::CreateCommandQueue() {
  for (unsigned int i=0; i<NUM_QUEUE_INDICES; ++i) {
    commandQueues_[i]=clCreateCommandQueue(context_, devices_[0], 0, &error_);
    if (!CheckSuccess(error_, "CreateCommandQueue()")) {
      return false;
    }
  }
  return true;
}

bool CLManager::CreateProgram(std::string _programName,
                              std::string _fileName) {
  // Make sure program doesn't already exist. If it does, delete it.
  if (clPrograms_.find(_programName) != clPrograms_.end()) {
    delete clPrograms_[_programName];
    clPrograms_.erase(_programName);
  }

  // Create new program and save pointer in map
  CLProgram *program = CLProgram::New(_programName, this);
  clPrograms_[_programName] = program;
 
  // Create program
  return program->CreateProgram(_fileName);
}


bool CLManager::BuildProgram(std::string _programName) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->BuildProgram();
}


bool CLManager::CreateKernel(std::string _programName) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->CreateKernel();
}


bool CLManager::AddTexture(std::string _programName, unsigned int _argNr,
                           Texture *_texture, TextureType _textureType,
                           Permissions _permissions) {
  cl_mem_flags flag;
  switch (_permissions) {
    case CLManager::READ_ONLY:
      flag = CL_MEM_READ_ONLY;
      break;
    case CLManager::WRITE_ONLY:
      flag = CL_MEM_WRITE_ONLY;
      break;
    case CLManager::READ_WRITE:
      flag = CL_MEM_READ_WRITE;
      break;
    default:
      ERROR("Unknown permission type");
      return false;
  }

  GLuint GLTextureType;
  switch (_textureType) {
    case TEXTURE_1D:
     ERROR("Texture 1D unimplemented");
     return false;
     break;
    case TEXTURE_2D:
      GLTextureType = GL_TEXTURE_2D;
      break;
    case TEXTURE_3D:
      GLTextureType = GL_TEXTURE_3D;
      break;
    default:
      ERROR("Unknown texture type");
      return false;
  }

  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->AddTexture(_argNr, _texture,
                                               GLTextureType, flag);
}


bool CLManager::AddTransferFunction(std::string _programName,
                                    unsigned int _argNr, 
                                    TransferFunction *_transferFunction) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->AddTransferFunction(_argNr, 
                                                      _transferFunction);
}


bool CLManager::AddKernelConstants(std::string _programName, 
                                   unsigned int _argNr, 
                                   KernelConstants *_kernelConstants) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->AddKernelConstants(_argNr,
                                                     _kernelConstants);
}


bool CLManager::AddIntArray(std::string _programName, unsigned int _argNr,
                            int *_intArray, unsigned int _size,
                            Permissions _permissions) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->AddIntArray(_argNr, _intArray,
                                              _size, _permissions);
}


bool CLManager::PrepareProgram(std::string _programName) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->PrepareProgram();
}


bool CLManager::LaunchProgram(std::string _programName) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->LaunchProgram();
}


bool CLManager::FinishProgram(std::string _programName) {
  if (clPrograms_.find(_programName) == clPrograms_.end()) {
    ERROR("Program " << _programName << " not found");
    return false;
  }
  return clPrograms_[_programName]->FinishProgram();
}


bool CLManager::FinishQueue(QueueIndex _queueIndex) {
  clFinish(commandQueues_[_queueIndex]);
  return true;
}


bool CLManager::CheckSuccess(cl_int _error, std::string _location) const {
  if (_error == CL_SUCCESS) {
    return true;
  } else {
    ERROR(_location);
    ERROR(ErrorString(_error));
    return false;
  }
}


std::string CLManager::ErrorString(cl_int _error) const {
  switch (_error) {
    case CL_SUCCESS:
      return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:
      return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
      return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
      return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
      return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
      return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
      return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
      return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
      return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
      return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
      return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
      return "CL_MAP_FAILURE";
    case CL_INVALID_VALUE:
      return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
      return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
      return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
      return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
      return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
      return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
      return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
      return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
      return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
      return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
      return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
      return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
      return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
      return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
      return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
      return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
      return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
      return "CL_INVALID_KERNEL_DEFINITION";
    case  CL_INVALID_KERNEL:
      return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
      return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
      return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
      return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
      return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
      return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
      return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
      return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
      return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
      return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:
      return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:
      return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
      return "CL_INVALID_GL_OBJECT";
    case  CL_INVALID_BUFFER_SIZE:
      return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
      return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:
      return "CL_INVALID_GLOBAL_WORK_SIZE";
    default:
      std::stringstream ss;
      std::string code;
      ss << "Unknown OpenCL error code - " << _error;
      return ss.str();
  }
}


