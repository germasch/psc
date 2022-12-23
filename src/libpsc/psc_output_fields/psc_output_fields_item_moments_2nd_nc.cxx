
#include "fields.hxx"
#include <psc/moment.hxx>

#include <math.h>

// ======================================================================
// rho

template <typename MF, typename D>
class Moment_rho_2nd_nc : public ItemMomentCRTP<Moment_rho_2nd_nc<MF, D>, MF>
{
public:
  using Base = ItemMomentCRTP<Moment_rho_2nd_nc<MF, D>, MF>;
  using Mfields = MF;
  using dim_t = D;
  using real_t = typename Mfields::real_t;
  using moment_type =
    psc::moment::moment_rho<psc::deposit::code::Deposit2ndNc, dim_t>;

  explicit Moment_rho_2nd_nc(const Grid_t& grid) : Base{grid} {}

  template <typename Mparticles>
  auto operator()(const Mparticles& mprts)
  {
    Int3 ib = -mprts.grid().ibn;
    auto mres = psc::mflds::zeros<real_t>(mprts.grid(), 1, ib);
    moment_type{}(mres, ib, mprts);
    Base::bnd_.add_ghosts(mprts.grid(), mres, ib);
    return mres;
  }

  auto storage() = delete;
};
