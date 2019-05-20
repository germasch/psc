
#include <adios2.h>

#include <iostream>
#include <utility>
#include <cmath>

void test1()
{
  using T = unsigned long;

  adios2::ADIOS ad("adios.xml", MPI_COMM_WORLD, adios2::DebugON);

  {
    adios2::IO io_writer = ad.DeclareIO("io_writer");
    auto var = io_writer.DefineVariable<T>("var");
    auto writer = io_writer.Open("test", adios2::Mode::Write);
    
    unsigned long test = 99;
    //writer.Put(var, test);
    writer.Close();
  }

  {
    adios2::IO io_reader = ad.DeclareIO("io_reader");
    auto reader = io_reader.Open("test", adios2::Mode::Read);
    auto var = io_reader.InquireVariable<T>("var");
    assert(static_cast<bool>(var));
    
    T test;
    reader.Get(var, test);
    reader.Close();

    assert(test == 99);
  }
}

void test2()
{
  using T = int32_t;

  adios2::ADIOS ad("adios.xml", MPI_COMM_WORLD, adios2::DebugON);

  {
    adios2::IO io_writer = ad.DeclareIO("io_writer");
    auto var = io_writer.DefineVariable<T>("var");
    auto var2 = io_writer.DefineVariable<T>("var2");
    auto writer = io_writer.Open("test", adios2::Mode::Write);
    io_writer.RemoveVariable("var");
    auto var3 = io_writer.DefineVariable<T>("var3");
    std::cout << "var2 name " << var2.Name() << std::endl;
    std::cout << "var3 name " << var3.Name() << std::endl;
    
    T test = 99;
    writer.Put(var3, test);
    writer.Close();
  }
}

struct Base
{
  std::string type;
};

struct A : Base
{
  int val = 99;
};

struct B : Base
{
  std::string val = "hi";
};

struct Print
{
  template <typename T>
  void operator()(T& t, const std::string& msg)
  {
    std::cout << msg << t.val << std::endl;
  }
};

template <typename Visitor, typename... Args>
void visit(Visitor&& v, const Base& p, Args&&... args)
{
  if (p.type == "A") {
    v(reinterpret_cast<const A&>(p), std::forward<Args>(args)...);
  } else if (p.type == "B") {
    v(reinterpret_cast<const B&>(p), std::forward<Args>(args)...);
  }
};

void test3()
{
  A a;
  a.type = "A";

  B b;
  b.type = "B";

  Base& pa = a;
  visit(Print{}, pa, "pa = ");
  Base& pb = b;
  visit(Print{}, pb, "pb = ");
}

template<typename T>
struct Test
{
  void print(T t)
  {
    std::cout << t;
  }

  template<typename U = T>
  typename std::enable_if<std::is_arithmetic<U>::value, T>::type func(T t)
  {
    return sqrtf(t);
  }
};

template class Test<int>;
template class Test<float>;
template class Test<std::string>;

// ----------------------------------------------------------------------

#if 0
#include "MetaStuff/include/Meta.h"
#include "MetaStuff/example/Person.h"

template <typename Class, typename F,
	  typename = typename std::enable_if<meta::isRegistered<Class>()>::type>
void doForAllMembers(F&& f);

// version for non-registered classes (to generate less template stuff)
template <typename Class, typename F,
	  typename = typename std::enable_if_t<!meta::isRegistered<Class>()>::type,
	  typename = void>
void doForAllMembers(F&& f);

template <typename Class, typename F, typename>
void doForAllMembers(F&& f)
{
  meta::detail::for_tuple(std::forward<F>(f), meta::getMembers<Class>());
}

using Tuple = std::tuple<std::string, int>;

template <typename F>
void _doForAllMembers(Tuple t, F&& f)
{
  f(std::get<0>(t));
  f(std::get<1>(t));
}

template<typename T>
void print(T val)
{
  std::cout << "Val " << val << std::endl;
}

void test4()
{
  Person person;
  person.age = 40;
  person.name = "Me";
  doForAllMembers<Person>(
    [&person](const auto& member)
    {
      using MemberT = meta::get_member_type<decltype(member)>;
      std::cout << "* " << member.getName() <<
        ", value = " << member.get(person) <<
        ", type = " << typeid(MemberT).name() << '\n';
    });

  Tuple t = {"hi", 99};

  _doForAllMembers(
		   t,
		   [](const auto& val)
		   {
		     print(val);
		     std::cout << "* " << val << std::endl;
		   });
  
}
#endif

// ======================================================================

// mp_list

template<class... T> struct mp_list {};

// mp_rename

template<class A, template<class...> class B> struct mp_rename_impl;

template<template<class...> class A, class... T, template<class...> class B>
struct mp_rename_impl<A<T...>, B>
{
  using type = B<T...>;
};

template<class A, template<class...> class B>
using mp_rename = typename mp_rename_impl<A, B>::type;

// mp_size

template<class... T> using mp_length = std::integral_constant<std::size_t, sizeof...(T)>;

template<class L> using mp_size = mp_rename<L, mp_length>;

// mp_push_front

template<class L, class... T> struct mp_push_front_impl;

template<template<class...> class L, class... U, class... T>
    struct mp_push_front_impl<L<U...>, T...>
{
    using type = L<T..., U...>;
};

template<class L, class... T>
    using mp_push_front = typename mp_push_front_impl<L, T...>::type;

// add_pointer

template<class T> using add_pointer = T*;

// mp_transform

template<template<class...> class F, class L> struct mp_transform_impl;

template<template<class...> class F, class L>
    using mp_transform = typename mp_transform_impl<F, L>::type;

template<template<class...> class F, template<class...> class L, class... T>
    struct mp_transform_impl<F, L<T...>>
{
    using type = L<F<T>...>;
};

//

template <class T>
struct Variable
{};

using MyTypes = mp_list<double, std::string>;

using MyTuple = mp_rename<MyTypes, std::tuple>;

using MyVars = mp_transform<Variable, MyTuple>;

void test5()
{
  MyTuple vars = { 1.5, "Hi" };
  std::cout << "size = " << mp_size<MyTypes>::value << std::endl;
  using T = mp_push_front<MyTypes, float>;
  std::cout << typeid(T).name() << std::endl;
  std::cout << typeid(MyVars).name() << std::endl;
}


int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  auto comm = MPI_COMM_WORLD;
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  test5();
  return 0;

  //auto configfile = "adios2.xml";
  adios2::ADIOS ad(comm, adios2::DebugON);

  adios2::IO bpio = ad.DeclareIO("writer");

  const size_t n = 10;
  const size_t N = n * size;
  double T[n];
  for (int i = 0; i < n; i++) {
    T[i] = i + n * rank;
  }

  auto varSize = bpio.DefineVariable<int>("Size");
  auto varRank = bpio.DefineVariable<int>("Rank");
  auto varT = bpio.DefineVariable<double>(
        "T",
        // Global dimensions
        {N},
        // starting offset of the local array in the global space
        {n*rank},
        // local size, could be defined later using SetSelection()
        {n});

  // Promise that we are not going to change the variable sizes nor add new
  // variables
  //bpio.LockDefinitions();
  
  auto bpWriter = bpio.Open("test.bp", adios2::Mode::Write, comm);

  if (rank == 0) {
    bpWriter.Put(varSize, size);
  }

  bpWriter.Put(varRank, rank);

#if 0 // FIXME, crashes bpls -D
  auto varRank2 = bpio.DefineVariable<int>("Rank2", {adios2::LocalValueDim});
  bpWriter.Put(varRank2, rank);
#endif

#if 0 // local array, doesn't seem to work right, doesn't show in bpls unless we skip "T" below
  auto varTlocal = bpio.DefineVariable<double>("Tlocal", {}, {}, {n});
  double Tlocal[n] = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
  bpWriter.Put(varTlocal, Tlocal);
#endif
  
#if 1
  for (int s = 0; s < 3; s++) {
    bpWriter.BeginStep();
    bpWriter.Put(varT, T);
    bpWriter.EndStep();
  }
#endif
  
  bpWriter.Close();  

  MPI_Finalize();
  return 0;
}
