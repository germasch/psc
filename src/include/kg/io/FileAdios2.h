
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
  Dims shapeVariable(const std::string& name) const;

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

// ======================================================================
// File

class File
{
public:
  File(adios2::Engine engine, adios2::IO io) : impl_{new FileAdios2{engine, io}}
  {}

  void close()
  {
    impl_->close();
  }
  
  void performPuts()
  {
    impl_->performPuts();
  }
  
  void performGets()
  {
    impl_->performGets();
  }

  template <typename T>
  void putVariable(const std::string& name, const T* data, Mode launch,
                   const Dims& shape, const Box<Dims>& selection,
                   const Box<Dims>& memory_selection)
  {
    impl_->putVariable(name, data, launch, shape, selection, memory_selection);
  }

  template <typename T>
  void getVariable(const std::string& name, T* data, Mode launch,
                   const Box<Dims>& selection,
                   const Box<Dims>& memory_selection)
  {
    impl_->getVariable(name, data, launch, selection, memory_selection);
  }

  template <typename T>
  Dims shapeVariable(const std::string& name) const
  {
    return impl_->shapeVariable<T>(name);
  }

  template <typename T>
  void getAttribute(const std::string& name, std::vector<T>& data)
  {
    impl_->getAttribute(name, data);
  }

  template <typename T>
  void putAttribute(const std::string& name, const T* data, size_t size)
  {
    impl_->putAttribute(name, data, size);
  }

  template <typename T>
  void putAttribute(const std::string& name, const T& datum)
  {
    impl_->putAttribute(name, datum);
  }


private:
  std::unique_ptr<FileAdios2> impl_;
};

} // namespace io
} // namespace kg

#include "FileAdios2.inl"
