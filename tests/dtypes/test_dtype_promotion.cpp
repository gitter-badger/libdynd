//
// Copyright (C) 2011-13 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <cmath>
#include "inc_gtest.hpp"

#include "dynd/dtype_promotion.hpp"

using namespace std;
using namespace dynd;

template<class S, class T, class U>
void dtype_promotion_matches_cxx_test(S, T, U) {
    EXPECT_EQ(ndt::make_dtype<U>(), promote_dtypes_arithmetic(ndt::make_dtype<S>(), ndt::make_dtype<T>()));
    if (ndt::make_dtype<U>() != promote_dtypes_arithmetic(ndt::make_dtype<S>(), ndt::make_dtype<T>()))
        cout << "S: " << ndt::make_dtype<S>() << ", T: " << ndt::make_dtype<T>() << ", U: " << ndt::make_dtype<U>() << "\n";
}

template<class S, class T>
void dtype_promotion_matches_cxx() {
    S a = S();
    T b = T();
    dtype_promotion_matches_cxx_test(a, b, a + b);
}

#define TEST_ALL_SECOND(first) \
    dtype_promotion_matches_cxx<first, dynd_bool>(); \
    dtype_promotion_matches_cxx<first, int8_t>(); \
    dtype_promotion_matches_cxx<first, int16_t>(); \
    dtype_promotion_matches_cxx<first, int32_t>(); \
    dtype_promotion_matches_cxx<first, int64_t>(); \
    dtype_promotion_matches_cxx<first, uint8_t>(); \
    dtype_promotion_matches_cxx<first, uint16_t>(); \
    dtype_promotion_matches_cxx<first, uint32_t>(); \
    dtype_promotion_matches_cxx<first, uint64_t>(); \
    dtype_promotion_matches_cxx<first, float>(); \
    dtype_promotion_matches_cxx<first, double>()

#define TEST_ALL_FIRST() \
    TEST_ALL_SECOND(dynd_bool); \
    TEST_ALL_SECOND(int8_t); \
    TEST_ALL_SECOND(int16_t); \
    TEST_ALL_SECOND(int32_t); \
    TEST_ALL_SECOND(int64_t); \
    TEST_ALL_SECOND(uint8_t); \
    TEST_ALL_SECOND(uint16_t); \
    TEST_ALL_SECOND(uint32_t); \
    TEST_ALL_SECOND(uint64_t); \
    TEST_ALL_SECOND(float); \
    TEST_ALL_SECOND(double)

TEST(DTypePromotion, MatchesCxx) {
    TEST_ALL_FIRST();
}

#undef TEST_ALL_FIRST
#undef TEST_ALL_SECOND

TEST(DTypePromotion, IntWithComplex) {
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int8_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int16_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int32_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int64_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint8_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint16_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint32_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint64_t>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());

    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int8_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int16_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int32_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<int64_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint8_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint16_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint32_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<uint64_t>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
}

TEST(DTypePromotion, FloatWithComplex) {
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<float>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<float>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<double>(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<double>(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<float> >(), ndt::make_dtype<float>()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<double> >(), ndt::make_dtype<float>()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<float> >(), ndt::make_dtype<double>()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<double> >(), ndt::make_dtype<double>()), ndt::make_dtype<complex<double> >());
}

TEST(DTypePromotion, ComplexWithComplex) {
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<float> >(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<float> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<float> >(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<double> >(), ndt::make_dtype<complex<float> >()), ndt::make_dtype<complex<double> >());
    EXPECT_EQ(promote_dtypes_arithmetic(ndt::make_dtype<complex<double> >(), ndt::make_dtype<complex<double> >()), ndt::make_dtype<complex<double> >());
}
