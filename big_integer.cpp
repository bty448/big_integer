#include "big_integer.h"
#include <cstddef>
#include <string>
#include <ostream>
#include <stdexcept>
#include <algorithm>

big_integer::big_integer(long long a)
    : is_positive(a >= 0)
{
    unsigned long long temp;
    if (a == std::numeric_limits<long long>::min()) {
        temp = static_cast<unsigned long long>(~a) + 1;
    } else {
        temp = std::abs(a);
    }
    number.resize(2);
    number[1] = temp / big_integer::base;
    number[0] = temp % big_integer::base;
    trim();
}

big_integer::big_integer(unsigned long long a)
    : is_positive(true)
{
    number.resize(2);
    number[1] = a / big_integer::base;
    number[0] = a % big_integer::base;
    trim();
}

big_integer::big_integer(std::string const& str)
    : big_integer(0)
{
    if (str == "-" || str == "+" || str.empty()) {
        throw std::invalid_argument("empty number given to the constructor");
    }
    size_t last_digit_index = (str[0] == '-' || str[0] == '+') ? 1 : 0;
    size_t first_step_size = (str.size() - last_digit_index) % big_integer::buffer_base_cnt_bits;
    size_t step_size = first_step_size;
    for (size_t i = last_digit_index; i < str.size();) {
        uint32_t cur_coef = 0;
        uint32_t tens_power = 1;
        for (size_t j = step_size; j > 0; --j, tens_power *= 10) {
            char cur_char = str[i + j - 1];
            if (cur_char < '0' || cur_char > '9') {
                throw std::invalid_argument("non-numerical string given to the constructor");
            }
            cur_coef += (cur_char - '0') * tens_power;
        }
        i += step_size;
        if (step_size == first_step_size) {
            step_size = big_integer::buffer_base_cnt_bits;
        }
        *this *= big_integer::buffer_base;
        *this += cur_coef;
    }
    if (str[0] == '-') {
        is_positive = false;
    }
}

void big_integer::swap(big_integer& other) {
    std::swap(number, other.number);
    std::swap(is_positive, other.is_positive);
}

void big_integer::trim() {
    while (number.size() > 1 && number.back() == 0) {
        number.pop_back();
    }
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    if (!is_positive && rhs.is_positive) {
        is_positive = true;
        if (*this <= rhs) {
            *this = rhs - *this;
        } else {
            *this -= rhs;
            is_positive = false;
        }
        return *this;
    }
    if (is_positive && !rhs.is_positive) {
        big_integer rhs_copy(rhs);
        rhs_copy.is_positive = true;
        if (*this <= rhs_copy) {
            *this = rhs_copy - *this;
            is_positive = false;
        } else {
            *this -= rhs_copy;
        }
        return *this;
    }
    uint64_t carry = 0;
    uint64_t cur_num_place = 0;
    size_t cnt_num_places = std::max(number.size(), rhs.number.size());
    number.resize(cnt_num_places);
    for (size_t i = 0; i < cnt_num_places; i++) {
        cur_num_place = static_cast<uint64_t>(number[i])
                        + ((i < rhs.number.size()) ? static_cast<uint64_t>(rhs.number[i]) : 0)
                        + carry;
        number[i] = cur_num_place % big_integer::base;
        carry = cur_num_place / big_integer::base;
    }
    if (carry > 0) {
        number.push_back(carry);
    }
    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    if (is_positive && !rhs.is_positive) {
        big_integer rhs_copy(rhs);
        rhs_copy.is_positive = true;
        return *this += rhs_copy;
    }
    if (!is_positive && rhs.is_positive) {
        big_integer rhs_copy(rhs);
        rhs_copy.is_positive = false;
        return *this += rhs_copy;
    }
    if ((is_positive && *this < rhs) || (!is_positive && *this > rhs)) {
        big_integer rhs_copy(rhs);
        rhs_copy.swap(*this);
        *this -= rhs_copy;
        is_positive = !is_positive;
        return *this;
    }
    int64_t carry = 0;
    int64_t cur_num_place = 0;
    for (size_t i = 0; i < number.size(); ++i) {
        cur_num_place = static_cast<int64_t>(number[i])
                        - ((i < rhs.number.size()) ? static_cast<int64_t>(rhs.number[i]) : 0)
                        - carry;
        if (cur_num_place < 0) {
            cur_num_place += big_integer::base;
            carry = 1;
        } else {
            carry = 0;
        }
        number[i] = cur_num_place;
    }
    trim();
    return *this;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    big_integer result;
    result.number.resize(number.size() + rhs.number.size());
    for (size_t i = 0; i < rhs.number.size(); ++i) {
        uint64_t carry = 0, cur_num_place = 0;
        for (size_t j = 0; j < number.size(); ++j) {
            cur_num_place = static_cast<uint64_t>(result.number[i + j])
                            + static_cast<uint64_t>(number[j]) * static_cast<uint64_t>(rhs.number[i])
                            + carry;
            result.number[i + j] = cur_num_place & big_integer::all_bits_one;
            carry = cur_num_place >> big_integer::base_cnt_bits;
        }
        if (carry != 0) {
            result.number[i + number.size()] = carry;
        }
    }
    result.trim();
    result.is_positive = is_positive == rhs.is_positive;
    result.swap(*this);
    return *this;
}

big_integer::division_result big_integer::short_division(big_integer const& dividend,
                                                         uint32_t divisor, bool divisor_is_positive) {
    if (divisor == 0) {
        throw std::runtime_error("Division by zero");
    }
    big_integer quotient(dividend);
    uint64_t cur_num_place = 0;
    uint64_t carry = 0;
    for (size_t i = dividend.number.size(); i > 0; --i) {
        cur_num_place = static_cast<uint64_t>(dividend.number[i - 1])
                        + carry * big_integer::base;
        quotient.number[i - 1] = cur_num_place / divisor;
        carry = cur_num_place % divisor;
    }
    big_integer remainder(carry);
    quotient.is_positive = dividend.is_positive == divisor_is_positive;
    remainder.is_positive = dividend.is_positive;
    quotient.trim();
    return {quotient, remainder};
}

big_integer::division_result big_integer::division(big_integer const& dividend, big_integer const& divisor) {
    if (divisor.number.size() == 1) {
        return short_division(dividend, divisor.number[0], divisor.is_positive);
    }
    big_integer dividend_copy = dividend;
    big_integer divisor_copy = divisor;
    dividend_copy.is_positive = divisor_copy.is_positive = true;
    if (dividend_copy < divisor_copy) {
        return {0, dividend};
    }
    uint32_t k = 0;
    uint32_t significant_place = divisor.number.back();
    while (significant_place <= (big_integer::all_bits_one >> 1)) { //at most 32 iterations
        significant_place <<= 1;
        ++k;
    }
    dividend_copy <<= k;
    divisor_copy <<= k;
    uint32_t n = dividend_copy.number.size();
    uint32_t m = divisor_copy.number.size();
    big_integer quotient;
    quotient.number.resize(n - m + 1, 0);
    big_integer shifted_divisor = divisor_copy << (big_integer::base_cnt_bits * (n - m));
    if (dividend_copy >= shifted_divisor) {
        quotient.number[n - m] = 1;
        dividend_copy -= shifted_divisor;
    }
    shifted_divisor >>= big_integer::base_cnt_bits;
    uint64_t prediction_divisor = static_cast<uint64_t>(divisor_copy.number.back());
    for (size_t j = n - m; j > 0; --j, shifted_divisor >>= big_integer::base_cnt_bits) {
        uint64_t prediction_dividend = ((m + j - 1 < dividend_copy.number.size())
                                            ? static_cast<uint64_t>(dividend_copy.number[m + j - 1])
                                            : 0)
                                       * big_integer::base
                                       + ((m + j - 2 < dividend_copy.number.size())
                                            ? static_cast<uint64_t>(dividend_copy.number[m + j - 2])
                                            : 0);
        uint64_t prediction = std::min(prediction_dividend / prediction_divisor, big_integer::base - 1);
        dividend_copy -= shifted_divisor * prediction;
        while (!dividend_copy.is_positive) { // it's proven that this works for at most 2 iterations...
            --prediction;
            dividend_copy += shifted_divisor;
        }
        quotient.number[j - 1] = prediction;
    }
    //dividend_copy has become a remainder
    dividend_copy >>= k;
    quotient.is_positive = dividend.is_positive == divisor.is_positive;
    dividend_copy.is_positive = dividend.is_positive;
    quotient.trim();
    dividend_copy.trim();
    return {quotient, dividend_copy};
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
    *this = division(*this, rhs).quotient;
    return *this;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    *this = division(*this, rhs).remainder;
    return *this;
}

void big_integer::to_twos_complement() {
    if (!is_positive) {
        inverse();
        --*this;
    }
}

big_integer& big_integer::bitwise_operation(uint32_t (*operation)(const uint32_t, const uint32_t),
                                            big_integer other) {
    to_twos_complement();
    other.to_twos_complement();
    if (number.size() < other.number.size()) {
        number.resize(other.number.size(), is_positive ? 0 : big_integer::all_bits_one);
    }
    if (other.number.size() < number.size()) {
        other.number.resize(number.size(), other.is_positive ? 0 : big_integer::all_bits_one);
    }
    for (size_t i = 0; i < number.size(); ++i) {
        number[i] = operation(number[i], other.number[i]);
    }
    is_positive = operation(!is_positive, !other.is_positive) == 0;
    to_twos_complement();
    trim();
    return *this;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    return bitwise_operation([](uint32_t const a, uint32_t const b) {return a & b;}, rhs);
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    return bitwise_operation([](uint32_t const a, uint32_t const b) {return a | b;}, rhs);
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    return bitwise_operation([](uint32_t const a, uint32_t const b) {return a ^ b;}, rhs);
}

big_integer& big_integer::operator<<=(int rhs) {
    to_twos_complement();
    uint32_t cnt_insert = rhs / big_integer::base_cnt_bits, rem = rhs % big_integer::base_cnt_bits;
    if (cnt_insert > 0) {
        number.insert(number.begin(), cnt_insert, 0);
    }
    if (rem > 0) {
        const uint32_t cnt_rest_bits = big_integer::base_cnt_bits - rem;
        uint32_t carry = 0, preserve_neg = 0;
        if (!is_positive) {
            preserve_neg = big_integer::all_bits_one << rem;
        }
        for (size_t i = cnt_insert; i < number.size(); ++i) {
            uint32_t temp_carry = number[i] >> cnt_rest_bits;
            number[i] = (number[i] << rem) + carry;
            carry = temp_carry;
        }
        number.push_back(carry);
        number.back() += preserve_neg;
    }
    to_twos_complement();
    trim();
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    to_twos_complement();
    uint32_t cnt_erase = rhs / big_integer::base_cnt_bits, rem = rhs % big_integer::base_cnt_bits;
    if (cnt_erase > 0) {
        if (cnt_erase >= number.size()) {
            *this = 0;
            return *this;
        }
        number.erase(number.begin(), number.begin() + cnt_erase);
    }
    if (rem > 0) {
        const uint32_t cnt_rest_bits = big_integer::base_cnt_bits - rem;
        uint32_t carry = 0;
        if (!is_positive) {
            carry = big_integer::all_bits_one << cnt_rest_bits;
        }
        for (size_t i = number.size(); i > 0; --i) {
            uint32_t temp_carry = number[i - 1] << cnt_rest_bits;
            number[i - 1] = (number[i - 1] >> rem) + carry;
            carry = temp_carry;
        }
    }
    to_twos_complement();
    trim();
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    big_integer result = big_integer(*this);
    result.is_positive = !result.is_positive;
    return result;
}

big_integer big_integer::operator~() const {
    big_integer res(*this);
    res.number.push_back(0);
    res.to_twos_complement();
    res.inverse();
    if (res.number.back() == big_integer::all_bits_one) {
        res.is_positive = false;
    }
    res.to_twos_complement();
    res.trim();
    return res;
}

void big_integer::inverse() {
    std::for_each(number.begin(), number.end(), [](uint32_t& a) {a = ~a;});
}

big_integer& big_integer::operator++() {
    if (!is_positive) {
        is_positive = true;
        --*this;
        is_positive = false;
        return *this;
    }
    trim();
    for (size_t i = 0; i < number.size(); i++) {
        if (number[i] == big_integer::all_bits_one) {
            number[i] = 0;
        } else {
            ++number[i];
            break;
        }
    }
    if (number.size() > 1 && number.back() == 0) {
        number.push_back(1);
    }
    return *this;
}

big_integer big_integer::operator++(int) {
    big_integer ret(*this);
    ++*this;
    return ret;
}

big_integer& big_integer::operator--() {
    if (!is_positive || (number.size() == 1 && number[0] == 0)) {
        is_positive = true;
        ++*this;
        is_positive = false;
        return *this;
    }
    for (size_t i = 0; i < number.size(); ++i) {
        if (number[i] == 0) {
            number[i] = big_integer::all_bits_one;
        } else {
            --number[i];
            break;
        }
    }
    trim();
    return *this;
}

big_integer big_integer::operator--(int) {
    big_integer ret(*this);
    --*this;
    return ret;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

big_integer::comparison_result big_integer::inverse_comparison(big_integer::comparison_result comparison) {
    switch (comparison) {
        case big_integer::comparison_result::greater : return big_integer::comparison_result::less;
        case big_integer::comparison_result::less : return big_integer::comparison_result::greater;
        default : return comparison;
    }
}

big_integer::comparison_result big_integer::compare(big_integer const& other) const {
    if (number.size() == 1 && number[0] == 0 && other.number.size() == 1 && other.number[0] == 0) {
        return big_integer::comparison_result::equal;
    }
    if (is_positive && !other.is_positive) {
        return big_integer::comparison_result::greater;
    }
    if (!is_positive && other.is_positive) {
        return big_integer::comparison_result::less;
    }
    big_integer::comparison_result result = big_integer::comparison_result::greater;
    if (!is_positive && !other.is_positive) {
        result = big_integer::comparison_result::less;
    }
    if (number.size() < other.number.size()) {
        return big_integer::inverse_comparison(result);
    } else if (number.size() > other.number.size()) {
        return result;
    } else {
        for (size_t i = number.size(); i > 0; --i) {
            if (number[i - 1] < other.number[i - 1]) {
                return big_integer::inverse_comparison(result);
            } else if (number[i - 1] > other.number[i - 1]) {
                return result;
            }
        }
        return big_integer::comparison_result::equal;
    }
}

bool operator==(big_integer const& a, big_integer const& b) {
    return a.compare(b) == big_integer::comparison_result::equal;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    return a.compare(b) == big_integer::comparison_result::less;
}

bool operator>(big_integer const& a, big_integer const& b) {
    return a.compare(b) == big_integer::comparison_result::greater;
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

std::string to_string(big_integer const& a) {
    if (a == 0) {
        return "0";
    }
    big_integer copy(a);
    copy.is_positive = true;
    std::vector<std::string> vs;
    size_t res_size = (a.is_positive ? 0 : 1);
    while (copy != 0) {
        big_integer::division_result div_res = big_integer::short_division(copy, big_integer::buffer_base, true);
        std::string str_remainder = std::to_string(div_res.remainder.number[0]);
        if (div_res.quotient == 0) {
            vs.push_back(str_remainder);
            res_size += str_remainder.size();
        } else {
            vs.push_back(std::string(big_integer::buffer_base_cnt_bits - str_remainder.size(), '0')
                             .append(str_remainder));
            res_size += big_integer::buffer_base_cnt_bits;
        }
        copy = div_res.quotient;
    }
    std::string res(res_size, '0');
    if (!a.is_positive) {
        res[0] = '-';
    }
    size_t vs_ind = vs.size() - 1;
    for (size_t i = (a.is_positive ? 0 : 1), j = 0; i < res_size; ++i, ++j) {
        if (j == vs[vs_ind].size()) {
            --vs_ind;
            j = 0;
        }
        res[i] = vs[vs_ind][j];
    }
    return res;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}
