#pragma once

#include <iosfwd>
#include <string>
#include <vector>

struct big_integer
{
    static constexpr const uint32_t base_cnt_bits = 32;
    static constexpr const uint64_t base = (static_cast<uint64_t>(1)) << base_cnt_bits;
    static constexpr const uint32_t all_bits_one = static_cast<uint32_t>(big_integer::base - 1);
    static constexpr const uint32_t buffer_base = 1'000'000'000;
    static constexpr const uint32_t buffer_base_cnt_bits = 9;

    big_integer() : big_integer(0) {}
    big_integer(big_integer const& other) = default;
    big_integer(short a) : big_integer(static_cast<long long>(a)) {}
    big_integer(unsigned short a) : big_integer(static_cast<unsigned long long>(a)) {}
    big_integer(int a) : big_integer(static_cast<long long>(a)) {}
    big_integer(unsigned int a) : big_integer(static_cast<unsigned long long>(a)) {}
    big_integer(long a) : big_integer(static_cast<long long>(a)) {}
    big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}
    big_integer(long long a);
    big_integer(unsigned long long a);
    explicit big_integer(std::string const& str);
    ~big_integer() = default;

    void swap(big_integer& other);

    big_integer& operator=(big_integer const& other) = default;

    big_integer& operator+=(big_integer const& rhs);
    big_integer& operator-=(big_integer const& rhs);
    big_integer& operator*=(big_integer const& rhs);
    big_integer& operator/=(big_integer const& rhs);
    big_integer& operator%=(big_integer const& rhs);

    big_integer& operator&=(big_integer const& rhs);
    big_integer& operator|=(big_integer const& rhs);
    big_integer& operator^=(big_integer const& rhs);

    big_integer& operator<<=(int rhs);
    big_integer& operator>>=(int rhs);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    friend bool operator==(big_integer const& a, big_integer const& b);
    friend bool operator!=(big_integer const& a, big_integer const& b);
    friend bool operator<(big_integer const& a, big_integer const& b);
    friend bool operator>(big_integer const& a, big_integer const& b);
    friend bool operator<=(big_integer const& a, big_integer const& b);
    friend bool operator>=(big_integer const& a, big_integer const& b);

    friend std::string to_string(big_integer const& a);

private:
    std::vector<uint32_t> number;
    bool is_positive;

    void trim();
    void to_twos_complement();

    struct division_result;
    static division_result division(big_integer const&, big_integer const&);
    static division_result short_division(big_integer const&, uint32_t const, bool const);

    void inverse();

    enum class comparison_result {less, equal, greater};
    static big_integer::comparison_result inverse_comparison(big_integer::comparison_result);
    big_integer::comparison_result compare(big_integer const& other) const;

    big_integer& bitwise_operation(uint32_t (*operation)(uint32_t const, uint32_t const), big_integer other);
};

struct big_integer::division_result {
    big_integer quotient;
    big_integer remainder;
};

big_integer operator+(big_integer a, big_integer const& b);
big_integer operator-(big_integer a, big_integer const& b);
big_integer operator*(big_integer a, big_integer const& b);
big_integer operator/(big_integer a, big_integer const& b);
big_integer operator%(big_integer a, big_integer const& b);

big_integer operator&(big_integer a, big_integer const& b);
big_integer operator|(big_integer a, big_integer const& b);
big_integer operator^(big_integer a, big_integer const& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(big_integer const& a, big_integer const& b);
bool operator!=(big_integer const& a, big_integer const& b);
bool operator<(big_integer const& a, big_integer const& b);
bool operator>(big_integer const& a, big_integer const& b);
bool operator<=(big_integer const& a, big_integer const& b);
bool operator>=(big_integer const& a, big_integer const& b);

std::string to_string(big_integer const& a);
std::ostream& operator<<(std::ostream& s, big_integer const& a);
