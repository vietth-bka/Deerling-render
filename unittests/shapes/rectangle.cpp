#include <catch_amalgamated.hpp>
#include <samplers/independent.cpp>
#include <shapes/rectangle.cpp>

using namespace lightwave;

TEST_CASE( "Rectangle tests", "[rectangle]" ) {
    const Properties props;
    const Rectangle rectangle { props }; // poor little rectangle has no properties

    SECTION( "Rectangle reports correct bounds" ) {
        const Bounds expectedBounds { Point{ -1, -1, 0 }, Point{ +1, +1, 0 } };
        REQUIRE( rectangle.getBoundingBox() == expectedBounds );
    }

    SECTION( "Rectangle reports correct intersections" ) {
        Intersection its;
        Independent sampler { props };

        SECTION( "Rectangle can be hit" ) {
            const Ray ray { Point(-1, 0, -1), Vector(1, 0, 1).normalized() };
            REQUIRE( rectangle.intersect(ray, its, sampler) );
            REQUIRE( its.t == std::sqrt(2.0f) );
        }

        SECTION( "Rectangle can be missed" ) {
            const Ray ray { Point(-1, 2, -1), Vector(1, 0, 1).normalized() };
            REQUIRE( !rectangle.intersect(ray, its, sampler) );
            REQUIRE( std::isinf(its.t) );
        }
    }
}
