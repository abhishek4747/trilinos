/* @HEADER@ */
/* @HEADER@ */

#ifndef POINT_H
#define POINT_H

#include "SundanceDefs.hpp"
#include "SundanceExceptions.hpp"
#include "Teuchos_Utils.hpp"



namespace SundanceStdMesh
{
  using namespace SundanceUtils;
  using std::string;

  using std::ostream;

  /**
   * \ingroup UserLevelGeometry
   * Point represents a spatial point.
   */
  class Point
    {
    public:
      /* empty ctor */
      inline Point();
      inline Point(const double& x);
      inline Point(const double& x, const double& y);
      inline Point(const double& x, const double& y, const double& z);
      inline Point(const Point& other);
      inline Point& operator=(const Point& other);

      inline int dim() const {return dim_;}
      inline double& operator[](int i);
      inline const double& operator[](int i) const ;
      inline void resize(int i);

      /* reflexive arithmetic operators */

      inline Point& operator+=(const Point& p) ;
      inline Point& operator-=(const Point& p) ;
      inline Point& operator*=(const double& a) ;
      inline Point& operator/=(const double& a) ;

      /* unary plus and minus */

      inline Point operator+() const ;
      inline Point operator-() const ;

      /* binary operations with Points */

      inline Point operator+(const Point& p) const ;
      inline Point operator-(const Point& p) const ;
      inline double operator*(const Point& p) const ;

      /* binary operations with doubles */

      inline Point operator*(const double& a) const ;
      inline Point operator/(const double& a) const ;

      inline string toString() const ;

      static bool unitTest() ;

    protected:
      void boundsCheck(int i) const ;
      int dim_;
      double x_[3];
    };
}


namespace std
{
  ostream& operator<<(ostream& os, const SundanceStdMesh::Point& p);

}

namespace SundanceStdMesh
{
  inline Point operator*(const double& a, const Point& p);

  inline Point cross(const Point& x, const Point& y);



  inline Point::Point()
    : dim_(0)
    {;}

  inline Point::Point(const double& x)
    : dim_(1)
    {
      x_[0] = x;
    }

  inline Point::Point(const double& x, const double& y)
    : dim_(2)
    {
      x_[0] = x;
      x_[1] = y;
    }

  inline Point::Point(const double& x, const double& y, const double& z)
    : dim_(3)
    {
      x_[0] = x;
      x_[1] = y;
      x_[2] = z;
    }

  inline Point::Point(const Point& other)
    : dim_(other.dim_)
    {
      for (int i=0; i<dim_; i++) x_[i] = other.x_[i];
    }

  Point& Point::operator=(const Point& other)
    {
      if (&other==this) return *this;

      dim_ = other.dim_;
      for (int i=0; i<dim_; i++) x_[i] = other.x_[i];
      return *this;
    }

  double& Point::operator[](int i)
    {
#ifndef NOBOUNDSCHECK
      boundsCheck(i);
#endif
      return x_[i];
    }

  const double& Point::operator[](int i) const
    {
#ifndef NOBOUNDSCHECK
      boundsCheck(i);
#endif
      return x_[i];
    }

  void Point::resize(int i)
    {
#ifndef NOBOUNDSCHECK
      TEST_FOR_EXCEPTION(i < 0 || i>3, RuntimeError,
                         "void Point::resize() invalid dimension");
#endif
      dim_ = i;
    }

  Point& Point::operator+=(const Point& p)
    {
      TEST_FOR_EXCEPTION(p.dim() != dim_, RuntimeError,
                         "Point::operator+=() dimension mismatch "
                         "operands are: " << *this << " and " 
                         << p );

      for (int i=0; i<dim_; i++) x_[i] += p.x_[i];
      return *this;
    }

  Point& Point::operator-=(const Point& p)
    {

      TEST_FOR_EXCEPTION(p.dim() != dim_, RuntimeError,
                         "Point::operator-=() dimension mismatch "
                         "operands are: " << *this << " and " 
                         << p );
      
      for (int i=0; i<dim_; i++) x_[i] -= p.x_[i];
      return *this;
    }

  Point& Point::operator*=(const double& a)
    {
      for (int i=0; i<dim_; i++) x_[i] *= a;
      return *this;
    }

  Point& Point::operator/=(const double& a)
    {
      for (int i=0; i<dim_; i++) x_[i] /= a;
      return *this;
    }

  Point Point::operator-() const
    {
      Point rtn(*this);
      for (int i=0; i<dim_; i++) rtn.x_[i] = -rtn.x_[i];
      return rtn;
    }

  Point Point::operator+() const
    {
      return *this;
    }

  Point Point::operator+(const Point& p) const
    {
      Point rtn(*this);
      rtn += p;
      return rtn;
    }

  Point Point::operator-(const Point& p) const
    {
      Point rtn(*this);
      rtn -= p;
      return rtn;
    }

  double Point::operator*(const Point& p) const
    {
      double rtn = 0.0;

      TEST_FOR_EXCEPTION(p.dim() != dim_, RuntimeError,
                         "Point::operator*() dimension mismatch "
                         "operands are: " << *this << " and " 
                         << p );
      
      for (int i=0; i<dim_; i++) rtn += x_[i]*p.x_[i];
      return rtn;
    }

  Point Point::operator*(const double& a) const
    {
      Point rtn(*this);
      rtn *= a;
      return rtn;
    }

  Point Point::operator/(const double& a) const
    {
      Point rtn(*this);
      rtn /= a;
      return rtn;
    }

  Point operator*(const double& a, const Point& p)
    {
      return p.operator*(a);
    }

  inline Point cross(const Point& a, const Point& b)
  {
    return Point( a[1]*b[2] - b[1]*a[2], 
                  -a[0]*b[2] + a[2]*b[0],
                  a[0]*b[1] - a[1]*b[0]);
  }

  inline string Point::toString() const
    {
      string rtn = "{";
      for (int i=0; i<dim(); i++)
        {
          rtn += Teuchos::toString(x_[i]);
          if (i<dim()-1) rtn += ", ";
        }
      rtn += "}";
      return rtn;
    }
}

namespace Teuchos
{
  inline string toString(const SundanceStdMesh::Point& x)
    {
      return x.toString();
    }
}

namespace std
{
  ostream& operator<<(ostream& os, const SundanceStdMesh::Point& p);
}


#endif





