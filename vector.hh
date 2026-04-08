#pragma once
#include <cstdlib>
#include <iostream>

namespace fast {
    class Vector3 {
        public:
            double x_;
            double y_;
            double z_;

            Vector3() = default;
            Vector3(double x, double y, double z);
            ~Vector3() = default;

            friend Vector3 operator+(const Vector3 &lhs, const Vector3 &rhs);
            friend Vector3 operator-(const Vector3 &lhs, const Vector3 &rhs);
            friend double operator*(const Vector3 &lhs, const Vector3 &rhs);
            friend Vector3 operator*(const Vector3 &lhs, double scalar);
            friend Vector3 operator*(double scalar, const Vector3 &rhs);
            friend Vector3 operator/(const Vector3 &lhs, double scalar);
            friend Vector3 operator^(const Vector3 &lhs, const Vector3 &rhs);
            friend bool operator==(const Vector3 &lhs, const Vector3 &rhs);
            friend bool operator!=(const Vector3 &lhs, const Vector3 &rhs);
            Vector3 operator-() const;
            Vector3& operator=(const Vector3 &other);

            void normalize();
            double norm();

            friend std::ostream& operator<<(std::ostream &out,Vector3 &vect);
    };

    /*
    class Vector4 {
        public:
            double x_;
            double y_;
            double z_;
            double w_;

            Vector4(double x, double y, double z, double w);
            ~Vector4();

            friend Vector4 operator+(const Vector4 &lhs, const Vector4 &rhs);
            friend Vector4 operator-(const Vector4 &lhs, const Vector4 &rhs);
            friend Vector4 operator*(const Vector4 &lhs, const Vector4 &rhs);
            friend Vector4 operator*(const Vector4 &lhs, double scalar);
            friend Vector4 operator*(double scalar, const Vector4 &rhs);
            friend Vector4 operator^(const Vector4 &lhs, const Vector4 &rhs);

            friend std::ostream& operator<<(std::ostream &out,Vector4 &vect);
    };
    */
}
