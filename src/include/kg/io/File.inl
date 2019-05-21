
namespace kg
{
namespace io
{

inline File::File(adios2::Engine engine, adios2::IO io)
  : impl_{new FileAdios2{engine, io}}
{}

inline void File::close()
{
  impl_->close();
}

inline void File::performPuts()
{
  impl_->performPuts();
}

inline void File::performGets()
{
  impl_->performGets();
}

template <typename T>
inline void File::putVariable(const std::string& name, const T* data,
                              Mode launch, const Dims& shape,
                              const Box<Dims>& selection,
                              const Box<Dims>& memory_selection)
{
  FileBase::TypeConstPointer dataVar = data;
  impl_->putVariable(name, dataVar, launch, shape, selection, memory_selection);
}

template <typename T>
inline void File::getVariable(const std::string& name, T* data, Mode launch,
                              const Box<Dims>& selection,
                              const Box<Dims>& memory_selection)
{
  FileBase::TypePointer dataVar = data;
  impl_->getVariable(name, dataVar, launch, selection, memory_selection);
}

inline Dims File::shapeVariable(const std::string& name) const
{
  return impl_->shapeVariable(name);
}

template <typename T>
inline void File::getAttribute(const std::string& name, T* data)
{
  FileBase::TypePointer dataVar = data;
  impl_->getAttribute(name, dataVar);
}

template <typename T>
inline void File::putAttribute(const std::string& name, const T* data,
                               size_t size)
{
  FileBase::TypeConstPointer dataVar = data;
  impl_->putAttribute(name, dataVar, size);
}

inline size_t File::sizeAttribute(const std::string& name) const
{
  return impl_->sizeAttribute(name);
}

} // namespace io
} // namespace kg
