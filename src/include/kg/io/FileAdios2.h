
#pragma once

#include <adios2.h>

#include "mpark/variant.hpp"

namespace kg
{
namespace io
{

// ======================================================================
// FileBase

class FileBase
{
public:
  using TypePointer = mpark::variant<int*, unsigned long*, double*>;
  using TypeConstPointer =
    mpark::variant<const int*, const unsigned long*, const double*>;

  virtual ~FileBase() = default;

  virtual void close() = 0;
  virtual void performPuts() = 0;
  virtual void performGets() = 0;

  virtual void putVariable(const std::string& name, TypeConstPointer data,
                           Mode launch, const Dims& shape,
                           const Box<Dims>& selection,
                           const Box<Dims>& memory_selection) = 0;

  virtual void getVariable(const std::string& name, TypePointer data, Mode launch,
			   const Box<Dims>& selection,
			   const Box<Dims>& memory_selection) = 0;
};

// ======================================================================
// FileAdios2

class FileAdios2 : FileBase
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

  template <typename T>
  Dims shapeVariable(const std::string& name) const;

  template <typename T>
  void getAttribute(const std::string& name, std::vector<T>& data);

  template <typename T>
  void putAttribute(const std::string& name, const T* data, size_t size);

  template <typename T>
  void putAttribute(const std::string& name, const T& datum);

private:
  struct PutVariable;

  template <typename T>
  void putVariable(const std::string& name, const T* data, Mode launch,
                   const Dims& shape, const Box<Dims>& selection,
                   const Box<Dims>& memory_selection);

  struct GetVariable;

  template <typename T>
  void getVariable(const std::string& name, T* data, Mode launch,
                   const Box<Dims>& selection,
                   const Box<Dims>& memory_selection);

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

  void close() { impl_->close(); }

  void performPuts() { impl_->performPuts(); }

  void performGets() { impl_->performGets(); }

  template <typename T>
  void putVariable(const std::string& name, const T* data, Mode launch,
                   const Dims& shape, const Box<Dims>& selection,
                   const Box<Dims>& memory_selection)
  {
    FileBase::TypeConstPointer dataVar = data;
    impl_->putVariable(name, dataVar, launch, shape, selection,
                       memory_selection);
  }

  template <typename T>
  void getVariable(const std::string& name, T* data, Mode launch,
                   const Box<Dims>& selection,
                   const Box<Dims>& memory_selection)
  {
    FileBase::TypePointer dataVar = data;
    impl_->getVariable(name, dataVar, launch, selection, memory_selection);
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
