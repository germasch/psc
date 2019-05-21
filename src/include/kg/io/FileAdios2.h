
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
  using TypePointer = mpark::variant<int*, unsigned long*, double*, std::string*>;
  using TypeConstPointer =
    mpark::variant<const int*, const unsigned long*, const double*, const std::string*>;

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
  virtual Dims shapeVariable(const std::string& name) const = 0;

  virtual void getAttribute(const std::string& name, TypePointer data) = 0;
  virtual void putAttribute(const std::string& name, TypeConstPointer data, size_t size) = 0;
  virtual Dims shapeAttribute(const std::string& name) const = 0;
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
  Dims shapeVariable(const std::string& name) const override;

  void getAttribute(const std::string& name, TypePointer data) override;
  void putAttribute(const std::string& name, TypeConstPointer data, size_t size) override;
  Dims shapeAttribute(const std::string& name) const override;

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

  Dims shapeVariable(const std::string& name) const
  {
    return impl_->shapeVariable(name);
  }

  template <typename T>
  void getAttribute(const std::string& name, std::vector<T>& vec)
  {
    auto shape = impl_->shapeAttribute(name);
    assert(shape.size() == 1);
    vec.resize(shape[0]);
    FileBase::TypePointer dataVar = vec.data();
    impl_->getAttribute(name, dataVar);
  }

  template <typename T>
  void putAttribute(const std::string& name, const T* data, size_t size)
  {
    FileBase::TypeConstPointer dataVar = data;
    impl_->putAttribute(name, dataVar, size);
  }

  template <typename T>
  void putAttribute(const std::string& name, const T& datum)
  {
    putAttribute(name, &datum, 1);
  }

  Dims shapeAttribute(const std::string& name) const
  {
    return impl_->shapeAttribute(name);
  }

private:
  std::unique_ptr<FileAdios2> impl_;
};

} // namespace io
} // namespace kg

#include "FileAdios2.inl"
