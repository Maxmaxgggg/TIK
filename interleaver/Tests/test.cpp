#include "pch.h"
#include <Header.h>
#include <random>
namespace {
    /*!
     * Эталонная функция: преобразует входную строку в биты и разбивает на k групп по n бит с интерливингом
     */
    std::vector<std::string> referenceInterleaveBits(
        const std::string& input,
        std::size_t n,
        std::size_t k
    ) {
        std::string bits;
        bits.reserve(input.size() * 8);
        for (unsigned char c : input) {
            bits += std::bitset<8>(c).to_string();
        }
        /*for (size_t i = 0; i < bits.size(); i++){
            std::cout << bits[i];
            if ((i+1) % n == 0) {
                std::cout << ' ';
            }
        }
        std::cout << '\n';*/

        std::vector<std::string> groups(k);
        for (std::size_t g = 0; g < k; ++g) {
            for (std::size_t pos = g * n; pos < bits.size(); pos += n * k) {
                groups[g] += bits.substr(pos, n);
            }
        }
        /*for (std::size_t g = 0; g < k; ++g) {
            for (size_t i = 0; i < groups[g].length(); i++){
                std::cout << groups[g][i];
                if ((i + 1) % n == 0)
                    std::cout << ' ';

            }
            std::cout << '\n';
        }*/
        return groups;
    }
    std::vector<std::string> streamsToBitStrings(
        const std::vector<std::ostringstream>& out_streams
    ) {
        std::vector<std::string> result;
        result.reserve(out_streams.size());
        for (const auto& oss : out_streams) {
            std::string s = oss.str();               // получаем данные из потока
            std::string bits;
            bits.reserve(s.size() * 8);
            for (unsigned char c : s) {
                bits += std::bitset<8>(c).to_string();
            }
            result.push_back(std::move(bits));
        }
        return result;
    }
    std::string generateRandomString(std::size_t n, std::size_t k) {
        std::string result;
        result.reserve(n * k);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(32, 126);  // int вместо char

        for (std::size_t i = 0; i < n * k; ++i) {
            result.push_back(static_cast<char>(dist(gen)));  // приведение к char
        }

        return result;
    }
}

// Тестирование функции interleaveBits() против эталонной реализации
TEST(InterleaveBitsTest1, MatchesReferenceSmall) {
    std::size_t n = 15;
    std::size_t k = 3;
    std::string input = generateRandomString(n,k);
    std::istringstream in(input);
    std::vector<std::ostringstream> out(k);
    std::vector<std::string> expected = referenceInterleaveBits(input, n, k);
    
    interleave(in, out, n, k);
    std::vector<std::string> result = streamsToBitStrings(out);
    EXPECT_EQ(result, expected);
}
TEST(InterleaveBitsTest2, MatchesReferenceSmall) {
    std::size_t n = 100;
    std::size_t k = 7;
    std::string input = generateRandomString(n, k);
    std::istringstream in(input);
    std::vector<std::ostringstream> out(k);
    std::vector<std::string> expected = referenceInterleaveBits(input, n, k);

    interleave(in, out, n, k);
    std::vector<std::string> result = streamsToBitStrings(out);
    EXPECT_EQ(result, expected);
}
TEST(InterleaveBitsTest3, MatchesReferenceSmall) {
    std::size_t n = 17;
    std::size_t k = 3;
    std::string input = generateRandomString(n, k);
    std::istringstream in(input);
    std::vector<std::ostringstream> out(k);
    std::vector<std::string> expected = referenceInterleaveBits(input, n, k);

    interleave(in, out, n, k);
    std::vector<std::string> result = streamsToBitStrings(out);
    EXPECT_EQ(result, expected);
}
TEST(InterleaveBitsTest4, MatchesReferenceSmall) {
    std::size_t n = 2048;
    std::size_t k = 100;
    std::string input = generateRandomString(n, k);
    std::istringstream in(input);
    std::vector<std::ostringstream> out(k);
    //std::vector<std::string> expected = referenceInterleaveBits(input, n, k);

    interleave(in, out, n, k);
    //std::vector<std::string> result = streamsToBitStrings(out);
    EXPECT_EQ(1, 1);
}