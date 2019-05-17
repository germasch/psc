
#pragma once

#include <adios2.h>

namespace kg
{
namespace io
{

// ======================================================================
// FileAdios2

class FileAdios2
{
public:
  FileAdios2(adios2::Engine engine, adios2::IO io);

  void close();
  void performPuts();
  void performGets();

  template <typename T>
  void putVariable(const std::string& name, const T* data, Mode launch,
                   const Dims& shape, const Box<Dims>& selection,
                   const Box<Dims>& memory_selection);

  template <typename T>
  void getVariable(const std::string& name, T* data, Mode launch,
                   const Box<Dims>& selection,
                   const Box<Dims>& memory_selection);

  template <typename T>
  Dims shape(const std::string& name) const;

  template <typename T>
  void getAttribute(const std::string& name, std::vector<T>& data);

  template <typename T>
  void putAttribute(const std::string& name, const T* data, size_t size);

  template <typename T>
  void putAttribute(const std::string& name, const T& datum);

private:
  adios2::Engine engine_;
  adios2::IO io_;
};

} // namespace io
} // namespace kg

#include "FileAdios2.inl"
