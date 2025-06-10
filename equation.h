#ifndef EQUATION_H
#define EQUATION_H

#include <string>

namespace calc {

struct Result
{
    std::string what; // the reason of failure
    double result{0.0};
    bool ok{true}; // true if no issues happened, otherwise - false
};

Result calculate(const char *equation);

} // // namespace calc

#endif // EQUATION_H
