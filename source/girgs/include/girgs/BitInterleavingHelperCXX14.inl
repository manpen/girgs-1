// This code performs compile-time checks of the BitInterleaving Helper.
// However, it relies on C++14 features. As even the preprocessor stumbles
// over some features (i.e. ' in number literals), the cannot even include
// this file unless we are compiling for C++14 (or above).

// bitInterleavingMask
static_assert(bitInterleavingMask(8) == 0x00FF00FF'00FF00FF, "UnitTest failed");
static_assert(bitInterleavingMask(1) == 0x55555555'55555555, "UnitTest failed");

// 1d (nothing to do)
static_assert(BitInterleavingHelper<1>::interleave({0}, 32) == 0, "UnitTest failed");
static_assert(BitInterleavingHelper<1>::interleave({123456789}, 32) == 123456789, "UnitTest failed");

// 2d
static_assert(BitInterleavingHelper<2>::interleave({0b00000000'00000000, 0b00000000'00000000}, 0)
              == 0b00000000'00000000'00000000'00000000, "UnitTest failed");
static_assert(BitInterleavingHelper<2>::interleave({0b11111111'11111111, 0b00000000'00000000}, 0)
              == 0b01010101'01010101'01010101'01010101, "UnitTest failed");
static_assert(BitInterleavingHelper<2>::interleave({0b00000000'00000000, 0b11111111'11111111}, 0)
              == 0b10101010'10101010'10101010'10101010, "UnitTest failed");
static_assert(BitInterleavingHelper<2>::interleave({0b11111111'00000000, 0b00000000'11111111}, 0)
              == 0b01010101'01010101'10101010'10101010, "UnitTest failed");

// 3d
static_assert(BitInterleavingHelper<3>::interleave({0b00'00000000, 0b00'00000000, 0b00'00000000}, 0)
              == 0b00000000'00000000'00000000'00000000, "UnitTest failed");
static_assert(BitInterleavingHelper<3>::interleave({0b11'11111111, 0b00'00000000, 0b00'00000000}, 0)
              == 0b00001001'00100100'10010010'01001001, "UnitTest failed");
static_assert(BitInterleavingHelper<3>::interleave({0b00'00000000, 0b11'11111111, 0b00'00000000}, 0)
              == 0b00010010'01001001'00100100'10010010, "UnitTest failed");
static_assert(BitInterleavingHelper<3>::interleave({0b00'00000000, 0b00'00000000, 0b11'11111111}, 0)
              == 0b00100100'10010010'01001001'00100100, "UnitTest failed");

// 4d
static_assert(BitInterleavingHelper<4>::interleave({0b00000000, 0b00000000, 0b00000000, 0b00000000}, 0)
              == 0b00000000'00000000'00000000'00000000, "UnitTest failed");
static_assert(BitInterleavingHelper<4>::interleave({0b11111111, 0b00000000, 0b00000000, 0b00000000}, 0)
              == 0b00010001'00010001'00010001'00010001, "UnitTest failed");
static_assert(BitInterleavingHelper<4>::interleave({0b00000000, 0b11111111, 0b00000000, 0b00000000}, 0)
              == 0b00100010'00100010'00100010'00100010, "UnitTest failed");
static_assert(BitInterleavingHelper<4>::interleave({0b00000000, 0b00000000, 0b11111111, 0b00000000}, 0)
              == 0b01000100'01000100'01000100'01000100, "UnitTest failed");
static_assert(BitInterleavingHelper<4>::interleave({0b00000000, 0b00000000, 0b00000000, 0b11111111}, 0)
              == 0b10001000'10001000'10001000'10001000, "UnitTest failed");

// 5d
static_assert(BitInterleavingHelper<5>::interleave({0b000000, 0b000000, 0b000000, 0b000000, 0b000000}, 6)
              == 0b00000000'00000000'00000000'00000000, "UnitTest failed");
static_assert(BitInterleavingHelper<5>::interleave({0b111111, 0b000000, 0b000000, 0b000000, 0b000000}, 6)
              == 0b00000001'00001000'010000100'00100001, "UnitTest failed");
static_assert(BitInterleavingHelper<5>::interleave({0b000000, 0b111111, 0b000000, 0b000000, 0b000000}, 6)
              == 0b00000010'00010000'100001000'01000010, "UnitTest failed");
static_assert(BitInterleavingHelper<5>::interleave({0b000000, 0b000000, 0b111111, 0b000000, 0b000000}, 6)
              == 0b00000100'00100001'000010000'10000100, "UnitTest failed");
static_assert(BitInterleavingHelper<5>::interleave({0b000000, 0b000000, 0b000000, 0b111111, 0b000000}, 6)
              == 0b00001000'01000010'000100001'00001000, "UnitTest failed");
static_assert(BitInterleavingHelper<5>::interleave({0b000000, 0b000000, 0b000000, 0b000000, 0b111111}, 6)
              == 0b00010000'10000100'001000010'00010000, "UnitTest failed");
