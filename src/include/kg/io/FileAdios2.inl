
namespace kg
{
namespace io
{

// ======================================================================
// FileAdios2

inline FileAdios2::FileAdios2(adios2::Engine engine, adios2::IO io)
  : engine_{engine}, io_{io}
{}

inline void FileAdios2::close()
{
  engine_.Close();
}

inline void FileAdios2::performPuts()
{
  engine_.PerformPuts();
}

inline void FileAdios2::performGets()
{
  engine_.PerformGets();
}

template <typename T>
inline void FileAdios2::putVariable(const std::string& name, const T* data,
                                    const Mode launch, const Dims& shape,
                                    const Box<Dims>& selection,
                                    const Box<Dims>& memory_selection)
{
  auto v = io_.InquireVariable<T>(name);
  if (!v) {
    v = io_.DefineVariable<T>(name, shape);
  }
  if (!selection.first.empty()) {
    v.SetSelection(selection);
  }
  if (!memory_selection.first.empty()) {
    v.SetMemorySelection(memory_selection);
  }
  engine_.Put(v, data, launch);
}

template <typename T>
inline void FileAdios2::getVariable(const std::string& name, T* data,
                                    const Mode launch,
                                    const Box<Dims>& selection,
                                    const Box<Dims>& memory_selection)
{
  auto& io = const_cast<adios2::IO&>(io_); // FIXME
  auto v = io.InquireVariable<T>(name);
  if (!selection.first.empty()) {
    v.SetSelection(selection);
  }
  if (!memory_selection.first.empty()) {
    v.SetMemorySelection(memory_selection);
  }
  engine_.Get(v, data, launch);
}

struct FileAdios2::PutVariable
{
  PutVariable(FileAdios2& self, const std::string& name, Mode launch,
              const Dims& shape, const Box<Dims>& selection,
              const Box<Dims>& memory_selection)
    : self{self},
      name{name},
      launch{launch},
      shape{shape},
      selection{selection},
      memory_selection{memory_selection}
  {}

  template <typename T>
  void operator()(const T* data)
  {
    self.putVariable(name, data, launch, shape, selection, memory_selection);
  }

  FileAdios2& self;
  const std::string& name;
  Mode launch;
  const Dims& shape;
  const Box<Dims>& selection;
  const Box<Dims>& memory_selection;
};

inline void FileAdios2::putVariable(const std::string& name,
                                    TypeConstPointer data, Mode launch,
                                    const Dims& shape,
                                    const Box<Dims>& selection,
                                    const Box<Dims>& memory_selection)
{
  mpark::visit(
    PutVariable{*this, name, launch, shape, selection, memory_selection}, data);
}

struct FileAdios2::GetVariable
{
  GetVariable(FileAdios2& self, const std::string& name, Mode launch,
              const Box<Dims>& selection,
              const Box<Dims>& memory_selection)
    : self{self},
      name{name},
      launch{launch},
      selection{selection},
      memory_selection{memory_selection}
  {}

  template <typename T>
  void operator()(T* data)
  {
    self.getVariable(name, data, launch, selection, memory_selection);
  }

  FileAdios2& self;
  const std::string& name;
  Mode launch;
  const Box<Dims>& selection;
  const Box<Dims>& memory_selection;
};

inline void FileAdios2::getVariable(const std::string& name,
                                    TypePointer data, Mode launch,
                                    const Box<Dims>& selection,
                                    const Box<Dims>& memory_selection)
{
  mpark::visit(
    GetVariable{*this, name, launch, selection, memory_selection}, data);
}

template <typename T>
inline Dims FileAdios2::shapeVariable(const std::string& name) const
{
  auto& io = const_cast<adios2::IO&>(io_); // FIXME
  auto v = io.InquireVariable<T>(name);
  return v.Shape();
}

template <typename T>
inline void FileAdios2::getAttribute(const std::string& name,
                                     std::vector<T>& data)
{
  auto attr = io_.InquireAttribute<T>(name);
  assert(attr);
  data = attr.Data();
}

template <typename T>
inline void FileAdios2::putAttribute(const std::string& name, const T* data,
                                     size_t size)
{
  // if (mpiRank() != 0) { // FIXME, should we do this?
  //   return;
  // }
  auto attr = io_.InquireAttribute<T>(name);
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!\n", name.c_str());
  } else {
    io_.DefineAttribute<T>(name, data, size);
  }
}

template <typename T>
inline void FileAdios2::putAttribute(const std::string& name, const T& datum)
{
  // if (mpiRank() != 0) { // FIXME, should we do this?
  //   return;
  // }
  auto attr = io_.InquireAttribute<T>(name);
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!\n", name.c_str());
  } else {
    io_.DefineAttribute<T>(name, datum);
  }
}

} // namespace io
} // namespace kg
