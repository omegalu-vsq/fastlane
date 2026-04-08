#include "point.hh"

#include <iostream>
#include <cstdlib>

namespace fast {

    Point3::Point3(double x, double y, double z) : x_(x), y_(y), z_(z)
    {}

    Vector3 operator-(const Point3 &lhs, const Point3 &rhs) {
        return Vector3(lhs.x_ - rhs.x_, lhs.y_ - rhs.y_, lhs.z_ - rhs.z_);
    }

    bool operator==(const Point3 &lhs, const Point3 &rhs) {
        return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_ && lhs.z_ == rhs.z_;
    }

    bool operator!=(const Point3 &lhs, const Point3 &rhs){
        return !(lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_ && lhs.z_ == rhs.z_);
    }

    Point3& Point3::operator=(const Point3 &other) {
        this->x_ = other.x_;
        this->y_ = other.y_;
        this->z_ = other.z_;

        return *this;
    }

    std::ostream& operator<<(std::ostream &out, Point3 &pt) {
        out << '(' << pt.x_ << " ," << pt.y_ << " ," << pt.z_ << ')' << std::endl;
        return out;
    }

    Point3 operator+(Point3 pt, Vector3 vect) {
        return Point3(pt.x_ + vect.x_, pt.y_ + vect.y_, pt.z_ + vect.z_);
    }

    Point3 operator-(Point3 pt, Vector3 vect) {
        return Point3(pt.x_ - vect.x_, pt.y_ - vect.y_, pt.z_ - vect.z_);
    }

    /*
    Point4::Point4(double x, double y, double z, double w) : x_(x), y_(y), z_(z), w_(w)
    {}
    */


}
