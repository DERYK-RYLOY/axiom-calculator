// ============================================================================
//  Advanced Calculator  -  C++17 (portable: Windows / macOS / Linux / Termux)
// ----------------------------------------------------------------------------
//  A console calculator built around a recursive-descent expression parser.
//  FEATURES
//   * Precedence, parentheses, unary signs, postfix factorial, right-assoc ^
//   * ~50 scientific functions (trig, hyperbolic, logs, roots, stats, integer)
//   * Constants, user variables, automatic `ans`, memory slot
//   * DEG/RAD mode, expression history, help, variadic sum/average
//   * Uses ONLY the C++ standard library -> compiles anywhere C++17 exists.
// ============================================================================

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace calc {

constexpr double PI = 3.14159265358979323846;

// ---------------------------------------------------------------------------
//  Tokeniser
// ---------------------------------------------------------------------------
enum class TokType { Number, Ident, Op, LParen, RParen, Comma };

struct Token {
    TokType type;
    std::string text;
    double number = 0.0;
};

static bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
static bool isIdentPart(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::vector<Token> tokenize(const std::string& input) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    while (i < input.size()) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (std::isspace(c)) { ++i; continue; }

        // Number (supports decimals and scientific notation: 1.5e-3)
        if (std::isdigit(c) || (c == '.' && i + 1 < input.size() &&
                                std::isdigit(static_cast<unsigned char>(input[i + 1])))) {
            std::size_t start = i;
            while (i < input.size() &&
                   (std::isdigit(static_cast<unsigned char>(input[i])) || input[i] == '.')) ++i;
            if (i < input.size() && (input[i] == 'e' || input[i] == 'E')) {
                std::size_t expPos = i;
                ++i;
                if (i < input.size() && (input[i] == '+' || input[i] == '-')) ++i;
                if (i < input.size() && std::isdigit(static_cast<unsigned char>(input[i]))) {
                    while (i < input.size() && std::isdigit(static_cast<unsigned char>(input[i]))) ++i;
                } else {
                    i = expPos;
                }
            }
            std::string text = input.substr(start, i - start);
            Token t{TokType::Number, text, 0.0};
            try {
                std::size_t used = 0;
                t.number = std::stod(text, &used);
                if (used != text.size()) throw std::invalid_argument("trailing");
            } catch (...) {
                throw std::runtime_error("Invalid number: " + text);
            }
            tokens.push_back(t);
            continue;
        }

        // Identifier (function / constant / variable / command-like name)
        if (isIdentStart(static_cast<char>(c))) {
            std::size_t start = i;
            while (i < input.size() && isIdentPart(input[i])) ++i;
            tokens.push_back({TokType::Ident, input.substr(start, i - start), 0.0});
            continue;
        }

        switch (input[i]) {
            case '+': case '-': case '*': case '/': case '%':
            case '^': case '!': case '=':
                tokens.push_back({TokType::Op, std::string(1, input[i]), 0.0}); ++i; break;
            case '(': tokens.push_back({TokType::LParen, "(", 0.0}); ++i; break;
            case ')': tokens.push_back({TokType::RParen, ")", 0.0}); ++i; break;
            case ',': tokens.push_back({TokType::Comma, ",", 0.0}); ++i; break;
            default:
                throw std::runtime_error(std::string("Unexpected character: '") + input[i] + "'");
        }
    }
    return tokens;
}

// ---------------------------------------------------------------------------
//  Function table (arity -1 means "variadic")
// ---------------------------------------------------------------------------
struct FuncDef { int arity; };

static const std::map<std::string, FuncDef>& functions() {
    static const std::map<std::string, FuncDef> table = {
        // Trigonometry / inverse / hyperbolic
        {"sin",{1}}, {"cos",{1}}, {"tan",{1}}, {"sec",{1}}, {"csc",{1}}, {"cot",{1}},
        {"asin",{1}}, {"acos",{1}}, {"atan",{1}},
        {"sinh",{1}}, {"cosh",{1}}, {"tanh",{1}},
        {"asinh",{1}}, {"acosh",{1}}, {"atanh",{1}},
        // Powers, roots, logs, exponentials
        {"sqrt",{1}}, {"cbrt",{1}}, {"abs",{1}},
        {"ln",{1}}, {"log",{1}}, {"log2",{1}}, {"exp",{1}},
        {"pow",{2}}, {"root",{2}}, {"logn",{2}}, {"hypot",{2}},
        // Rounding / utility
        {"floor",{1}}, {"ceil",{1}}, {"round",{1}}, {"trunc",{1}}, {"sign",{1}},
        {"deg",{1}}, {"rad",{1}}, {"clamp",{3}}, {"lerp",{3}},
        // Integer / combinatorics
        {"gcd",{2}}, {"lcm",{2}}, {"nCr",{2}}, {"nPr",{2}},
        // Statistics (variadic)
        {"sum",{-1}}, {"avg",{-1}}, {"mean",{-1}}, {"max",{-1}}, {"min",{-1}},
        // Random
        {"random",{0}}, {"randint",{2}},
    };
    return table;
}

static bool isFunction(const std::string& name) {
    return functions().count(name) != 0;
}

// ---------------------------------------------------------------------------
//  Numeric helpers
// ---------------------------------------------------------------------------
static double factorial(double x) {
    if (x < 0 || std::floor(x) != x)
        throw std::runtime_error("factorial (!) needs a non-negative integer");
    if (x > 170) throw std::runtime_error("factorial result is too large");
    double result = 1.0;
    for (int i = 2; i <= static_cast<int>(x); ++i) result *= i;
    return result;
}

static long long toInt(double x, const std::string& what) {
    if (!std::isfinite(x) || std::floor(x) != x || std::llabs(static_cast<long long>(x)) > 9e15)
        throw std::runtime_error(what + " expects integers");
    return static_cast<long long>(x);
}

static double binom(double n, double k) {
    long long N = toInt(n, "nCr"), K = toInt(k, "nCr");
    if (N < 0 || K < 0 || K > N) return 0;
    if (K > N - K) K = N - K;
    double r = 1;
    for (long long i = 1; i <= K; ++i) {
        r = r * static_cast<double>(N - K + i) / static_cast<double>(i);
        if (!std::isfinite(r)) throw std::runtime_error("nCr result is too large");
    }
    return std::round(r);
}

static double perm(double n, double k) {
    long long N = toInt(n, "nPr"), K = toInt(k, "nPr");
    if (N < 0 || K < 0 || K > N) return 0;
    double r = 1;
    for (long long i = 0; i < K; ++i) {
        r *= static_cast<double>(N - i);
        if (!std::isfinite(r)) throw std::runtime_error("nPr result is too large");
    }
    return r;
}

// ---------------------------------------------------------------------------
//  Function dispatcher
// ---------------------------------------------------------------------------
static double callFunction(const std::string& name,
                           const std::vector<double>& a, bool degrees) {
    auto it = functions().find(name);
    if (it == functions().end()) throw std::runtime_error("Unknown function: " + name);
    int arity = it->second.arity;
    if (arity >= 0 && static_cast<int>(a.size()) != arity)
        throw std::runtime_error(name + "() expects " + std::to_string(arity) +
                                 " argument(s), got " + std::to_string(a.size()));
    if (arity < 0 && a.empty())
        throw std::runtime_error(name + "() expects at least one argument");

    const double d2r = PI / 180.0, r2d = 180.0 / PI;
    double x = a.empty() ? 0 : a[0];

    // Trigonometry (degree mode converts input)
    if (name == "sin")   return std::sin (x * (degrees ? d2r : 1));
    if (name == "cos")   return std::cos (x * (degrees ? d2r : 1));
    if (name == "tan")   return std::tan (x * (degrees ? d2r : 1));
    if (name == "sec")   return 1.0 / std::cos (x * (degrees ? d2r : 1));
    if (name == "csc")   return 1.0 / std::sin (x * (degrees ? d2r : 1));
    if (name == "cot")   return 1.0 / std::tan (x * (degrees ? d2r : 1));
    // Inverse trig (degree mode converts output)
    if (name == "asin")  { double v = std::asin (x); return degrees ? v * r2d : v; }
    if (name == "acos")  { double v = std::acos (x); return degrees ? v * r2d : v; }
    if (name == "atan")  { double v = std::atan (x); return degrees ? v * r2d : v; }
    if (name == "atan2") { double v = std::atan2 (a[0], a[1]); return degrees ? v * r2d : v; }
    // Hyperbolic
    if (name == "sinh")  return std::sinh (x);
    if (name == "cosh")  return std::cosh (x);
    if (name == "tanh")  return std::tanh (x);
    if (name == "asinh") return std::asinh (x);
    if (name == "acosh") return std::acosh (x);
    if (name == "atanh") return std::atanh (x);
    // Roots / logs / exponentials
    if (name == "sqrt")  {
        if (x < 0) throw std::runtime_error("sqrt() domain error (negative input)");
        return std::sqrt(x);
    }
    if (name == "cbrt")  return std::cbrt (x);
    if (name == "abs")   return std::fabs (x);
    if (name == "ln" || name == "log" || name == "log2") {
        if (x <= 0) throw std::runtime_error(name + "() domain error (input must be > 0)");
        if (name == "ln") return std::log(x);
        if (name == "log") return std::log10(x);
        return std::log2(x);
    }
    if (name == "logn") {
        if (a[0] <= 0 || a[1] <= 0 || a[0] == 1)
            throw std::runtime_error("logn(base,x) needs base>0, base!=1, x>0");
        return std::log(a[1]) / std::log(a[0]);
    }
    if (name == "exp")   return std::exp (x);
    if (name == "pow")   return std::pow (a[0], a[1]);
    if (name == "root")  {
        double n = a[1];
        if (n == 0) throw std::runtime_error("root() degree cannot be zero");
        if (a[0] < 0 && std::floor(n) == n && std::fmod(n, 2.0) == 1.0)
            return -std::pow(-a[0], 1.0 / n);
        if (a[0] < 0) throw std::runtime_error("root() domain error (even root of negative)");
        return std::pow(a[0], 1.0 / n);
    }
    if (name == "hypot") return std::hypot (a[0], a[1]);
    // Rounding / utility
    if (name == "floor") return std::floor (x);
    if (name == "ceil")  return std::ceil  (x);
    if (name == "round") return std::round (x);
    if (name == "trunc") return std::trunc (x);
    if (name == "sign")  return (x > 0) - (x < 0);
    if (name == "deg")   return x * r2d;
    if (name == "rad")   return x * d2r;
    if (name == "clamp") {
        if (a[1] > a[2]) throw std::runtime_error("clamp(x,lo,hi): lo must be <= hi");
        return std::min(std::max(x, a[1]), a[2]);
    }
    if (name == "lerp")  return a[0] + (a[1] - a[0]) * a[2];
    // Integer / combinatorics
    if (name == "gcd")   {
        long long p = std::llabs(toInt(a[0], "gcd")), q = std::llabs(toInt(a[1], "gcd"));
        return static_cast<double>(std::gcd(p, q));
    }
    if (name == "lcm")   {
        long long p = std::llabs(toInt(a[0], "lcm")), q = std::llabs(toInt(a[1], "lcm"));
        return static_cast<double>(std::lcm(p, q));
    }
    if (name == "nCr")   return binom(a[0], a[1]);
    if (name == "nPr")   return perm(a[0], a[1]);
    // Statistics (variadic)
    if (name == "sum" || name == "avg" || name == "mean") {
        double s = 0;
        for (double v : a) s += v;
        if (name == "sum") return s;
        return s / static_cast<double>(a.size());
    }
    if (name == "max")   return *std::max_element(a.begin(), a.end());
    if (name == "min")   return *std::min_element(a.begin(), a.end());
    // Random
    if (name == "random") return static_cast<double>(std::rand()) / RAND_MAX;
    if (name == "randint") {
        long long lo = toInt(a[0], "randint"), hi = toInt(a[1], "randint");
        if (lo > hi) std::swap(lo, hi);
        return static_cast<double>(lo + std::rand() % (hi - lo + 1));
    }
    throw std::runtime_error("Unimplemented function: " + name);
}

// ---------------------------------------------------------------------------
//  Parser / evaluator
// ---------------------------------------------------------------------------
class Parser {
public:
    Parser(const std::vector<Token>& tokens,
           std::map<std::string, double>& vars, bool degrees)
        : m_tokens(tokens), m_vars(vars), m_degrees(degrees) {}

    double parse() {
        double value = expression();
        if (m_pos != m_tokens.size())
            throw std::runtime_error("Unexpected token: '" + m_tokens[m_pos].text + "'");
        return value;
    }

private:
    const std::vector<Token>& m_tokens;
    std::map<std::string, double>& m_vars;
    bool m_degrees;
    std::size_t m_pos = 0;

    const Token& peek() const { return m_tokens[m_pos]; }
    bool atEnd() const { return m_pos >= m_tokens.size(); }

    bool matchOp(const std::string& op) {
        if (!atEnd() && m_tokens[m_pos].type == TokType::Op && m_tokens[m_pos].text == op) {
            ++m_pos; return true;
        }
        return false;
    }
    bool matchType(TokType type) {
        if (!atEnd() && m_tokens[m_pos].type == type) { ++m_pos; return true; }
        return false;
    }
    void expectType(TokType type, const std::string& what) {
        if (!matchType(type)) throw std::runtime_error("Expected " + what);
    }

    // expr  := term (('+'|'-') term)*
    double expression() {
        double value = term();
        while (!atEnd() && m_tokens[m_pos].type == TokType::Op) {
            const std::string& op = m_tokens[m_pos].text;
            if (op != "+" && op != "-") break;
            ++m_pos;
            double rhs = term();
            value = (op == "+") ? value + rhs : value - rhs;
        }
        return value;
    }

    // term  := unary (('*'|'/'|'%') unary)*
    double term() {
        double value = unary();
        while (!atEnd() && m_tokens[m_pos].type == TokType::Op) {
            const std::string& op = m_tokens[m_pos].text;
            if (op != "*" && op != "/" && op != "%") break;
            ++m_pos;
            double rhs = unary();
            if (op == "*") value *= rhs;
            else if (op == "/") {
                if (rhs == 0) throw std::runtime_error("Division by zero");
                value /= rhs;
            } else {
                if (rhs == 0) throw std::runtime_error("Modulo by zero");
                value = std::fmod(value, rhs);
            }
        }
        return value;
    }

    // unary := ('+'|'-') unary | power
    double unary() {
        if (!atEnd() && m_tokens[m_pos].type == TokType::Op) {
            if (matchOp("-")) return -unary();
            if (matchOp("+")) return  unary();
        }
        return power();
    }

    // power := primary ('^' unary)?   (right-associative)
    double power() {
        double base = primary();
        if (matchOp("^")) return std::pow(base, unary());
        return base;
    }

    // primary := (number | call | ident | '(' expr ')') '!'*
    double primary() {
        double value;
        if (!atEnd() && m_tokens[m_pos].type == TokType::Number) {
            value = m_tokens[m_pos].number; ++m_pos;
        } else if (!atEnd() && m_tokens[m_pos].type == TokType::LParen) {
            ++m_pos;
            value = expression();
            expectType(TokType::RParen, "')'");
        } else if (!atEnd() && m_tokens[m_pos].type == TokType::Ident) {
            std::string name = m_tokens[m_pos].text;
            ++m_pos;
            if (!atEnd() && m_tokens[m_pos].type == TokType::LParen) {
                if (!isFunction(name)) throw std::runtime_error("Unknown function: " + name);
                ++m_pos;
                std::vector<double> args;
                if (!matchType(TokType::RParen)) {
                    do { args.push_back(expression()); } while (matchType(TokType::Comma));
                    expectType(TokType::RParen, "')' after arguments");
                }
                value = callFunction(name, args, m_degrees);
            } else {
                auto it = m_vars.find(name);
                if (it == m_vars.end()) throw std::runtime_error("Unknown identifier: " + name);
                value = it->second;
            }
        } else {
            if (atEnd()) throw std::runtime_error("Unexpected end of expression");
            throw std::runtime_error("Unexpected token: '" + m_tokens[m_pos].text + "'");
        }
        while (matchOp("!")) value = factorial(value);
        return value;
    }
};

// ---------------------------------------------------------------------------
//  REPL helpers
// ---------------------------------------------------------------------------
static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

static std::string format(double v) {
    if (std::isnan(v)) return "nan";
    if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
    std::ostringstream out;
    out << std::setprecision(12) << std::defaultfloat << v;
    return out.str();
}

static void printHelp() {
    std::cout <<
        "\n==================== ADVANCED CALCULATOR - REFERENCE ====================\n"
        " OPERATORS   + - * / % ^   unary + / -   postfix !   ( ) ,\n"
        " TRIG       sin cos tan sec csc cot asin acos atan atan2(y,x)\n"
        " HYPERBOLIC sinh cosh tanh asinh acosh atanh\n"
        " POWERS     sqrt cbrt abs pow(x,y) root(x,n) logn(base,x) exp hypot(x,y)\n"
        " LOGS       ln log (base10) log2\n"
        " ROUNDING   floor ceil round trunc sign deg rad clamp(x,lo,hi) lerp(a,b,t)\n"
        " INTEGER    gcd(x,y) lcm(x,y) nCr(n,k) nPr(n,k)\n"
        " STATS      sum(...) avg(...) mean(...) max(...) min(...)\n"
        " RANDOM     random() randint(lo,hi)\n"
        " CONSTANTS  pi e tau phi inf nan\n"
        " VARIABLES  x = 5   -> use x later   |   ans = last result   |   mem\n"
        " COMMANDS   help ? history hist vars clear deg rad mode mem memclear exit\n"
        "========================================================================\n\n";
}

}  // namespace calc

int main() {
    using namespace calc;
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::map<std::string, double> vars = {
        {"pi", PI}, {"e", std::exp(1.0)}, {"tau", 2 * PI},
        {"phi", (1 + std::sqrt(5.0)) / 2}, {"ans", 0.0},
        {"inf", std::numeric_limits<double>::infinity()},
        {"nan", std::numeric_limits<double>::quiet_NaN()},
        {"mem", 0.0},
    };
    std::vector<std::string> history;
    bool degrees = false;

    std::cout << "================================================================\n"
              << "              ADVANCED CALCULATOR  (C++17)\n"
              << "================================================================\n"
              << " Portable: Windows | macOS | Linux | Android (Termux)\n"
              << " Type 'help' for all functions, 'exit' to quit.\n"
              << " Mode: " << (degrees ? "DEG" : "RAD") << "\n\n";

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) { std::cout << "Bye!\n"; break; }

        auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        auto last = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(first, last - first + 1);
        std::string cmd = lower(trimmed);

        // ---- Commands -----------------------------------------------------
        if (cmd == "exit" || cmd == "quit" || cmd == "q") { std::cout << "Bye!\n"; break; }
        if (cmd == "help" || cmd == "?") { printHelp(); continue; }
        if (cmd == "history" || cmd == "hist") {
            if (history.empty()) std::cout << "  (history is empty)\n";
            else for (std::size_t i = 0; i < history.size(); ++i)
                std::cout << "  " << (i + 1) << ". " << history[i] << "\n";
            std::cout << "\n"; continue;
        }
        if (cmd == "clear") { history.clear(); std::cout << "History cleared.\n\n"; continue; }
        if (cmd == "vars") {
            for (const auto& kv : vars)
                std::cout << "  " << kv.first << " = " << format(kv.second) << "\n";
            std::cout << "\n"; continue;
        }
        if (cmd == "deg" || cmd == "rad") {
            degrees = (cmd == "deg");
            std::cout << "Angle mode: " << (degrees ? "DEG" : "RAD") << "\n\n"; continue;
        }
        if (cmd == "mode") { std::cout << "Angle mode: " << (degrees ? "DEG" : "RAD") << "\n\n"; continue; }
        if (cmd == "mem") { std::cout << "mem = " << format(vars["mem"]) << "\n\n"; continue; }
        if (cmd == "memclear") { vars["mem"] = 0.0; std::cout << "Memory cleared.\n\n"; continue; }

        // ---- Evaluate -----------------------------------------------------
        try {
            std::vector<Token> tokens = tokenize(trimmed);
            if (tokens.empty()) continue;

            bool assignment = false;
            std::string target;
            if (tokens.size() >= 3 && tokens[0].type == TokType::Ident &&
                tokens[1].type == TokType::Op && tokens[1].text == "=") {
                target = tokens[0].text;
                if (isFunction(target))
                    throw std::runtime_error("Cannot assign to a function name: " + target);
                tokens.erase(tokens.begin(), tokens.begin() + 2);
                assignment = true;
            }

            Parser parser(tokens, vars, degrees);
            double result = parser.parse();

            if (!std::isfinite(result)) {
                std::cout << "  => " << format(result) << "  (not finite)\n\n";
                continue;
            }
            vars["ans"] = result;
            if (assignment) vars[target] = result;

            std::ostringstream entry;
            if (assignment) entry << target << " = ";
            entry << trimmed << "  =>  " << format(result);
            history.push_back(entry.str());

            std::cout << "  => " << format(result) << "\n\n";
        } catch (const std::exception& ex) {
            std::cout << "  Error: " << ex.what() << "\n\n";
        }
    }
    return 0;
}
