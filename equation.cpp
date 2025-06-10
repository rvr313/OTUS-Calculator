#include "equation.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <set>
#include <stack>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace calc {

// -------------------------------------------------------------------------------------------------

enum class Operation
{
    un_min,
    add,
    sub,
    mul,
    div,
    pow,
    sqrt
};

using Value = std::variant<Operation, std::string, double>;

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace calc


namespace {

// -------------------------------------------------------------------------------------------------

// This function converts the string to a number only if the full string presents a number.
bool str_to_number(const char* str, double& number)
{
    char* endptr = const_cast<char*>(str); // Need const_cast to satisfy strtod().
    double temp_number = strtod(str, &endptr);
    if (endptr != str && *endptr == '\0')
    {
        number = temp_number;
        return true;
    }
    return false;
}

// -------------------------------------------------------------------------------------------------

using Tokens = std::vector<std::string>;

Tokens getTokens(const char* expression)
{
    // Sanity check.
    if (expression == nullptr)
        return {};

    auto is_symbol_token = [](char c)
    {
        switch (c)
        {
        case '-':
        case '+':
        case '*':
        case '/':
        case '^':
        case '(':
        case ')':
            return true;
        }
        return false;
    };

    const char* c = expression;
    Tokens tokens;

    while (*c != '\0')
    {
        while (isspace(*c))
            ++c;

        const char* start_token_character = c;
        std::string token;

        while (*c != '\0')
        {
            bool token_found = false;

            if (start_token_character == c && is_symbol_token(*c))
            {
                token_found = true;
                token.push_back(*c);
                ++c;
            }
            else if (start_token_character == c && (isdigit(*c) || *c == '.'))
            {
                token_found = true;               
                char* endptr{};
                std::strtod(c, &endptr);
                token = std::string(c, endptr - c);
                c += token.size();
            }
            else if (isspace(*c) || is_symbol_token(*c))
            {
                token_found = true;
                // End of word.
            }
            else
            {
                token.push_back(*c);
                ++c;
                // End of word.            }
            }

            if (token_found)
                break;
        }

        if (!token.empty())
            tokens.push_back(token);
    }

    return tokens;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//      RPN_Builder
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// -------------------------------------------------------------------------------------------------

// This calss build reverse Polish notation from the given expression tokens. The used algorithms is
// https://en.wikipedia.org/wiki/Shunting_yard_algorithm by Dijkstra.
// The algoritms also checks a correctness of the given expression tokens.
class RPN_Builder
{
public:
    explicit RPN_Builder();

    const std::vector<calc::Value>& reverse_polish_notation() const;
    const std::string& what() const;

    bool operator()(const Tokens& expression_tokens);

private:
    enum class TokType : unsigned int
    {
        number,
        var,
        open,
        close,
        un_min,
        add,
        sub,
        mul,
        div,
        pow,
        sqrt,
        undefined,
    };

    // Converts input token to token type.
    TokType to_token_type(const std::string& token, double& number) const;

    // Converts private TokType to public calc::Operation.
    calc::Operation to_operation(TokType ttype) const;

    // Returns priority of math operations and parentheses.
    uint32_t priority(TokType ttype) const;

    // Checks correctness of tokens order - previous and current tokens.
    bool is_correct_token_order(TokType prev_ttype, TokType curr_ttype) const;

    // These functions fill 'what_happened_message_' and return false.
    bool extra_parentheses_happened(const std::string& type);
    bool incorrect_token_order_happened(
        TokType         prev_ttype,
        const std::string& prev_token,
        TokType         curr_ttype,
        const std::string& curr_token);
    bool no_operands_found();

    // Keep built reverse Polish notation.
    std::vector<calc::Value> rp_notation_;

    // Map keeps tokens and its corresponding types, unspecified tokens are numbers or variables.
    std::unordered_map<std::string, TokType> tokens_map_;

    // Map keeps the correct order of two nearest tokens. The key is a previous token type and
    // value is a set of permitted current token types.
    std::unordered_map<TokType, std::set<TokType>> prev_curr_token_map_;

    // Keeps error message for upper caller.
    std::string what_happened_message_;
};

// -------------------------------------------------------------------------------------------------

// +-------------+--------------- +
// |   Previous  | Next permitted |
// |  token type |   token type   |      Table shows the correct order of two nearest token types -
// +-------------+----------------+      previous and current, it is kept in 'prev_curr_token_map_'.
// |             |       +        |      It is used to check correctness of input expression tokens
// |    number   |       - (sub)  |      from which the reverse Polish notation is built.
// |   variable  |       *        |
// |      )      |       /        |      The undefined token is make sense for previous token and
// |             |       ^        |      means that the next token after undefined previous is the
// |             |       )        |      first token in expression.
// +-------------+----------------+
// |    +        |                |
// |    - (un)   |    sqrt        |
// |    - (sub)  |    number      |
// |    *        |   variable     |
// |    /        |      (         |
// |    ^        |                |
// +-------------+--------------- +
// |    sqrt     |      (         |
// +-------------+--------------- +
// |             |      - (un)    |
// |  undefined  |    sqrt        |
// |      (      |    number      |
// |             |   variable     |
// |             |      (         |
// +-------------+--------------- +
RPN_Builder::RPN_Builder()
{
    tokens_map_["("]    = TokType::open;
    tokens_map_[")"]    = TokType::close;
    tokens_map_["+"]    = TokType::add;
    tokens_map_["-"]    = TokType::sub;
    tokens_map_["*"]    = TokType::mul;
    tokens_map_["/"]    = TokType::div;
    tokens_map_["^"]    = TokType::pow;
    tokens_map_["sqrt"] = TokType::sqrt;

    // Fill 'prev_curr_token_map_' to help faster checking of token order correstness.
    for (TokType ttype : {TokType::number, TokType::var, TokType::close})
    {
        prev_curr_token_map_[ttype] = {
            TokType::add,
            TokType::sub,
            TokType::mul,
            TokType::div,
            TokType::pow,
            TokType::close
        };
    }
    std::initializer_list<TokType> op_list = {
        TokType::un_min,
        TokType::add,
        TokType::sub,
        TokType::mul,
        TokType::div,
        TokType::pow
    };
    for (TokType ttype : op_list)
    {
        prev_curr_token_map_[ttype] = {
            TokType::number,
            TokType::var,
            TokType::open,
            TokType::sqrt
        };
    }
    for (TokType ttype : {TokType::undefined, TokType::open})
    {
        prev_curr_token_map_[ttype] = {
            TokType::number,
            TokType::var,
            TokType::open,
            TokType::un_min,
            TokType::sqrt
        };
    }
    prev_curr_token_map_[TokType::sqrt] = {TokType::open};
}

// -------------------------------------------------------------------------------------------------

const std::vector<calc::Value>& RPN_Builder::reverse_polish_notation() const
{
    return rp_notation_;
}

// -------------------------------------------------------------------------------------------------

const std::string& RPN_Builder::what() const
{
    return what_happened_message_;
}

// -------------------------------------------------------------------------------------------------

bool RPN_Builder::operator()(const Tokens& expression_tokens)
{
    rp_notation_.clear();
    std::string prev_token;
    TokType prev_ttype = TokType::undefined;
    TokType curr_ttype = TokType::undefined;
    std::stack<TokType> operations;
    size_t operand_count = 0;

    for (const std::string& token : expression_tokens)
    {
        double number = 0.0;
        curr_ttype = to_token_type(token, number);
        bool maybe_un_min = prev_ttype == TokType::undefined || prev_ttype == TokType::open;
        if (maybe_un_min && curr_ttype == TokType::sub)
            curr_ttype = TokType::un_min;
        if (!is_correct_token_order(prev_ttype, curr_ttype))
            return incorrect_token_order_happened(prev_ttype, prev_token, curr_ttype, token);

        switch (curr_ttype)
        {
        case TokType::number:
        {
            calc::Value value = number;
            rp_notation_.push_back(value);
            ++operand_count;
            break;
        }
        case TokType::var:
        {
            calc::Value value = token;
            rp_notation_.push_back(value);
            ++operand_count;
            break;
        }
        case TokType::sqrt:
        case TokType::open:
        {
            operations.push(curr_ttype);
            break;
        }
        case TokType::close:
        {
            while(!operations.empty() && operations.top() != TokType::open)
            {
                calc::Value value = to_operation(operations.top());
                rp_notation_.push_back(value);
                operations.pop();
            }
            if (operations.empty())
                return extra_parentheses_happened("close");
            else
                operations.pop(); // Open parenthesis is found.

            break;
        }
        case TokType::add:
        case TokType::sub:
        case TokType::mul:
        case TokType::div:
        case TokType::pow:
        case TokType::un_min:
        {
            uint32_t curr_prior = priority(curr_ttype);
            uint32_t top_prior = operations.empty() ? 0 : priority(operations.top());
            while (top_prior >= curr_prior)
            {
                calc::Value value = to_operation(operations.top());
                rp_notation_.push_back(value);
                operations.pop();
                top_prior = operations.empty() ? 0 : priority(operations.top());
            }
            operations.push(curr_ttype);
            break;
        }
        case TokType::undefined:
        {
            // We should not be here.
            throw std::runtime_error("Incorrect expression");
        }
        }
        prev_ttype = curr_ttype;
        prev_token = token;
    }

    if (operand_count == 0)
        return no_operands_found();

    while (!operations.empty())
    {
        if (operations.top() == TokType::open)
            return extra_parentheses_happened("open");

        calc::Value value = to_operation(operations.top());
        rp_notation_.push_back(value);
        operations.pop();
    }

    return true;
}

// -------------------------------------------------------------------------------------------------

RPN_Builder::TokType RPN_Builder::to_token_type(const std::string& token, double& number) const
{
    auto ttype_iter = tokens_map_.find(token);
    if (ttype_iter != tokens_map_.end())
        return ttype_iter->second;

    if (str_to_number(token.c_str(), number))
    {
        return TokType::number;
    }

    return TokType::var;
}

// -------------------------------------------------------------------------------------------------

calc::Operation RPN_Builder::to_operation(TokType ttype) const
{
    switch (ttype)
    {
    case TokType::un_min:
        return calc::Operation::un_min;
    case TokType::add:
        return calc::Operation::add;
    case TokType::sub:
        return calc::Operation::sub;
    case TokType::mul:
        return calc::Operation::mul;
    case TokType::div:
        return calc::Operation::div;
    case TokType::pow:
        return calc::Operation::pow;
    case TokType::sqrt:
        return calc::Operation::sqrt;
    case TokType::number:
    case TokType::var:
    case TokType::open:
    case TokType::close:
    case TokType::undefined:
        break;
    }

    throw std::runtime_error("Incorrect expression");
    return calc::Operation::un_min;
}

// -------------------------------------------------------------------------------------------------

uint32_t RPN_Builder::priority(TokType ttype) const
{
    switch (ttype)
    {
    case TokType::open:
    case TokType::close:
        return 0;
    case TokType::add:
    case TokType::sub:
        return 1;
    case TokType::mul:
    case TokType::div:
        return 2;
    case TokType::pow:
        return 3;
    case TokType::un_min:
        return 4;
    case TokType::sqrt:
        return 5;
    case TokType::var:
    case TokType::number:
    case TokType::undefined:
        break;
    }
    throw std::runtime_error("Incorrect expression");
    return 0;
}

// -------------------------------------------------------------------------------------------------

bool RPN_Builder::is_correct_token_order(TokType prev_ttype, TokType curr_ttype) const
{
    if(curr_ttype == TokType::undefined) {
        throw std::runtime_error("Incorrect expression");
    }

    const std::set<TokType>& curr_ttype_set = prev_curr_token_map_.at(prev_ttype);
    return curr_ttype_set.find(curr_ttype) != curr_ttype_set.end();
}

// -------------------------------------------------------------------------------------------------

bool RPN_Builder::extra_parentheses_happened(const std::string& type)
{
    std::stringstream ss;
    ss << "Found extra " << type << " parentheses.";
    what_happened_message_ = ss.str();

    return false;
}

// -------------------------------------------------------------------------------------------------

bool
RPN_Builder::incorrect_token_order_happened(
    TokType         prev_ttype,
    const std::string& prev_token,
    TokType         curr_ttype,
    const std::string& curr_token)
{
    if (curr_ttype == TokType::undefined) {
        throw std::runtime_error("Incorrect expression");
    }

    auto ttype_to_str = [](TokType ttype, const std::string& token) -> std::string
    {
        switch (ttype)
        {
        case TokType::number:    return "number " + token;
        case TokType::var:       return "variable " + token;
        case TokType::open:      return "open parenthesis";
        case TokType::close:     return "close parenthesis";
        case TokType::un_min:    return "unary minus '-'";
        case TokType::add:       return "addition sign '+'";
        case TokType::sub:       return "subtraction sign '-'";
        case TokType::mul:       return "multiplication sign '*'";
        case TokType::div:       return "division sign '/'";
        case TokType::pow:       return "power sign '^'";
        case TokType::sqrt:      return "sqrt function";
        case TokType::undefined: break;
        }
        return "undefined";
    };

    std::stringstream ss;
    ss << "Incorrect order of operands and operations in the expression.\n" << "The "
       << ttype_to_str(curr_ttype, curr_token) << " cannot be ";
    if (prev_ttype == TokType::undefined)
        ss << "the first in an expression.";
    else
    {
        ss << "after the " << ttype_to_str(prev_ttype, prev_token);
    }
    what_happened_message_ = ss.str();

    return false;
}

// -------------------------------------------------------------------------------------------------

bool RPN_Builder::no_operands_found()
{
    what_happened_message_ = "Expression does not contain any operands.";

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Calculator
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// -------------------------------------------------------------------------------------------------

double pop_value(std::stack<double>& stack)
{
    if (stack.empty()) {
        throw std::runtime_error("Incorrect expression");
    }

    double value = stack.top();
    stack.pop();
    return value;
}

// -------------------------------------------------------------------------------------------------

template <class F>
double binary_operation(F op, std::stack<double>& stack)
{
    return op(pop_value(stack), pop_value(stack));
}

// -------------------------------------------------------------------------------------------------

template <class F>
double unary_operation(F op, std::stack<double>& stack)
{
    return op(pop_value(stack));
}

double calculate(calc::Operation op, std::stack<double>& stack)
{
    switch (op)
    {
    case calc::Operation::add:
        return binary_operation([](double a, double b){ return a + b; }, stack);
    case calc::Operation::sub:
        return binary_operation([](double a, double b){ return a - b; }, stack);
    case calc::Operation::mul:
        return binary_operation([](double a, double b){ return a * b; }, stack);
    case calc::Operation::div:
        return binary_operation([](double a, double b){
            if (b == 0.0) {
                throw std::runtime_error("Divizion on zero is not defined");
            }
            return a / b;
        }, stack);
    case calc::Operation::pow:
        return binary_operation([](double a, double b){ return pow(a, b); }, stack);
    case calc::Operation::un_min:
        return unary_operation([](double a){ return -a; }, stack);
    case calc::Operation::sqrt:
        return unary_operation([](double a){ return sqrt(a); }, stack);
    }

    throw std::runtime_error("Incorrect expression");
    return 0;
}


} // anonymous namespace

namespace calc {

Result calculate(const char *equation)
{
    Tokens tokens = getTokens(equation);
    RPN_Builder builder;
    try {
        if (!builder(tokens)) {
            return {"Incorrect expression", 0.0, false};
        }

        std::stack<double> stack;
        for (const auto& item: builder.reverse_polish_notation()) {
            std::visit(overloaded{
                [&stack](Operation arg) { stack.push(::calculate(arg, stack)); },
                [&stack](double arg) { stack.push(arg); },
                [&stack](const std::string& arg) { ; }
            }, item);
        }
        return {{}, stack.top(), true};
    }
    catch (const std::exception &e) {
        return {e.what(), 0.0, false};
    }
    catch (...) {
        return {"Something went wrong", 0.0, false};
    }
}

} // namespace calc

