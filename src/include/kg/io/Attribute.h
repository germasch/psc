
#pragma once

#include "Engine.h"

#include <vec3.hxx>

namespace kg
{
namespace io
{
namespace detail
{

// ======================================================================
// detail::Attribute
//
// Handles T being one of the base adios2 types (or arrays thereof)

template <typename T>
struct Attribute
{
  Attribute(const std::string& name, Engine& engine) : name_{name} {}

  void put(Engine& writer, const T* data, size_t size)
  {
    if (writer.mpiRank() != 0) { // FIXME, should we do this?
      return;
    }
    auto attr = writer.io_.InquireAttribute<T>(name_);
    if (attr) {
      mprintf("attr '%s' already exists -- ignoring it!", name_.c_str());
    } else {
      writer.io_.DefineAttribute<T>(name_, data, size);
    }
  }

  void put(Engine& writer, const T& datum)
  {
    if (writer.mpiRank() != 0) { // FIXME, should we do this?
      return;
    }
    auto attr = writer.io_.InquireAttribute<T>(name_);
    if (attr) {
      mprintf("attr '%s' already exists -- ignoring it!", name_.c_str());
    } else {
      writer.io_.DefineAttribute<T>(name_, datum);
    }
  }

  void get(Engine& reader, std::vector<T>& data)
  {
    auto attr = reader.io_.InquireAttribute<T>(name_);
    assert(attr);
    data = attr.Data();
  }

  void get(Engine& reader, T& datum)
  {
    auto attr = reader.io_.InquireAttribute<T>(name_);
    assert(attr);
    auto data = attr.Data();
    assert(data.size() == 1);
    datum = data[0];
  }

protected:
  const std::string name_;
};

}; // namespace detail

// ======================================================================
// Attribute<T>
//
// single value

template <typename T, typename Enable = void>
class Attribute
{
  using DataType = T;

public:
  Attribute(const std::string& name, Engine& engine) : attr_{name, engine} {}

  void put(Engine& writer, const T& value, const Mode launch = Mode::Deferred)
  {
    attr_.put(writer, value);
  }

  void get(Engine& reader, T& value, const Mode launch = Mode::Deferred)
  {
    attr_.get(reader, value);
  }

private:
  detail::Attribute<DataType> attr_;
};

// ======================================================================
// Attribute<std::vector>

template <class T>
class Attribute<std::vector<T>>
{
  using DataType = T;

public:
  using value_type = std::vector<T>;

  Attribute(const std::string& name, Engine& engine) : attr_{name, engine} {}

  void put(Engine& writer, const value_type& vec,
           const Mode launch = Mode::Deferred)
  {
    attr_.put(writer, vec.data(), vec.size());
  }

  void get(Engine& reader, value_type& vec, const Mode launch = Mode::Deferred)
  {
    attr_.get(reader, vec);
  }

private:
  detail::Attribute<DataType> attr_;
};

// ======================================================================
// Attribute<Vec3>

template <class T>
class Attribute<Vec3<T>>
{
  using DataType = T;

public:
  using value_type = Vec3<T>;

  Attribute(const std::string& name, Engine& engine) : attr_{name, engine} {}

  void put(Engine& writer, const value_type& vec,
           const Mode launch = Mode::Deferred)
  {
    attr_.put(writer, vec.data(), 3);
  }

  void get(Engine& reader, value_type& data, const Mode launch = Mode::Deferred)
  {
    std::vector<DataType> vals;
    attr_.get(reader, vals);
    data = {vals[0], vals[1], vals[2]};
  }

private:
  detail::Attribute<DataType> attr_;
};

} // namespace io
} // namespace kg
