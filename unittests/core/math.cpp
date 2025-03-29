#include <catch_amalgamated.hpp>
#include <lightwave/math.hpp>

using namespace lightwave;

// clang-format off

TEST_CASE( "Vector tests", "[math]" ) {
    const Vector a { 1,  2, 3 };
    const Vector b { 0, -1, 1 };

    SECTION( "Addition" ) {
        REQUIRE( a + b == Vector { 1, 1, 4 });
    }
    SECTION( "Subtraction" ) {
        REQUIRE( a - b == Vector { 1, 3, 2 });
    }
    SECTION( "Dot product" ) {
        REQUIRE( a.dot(b) == 1 );
    }
    SECTION( "Cross product" ) {
        REQUIRE( a.cross(b) == Vector { 5, -1, -1 });
    }
}

TEST_CASE( "Matrix tests", "[math]" ) {
    const Matrix3x3 m {
        1,  0, 1,
        0,  2, 2,
        1, -1, 1,
    };
    const Vector a { 1, 2, 3 };

    SECTION( "Vector product" ) {
        REQUIRE( m * a == Vector { 4, 10, 2 });
    }
    SECTION( "Matrix product" ) {
        REQUIRE( m * m == Matrix3x3 {
            2, -1, 2,
            2,  2, 6,
            2, -3, 0,
        });
    }
    SECTION( "Determinant" ) {
        REQUIRE( m.determinant() == 2 );
    }
    SECTION( "Transpose" ) {
        REQUIRE( m.transpose() == Matrix3x3 {
            1, 0,  1,
            0, 2, -1,
            1, 2,  1,
        });
    }
}
