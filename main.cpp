// World state and command parsing are separate so each part is easier to follow.
// Build: g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp -o main.exe
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// An unnamed namespace keeps these helpers local to this source file.
namespace {

// constexpr values are fixed at compile time instead of being mutable globals.
constexpr double pi = 3.1416; // Preserve the original movement calculations.
constexpr float radius = 256.0f;
constexpr std::size_t tokenWidth = 8; // Commands and parameter chunks are eight characters.
constexpr std::size_t maxEvaluationDepth = 256; // Bound recursion before the call stack overflows.

// Three named floats replace temporary vectors: no heap allocation for XYZ math.
struct Position {
    float x = 0;
    float y = 0;
    float z = 0;

    // const leaves XYZ unchanged; noexcept says this calculation cannot throw.
    float squaredLength() const noexcept {
        // Starting at zero fixes the old uninitialized sum; order preserves rounding.
        float sum = 0;
        sum += x * x;
        sum += y * y;
        sum += z * z;
        return sum;
    }

    // sqrt(x*x + y*y + z*z) is the distance from the origin.
    float length() const noexcept { return std::sqrt(squaredLength()); }
};

// Named fields replace the old float array; size_t stores a ball index exactly.
struct ClosestBall {
    float distance = -1; // Sentinel: no ball was found, so index must not be used.
    Position delta{-1, -1, -1};
    std::size_t index = 0;
};

// string_view reads existing characters without copying their string storage.
// Integer multiply-and-add also replaces the old repeated floating-point pow calls.
int fromBinary(std::string_view text) noexcept {
    int value = 0;
    for (char digit : text) {
        // Shift the accumulated binary value left, then add the next digit.
        value = value * 2 + (digit - '0');
    }
    return value;
}

int base19Digit(char digit) noexcept {
    // Direct mapping replaces a loop through separate letter and number arrays.
    // Character subtraction maps '0'..'9' to 0..9 and 'A'..'I' to 10..18.
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'A' && digit <= 'I') return digit - 'A' + 10;
    return 0; // Unrecognized digits contribute zero, as in the original converter.
}

// Keep position, balls, and movement caches together instead of in global variables.
class World {
public:
    // Store a reference to the output stream so tests can capture the same messages.
    explicit World(std::ostream& output) : output_(output) {}

    void placeBall() {
        balls_.push_back(position_);
        closest_.reset(); // A new ball makes the previous nearest-ball result stale.
    }

    void getBall() {
        // Copy one search result so its index stays available while changing balls_.
        const ClosestBall ball = closestBall();
        output_ << "Distance to ball: " << ball.distance << '\n';
        if (ball.distance >= 0 && ball.distance <= 5) {
            output_ << " Got Ball\n";
            // Fill the removed slot with the last ball: O(1), with no shifting.
            balls_[ball.index] = balls_.back();
            balls_.pop_back();
            closest_.reset(); // Deletion changes which ball and index are nearest.
        }
    }

    void autoGetBall(std::istream& input) const {
        const ClosestBall ball = closestBall();
        if (ball.distance < 0) { // Stop before using offsets from a missing ball.
            output_ << "No balls available to collect.\n";
            return;
        }

        std::string line;
        // Flush prompts explicitly because automatic flushing is disabled in main.
        output_ << "How many times to move to get to ball: " << std::flush;
        if (!std::getline(input, line)) return; // End of input is a normal stop.

        int steps = 0;
        // stoi can throw for text or oversized numbers; report them instead of crashing.
        try {
            steps = std::stoi(line);
        } catch (const std::invalid_argument&) {
            output_ << "Step count must be a positive integer.\n";
            return;
        } catch (const std::out_of_range&) {
            output_ << "Step count must be a positive integer.\n";
            return;
        }
        if (steps <= 0) { // A positive count is required before dividing by it.
            output_ << "Step count must be > 0\n";
            return;
        }

        const Position delta = ball.delta;
        const float hypotenuse = ball.distance; // Reuse the length already found by the search.
        if (hypotenuse == 0) { // Avoid dividing by zero when already at the ball.
            output_ << "Already at the closest ball.\n";
            return;
        }

        // Distance / steps gives travel per move; the cast makes the conversion explicit.
        const float requiredSpeed = hypotenuse / static_cast<float>(steps);
        int requiredTurn = 0;
        output_ << '\n' << "Dy, Dx, Dz " << delta.y << ", " << delta.x << ", " << delta.z;
        if (delta.y < 0) {
            // asin gives radians; 180/pi converts to degrees, then int truncates them.
            const int degree = static_cast<int>(std::asin(-delta.y / hypotenuse) * 180 / pi);
            output_ << '\n' << "degree: " << degree << "  ";
            // Retain the original 350-degree rule and integer turn calculation.
            // The ternary chooses the target angle according to the ball's X side.
            const int target = delta.x < 0 ? 180 + degree : 350 - degree;
            requiredTurn = target - xyDirection_;
        } else if (delta.x < 0) { // Correct the angle for a ball on the negative X side.
            const int target = static_cast<int>(180 - std::asin(delta.y / hypotenuse) * 180 / pi);
            requiredTurn = target - xyDirection_;
        } else {
            const int degree = static_cast<int>(std::asin(delta.y / hypotenuse) * 180 / pi);
            requiredTurn = degree - xyDirection_;
        }
        // requiredTurn/19 is integer division, kept for the original base-19 hint.
        output_ << '\n' << "To get to the closest ball, set speed to " << requiredSpeed
                << " and turn " << requiredTurn << " 'it's " << requiredTurn / 19 << '\n';
    }

    void turn(std::string_view degrees) {
        if (degrees.size() != 4) return;
        std::array<char, 4> digits{}; // Four fixed characters need no dynamic allocation.
        for (std::size_t i = 0; i < digits.size(); ++i) {
            // toupper requires an unsigned-char value; normalize lowercase base-19 letters.
            digits[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(degrees[i])));
        }
        // Each pair is highDigit*19 + lowDigit; modulo 360 wraps a full revolution.
        xzDirection_ = (xzDirection_ + base19Digit(digits[0]) * 19 + base19Digit(digits[1])) % 360;
        xyDirection_ = (xyDirection_ + base19Digit(digits[2]) * 19 + base19Digit(digits[3])) % 360;
        movement_.reset(); // Turning invalidates the cached movement direction.
    }

    void setSpeed(std::string_view speed) {
        speed_ = fromBinary(speed);
        movement_.reset(); // A new speed changes how far each cached step travels.
        output_ << "Set speed to " << speed_ << '\n';
    }

    void move() {
        // Recalculate trigonometry only after speed or direction changes.
        if (!movement_) {
            // Degrees become radians; the minus sign keeps the original turn convention.
            const float xyRadians = static_cast<float>(-xyDirection_ * pi / 180);
            const float xzRadians = static_cast<float>(-xzDirection_ * pi / 180);
            // Tangents give Y/X and Z/X slopes; dividing by length scales to speed_.
            const Position direction{1, std::tan(xyRadians), std::tan(xzRadians)};
            const float dx = static_cast<float>(speed_) / direction.length();
            const float dy = dx * direction.y;
            const float dz = dx * direction.z;
            // Round each step to two decimals; double storage preserves the old addition.
            movement_ = std::array<double, 3>{std::round(100.0 * dx) / 100.0,
                                             std::round(100.0 * dy) / 100.0,
                                             std::round(100.0 * dz) / 100.0};
        }

        // *movement_ unwraps the optional array; [0], [1], [2] are its XYZ steps.
        // Cast back to float and apply each wall check just as the original did.
        position_.x = hitWall(static_cast<float>(position_.x + (*movement_)[0]), 'X');
        position_.y = hitWall(static_cast<float>(position_.y + (*movement_)[1]), 'Y');
        position_.z = hitWall(static_cast<float>(position_.z + (*movement_)[2]), 'Z');
        closest_.reset(); // Changing the player's position changes every ball distance.
    }

    // Stream chaining preserves the existing messages; '\n' avoids flushing each line.
    void giveInfo() const {
        output_ << '\n' << "You are at: " << position_.x << ", " << position_.y << ", " << position_.z
                << '\n' << "with a x/y-direction: " << xyDirection_ << " Degrees"
                << '\n' << "with a x/z-direction: " << xzDirection_ << " Degrees"
                << '\n' << "Speed: " << speed_
                << '\n' << "Distance from origin: " << 0.01 * std::round(100 * position_.length())
                << '\n' << "Distance to closest Ball: " << closestBall().distance
                << '\n' << "Number of balls: " << balls_.size() << "\n\n";
    }

private:
    // Return a reference to the cached result so repeated status checks need no scan.
    const ClosestBall& closestBall() const {
        if (closest_) return *closest_; // An engaged optional holds a valid cached answer.

        ClosestBall result;
        // Infinity lets the first finite candidate win without a special first-ball case.
        float shortestSquared = std::numeric_limits<float>::infinity();
        float shortestDistance = std::numeric_limits<float>::infinity();
        for (std::size_t i = 0; i < balls_.size(); ++i) {
            // Subtract player coordinates to get the direction and distance to this ball.
            const Position delta{balls_[i].x - position_.x,
                                 balls_[i].y - position_.y,
                                 balls_[i].z - position_.z};
            const float squared = delta.squaredLength();
            if (squared < shortestSquared) { // Skip sqrt for candidates that cannot be closer.
                const float distance = std::sqrt(squared);
                // Compare rounded distances too: equal distances keep the first ball.
                if (distance < shortestDistance) {
                    shortestSquared = squared;
                    shortestDistance = distance;
                    result = {distance, delta, i};
                }
            }
        }
        closest_ = result; // Cache even the no-ball sentinel until the world changes.
        return *closest_;
    }

    float hitWall(float coordinate, char axis) const {
        if (std::abs(coordinate) >= radius) {
            // coordinate/abs(coordinate) is +1 or -1, selecting the wall's side.
            // Keep the old -0.01 offset on both sides to preserve the same positions.
            coordinate = radius * coordinate / std::abs(coordinate) - 0.01f;
            output_ << '\n' << "Caution: Hit Wall " << axis << "\n\n";
        }
        return coordinate;
    }

    std::ostream& output_;
    Position position_;
    std::vector<Position> balls_;
    int xyDirection_ = 0;
    int xzDirection_ = 0;
    int speed_ = 1;
    // optional distinguishes "not calculated yet" from a valid zero-valued movement.
    std::optional<std::array<double, 3>> movement_;
    // mutable permits caching inside const queries without changing the world itself.
    mutable std::optional<ClosestBall> closest_;
};

// Keep a view of the original token and decode its numeric value only once.
struct Token {
    std::string_view text;
    int code = 0;
    bool binary = true; // Base-19 parameter text must not execute as a binary command.
};

// Named integer fields replace repeated string-key lookups in the old parser map.
struct ParserState {
    int parentheses = 0; // Number of opening parentheses still awaiting a close.
    int parameterFor = 0; // Opcode whose argument or body is being collected; zero means none.
    int expected = 0; // Parameter progress: opening token, value, then closing token.
};

// RAII guard: entering a scope raises depth, and every exit automatically lowers it.
struct EvaluationScope {
    explicit EvaluationScope(std::size_t& depth) : depth_(depth) { ++depth_; }
    ~EvaluationScope() { --depth_; }
    // Copying the guard would decrement the same counter twice, so copies are forbidden.
    EvaluationScope(const EvaluationScope&) = delete;
    EvaluationScope& operator=(const EvaluationScope&) = delete;

private:
    std::size_t& depth_;
};

// Parsing and function definitions live here; World handles the actual game state.
class Interpreter {
public:
    Interpreter(std::istream& input, std::ostream& output)
        : input_(input), output_(output), world_(output) {} // Initialize stored stream references.

    void run() {
        printInstructions();
        world_.giveInfo();
        // The short "1" requests another input line; false stops on EOF or an error.
        while (evaluate("1", true)) {}
    }

private:
    std::vector<Token> tokenize(const std::string& text) {
        std::vector<Token> tokens;
        tokens.reserve(text.size() / tokenWidth); // Reserve once instead of repeatedly growing.
        const std::string_view view(text);
        std::size_t begin = 0;
        while (begin < text.size()) {
            // Like the old stringstream, accept spaces and tabs between input words.
            if (std::isspace(static_cast<unsigned char>(text[begin]))) {
                ++begin;
                continue;
            }
            std::size_t end = begin;
            while (end < text.size() && !std::isspace(static_cast<unsigned char>(text[end]))) ++end;
            // Split joined commands into eight-character views; ignore incomplete tails.
            for (std::size_t offset = begin; end - offset >= tokenWidth; offset += tokenWidth) {
                Token token{view.substr(offset, tokenWidth)};
                for (char digit : token.text) {
                    // Decode and check for binary digits in the same pass through the token.
                    token.code = token.code * 2 + (digit - '0');
                    token.binary = token.binary && (digit == '0' || digit == '1');
                }
                output_ << '\n' << "cmd " << token.text;
                tokens.push_back(token);
            }
            begin = end;
        }
        return tokens;
    }

    bool execute(const Token& token) {
        if (!token.binary) return false; // Letters belong to parameters or custom names.
        // Dispatch decoded command IDs directly instead of comparing several strings.
        switch (token.code) {
        case 0: world_.move(); return true;
        case 3: world_.giveInfo(); return true;
        case 4: world_.getBall(); return true;
        case 5: world_.placeBall(); return true;
        case 6: world_.autoGetBall(input_); return true;
        default: return false;
        }
    }

    // nameable is false for recursive calls, so stored conditionals do not ask for new names.
    bool evaluate(std::string_view input, bool nameable) {
        if (depth_ >= maxEvaluationDepth) {
            output_ << "Maximum command nesting depth exceeded.\n";
            return false;
        }
        const EvaluationScope scope(depth_); // Also restores depth when returning early.
        // Own the storage behind token views, including during recursive calls
        // and redefinitions of a function currently being evaluated.
        std::string text(input);
        if (text.size() < 2) {
            output_ << "Enter next command: " << std::flush;
            if (!std::getline(input_, text)) return false; // Prevent the old endless EOF loop.
        }
        const auto tokens = tokenize(text);
        ParserState state;
        std::string statement;

        for (const Token& token : tokens) {
            const int code = token.code;
            // 9 is '(' and 10 is ')'; update nesting before deciding if a body is finished.
            if (code == 10) --state.parentheses;
            if (code == 9) ++state.parentheses;

            // Keep the existing trace layout; its unused state fields are zero.
            output_ << "Main: " << statement << ".  " << code << "  " << state.parentheses
                    << "  0  " << state.parameterFor << "  0  0  " << state.expected << '\n';

            if (state.parentheses == 0) {
                if (state.parameterFor == 135) { // 135 defines a reusable function body.
                    state.parameterFor = 0;
                    std::string name;
                    output_ << "Enter 8-bit function code, should be different than the original functions: " << std::flush;
                    if (!std::getline(input_, name)) return false;
                    functions_[name] = statement; // Save its text; execute it only when called.
                    output_ << "Main" << statement << '\n';
                    statement.clear();
                } else if (state.parameterFor == 136) { // 136 defines/evaluates a conditional.
                    state.parameterFor = 0;
                    if (nameable) {
                        std::string name;
                        output_ << "Enter 8-bit conditional code, should be different than the original functions: " << std::flush;
                        if (!std::getline(input_, name)) return false;
                        // Reattach IF and parentheses so later calls recheck the condition.
                        functions_[name] = "10001000 00001001" + statement + "00001010";
                    }
                    const auto body = evaluateCondition(statement);
                    // An empty optional means failure; otherwise execute its returned body.
                    if (!body || !evaluate(*body, false)) return false;
                    statement.clear();
                }
            // Drop the outer parentheses, but retain pairs belonging to nested commands.
            } else if ((code != 9 && code != 10) || state.parentheses > 1 ||
                       (state.parentheses == 1 && code == 10)) {
                statement.append(token.text);
            }

            // Functions and IFs collect bodies; other high-bit commands collect one argument.
            if (state.parameterFor != 0 && state.parameterFor != 135 && state.parameterFor != 136) {
                if (state.expected == 2) { // The value was read; this token finishes the argument.
                    state.expected = 0;
                    output_ << "Main statement: " << statement << '\n';
                    if (state.parameterFor == 129) { // Turn opcode: 10000001.
                        if (statement.size() < 6) {
                            output_ << "Invalid turn parameter.\n";
                            return false;
                        }
                        // Keep the four base-19 digits between the two-character wrappers.
                        statement = statement.substr(2, 4);
                        output_ << '\n' << statement << '\n';
                        world_.turn(statement);
                    }
                    if (state.parameterFor == 130) world_.setSpeed(statement); // Speed: 10000010.
                    state.parameterFor = 0;
                } else {
                    // Retain only this token; on the second pass it is the argument value.
                    statement.assign(token.text);
                    ++state.expected;
                }
            } else if (state.parameterFor == 0 && !execute(token)) {
                // A leading 1 marks the original protocol's parameter-taking command group.
                if (token.text.front() == '1') state.parameterFor = code;
                // One hash lookup replaces contains() followed by another lookup with [].
                const auto function = functions_.find(std::string(token.text));
                if (function != functions_.end()) {
                    output_ << function->second;
                    if (!evaluate(function->second, false)) return false; // Reuse the same parser.
                }
            }
        }
        return true;
    }

    bool checkCondition(std::string_view condition) {
        // A condition contains three bytes: left value, comparison ID, and right value.
        // "index" remains a literal number here, matching the original behavior.
        const int index = fromBinary(condition.substr(0, 8));
        // Keep the comparison inside the four-element array, even for malformed input.
        const int comparison = std::clamp(fromBinary(condition.substr(8, 8)), 0, 3);
        const int value = fromBinary(condition.substr(16, 8));
        const std::array<bool, 4> comparisons{index < value, index > value, index == value, index != value};
        // IDs 0, 1, 2, 3 select <, >, ==, !=; size_t is the array's index type.
        const bool result = comparisons[static_cast<std::size_t>(comparison)];
        output_ << index << "  " << comparison << "  " << value << " is " << result << '\n';
        return result;
    }

    // nullopt signals a parse failure; an empty string is a valid false-condition body.
    std::optional<std::string> evaluateCondition(std::string_view statement) {
        if (depth_ >= maxEvaluationDepth) {
            output_ << "Maximum command nesting depth exceeded.\n";
            return std::nullopt;
        }
        const EvaluationScope scope(depth_);
        bool readingCondition = true;
        std::string body;
        std::string condition;
        std::string nestedCondition;
        std::vector<bool> results; // Keep each comparison result until AND/OR reduction.
        std::vector<std::string_view> operations; // Views avoid copying AND/OR token strings.
        int nested = 0;
        int parentheses = 0;

        output_ << "Full conditional statement: " << statement << '\n';
        for (std::size_t i = 0; i < statement.size() / tokenWidth; ++i) {
            const std::string_view command = statement.substr(tokenWidth * i, tokenWidth);
            // "b" marks a nested IF just completed; clear it before the next token.
            if (nestedCondition == "b") nestedCondition.clear();
            if (command == "10001000" && !readingCondition) {
                nestedCondition.clear();
                ++nested;
            }
            // Count parentheses only inside an IF body, not inside comparison data.
            if (command == "00001001" && !readingCondition && nested > 0) ++parentheses;
            if (command == "00001010" && !readingCondition && nested > 0) {
                if (parentheses > 0) --parentheses;
                if (parentheses == 0) { // The nested IF is complete and can now be evaluated.
                    const auto nestedBody = evaluateCondition(nestedCondition);
                    if (!nestedBody) return std::nullopt;
                    body += *nestedBody;
                    nestedCondition = "b";
                    --nested;
                }
            }

            if (readingCondition) {
                // Every group is three comparison bytes plus an AND/OR or first body token.
                if ((i + 1) % 4 == 0) {
                    // Within conditions, IDs 4/5 mean OR/AND rather than get/place ball.
                    if (command != "00000100" && command != "00000101") {
                        if (command != "10001000") body.append(command);
                        else ++nested;
                        results.push_back(checkCondition(condition));
                        condition.clear();
                        readingCondition = false;
                    } else {
                        results.push_back(checkCondition(condition));
                        condition.clear();
                        operations.push_back(command);
                    }
                } else {
                    condition.append(command);
                }
            } else if (nested > 0) {
                // Omit the nested IF's own header/wrapper but keep deeper parentheses.
                if (command != "10001000" && (command != "00001001" || nested < parentheses)) {
                    nestedCondition.append(command);
                }
            } else if (nestedCondition.empty()) {
                body.append(command);
            }
            output_ << "Command: " << command << "  NonCond: " << body << "  condition: " << condition
                    << "  newCond: " << nestedCondition << "  isNested: " << nested
                    << "  paraCount: " << parentheses << '\n';
        }

        // A valid chain needs one more comparison result than joining operators.
        if (results.empty() || operations.size() + 1 != results.size()) {
            output_ << "Invalid conditional statement.\n";
            return std::nullopt;
        }
        // Preserve the original swap-and-pop reduction order for mixed AND/OR.
        // Reducing in place avoids copying both vectors for every conditional.
        while (results.size() > 1) {
            output_ << "iteration\n";
            const bool second = results[1];
            const std::string_view operation = operations[0];
            // Remove consumed entries by filling their slots with the last entries.
            // This is O(1), but deliberately keeps the old non-left-to-right order.
            results[1] = results.back();
            results.pop_back();
            operations[0] = operations.back();
            operations.pop_back();
            output_ << results[0] << "  " << operation << "  " << second << '\n';
            if (operation == "00000100") results[0] = results[0] || second;
            if (operation == "00000101") results[0] = results[0] && second;
        }
        if (!results[0]) body.clear(); // A false combined condition yields no body commands.
        output_ << '\n' << "nonCondStatement: " << body << '\n';
        return body;
    }

    // Adjacent string literals concatenate at compile time; keep the existing help text.
    void printInstructions() const {
        output_ << "Instructions:\n"
                   "000 to move\n"
                   "001 to turn--include 4 digit base-19 input between any 2 characters\n"
                   "010 to set speed--include 4 digit binary input between any 2 characters\n"
                   "011 to display current status\n"
                   "100 to get ball\n"
                   "101 to place ball\n"
                   "110 to auto get ball\n\n"
                   "Turning is a 4 digit nonadecimal or base-19 code. The first 2 digits are turning in the xz plane,\n"
                   " the second 2 digits are for turning in the xy plane A(10) B C D E F G H I(18)\n\n"
                   "Use ..fun.. to create a command then enter the name for the command.\n"
                   "Reminder: don't call the command you created in the command\n"
                   "First 2 digits for turning are for the xz direction, the next two are for xy direction:\n"
                   " 90 degrees is 4E, 180 is 99, 270 is E4, 360 is II(max 19^2-1)\n\n"
                   "For conditionals, use 111 to start and con to end the conditional, and use 3 digit binary inputs for the index, comparison operator, and value to compare against\n"
                   "The comparison operators are: < is 000, > is 001, == is 010, != is 011\n"
                   "NEVER USE 111 unless for conditionals as you need to have equal numbers of 111 and con\n";
    }

    std::istream& input_;
    std::ostream& output_;
    World world_;
    std::unordered_map<std::string, std::string> functions_; // Names map to saved command text.
    std::size_t depth_ = 0;
};

} // namespace

int main() {
    // Avoid C/C++ stream synchronization overhead; this program uses only C++ streams.
    std::ios::sync_with_stdio(false);
    // Stop flushing cout before every read; explicit prompt flushes keep input usable.
    std::cin.tie(nullptr);
    Interpreter interpreter(std::cin, std::cout);
    interpreter.run();
}
