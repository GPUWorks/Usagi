#include <Usagi/Preprocessor/make_nested_namespace.hpp>
#include <Usagi/Preprocessor/infix_join.hpp>
#include <Usagi/Preprocessor/op.hpp>

namespace a
{
namespace b
{

constexpr int c = 1;

}
}

static_assert(YUKI_INFIX_JOIN(YUKI_OP_SCOPE, a, b, c) == 1, "infix_join");

namespace
{

YUKI_MAKE_NESTED_NAMESPACE((foo, bar, baz), (constexpr int hello = 5;))

}

static_assert(YUKI_INFIX_JOIN(YUKI_OP_SCOPE, foo, bar, baz, hello) == 5, "make_nested_namespace");

// root namespace test
namespace
{

constexpr int world = 10;

}
static_assert(YUKI_INFIX_JOIN(YUKI_OP_SCOPE) world == 10, "infix_join in root ns");

#define YUKI_INFIX_TEST_TRANSFORM(r, data, x) x * 2

static_assert(YUKI_TRANSFORM_INFIX_JOIN(YUKI_OP_ADD, YUKI_INFIX_TEST_TRANSFORM, BOOST_PP_EMPTY(), 1, 2, 3) == 12, "");
static_assert((YUKI_TRANSFORM_INFIX_JOIN(YUKI_OP_COMMA, YUKI_INFIX_TEST_TRANSFORM, BOOST_PP_EMPTY(), 1, 2, 3)) == 6, "");
