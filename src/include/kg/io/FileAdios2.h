
#pragma once

#include <adios2.h>
#include "FileBase.h"

namespace kg
{
namespace io
{

// ======================================================================
// FileAdios2

class FileAdios2 : public FileBase
{
public:
  FileAdios2(adios2::Engine engine, adios2::IO io);

  void close() override;
  void performPuts() override;
  void performGets() override;

  void putVariable(const std::string& name, TypeConstPointer data, Mode launch,
                   const Dims& shape, const Box<Dims>& selection,
                   const Box<Dims>& memory_selection) override;
  void getVariable(const std::string& name, TypePointer data, Mode launch,
                   const Box<Dims>& selection,
                   const Box<Dims>& memory_selection) override;
  Dims shapeVariable(const std::string& name) const override;

  void getAttribute(const std::string& name, TypePointer data) override;
  void putAttribute(const std::string& name, TypeConstPointer data,
                    size_t size) override;
  size_t sizeAttribute(const std::string& name) const override;

private:
  struct PutVariable;
  struct GetVariable;
  struct GetAttribute;
  struct PutAttribute;

  template <typename T>
  void putVariable(const std::string& name, const T* data, Mode launch,
                   const Dims& shape, const Box<Dims>& selection,
                   const Box<Dims>& memory_selection);

  template <typename T>
  void getVariable(const std::string& name, T* data, Mode launch,
                   const Box<Dims>& selection,
                   const Box<Dims>& memory_selection);

  template <typename T>
  void getAttribute(const std::string& name, T* data);

  template <typename T>
  void putAttribute(const std::string& name, const T* data, size_t size);

  adios2::Engine engine_;
  adios2::IO io_;
};

} // namespace io
} // namespace kg

#include "FileAdios2.inl"
