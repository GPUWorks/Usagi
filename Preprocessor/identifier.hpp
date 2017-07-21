#pragma once

#include "infix_join.hpp"
#include "unpack.hpp"
#include "op.hpp"

#define YUKI_CANONICAL_SCOPE(...) ::YUKI_INFIX_JOIN(YUKI_OP_SCOPE, __VA_ARGS__)

/**
* \brief Get fully qualified name of class.
* \param _namespaces The namespaces the class is in, surrounded by a pair of parentheses and separated by commas.
* \param _class The unqualified identifier of the class.
*/
#define YUKI_CANONICAL_CLASS_ID(_namespaces, _class) YUKI_CANONICAL_SCOPE(YUKI_HEAD_UNPACK _namespaces _class)

/**
* \brief Expand to using declaration of the namespace, if the namespace parameter is not empty.
* \param _namespaces The namespace surrounded by a pair of parentheses and separated by commas.
*/
#define YUKI_USE_NAMESPACE(_namespaces) \
    /* directly use arguments inside the if macro since none of them has commas */ \
    BOOST_PP_IF( \
        BOOST_VMD_IS_EMPTY(YUKI_UNPACK _namespaces), \
        BOOST_PP_EMPTY(), \
        YUKI_UNPACK(using namespace YUKI_CANONICAL_SCOPE(YUKI_UNPACK _namespaces);) \
    ) \
/**/