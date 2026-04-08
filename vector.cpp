#include "vector.hh"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace fast {

    // VECTOR 3
    Vector3::Vector3(double x, double y, double z) : x_(x), y_(y), z_(z)
    {}

    Vector3 operator+(const Vector3 &lhs, const Vector3 &rhs) {
        return Vector3(lhs.x_ + rhs.x_, lhs.y_ + rhs.y_, lhs.z_ + rhs.z_);
    }

    Vector3 operator-(const Vector3 &lhs, const Vector3 &rhs) {
        return Vector3(lhs.x_ - rhs.x_, lhs.y_ - rhs.y_, lhs.z_ - rhs.z_);
    }

    Vector3 Vector3::operator-() const {
        return Vector3(-x_, -y_, -z_);
    }

    double operator*(const Vector3 &lhs, const Vector3 &rhs) {
        return lhs.x_ * rhs.x_ + lhs.y_ * rhs.y_ + lhs.z_ * rhs.z_;
    }

    Vector3 operator*(const Vector3 &lhs, double scalar){
        return Vector3(lhs.x_ * scalar, lhs.y_ * scalar, lhs.z_ * scalar);
    }

    Vector3 operator*(double scalar, const Vector3 &rhs) {
        return Vector3(rhs.x_ * scalar, rhs.y_ * scalar, rhs.z_ * scalar);
    }

    Vector3 operator/(const Vector3 &lhs, double scalar) {
        return Vector3(lhs.x_ / scalar, lhs.y_ / scalar, lhs.z_ / scalar);
    }

    Vector3 operator^(const Vector3 &lhs, const Vector3 &rhs) {
        double nx = lhs.y_ * rhs.z_ - lhs.z_ * rhs.y_;
        double ny = lhs.z_ * rhs.x_ - lhs.x_ * rhs.z_;
        double nz = lhs.x_ * rhs.y_ - lhs.y_ * rhs.x_;

        return Vector3(nx, ny, nz);
    }

    bool operator==(const Vector3 &lhs, const Vector3 &rhs) {
        return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_ && lhs.z_ == rhs.z_;
    }

    bool operator!=(const Vector3 &lhs, const Vector3 &rhs){
        return !(lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_ && lhs.z_ == rhs.z_);
    }

    Vector3& Vector3::operator=(const Vector3 &other) {
        this->x_ = other.x_;
        this->y_ = other.y_;
        this->z_ = other.z_;

        return *this;
    }

    std::ostream& operator<<(std::ostream &out,Vector3 &vect) {
        out << '(' << vect.x_ << " ," << vect.y_ << " ," << vect.z_ << ')' << std::endl;
        return out;
    }

    void Vector3::normalize() {
        double norm = std::sqrt(x_*x_ + y_*y_ + z_*z_);
        x_ /= norm;
        y_ /= norm;
        z_ /= norm;
    }

    double Vector3::norm() {
        return std::sqrt(x_*x_ + y_*y_ + z_*z_);
    }


    /*
    // VECTOR 4
    Vector4::Vector4(double x, double y, double z) : x_(x), y_(y), z_(z), w_(w)
    {}
    */
}
