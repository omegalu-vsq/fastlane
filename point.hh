#pragma once

#include "vector.hh"
#include <iostream>

namespace fast {
    class Point3 {
        public:
            double x_;
            double y_;
            double z_;

            Point3() = default;
            Point3(double x, double y, double z);
            Point3(const Point3&) = default;
            ~Point3() = default;

            friend Vector3 operator-(const Point3 &lhs, const Point3 &rhs);
            friend bool operator==(const Point3 &lhs, const Point3 &rhs);
            friend bool operator!=(const Point3 &lhs, const Point3 &rhs);
            Point3& operator=(const Point3 &other);

            friend std::ostream& operator<<(std::ostream &out,Point3 &vect);

            friend Point3 operator+(Point3 pt, Vector3 vect);
            friend Point3 operator-(Point3 pt, Vector3 vect);
    };

    /*
    class Point4 {
        public:
            double x_;
            double y_;
            double z_;
            double w_;

            Point4(double x, double y, double z, double w);

            ~Point4();
    };
    */
}
