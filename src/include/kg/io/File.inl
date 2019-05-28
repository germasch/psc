
namespace kg
{
namespace io
{

inline File::File(FileBase* impl) : impl_{impl} {}

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
                              const Extents& selection,
                              const Extents& memory_selection)
{
  FileBase::TypeConstPointer dataVar = data;
  impl_->putVariable(name, dataVar, launch, shape, selection, memory_selection);
}

template <typename T>
inline void File::getVariable(const std::string& name, T* data, Mode launch,
                              const Extents& selection,
                              const Extents& memory_selection)
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
