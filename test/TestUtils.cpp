#include "Utils.h"

#include "gtest/gtest.h"

using namespace EmbDebug;

class isHexStrTrue : public ::testing::TestWithParam<const char *> {};
class isHexStrFalse : public ::testing::TestWithParam<const char *> {};

TEST_P(isHexStrTrue, ReturnsTrue) {
  const char *str = GetParam();
  size_t len = strlen(str);
  ASSERT_TRUE(Utils::isHexStr(str, len));
}

INSTANTIATE_TEST_CASE_P(HexStrings, isHexStrTrue,
                        ::testing::Values("00", "1", "a", "A", "AB", "1A", "F",
                                          "FF", "8A", "FFFFFFFFFF",
                                          "0123456789"));

TEST_P(isHexStrFalse, ReturnsFalse) {
  const char *str = GetParam();
  size_t len = strlen(str);
  ASSERT_FALSE(Utils::isHexStr(str, len));
}

INSTANTIATE_TEST_CASE_P(NonHexStrings, isHexStrFalse,
                        ::testing::Values("1G", "ag", " ", ".", "?", "F+",
                                          "+FF", "8A 8", "FFFFZFFFFF",
                                          "0123456789_"));

typedef std::pair<int, uint8_t> char2HexCase;
class char2HexChars : public ::testing::TestWithParam<char2HexCase> {};

TEST_P(char2HexChars, ReturnsHexVal) {
  char2HexCase param = GetParam();
  ASSERT_EQ(Utils::char2Hex(param.first), param.second);
}

INSTANTIATE_TEST_CASE_P(
    Translatable, char2HexChars,
    ::testing::Values(
        char2HexCase{'0', 0}, char2HexCase{'1', 1}, char2HexCase{'2', 2},
        char2HexCase{'3', 3}, char2HexCase{'4', 4}, char2HexCase{'5', 5},
        char2HexCase{'6', 6}, char2HexCase{'7', 7}, char2HexCase{'8', 8},
        char2HexCase{'9', 9}, char2HexCase{'a', 10}, char2HexCase{'b', 11},
        char2HexCase{'c', 12}, char2HexCase{'d', 13}, char2HexCase{'e', 14},
        char2HexCase{'f', 15}, char2HexCase{'A', 10}, char2HexCase{'B', 11},
        char2HexCase{'C', 12}, char2HexCase{'D', 13}, char2HexCase{'E', 14},
        char2HexCase{'F', 15}));

class char2HexNonChars : public ::testing::TestWithParam<int> {};

// FIXME: Presently fails - can't return -1 as specified!
TEST_P(char2HexNonChars, DISABLED_ReturnsNegOne) {
  ASSERT_EQ(-1, Utils::char2Hex(GetParam()));
}

INSTANTIATE_TEST_CASE_P(NonTranslatable, char2HexNonChars,
                        ::testing::Values('g', 'G', '-', '+', 'Z', ' '));

typedef std::pair<uint8_t, char> hex2CharCase;
class hex2CharSingleDigit : public ::testing::TestWithParam<hex2CharCase> {};

TEST_P(hex2CharSingleDigit, ReturnsHexChar) {
  auto param = GetParam();
  ASSERT_EQ(Utils::hex2Char(param.first), param.second);
}

INSTANTIATE_TEST_CASE_P(
    Translatable, hex2CharSingleDigit,
    ::testing::Values(hex2CharCase{0, '0'}, hex2CharCase{1, '1'},
                      hex2CharCase{2, '2'}, hex2CharCase{3, '3'},
                      hex2CharCase{4, '4'}, hex2CharCase{5, '5'},
                      hex2CharCase{6, '6'}, hex2CharCase{7, '7'},
                      hex2CharCase{8, '8'}, hex2CharCase{9, '9'},
                      hex2CharCase{10, 'a'}, hex2CharCase{11, 'b'},
                      hex2CharCase{12, 'c'}, hex2CharCase{13, 'd'},
                      hex2CharCase{14, 'e'}, hex2CharCase{15, 'f'}));

TEST(hex2CharMultipleDigits, ReturnsNullChar) {
  for (uint8_t d = 0; d <= 239; d++)
    EXPECT_DEATH(Utils::hex2Char(d + 16), "d <= 0xf");
}
