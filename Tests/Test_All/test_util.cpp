#include <gtest/gtest.h>

#include <Usagi/Engine/Utility/TypeCast.hpp>

using namespace usagi;

namespace
{
struct A { virtual ~A() = default; };
struct B : A { };
struct C { virtual ~C() = default; };
}

TEST(TypeCastTest, IsInstanceOfTest)
{
	B b;
	C c;
	EXPECT_TRUE(is_instance_of<A>(&b));
	EXPECT_TRUE(is_instance_of<B>(&b));
	EXPECT_FALSE(is_instance_of<C>(&b));
	EXPECT_FALSE(is_instance_of<A>(&c));
	EXPECT_FALSE(is_instance_of<B>(&c));
	EXPECT_TRUE(is_instance_of<C>(&c));
}

TEST(TypeCastTest, IsInstanceOfConstTest)
{
	const B b;
	const C c;
	EXPECT_TRUE(is_instance_of<A>(&b));
	EXPECT_TRUE(is_instance_of<B>(&b));
	EXPECT_FALSE(is_instance_of<C>(&b));
	EXPECT_FALSE(is_instance_of<A>(&c));
	EXPECT_FALSE(is_instance_of<B>(&c));
	EXPECT_TRUE(is_instance_of<C>(&c));
}