/*
=============================================================================
 ONLINE EXAMINATION PROCTORING EVENT ANALYZER
 DAA Project-Based Learning - Project 26 (CSEG3053)
 Single-file C++17 program: domain model + every mandatory syllabus
 algorithm + menu interface + benchmark runner + automated tests +
 CSV / JSON export.
=============================================================================

 BUILD (Windows + VS Code with MinGW-w64 g++ from MSYS2, or Linux/macOS):
     g++ -std=c++17 -O2 -Wall -Wextra -o proctor proctor_analyzer.cpp

 RUN:
     proctor              interactive menu
     proctor --demo       full demonstration script (3 scenarios + stress test)
     proctor --test       automated unit + integration tests
     proctor --bench      full benchmark runner (writes benchmark_results.csv)
     proctor --bench-quick  smaller benchmark (about 10 seconds)
     proctor --samples    writes sample dataset CSV files for all scenarios
     proctor --report     prints complexity report and P/NP reduction

 VERSION CONTROL (publish to GitHub):
     git init
     git add proctor_analyzer.cpp Makefile .gitignore data/ docs/ scripts/
     git commit -m "DAA Project 26: Online Examination Proctoring Event Analyzer"
     git remote add origin <your-empty-github-repo-url>
     git push -u origin main
     git tag -a v1.0 -m "Demo/viva submission 21 Nov 2026"
     git push origin v1.0

 ETHICS: every score produced here is an INDICATOR for HUMAN REVIEW.
 It is never proof that a candidate cheated.

 RULE FOLLOWED: STL containers (vector, string) are used only for storage.
 Every assessed algorithm (sorting, searching, heap, graph algorithms,
 dynamic programming, matching, string matching ...) is written by hand.
 std::sort, std::priority_queue, std::binary_search are NOT used.

 SECTION MAP (search for "SECTION"):
   1  Utilities                      12 Number theory (CRT)
   2  Domain model                   13 String matching
   3  Sorting and searching          14 Data generator and validation
   4  Min-heap (priority queue)      15 Risk detection engine + timelines
   5  Divide and conquer             16 Persistence (CSV / JSON)
   6  Greedy algorithms              17 Review planning (integration)
   7  Graph algorithms               18 Algorithm workbench demos
   8  Dynamic programming            19 Benchmark runner
   9  Backtracking                   20 Automated tests
  10  Branch and bound               21 Complexity report and reduction
  11  Network flow and matching      22 Menus, demo script, main()
=============================================================================
*/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <climits>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <algorithm>   // used ONLY for std::min, std::max and std::swap
using namespace std;

// ============================================================================
// SECTION 1: UTILITIES
// ============================================================================

const long long INF = LLONG_MAX / 4;   // "infinity" that is safe to add to

// Deterministic random generator: same numbers on every compiler and OS,
// so every dataset and benchmark input is reproducible.
struct SimpleRandom {
    unsigned long long state;
    SimpleRandom(unsigned long long seed) { state = seed; }
    unsigned int next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return (unsigned int)(state >> 33);
    }
    int range(int low, int high) {           // inclusive range
        if (high <= low) return low;
        return low + (int)(next() % (unsigned int)(high - low + 1));
    }
    bool chance(int percent) { return range(1, 100) <= percent; }
};

struct Timer {
    chrono::steady_clock::time_point startTime;
    void start() { startTime = chrono::steady_clock::now(); }
    double millis() {
        chrono::steady_clock::time_point endTime = chrono::steady_clock::now();
        long long micros = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
        return micros / 1000.0;      // milliseconds
    }
};

string trim(const string &text) {
    size_t start = 0, end = text.size();
    while (start < end && isspace((unsigned char)text[start])) start++;
    while (end > start && isspace((unsigned char)text[end - 1])) end--;
    return text.substr(start, end - start);
}

// Strict whole-number parser (rejects "12abc", empty text and overflow).
bool parseLongStrict(const string &rawText, long long &value) {
    string text = trim(rawText);
    if (text.empty()) return false;
    size_t i = 0;
    bool negative = false;
    if (text[0] == '-' || text[0] == '+') { negative = (text[0] == '-'); i = 1; }
    if (i >= text.size()) return false;
    long long result = 0;
    for (; i < text.size(); i++) {
        if (!isdigit((unsigned char)text[i])) return false;
        int digit = text[i] - '0';
        if (result > (LLONG_MAX - digit) / 10) return false;
        result = result * 10 + digit;
    }
    value = negative ? -result : result;
    return true;
}

string readText(const string &prompt) {
    cout << prompt;
    string line;
    if (!getline(cin, line)) { cout << "\n[Input closed - exiting]\n"; exit(0); }
    return trim(line);
}

long long readNumber(const string &prompt, long long low, long long high) {
    while (true) {
        string line = readText(prompt);
        long long value = 0;
        if (parseLongStrict(line, value) && value >= low && value <= high) return value;
        cout << "  Invalid input. Enter a whole number from " << low << " to " << high << ".\n";
    }
}

int readInt(const string &prompt, int low, int high) { return (int)readNumber(prompt, low, high); }

void pauseForEnter() { readText("\nPress ENTER to continue..."); }

string formatDouble(double value, int decimals) {
    ostringstream out;
    out << fixed << setprecision(decimals) << value;
    return out.str();
}

string padRight(const string &text, int width) {
    if ((int)text.size() >= width) return text.substr(0, width);
    return text + string(width - text.size(), ' ');
}

string padLeft(const string &text, int width) {
    if ((int)text.size() >= width) return text;
    return string(width - text.size(), ' ') + text;
}

string formatClock(int seconds) {                 // exam time as hh:mm:ss
    if (seconds < 0) seconds = 0;
    ostringstream out;
    out << setfill('0') << setw(2) << seconds / 3600 << ":" << setw(2) << (seconds % 3600) / 60
        << ":" << setw(2) << seconds % 60;
    return out.str();
}

string clockAfterNine(int minutes) {              // review-day time, day starts 09:00
    int total = 9 * 60 + minutes;
    ostringstream out;
    out << setfill('0') << setw(2) << (total / 60) % 24 << ":" << setw(2) << total % 60;
    return out.str();
}

void printLine(char symbol = '-', int width = 78) { cout << string(width, symbol) << "\n"; }

void printHeader(const string &title) {
    cout << "\n";
    printLine('=');
    cout << " " << title << "\n";
    printLine('=');
}

string yesNo(bool value) { return value ? "YES" : "NO"; }

// ============================================================================
// SECTION 2: DOMAIN MODEL
// ============================================================================

const int EVENT_TYPE_COUNT = 8;
const int LOGIN_OK = 0;
const int LOGIN_FAILED = 1;
const int TAB_SWITCH = 2;
const int DEVICE_CHANGE = 3;
const int NETWORK_DROP = 4;
const int COPY_PASTE = 5;
const int IP_CHANGE = 6;
const int ANSWER_SUBMIT = 7;

const string EVENT_NAMES[EVENT_TYPE_COUNT] = {"LOGIN_OK", "LOGIN_FAILED", "TAB_SWITCH", "DEVICE_CHANGE",
                                              "NETWORK_DROP", "COPY_PASTE", "IP_CHANGE", "ANSWER_SUBMIT"};
// One-letter codes turn a candidate's behaviour into a string, e.g. "LSSTNLDS".
const char EVENT_CODES[EVENT_TYPE_COUNT] = {'L', 'F', 'T', 'D', 'N', 'C', 'I', 'S'};
const int DEFAULT_SEVERITY[EVENT_TYPE_COUNT] = {0, 2, 1, 3, 1, 2, 3, 0};

// Case categories (bit mask) used to match cases with reviewer expertise.
const int CAT_LOGIN = 1, CAT_BROWSER = 2, CAT_DEVICE = 4, CAT_NETWORK = 8;
const int CAT_ALL = 15;

string categoryName(int category) {
    if (category == CAT_LOGIN) return "LOGIN";
    if (category == CAT_BROWSER) return "BROWSER";
    if (category == CAT_DEVICE) return "DEVICE";
    if (category == CAT_NETWORK) return "NETWORK";
    if (category == 0) return "-";
    return "MIXED";
}

string skillsText(int mask) {
    string text;
    if (mask & CAT_LOGIN) text += "LOGIN ";
    if (mask & CAT_BROWSER) text += "BROWSER ";
    if (mask & CAT_DEVICE) text += "DEVICE ";
    if (mask & CAT_NETWORK) text += "NETWORK ";
    return trim(text);
}

int eventTypeFromText(const string &text) {
    for (int t = 0; t < EVENT_TYPE_COUNT; t++) {
        if (text == EVENT_NAMES[t]) return t;
        if (text.size() == 1 && text[0] == EVENT_CODES[t]) return t;
    }
    return -1;
}

struct Candidate {
    int candidateId = 0;
    string name;
    string examId;
    int primaryDeviceId = 0;
};

struct ExamEvent {
    int eventId = 0;
    int candidateId = 0;
    string examId;
    int eventType = 0;
    int timestamp = 0;      // seconds since the exam started
    int deviceId = 0;
    int severity = 0;       // 0 (informational) .. 5 (critical)
    string description;
};

struct Reviewer {
    int reviewerId = 0;
    string name;
    int availableMinutes = 0;
    int expertiseMask = 0;
};

struct Dataset {
    string scenarioName = "(none)";
    int scenarioNumber = 0;
    int examDurationSeconds = 7200;
    int reviewRooms = 3;
    bool relayPlanted = false;   // scenario contains a shared "relay" endpoint
    vector<Candidate> candidates;
    vector<ExamEvent> events;
    vector<Reviewer> reviewers;
};

// A review case / knapsack item: weight = review minutes, value = risk score.
struct KnapsackItem {
    int id = 0;
    string label;
    int weight = 0;
    int value = 0;
};

// ============================================================================
// SECTION 3: SORTING AND SEARCHING (Merge sort, Quick sort, Binary search)
// ============================================================================
// We sort SortItem records (key + original index). Any record type can be
// sorted by building SortItems, so one implementation serves the whole app.

struct SortItem {
    long long key;
    int index;
};

struct SortStats {
    long long comparisons = 0;
    long long moves = 0;
    int maxDepth = 0;
};

void mergeRanges(vector<SortItem> &items, vector<SortItem> &buffer, int left, int mid, int right,
                 SortStats &stats) {
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        stats.comparisons++;
        if (items[i].key <= items[j].key) buffer[k++] = items[i++];   // "<=" keeps merge sort STABLE
        else buffer[k++] = items[j++];
        stats.moves++;
    }
    while (i <= mid) { buffer[k++] = items[i++]; stats.moves++; }
    while (j <= right) { buffer[k++] = items[j++]; stats.moves++; }
    for (int x = left; x <= right; x++) { items[x] = buffer[x]; stats.moves++; }
}

void mergeSortRecursive(vector<SortItem> &items, vector<SortItem> &buffer, int left, int right, int depth,
                        SortStats &stats) {
    if (depth > stats.maxDepth) stats.maxDepth = depth;
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortRecursive(items, buffer, left, mid, depth + 1, stats);
    mergeSortRecursive(items, buffer, mid + 1, right, depth + 1, stats);
    mergeRanges(items, buffer, left, mid, right, stats);
}

// T(n) = 2T(n/2) + Theta(n)  ->  Theta(n log n) in every case. Extra space Theta(n).
void mergeSort(vector<SortItem> &items, SortStats &stats) {
    if (items.size() < 2) return;
    vector<SortItem> buffer(items.size());
    mergeSortRecursive(items, buffer, 0, (int)items.size() - 1, 1, stats);
}

const int PIVOT_LAST = 0;
const int PIVOT_MEDIAN_OF_THREE = 1;

int partitionLomuto(vector<SortItem> &items, int low, int high, SortStats &stats) {
    long long pivot = items[high].key;
    int i = low - 1;
    for (int j = low; j < high; j++) {
        stats.comparisons++;
        if (items[j].key <= pivot) {
            i++;
            swap(items[i], items[j]);
            stats.moves++;
        }
    }
    swap(items[i + 1], items[high]);
    stats.moves++;
    return i + 1;
}

void choosePivot(vector<SortItem> &items, int low, int high, int rule, SortStats &stats) {
    if (rule != PIVOT_MEDIAN_OF_THREE || high - low < 2) return;
    int mid = low + (high - low) / 2;
    stats.comparisons += 3;
    if (items[mid].key < items[low].key) swap(items[mid], items[low]);
    if (items[high].key < items[low].key) swap(items[high], items[low]);
    if (items[high].key < items[mid].key) swap(items[high], items[mid]);
    swap(items[mid], items[high]);      // median moves to the pivot position
    stats.moves++;
}

// Recurse on the SMALLER part and loop on the larger part, so the recursion
// depth stays O(log n) even in the O(n^2) worst case (safe on Windows' 1 MB stack).
void quickSortRecursive(vector<SortItem> &items, int low, int high, int rule, int depth, SortStats &stats) {
    if (depth > stats.maxDepth) stats.maxDepth = depth;
    while (low < high) {
        choosePivot(items, low, high, rule, stats);
        int p = partitionLomuto(items, low, high, stats);
        if (p - low < high - p) {
            quickSortRecursive(items, low, p - 1, rule, depth + 1, stats);
            low = p + 1;
        } else {
            quickSortRecursive(items, p + 1, high, rule, depth + 1, stats);
            high = p - 1;
        }
    }
}

void quickSort(vector<SortItem> &items, int rule, SortStats &stats) {
    if (items.size() < 2) return;
    quickSortRecursive(items, 0, (int)items.size() - 1, rule, 1, stats);
}

bool isSortedItems(const vector<SortItem> &items) {
    for (size_t i = 1; i < items.size(); i++)
        if (items[i - 1].key > items[i].key) return false;
    return true;
}

// Iterative binary search: returns index of target or -1. Theta(log n), O(1) space.
int binarySearchIterative(const vector<long long> &keys, long long target, long long &comparisons) {
    int low = 0, high = (int)keys.size() - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        comparisons++;
        if (keys[mid] == target) return mid;
        comparisons++;
        if (keys[mid] < target) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

// Recursive binary search: T(n) = T(n/2) + O(1) -> Theta(log n), O(log n) stack.
int binarySearchRecursive(const vector<long long> &keys, int low, int high, long long target,
                          long long &comparisons) {
    if (low > high) return -1;
    int mid = low + (high - low) / 2;
    comparisons++;
    if (keys[mid] == target) return mid;
    comparisons++;
    if (keys[mid] < target) return binarySearchRecursive(keys, mid + 1, high, target, comparisons);
    return binarySearchRecursive(keys, low, mid - 1, target, comparisons);
}

// First index whose key >= target (used for time-window queries).
int lowerBoundIterative(const vector<long long> &keys, long long target, long long &comparisons) {
    int low = 0, high = (int)keys.size();
    while (low < high) {
        int mid = low + (high - low) / 2;
        comparisons++;
        if (keys[mid] < target) low = mid + 1;
        else high = mid;
    }
    return low;
}

int linearSearch(const vector<long long> &keys, long long target, long long &comparisons) {
    for (size_t i = 0; i < keys.size(); i++) {
        comparisons++;
        if (keys[i] == target) return (int)i;
    }
    return -1;
}

vector<long long> keysOf(const vector<SortItem> &items) {
    vector<long long> keys;
    for (const SortItem &item : items) keys.push_back(item.key);
    return keys;
}

// ============================================================================
// SECTION 4: MIN-HEAP (hand-written priority queue)
// Used by Huffman coding, interval partitioning and least-cost branch & bound.
// ============================================================================

struct HeapItem {
    double key;
    int id;
};

bool heapLess(const HeapItem &a, const HeapItem &b) {
    if (a.key != b.key) return a.key < b.key;
    return a.id < b.id;      // tie-break on id keeps results deterministic
}

struct MinHeap {
    vector<HeapItem> data;
    bool empty() const { return data.empty(); }
    int size() const { return (int)data.size(); }
    HeapItem top() const { return data[0]; }

    void push(double key, int id) {                     // O(log n)
        data.push_back({key, id});
        int child = (int)data.size() - 1;
        while (child > 0) {
            int parent = (child - 1) / 2;
            if (!heapLess(data[child], data[parent])) break;
            swap(data[child], data[parent]);
            child = parent;
        }
    }

    HeapItem pop() {                                     // O(log n)
        HeapItem best = data[0];
        data[0] = data.back();
        data.pop_back();
        int parent = 0, n = (int)data.size();
        while (true) {
            int left = 2 * parent + 1, right = left + 1, smallest = parent;
            if (left < n && heapLess(data[left], data[smallest])) smallest = left;
            if (right < n && heapLess(data[right], data[smallest])) smallest = right;
            if (smallest == parent) break;
            swap(data[parent], data[smallest]);
            parent = smallest;
        }
        return best;
    }
};

// ============================================================================
// SECTION 5: DIVIDE AND CONQUER
// Maximum subarray, fast exponentiation, celebrity identification,
// classical and Strassen matrix multiplication.
// ============================================================================

struct SubarrayResult {
    int low = -1;
    int high = -1;
    long long sum = 0;
};

SubarrayResult maxCrossingSubarray(const vector<long long> &a, int low, int mid, int high, long long &ops) {
    long long leftSum = LLONG_MIN, sum = 0;
    int maxLeft = mid;
    for (int i = mid; i >= low; i--) {
        ops++;
        sum += a[i];
        if (sum > leftSum) { leftSum = sum; maxLeft = i; }
    }
    long long rightSum = LLONG_MIN;
    sum = 0;
    int maxRight = mid + 1;
    for (int j = mid + 1; j <= high; j++) {
        ops++;
        sum += a[j];
        if (sum > rightSum) { rightSum = sum; maxRight = j; }
    }
    SubarrayResult result;
    result.low = maxLeft;
    result.high = maxRight;
    result.sum = leftSum + rightSum;
    return result;
}

// T(n) = 2T(n/2) + Theta(n) -> Theta(n log n)
SubarrayResult maxSubarrayDC(const vector<long long> &a, int low, int high, long long &ops) {
    SubarrayResult result;
    if (low > high) return result;                     // empty input
    if (low == high) {
        ops++;
        result.low = low; result.high = high; result.sum = a[low];
        return result;
    }
    int mid = low + (high - low) / 2;
    SubarrayResult left = maxSubarrayDC(a, low, mid, ops);
    SubarrayResult right = maxSubarrayDC(a, mid + 1, high, ops);
    SubarrayResult cross = maxCrossingSubarray(a, low, mid, high, ops);
    if (left.sum >= right.sum && left.sum >= cross.sum) return left;
    if (right.sum >= left.sum && right.sum >= cross.sum) return right;
    return cross;
}

// Kadane's algorithm (dynamic-programming comparison), Theta(n).
SubarrayResult maxSubarrayKadane(const vector<long long> &a, long long &ops) {
    SubarrayResult best;
    if (a.empty()) return best;
    best.low = 0; best.high = 0; best.sum = a[0];
    long long current = a[0];
    int start = 0;
    for (int i = 1; i < (int)a.size(); i++) {
        ops++;
        if (current < 0) { current = a[i]; start = i; }
        else current += a[i];
        if (current > best.sum) { best.sum = current; best.low = start; best.high = i; }
    }
    return best;
}

// Brute force oracle for tests, Theta(n^2).
SubarrayResult maxSubarrayBrute(const vector<long long> &a, long long &ops) {
    SubarrayResult best;
    if (a.empty()) return best;
    best.sum = LLONG_MIN;
    for (int i = 0; i < (int)a.size(); i++) {
        long long sum = 0;
        for (int j = i; j < (int)a.size(); j++) {
            ops++;
            sum += a[j];
            if (sum > best.sum) { best.sum = sum; best.low = i; best.high = j; }
        }
    }
    return best;
}

// Fast exponentiation (base^exponent mod mod). Safe for mod <= 2e9.
// T(e) = T(e/2) + O(1) -> Theta(log e) multiplications.
long long fastPowerIterative(long long base, long long exponent, long long mod, long long &multiplications) {
    if (mod == 1) return 0;
    long long result = 1;
    base %= mod;
    if (base < 0) base += mod;
    while (exponent > 0) {
        if (exponent % 2 == 1) { result = result * base % mod; multiplications++; }   // odd: use one copy of base
        base = base * base % mod;                                                     // square the base
        multiplications++;
        exponent = exponent / 2;                                                      // halve the exponent
    }
    return result;
}

long long fastPowerRecursive(long long base, long long exponent, long long mod, long long &multiplications) {
    if (mod == 1) return 0;
    base %= mod;
    if (base < 0) base += mod;
    if (exponent == 0) return 1;
    long long half = fastPowerRecursive(base, exponent / 2, mod, multiplications);
    long long result = half * half % mod;
    multiplications++;
    if (exponent % 2 == 1) { result = result * base % mod; multiplications++; }
    return result;
}

long long naivePower(long long base, long long exponent, long long mod, long long &multiplications) {
    if (mod == 1) return 0;
    base %= mod;
    if (base < 0) base += mod;
    long long result = 1;
    for (long long i = 0; i < exponent; i++) { result = result * base % mod; multiplications++; }
    return result;
}

// Celebrity identification. knows[a][b] = 1 means endpoint a contacted b.
// A "celebrity" is contacted by everyone but contacts nobody -> in proctoring,
// a shared relay/answer server that every compromised device talks to.
bool askKnows(const vector<vector<int>> &knows, int a, int b, long long &questions) {
    questions++;
    return knows[a][b] == 1;
}

// Divide and conquer: find a candidate in each half, one question eliminates one.
// T(n) = 2T(n/2) + O(1) -> Theta(n) questions (Master theorem case 1).
int celebrityCandidateDC(const vector<vector<int>> &knows, int low, int high, long long &questions) {
    if (low == high) return low;
    int mid = low + (high - low) / 2;
    int a = celebrityCandidateDC(knows, low, mid, questions);
    int b = celebrityCandidateDC(knows, mid + 1, high, questions);
    if (askKnows(knows, a, b, questions)) return b;   // a knows someone, so a is not the celebrity
    return a;                                          // b is not known by a, so b is not the celebrity
}

int findCelebrity(const vector<vector<int>> &knows, long long &questions) {
    int n = (int)knows.size();
    if (n == 0) return -1;
    int candidate = celebrityCandidateDC(knows, 0, n - 1, questions);
    for (int i = 0; i < n; i++) {                      // verification pass, at most 2(n-1) questions
        if (i == candidate) continue;
        if (askKnows(knows, candidate, i, questions) || !askKnows(knows, i, candidate, questions)) return -1;
    }
    return candidate;
}

int findCelebrityBrute(const vector<vector<int>> &knows, long long &questions) {
    int n = (int)knows.size();
    for (int c = 0; c < n; c++) {
        bool ok = true;
        for (int i = 0; i < n && ok; i++) {
            if (i == c) continue;
            if (askKnows(knows, c, i, questions) || !askKnows(knows, i, c, questions)) ok = false;
        }
        if (ok) return c;
    }
    return -1;
}

typedef vector<vector<long long>> Matrix;

Matrix makeMatrix(int rows, int cols) { return Matrix(rows, vector<long long>(cols, 0)); }

// Classical multiplication Theta(n*m*p). Returns empty matrix on size mismatch.
Matrix multiplyClassical(const Matrix &A, const Matrix &B, long long &multiplications) {
    if (A.empty() || B.empty() || A[0].size() != B.size()) return Matrix();
    int n = (int)A.size(), m = (int)B.size(), p = (int)B[0].size();
    Matrix C = makeMatrix(n, p);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < p; j++) {
            long long sum = 0;
            for (int k = 0; k < m; k++) { sum += A[i][k] * B[k][j]; multiplications++; }
            C[i][j] = sum;
        }
    return C;
}

Matrix addMatrix(const Matrix &A, const Matrix &B) {
    int n = (int)A.size();
    Matrix C = makeMatrix(n, n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) C[i][j] = A[i][j] + B[i][j];
    return C;
}

Matrix subtractMatrix(const Matrix &A, const Matrix &B) {
    int n = (int)A.size();
    Matrix C = makeMatrix(n, n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) C[i][j] = A[i][j] - B[i][j];
    return C;
}

// Strassen on square power-of-two matrices.
// T(n) = 7T(n/2) + Theta(n^2) -> Theta(n^log2(7)) = Theta(n^2.807).
// leafSize = size at which we switch to the classical method (1 = pure Strassen).
Matrix strassenSquare(const Matrix &A, const Matrix &B, int leafSize, long long &multiplications) {
    int n = (int)A.size();
    if (n <= leafSize || n == 1) return multiplyClassical(A, B, multiplications);
    int h = n / 2;
    Matrix A11 = makeMatrix(h, h), A12 = makeMatrix(h, h), A21 = makeMatrix(h, h), A22 = makeMatrix(h, h);
    Matrix B11 = makeMatrix(h, h), B12 = makeMatrix(h, h), B21 = makeMatrix(h, h), B22 = makeMatrix(h, h);
    for (int i = 0; i < h; i++)
        for (int j = 0; j < h; j++) {
            A11[i][j] = A[i][j];     A12[i][j] = A[i][j + h];
            A21[i][j] = A[i + h][j]; A22[i][j] = A[i + h][j + h];
            B11[i][j] = B[i][j];     B12[i][j] = B[i][j + h];
            B21[i][j] = B[i + h][j]; B22[i][j] = B[i + h][j + h];
        }
    Matrix M1 = strassenSquare(addMatrix(A11, A22), addMatrix(B11, B22), leafSize, multiplications);
    Matrix M2 = strassenSquare(addMatrix(A21, A22), B11, leafSize, multiplications);
    Matrix M3 = strassenSquare(A11, subtractMatrix(B12, B22), leafSize, multiplications);
    Matrix M4 = strassenSquare(A22, subtractMatrix(B21, B11), leafSize, multiplications);
    Matrix M5 = strassenSquare(addMatrix(A11, A12), B22, leafSize, multiplications);
    Matrix M6 = strassenSquare(subtractMatrix(A21, A11), addMatrix(B11, B12), leafSize, multiplications);
    Matrix M7 = strassenSquare(subtractMatrix(A12, A22), addMatrix(B21, B22), leafSize, multiplications);
    Matrix C = makeMatrix(n, n);
    for (int i = 0; i < h; i++)
        for (int j = 0; j < h; j++) {
            C[i][j] = M1[i][j] + M4[i][j] - M5[i][j] + M7[i][j];
            C[i][j + h] = M3[i][j] + M5[i][j];
            C[i + h][j] = M2[i][j] + M4[i][j];
            C[i + h][j + h] = M1[i][j] - M2[i][j] + M3[i][j] + M6[i][j];
        }
    return C;
}

// Pads any rectangular pair to the next power of two, multiplies, crops.
Matrix multiplyStrassen(const Matrix &A, const Matrix &B, int leafSize, long long &multiplications) {
    if (A.empty() || B.empty() || A[0].size() != B.size()) return Matrix();
    int rows = (int)A.size(), inner = (int)B.size(), cols = (int)B[0].size();
    int size = 1;
    while (size < rows || size < inner || size < cols) size *= 2;
    Matrix PA = makeMatrix(size, size), PB = makeMatrix(size, size);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < inner; j++) PA[i][j] = A[i][j];
    for (int i = 0; i < inner; i++)
        for (int j = 0; j < cols; j++) PB[i][j] = B[i][j];
    Matrix PC = strassenSquare(PA, PB, max(1, leafSize), multiplications);
    Matrix C = makeMatrix(rows, cols);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) C[i][j] = PC[i][j];
    return C;
}

// ============================================================================
// SECTION 6: GREEDY ALGORITHMS
// Activity selection, interval partitioning, fractional knapsack, Huffman.
// ============================================================================

struct Interval {
    int id = 0;
    int start = 0;
    int finish = 0;
    string label;
};

// Greedy choice: always take the hearing that finishes earliest.
// Theta(n log n) for sorting + Theta(n) scan. Returns indices into intervals.
vector<int> activitySelection(const vector<Interval> &intervals, long long &ops) {
    vector<SortItem> order;
    for (int i = 0; i < (int)intervals.size(); i++) order.push_back({(long long)intervals[i].finish, i});
    SortStats stats;
    mergeSort(order, stats);
    ops += stats.comparisons;
    vector<int> chosen;
    long long lastFinish = LLONG_MIN;
    for (const SortItem &item : order) {
        ops++;
        const Interval &iv = intervals[item.index];
        if (iv.start >= lastFinish) {
            chosen.push_back(item.index);
            lastFinish = iv.finish;
        }
    }
    return chosen;
}

struct PartitionResult {
    int roomsNeeded = 0;
    vector<int> roomOf;      // room number for every interval
};

// Process by start time; reuse the room that frees up earliest (min-heap).
// The number of rooms equals the maximum overlap depth, which is optimal.
PartitionResult intervalPartitioning(const vector<Interval> &intervals, long long &ops) {
    PartitionResult result;
    result.roomOf.assign(intervals.size(), -1);
    vector<SortItem> order;
    for (int i = 0; i < (int)intervals.size(); i++) order.push_back({(long long)intervals[i].start, i});
    SortStats stats;
    mergeSort(order, stats);
    ops += stats.comparisons;
    MinHeap rooms;                                     // key = time the room becomes free
    for (const SortItem &item : order) {
        ops++;
        const Interval &iv = intervals[item.index];
        if (!rooms.empty() && rooms.top().key <= iv.start) {
            HeapItem freeRoom = rooms.pop();
            result.roomOf[item.index] = freeRoom.id;
            rooms.push(iv.finish, freeRoom.id);
        } else {
            int room = result.roomsNeeded++;
            result.roomOf[item.index] = room;
            rooms.push(iv.finish, room);
        }
    }
    return result;
}

struct FractionalResult {
    double totalValue = 0;
    vector<double> fraction;   // fraction of each item taken
    long long ops = 0;
};

// Sort by value/weight ratio (highest first) and fill greedily. Optimal for the
// FRACTIONAL problem only (partial review of a case is allowed).
FractionalResult fractionalKnapsack(const vector<KnapsackItem> &items, int capacity) {
    FractionalResult result;
    result.fraction.assign(items.size(), 0.0);
    vector<SortItem> order;
    for (int i = 0; i < (int)items.size(); i++) {
        double ratio = items[i].weight > 0 ? (double)items[i].value / items[i].weight : 0.0;
        order.push_back({-(long long)(ratio * 1000000.0), i});
    }
    SortStats stats;
    mergeSort(order, stats);
    result.ops += stats.comparisons;
    double remaining = capacity;
    for (const SortItem &item : order) {
        result.ops++;
        if (remaining <= 0) break;
        const KnapsackItem &it = items[item.index];
        if (it.weight <= 0) continue;
        if (it.weight <= remaining) {
            result.fraction[item.index] = 1.0;
            result.totalValue += it.value;
            remaining -= it.weight;
        } else {
            double part = remaining / it.weight;
            result.fraction[item.index] = part;
            result.totalValue += it.value * part;
            remaining = 0;
        }
    }
    return result;
}

struct HuffmanNode {
    long long frequency = 0;
    int symbol = -1;          // -1 for internal nodes
    int left = -1;
    int right = -1;
};

struct HuffmanCode {
    vector<HuffmanNode> nodes;
    int root = -1;
    string codes[256];
};

void assignHuffmanCodes(HuffmanCode &h, int node, const string &prefix) {
    if (node < 0) return;
    if (h.nodes[node].symbol >= 0) {
        h.codes[h.nodes[node].symbol] = prefix.empty() ? "0" : prefix;
        return;
    }
    assignHuffmanCodes(h, h.nodes[node].left, prefix + "0");
    assignHuffmanCodes(h, h.nodes[node].right, prefix + "1");
}

// Greedy: repeatedly merge the two least frequent trees. O(n + k log k).
HuffmanCode buildHuffman(const string &text, long long &ops) {
    HuffmanCode h;
    long long freq[256] = {0};
    for (char ch : text) { freq[(unsigned char)ch]++; ops++; }
    MinHeap heap;
    for (int s = 0; s < 256; s++) {
        if (freq[s] == 0) continue;
        HuffmanNode leaf;
        leaf.frequency = freq[s];
        leaf.symbol = s;
        h.nodes.push_back(leaf);
        heap.push((double)freq[s], (int)h.nodes.size() - 1);
    }
    if (h.nodes.empty()) return h;
    if (h.nodes.size() == 1) {           // only one distinct symbol: give it code "0"
        h.root = 0;
        h.codes[h.nodes[0].symbol] = "0";
        return h;
    }
    while (heap.size() > 1) {
        HeapItem a = heap.pop();
        HeapItem b = heap.pop();
        ops++;
        HuffmanNode parent;
        parent.frequency = h.nodes[a.id].frequency + h.nodes[b.id].frequency;
        parent.left = a.id;
        parent.right = b.id;
        h.nodes.push_back(parent);
        heap.push((double)parent.frequency, (int)h.nodes.size() - 1);
    }
    h.root = heap.pop().id;
    assignHuffmanCodes(h, h.root, "");
    return h;
}

string huffmanEncode(const string &text, const HuffmanCode &h) {
    string bits;
    for (char ch : text) bits += h.codes[(unsigned char)ch];
    return bits;
}

string huffmanDecode(const string &bits, const HuffmanCode &h) {
    string text;
    if (h.root < 0) return text;
    if (h.nodes[h.root].symbol >= 0) {         // single-symbol tree
        for (size_t i = 0; i < bits.size(); i++) text += (char)h.nodes[h.root].symbol;
        return text;
    }
    int node = h.root;
    for (char bit : bits) {
        node = (bit == '0') ? h.nodes[node].left : h.nodes[node].right;
        if (h.nodes[node].symbol >= 0) {
            text += (char)h.nodes[node].symbol;
            node = h.root;
        }
    }
    return text;
}

// ============================================================================
// SECTION 7: GRAPH ALGORITHMS
// Dijkstra (greedy), Prim and Kruskal (greedy MST),
// Bellman-Ford and Floyd-Warshall (dynamic programming on paths).
// ============================================================================

struct Edge {
    int from = 0;
    int to = 0;
    long long weight = 0;
};

struct Graph {
    int vertexCount = 0;
    bool directed = false;
    vector<string> names;
    vector<Edge> edges;
};

struct Neighbor {
    int to;
    long long weight;
};

vector<vector<Neighbor>> buildAdjacency(const Graph &g) {
    vector<vector<Neighbor>> adjacency(g.vertexCount);
    for (const Edge &e : g.edges) {
        adjacency[e.from].push_back({e.to, e.weight});
        if (!g.directed) adjacency[e.to].push_back({e.from, e.weight});
    }
    return adjacency;
}

struct PathResult {
    vector<long long> distance;
    vector<int> parent;
    bool negativeCycle = false;
    bool invalidInput = false;
    long long operations = 0;
};

// Dijkstra, array version: Theta(V^2 + E). Requires non-negative weights.
PathResult dijkstra(const Graph &g, int source) {
    PathResult result;
    int n = g.vertexCount;
    result.distance.assign(n, INF);
    result.parent.assign(n, -1);
    if (source < 0 || source >= n) { result.invalidInput = true; return result; }
    for (const Edge &e : g.edges)
        if (e.weight < 0) { result.invalidInput = true; return result; }
    vector<vector<Neighbor>> adjacency = buildAdjacency(g);
    vector<bool> done(n, false);
    result.distance[source] = 0;
    for (int iteration = 0; iteration < n; iteration++) {
        int u = -1;
        for (int v = 0; v < n; v++) {            // pick closest unfinished vertex (greedy choice)
            result.operations++;
            if (!done[v] && result.distance[v] < INF && (u == -1 || result.distance[v] < result.distance[u])) u = v;
        }
        if (u == -1) break;                       // remaining vertices are unreachable
        done[u] = true;
        for (const Neighbor &nb : adjacency[u]) {
            result.operations++;
            if (result.distance[u] + nb.weight < result.distance[nb.to]) {
                result.distance[nb.to] = result.distance[u] + nb.weight;
                result.parent[nb.to] = u;
            }
        }
    }
    return result;
}

// Bellman-Ford: relax every edge V-1 times; one more pass detects negative cycles.
// O(V*E). Works with negative weights.
PathResult bellmanFord(const Graph &g, int source) {
    PathResult result;
    int n = g.vertexCount;
    result.distance.assign(n, INF);
    result.parent.assign(n, -1);
    if (source < 0 || source >= n) { result.invalidInput = true; return result; }
    vector<Edge> directedEdges;
    for (const Edge &e : g.edges) {
        directedEdges.push_back(e);
        if (!g.directed) directedEdges.push_back({e.to, e.from, e.weight});
    }
    result.distance[source] = 0;
    for (int pass = 1; pass <= n - 1; pass++) {
        bool changed = false;
        for (const Edge &e : directedEdges) {
            result.operations++;
            if (result.distance[e.from] < INF && result.distance[e.from] + e.weight < result.distance[e.to]) {
                result.distance[e.to] = result.distance[e.from] + e.weight;
                result.parent[e.to] = e.from;
                changed = true;
            }
        }
        if (!changed) break;                      // early exit: best case Theta(E)
    }
    for (const Edge &e : directedEdges) {
        result.operations++;
        if (result.distance[e.from] < INF && result.distance[e.from] + e.weight < result.distance[e.to]) {
            result.negativeCycle = true;
            break;
        }
    }
    return result;
}

// Floyd-Warshall: dist_k[i][j] = min(dist_{k-1}[i][j], dist_{k-1}[i][k] + dist_{k-1}[k][j]).
// Theta(V^3) time, Theta(V^2) space.
vector<vector<long long>> floydWarshall(const Graph &g, vector<vector<int>> &nextHop, long long &ops,
                                        bool &negativeCycle) {
    int n = g.vertexCount;
    vector<vector<long long>> dist(n, vector<long long>(n, INF));
    nextHop.assign(n, vector<int>(n, -1));
    for (int i = 0; i < n; i++) { dist[i][i] = 0; nextHop[i][i] = i; }
    for (const Edge &e : g.edges) {
        if (e.weight < dist[e.from][e.to]) { dist[e.from][e.to] = e.weight; nextHop[e.from][e.to] = e.to; }
        if (!g.directed && e.weight < dist[e.to][e.from]) { dist[e.to][e.from] = e.weight; nextHop[e.to][e.from] = e.from; }
    }
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++) {
            if (dist[i][k] >= INF) continue;
            for (int j = 0; j < n; j++) {
                ops++;
                if (dist[k][j] >= INF) continue;
                if (dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    nextHop[i][j] = nextHop[i][k];
                }
            }
        }
    negativeCycle = false;
    for (int i = 0; i < n; i++)
        if (dist[i][i] < 0) negativeCycle = true;
    return dist;
}

string pathFromParents(const vector<int> &parent, int target, const vector<string> &names) {
    vector<int> reversed;
    int guard = 0;
    for (int v = target; v != -1 && guard <= (int)parent.size(); v = parent[v], guard++) reversed.push_back(v);
    string text;
    for (int i = (int)reversed.size() - 1; i >= 0; i--) {
        text += names[reversed[i]];
        if (i > 0) text += " -> ";
    }
    return text;
}

struct MSTResult {
    vector<Edge> chosen;
    long long totalWeight = 0;
    bool spanning = true;
    int components = 0;
    long long operations = 0;
};

// Prim with an adjacency matrix: Theta(V^2). Builds a spanning FOREST if the
// graph is disconnected (and reports spanning = false).
MSTResult primMST(const Graph &g) {
    MSTResult result;
    int n = g.vertexCount;
    if (n == 0) return result;
    vector<vector<long long>> weight(n, vector<long long>(n, INF));
    for (const Edge &e : g.edges) {
        if (e.weight < weight[e.from][e.to]) weight[e.from][e.to] = weight[e.to][e.from] = e.weight;
    }
    vector<long long> key(n, INF);
    vector<int> parent(n, -1);
    vector<bool> inTree(n, false);
    for (int added = 0; added < n; added++) {
        int u = -1;
        for (int v = 0; v < n; v++) {
            result.operations++;
            if (!inTree[v] && (u == -1 || key[v] < key[u])) u = v;
        }
        if (key[u] >= INF) { result.components++; key[u] = 0; }   // start a new tree
        inTree[u] = true;
        if (parent[u] != -1) {
            result.chosen.push_back({parent[u], u, key[u]});
            result.totalWeight += key[u];
        }
        for (int v = 0; v < n; v++) {
            result.operations++;
            if (!inTree[v] && weight[u][v] < key[v]) { key[v] = weight[u][v]; parent[v] = u; }
        }
    }
    result.spanning = (result.components <= 1);
    return result;
}

int findSet(vector<int> &parent, int x) {
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];    // path halving
        x = parent[x];
    }
    return x;
}

bool unionSets(vector<int> &parent, vector<int> &rankOf, int a, int b) {
    int ra = findSet(parent, a), rb = findSet(parent, b);
    if (ra == rb) return false;
    if (rankOf[ra] < rankOf[rb]) swap(ra, rb);
    parent[rb] = ra;
    if (rankOf[ra] == rankOf[rb]) rankOf[ra]++;
    return true;
}

// Kruskal: sort edges (our merge sort) then add the cheapest edge that joins
// two different components (union-find). Theta(E log E).
MSTResult kruskalMST(const Graph &g) {
    MSTResult result;
    int n = g.vertexCount;
    vector<SortItem> order;
    for (int i = 0; i < (int)g.edges.size(); i++) order.push_back({g.edges[i].weight, i});
    SortStats stats;
    mergeSort(order, stats);
    result.operations += stats.comparisons;
    vector<int> parent(n), rankOf(n, 0);
    for (int i = 0; i < n; i++) parent[i] = i;
    for (const SortItem &item : order) {
        result.operations++;
        const Edge &e = g.edges[item.index];
        if (unionSets(parent, rankOf, e.from, e.to)) {
            result.chosen.push_back(e);
            result.totalWeight += e.weight;
        }
    }
    result.components = n - (int)result.chosen.size();
    result.spanning = (n == 0) || ((int)result.chosen.size() == n - 1);
    return result;
}

// Examination infrastructure network (latency in ms; also used as cable cost).
Graph buildInfrastructureGraph(bool linkFailure, bool isolateCentreE) {
    Graph g;
    g.directed = false;
    g.names = {"ProctorServer", "NorthHub", "SouthHub", "BackupDC", "CentreA",
               "CentreB", "CentreC", "CentreD", "CentreE"};
    g.vertexCount = (int)g.names.size();
    int raw[16][3] = {{0, 1, 12}, {0, 2, 15}, {0, 3, 9}, {1, 3, 7}, {2, 3, 11}, {1, 4, 5}, {1, 5, 8}, {2, 6, 6},
                      {2, 7, 9},  {3, 5, 10}, {3, 6, 13}, {4, 5, 4}, {6, 7, 3}, {7, 8, 7}, {2, 8, 14}, {5, 8, 20}};
    for (int i = 0; i < 16; i++) {
        int a = raw[i][0], b = raw[i][1];
        if (linkFailure && ((a == 0 && b == 1) || (a == 1 && b == 3))) continue;
        if (isolateCentreE && (a == 8 || b == 8)) continue;
        g.edges.push_back({a, b, (long long)raw[i][2]});
    }
    return g;
}

Graph randomConnectedGraph(int vertices, int extraEdgesPerVertex, unsigned long long seed, bool directed) {
    Graph g;
    g.vertexCount = vertices;
    g.directed = directed;
    for (int i = 0; i < vertices; i++) g.names.push_back("V" + to_string(i));
    SimpleRandom rng(seed);
    for (int v = 1; v < vertices; v++) g.edges.push_back({rng.range(0, v - 1), v, (long long)rng.range(1, 100)});
    for (int k = 0; k < vertices * extraEdgesPerVertex; k++) {
        int a = rng.range(0, vertices - 1), b = rng.range(0, vertices - 1);
        if (a != b) g.edges.push_back({a, b, (long long)rng.range(1, 100)});
    }
    return g;
}

// ============================================================================
// SECTION 8: DYNAMIC PROGRAMMING
// 0/1 knapsack, matrix-chain multiplication, LCS, optimal BST, TSP (Held-Karp).
// (Bellman-Ford and Floyd-Warshall are in Section 7.)
// ============================================================================

struct KnapsackResult {
    string method;
    int bestValue = 0;
    int totalWeight = 0;
    vector<int> chosen;          // chosen[i] = 1 if item i selected
    long long operations = 0;
    long long nodesGenerated = 0;
    bool aborted = false;
    double millis = 0;
};

bool validateKnapsack(const vector<KnapsackItem> &items, int capacity, string &error) {
    if (capacity < 0) { error = "capacity must be >= 0"; return false; }
    for (const KnapsackItem &it : items) {
        if (it.weight < 1) { error = "every item needs weight >= 1 (item " + it.label + ")"; return false; }
        if (it.value < 0) { error = "values must be >= 0 (item " + it.label + ")"; return false; }
    }
    return true;
}

void fillTotals(KnapsackResult &r, const vector<KnapsackItem> &items) {
    r.totalWeight = 0;
    int value = 0;
    for (size_t i = 0; i < items.size(); i++)
        if (r.chosen[i]) { r.totalWeight += items[i].weight; value += items[i].value; }
    r.bestValue = value;
}

// table[i][w] = best value using the first i items with capacity w.
// Theta(n*W) time and space (pseudo-polynomial).
KnapsackResult knapsackDP(const vector<KnapsackItem> &items, int capacity) {
    KnapsackResult r;
    r.method = "0/1 Knapsack DP";
    int n = (int)items.size();
    r.chosen.assign(n, 0);
    if (capacity < 0) return r;
    vector<vector<int>> table(n + 1, vector<int>(capacity + 1, 0));
    for (int i = 1; i <= n; i++) {
        int w = items[i - 1].weight, v = items[i - 1].value;
        for (int c = 0; c <= capacity; c++) {
            r.operations++;
            table[i][c] = table[i - 1][c];
            if (w <= c && table[i - 1][c - w] + v > table[i][c]) table[i][c] = table[i - 1][c - w] + v;
        }
    }
    int c = capacity;
    for (int i = n; i >= 1; i--) {
        if (table[i][c] != table[i - 1][c]) { r.chosen[i - 1] = 1; c -= items[i - 1].weight; }
    }
    fillTotals(r, items);
    return r;
}

// Greedy-by-ratio heuristic for 0/1 knapsack (fast but NOT always optimal).
KnapsackResult knapsackGreedyRatio(const vector<KnapsackItem> &items, int capacity) {
    KnapsackResult r;
    r.method = "Greedy ratio (heuristic)";
    r.chosen.assign(items.size(), 0);
    vector<SortItem> order;
    for (int i = 0; i < (int)items.size(); i++)
        order.push_back({-(long long)((double)items[i].value / max(1, items[i].weight) * 1000000.0), i});
    SortStats stats;
    mergeSort(order, stats);
    r.operations = stats.comparisons;
    int used = 0;
    for (const SortItem &item : order) {
        r.operations++;
        if (used + items[item.index].weight <= capacity) { r.chosen[item.index] = 1; used += items[item.index].weight; }
    }
    fillTotals(r, items);
    return r;
}

struct MCMResult {
    long long minCost = 0;
    long long leftToRightCost = 0;
    string order;
    bool valid = false;
};

string buildParenthesis(const vector<vector<int>> &split, int i, int j, const vector<string> &names) {
    if (i == j) return names[i];
    int k = split[i][j];
    return "(" + buildParenthesis(split, i, k, names) + " x " + buildParenthesis(split, k + 1, j, names) + ")";
}

// dims has n+1 values: matrix i is dims[i] x dims[i+1].
// cost[i][j] = min over k of cost[i][k] + cost[k+1][j] + dims[i]*dims[k+1]*dims[j+1]. Theta(n^3).
MCMResult matrixChainOrder(const vector<long long> &dims, const vector<string> &names) {
    MCMResult result;
    int n = (int)dims.size() - 1;
    if (n < 1 || (int)names.size() < n) return result;
    vector<vector<long long>> cost(n, vector<long long>(n, 0));
    vector<vector<int>> split(n, vector<int>(n, 0));
    for (int length = 2; length <= n; length++)
        for (int i = 0; i + length - 1 < n; i++) {
            int j = i + length - 1;
            cost[i][j] = INF;
            for (int k = i; k < j; k++) {
                long long c = cost[i][k] + cost[k + 1][j] + dims[i] * dims[k + 1] * dims[j + 1];
                if (c < cost[i][j]) { cost[i][j] = c; split[i][j] = k; }
            }
        }
    result.minCost = cost[0][n - 1];
    result.order = buildParenthesis(split, 0, n - 1, names);
    for (int k = 1; k < n; k++) result.leftToRightCost += dims[0] * dims[k] * dims[k + 1];
    result.valid = true;
    return result;
}

// Longest common subsequence: Theta(n*m) time and space.
int lcsLength(const string &a, const string &b, string &common, long long &ops) {
    int n = (int)a.size(), m = (int)b.size();
    vector<vector<int>> table(n + 1, vector<int>(m + 1, 0));
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= m; j++) {
            ops++;
            if (a[i - 1] == b[j - 1]) table[i][j] = table[i - 1][j - 1] + 1;
            else table[i][j] = max(table[i - 1][j], table[i][j - 1]);
        }
    common.clear();
    int i = n, j = m;
    while (i > 0 && j > 0) {
        if (a[i - 1] == b[j - 1]) { common = a[i - 1] + common; i--; j--; }
        else if (table[i - 1][j] >= table[i][j - 1]) i--;
        else j--;
    }
    return table[n][m];
}

struct OBSTResult {
    long long cost = 0;
    vector<vector<int>> root;
};

// Optimal BST for sorted keys with search frequencies.
// cost[i][j] = sum(freq i..j) + min over r of cost[i][r-1] + cost[r+1][j]. Theta(n^3).
OBSTResult optimalBST(const vector<long long> &freq) {
    OBSTResult result;
    int n = (int)freq.size();
    if (n == 0) return result;
    vector<vector<long long>> cost(n, vector<long long>(n, 0));
    result.root.assign(n, vector<int>(n, -1));
    vector<long long> prefix(n + 1, 0);
    for (int i = 0; i < n; i++) prefix[i + 1] = prefix[i] + freq[i];
    for (int i = 0; i < n; i++) { cost[i][i] = freq[i]; result.root[i][i] = i; }
    for (int length = 2; length <= n; length++)
        for (int i = 0; i + length - 1 < n; i++) {
            int j = i + length - 1;
            long long total = prefix[j + 1] - prefix[i];
            cost[i][j] = INF;
            for (int r = i; r <= j; r++) {
                long long left = (r > i) ? cost[i][r - 1] : 0;
                long long right = (r < j) ? cost[r + 1][j] : 0;
                if (left + right + total < cost[i][j]) { cost[i][j] = left + right + total; result.root[i][j] = r; }
            }
        }
    result.cost = cost[0][n - 1];
    return result;
}

long long balancedBSTCost(const vector<long long> &freq, int low, int high, int depth) {
    if (low > high) return 0;
    int mid = (low + high) / 2;
    return freq[mid] * depth + balancedBSTCost(freq, low, mid - 1, depth + 1) + balancedBSTCost(freq, mid + 1, high, depth + 1);
}

void printOBSTTree(const vector<vector<int>> &root, const vector<string> &keys, int i, int j, int depth, const string &side) {
    if (i > j) return;
    int r = root[i][j];
    cout << "    " << string(depth * 4, ' ') << side << keys[r] << "\n";
    printOBSTTree(root, keys, i, r - 1, depth + 1, "L: ");
    printOBSTTree(root, keys, r + 1, j, depth + 1, "R: ");
}

struct TSPResult {
    long long cost = INF;
    vector<int> tour;
    long long operations = 0;
    bool valid = false;
};

// Held-Karp: dp[mask][last] = cheapest path that starts at 0, visits mask, ends at last.
// Theta(n^2 * 2^n) time, Theta(n * 2^n) space. Limited to n <= 16.
TSPResult tspHeldKarp(const vector<vector<long long>> &dist) {
    TSPResult result;
    int n = (int)dist.size();
    if (n == 0 || n > 16) return result;
    if (n == 1) { result.cost = 0; result.tour = {0, 0}; result.valid = true; return result; }
    int full = 1 << n;
    vector<vector<long long>> dp(full, vector<long long>(n, INF));
    vector<vector<int>> parent(full, vector<int>(n, -1));
    dp[1][0] = 0;
    for (int mask = 1; mask < full; mask++) {
        if (!(mask & 1)) continue;
        for (int last = 0; last < n; last++) {
            if (!(mask & (1 << last)) || dp[mask][last] >= INF) continue;
            for (int next = 0; next < n; next++) {
                if (mask & (1 << next)) continue;
                result.operations++;
                if (dist[last][next] >= INF) continue;
                int newMask = mask | (1 << next);
                long long cost = dp[mask][last] + dist[last][next];
                if (cost < dp[newMask][next]) { dp[newMask][next] = cost; parent[newMask][next] = last; }
            }
        }
    }
    int bestLast = -1;
    for (int last = 1; last < n; last++) {
        if (dp[full - 1][last] >= INF || dist[last][0] >= INF) continue;
        long long cost = dp[full - 1][last] + dist[last][0];
        if (cost < result.cost) { result.cost = cost; bestLast = last; }
    }
    if (bestLast == -1) return result;           // no Hamiltonian cycle exists
    vector<int> reversed;
    int mask = full - 1, current = bestLast;
    while (current != -1) {
        reversed.push_back(current);
        int previous = parent[mask][current];
        mask ^= (1 << current);
        current = previous;
    }
    result.tour.push_back(0);
    for (int i = (int)reversed.size() - 1; i >= 0; i--)
        if (reversed[i] != 0) result.tour.push_back(reversed[i]);
    result.tour.push_back(0);
    result.valid = true;
    return result;
}

// ============================================================================
// SECTION 9: BACKTRACKING
// N-Queens, sum of subsets, 0/1 knapsack by backtracking, TSP brute force.
// ============================================================================

struct NQueensResult {
    long long solutions = 0;
    long long nodes = 0;
    vector<int> firstSolution;      // column of the queen in each row
};

void placeQueens(int row, int n, vector<int> &colOfRow, vector<bool> &colUsed, vector<bool> &diagUsed,
                 vector<bool> &antiUsed, NQueensResult &result) {
    result.nodes++;
    if (row == n) {
        result.solutions++;
        if (result.firstSolution.empty()) result.firstSolution = colOfRow;
        return;
    }
    for (int col = 0; col < n; col++) {
        if (colUsed[col] || diagUsed[row - col + n - 1] || antiUsed[row + col]) continue;   // prune
        colUsed[col] = diagUsed[row - col + n - 1] = antiUsed[row + col] = true;
        colOfRow[row] = col;
        placeQueens(row + 1, n, colOfRow, colUsed, diagUsed, antiUsed, result);
        colUsed[col] = diagUsed[row - col + n - 1] = antiUsed[row + col] = false;
    }
}

NQueensResult solveNQueens(int n) {
    NQueensResult result;
    if (n <= 0) return result;
    vector<int> colOfRow(n, -1);
    vector<bool> colUsed(n, false), diagUsed(2 * n - 1, false), antiUsed(2 * n - 1, false);
    placeQueens(0, n, colOfRow, colUsed, diagUsed, antiUsed, result);
    return result;
}

struct SubsetResult {
    vector<vector<int>> solutions;     // original indices
    long long totalSolutions = 0;
    long long nodes = 0;
};

// values sorted ascending; prune when the sum is unreachable or overshoots.
void subsetSearch(const vector<int> &values, int index, int currentSum, int remaining, int target,
                  vector<int> &chosen, SubsetResult &result, int maxStored) {
    result.nodes++;
    if (currentSum == target) {
        result.totalSolutions++;
        if ((int)result.solutions.size() < maxStored) result.solutions.push_back(chosen);
        return;
    }
    if (index >= (int)values.size()) return;
    if (currentSum + remaining < target) return;          // cannot reach target any more
    if (currentSum + values[index] > target) return;      // sorted: all later values also overshoot
    chosen.push_back(index);
    subsetSearch(values, index + 1, currentSum + values[index], remaining - values[index], target, chosen, result, maxStored);
    chosen.pop_back();
    subsetSearch(values, index + 1, currentSum, remaining - values[index], target, chosen, result, maxStored);
}

SubsetResult sumOfSubsets(const vector<int> &rawValues, int target, int maxStored) {
    SubsetResult result;
    if (target <= 0) return result;
    for (int v : rawValues)
        if (v <= 0) return result;                         // only positive values allowed
    vector<SortItem> order;
    for (int i = 0; i < (int)rawValues.size(); i++) order.push_back({(long long)rawValues[i], i});
    SortStats stats;
    mergeSort(order, stats);
    vector<int> values;
    int total = 0;
    for (const SortItem &item : order) { values.push_back(rawValues[item.index]); total += rawValues[item.index]; }
    vector<int> chosen;
    subsetSearch(values, 0, 0, total, target, chosen, result, maxStored);
    for (vector<int> &solution : result.solutions)
        for (int &position : solution) position = order[position].index;    // back to original indices
    return result;
}

vector<int> orderByRatio(const vector<KnapsackItem> &items) {
    vector<SortItem> order;
    for (int i = 0; i < (int)items.size(); i++)
        order.push_back({-(long long)((double)items[i].value / max(1, items[i].weight) * 1000000.0), i});
    SortStats stats;
    mergeSort(order, stats);
    vector<int> result;
    for (const SortItem &item : order) result.push_back(item.index);
    return result;
}

// Upper bound by the fractional (greedy) relaxation of the remaining items.
double fractionalBound(const vector<KnapsackItem> &items, const vector<int> &order, int capacity, int level,
                       int weight, int value) {
    if (weight > capacity) return -1;
    double bound = value;
    int w = weight, i = level, n = (int)order.size();
    while (i < n && w + items[order[i]].weight <= capacity) {
        w += items[order[i]].weight;
        bound += items[order[i]].value;
        i++;
    }
    if (i < n) bound += (double)(capacity - w) * items[order[i]].value / items[order[i]].weight;
    return bound;
}

struct BacktrackState {
    vector<int> order;
    int capacity = 0;
    int bestValue = 0;
    vector<int> bestTaken;
    vector<int> currentTaken;
    long long nodes = 0;
};

void knapsackBacktrack(const vector<KnapsackItem> &items, BacktrackState &s, int level, int weight, int value) {
    s.nodes++;
    if (value > s.bestValue) { s.bestValue = value; s.bestTaken = s.currentTaken; }
    if (level == (int)s.order.size()) return;
    double bound = fractionalBound(items, s.order, s.capacity, level, weight, value);
    if (bound <= s.bestValue) return;                    // prune: this branch cannot beat the best
    int item = s.order[level];
    const KnapsackItem &it = items[item];
    if (weight + it.weight <= s.capacity) {              // branch 1: include
        s.currentTaken[item] = 1;
        knapsackBacktrack(items, s, level + 1, weight + it.weight, value + it.value);
        s.currentTaken[item] = 0;
    }
    knapsackBacktrack(items, s, level + 1, weight, value);      // branch 2: exclude
}

KnapsackResult knapsackBacktracking(const vector<KnapsackItem> &items, int capacity) {
    KnapsackResult r;
    r.method = "0/1 Knapsack Backtracking";
    BacktrackState s;
    s.order = orderByRatio(items);
    s.capacity = capacity;
    s.bestTaken.assign(items.size(), 0);
    s.currentTaken.assign(items.size(), 0);
    if (capacity >= 0) knapsackBacktrack(items, s, 0, 0, 0);
    r.chosen = s.bestTaken;
    r.nodesGenerated = s.nodes;
    r.operations = s.nodes;
    fillTotals(r, items);
    return r;
}

// Brute-force TSP by enumerating every permutation (exact oracle, Theta(n!)).
void tspPermute(const vector<vector<long long>> &dist, int current, int visitedCount, long long costSoFar,
                vector<bool> &visited, vector<int> &path, long long &bestCost, vector<int> &bestPath, long long &ops) {
    int n = (int)dist.size();
    if (visitedCount == n) {
        ops++;
        if (dist[current][0] < INF && costSoFar + dist[current][0] < bestCost) {
            bestCost = costSoFar + dist[current][0];
            bestPath = path;
        }
        return;
    }
    for (int next = 0; next < n; next++) {
        if (visited[next] || dist[current][next] >= INF) continue;
        ops++;
        visited[next] = true;
        path.push_back(next);
        tspPermute(dist, next, visitedCount + 1, costSoFar + dist[current][next], visited, path, bestCost, bestPath, ops);
        path.pop_back();
        visited[next] = false;
    }
}

TSPResult tspBruteForce(const vector<vector<long long>> &dist) {
    TSPResult result;
    int n = (int)dist.size();
    if (n == 0 || n > 11) return result;
    if (n == 1) { result.cost = 0; result.tour = {0, 0}; result.valid = true; return result; }
    vector<bool> visited(n, false);
    visited[0] = true;
    vector<int> path = {0}, bestPath;
    long long bestCost = INF;
    tspPermute(dist, 0, 1, 0, visited, path, bestCost, bestPath, result.operations);
    if (bestCost >= INF) return result;
    result.cost = bestCost;
    result.tour = bestPath;
    result.tour.push_back(0);
    result.valid = true;
    return result;
}

// ============================================================================
// SECTION 10: BRANCH AND BOUND (FIFO, LIFO, Least-Cost) for 0/1 knapsack
// ============================================================================

const int BB_FIFO = 0, BB_LIFO = 1, BB_LEAST_COST = 2;

struct BBNode {
    int level;        // number of items already decided (in ratio order)
    int weight;
    int value;
    double bound;
    int parent;       // index of parent node in the pool
    int takenItem;    // item included at this node, or -1
};

// Puts a live node into the right container for the chosen strategy.
void pushLiveNode(int strategy, int id, double bound, vector<int> &fifo, vector<int> &lifo, MinHeap &heap) {
    if (strategy == BB_FIFO) fifo.push_back(id);          // queue: add at the back
    else if (strategy == BB_LIFO) lifo.push_back(id);     // stack: add on top
    else heap.push(-bound, id);                           // min-heap on -bound = max-heap on bound
}

KnapsackResult knapsackBranchAndBound(const vector<KnapsackItem> &items, int capacity, int strategy,
                                      long long nodeLimit) {
    KnapsackResult r;
    if (strategy == BB_FIFO) r.method = "B&B FIFO (queue)";
    else if (strategy == BB_LIFO) r.method = "B&B LIFO (stack)";
    else r.method = "B&B Least-Cost";
    int n = (int)items.size();
    r.chosen.assign(n, 0);
    if (capacity < 0) return r;
    vector<int> order = orderByRatio(items);
    vector<BBNode> pool;
    pool.push_back({0, 0, 0, fractionalBound(items, order, capacity, 0, 0, 0), -1, -1});
    vector<int> fifo;          // queue stored in a vector + head index
    size_t head = 0;
    vector<int> lifo;          // stack
    MinHeap heap;              // key = -bound so the most promising node comes out first
    pushLiveNode(strategy, 0, pool[0].bound, fifo, lifo, heap);
    int best = 0, bestNode = 0;
    while (true) {
        int id;
        if (strategy == BB_FIFO) { if (head >= fifo.size()) break; id = fifo[head++]; }
        else if (strategy == BB_LIFO) { if (lifo.empty()) break; id = lifo.back(); lifo.pop_back(); }
        else { if (heap.empty()) break; id = heap.pop().id; }
        BBNode node = pool[id];
        r.operations++;
        if (node.bound <= best) {
            if (strategy == BB_LEAST_COST) break;   // every remaining node has a smaller bound
            continue;
        }
        if (node.level == n) continue;
        int item = order[node.level];
        const KnapsackItem &it = items[item];
        if (node.weight + it.weight <= capacity) {             // child 1: include
            BBNode child = {node.level + 1, node.weight + it.weight, node.value + it.value, 0, id, item};
            child.bound = fractionalBound(items, order, capacity, child.level, child.weight, child.value);
            pool.push_back(child);
            int childId = (int)pool.size() - 1;
            if (child.value > best) { best = child.value; bestNode = childId; }
            if (child.bound > best) pushLiveNode(strategy, childId, child.bound, fifo, lifo, heap);
        }
        BBNode child = {node.level + 1, node.weight, node.value, 0, id, -1};   // child 2: exclude
        child.bound = fractionalBound(items, order, capacity, child.level, child.weight, child.value);
        if (child.bound > best) {
            pool.push_back(child);
            pushLiveNode(strategy, (int)pool.size() - 1, child.bound, fifo, lifo, heap);
        }
        if ((long long)pool.size() > nodeLimit) { r.aborted = true; break; }
    }
    for (int v = bestNode; v != -1; v = pool[v].parent)
        if (pool[v].takenItem >= 0) r.chosen[pool[v].takenItem] = 1;
    r.nodesGenerated = (long long)pool.size();
    fillTotals(r, items);
    return r;
}

// ============================================================================
// SECTION 11: NETWORK FLOW AND BIPARTITE MATCHING
// ============================================================================

struct FlowNetwork {
    int nodeCount = 0;
    vector<string> names;
    vector<vector<long long>> capacity;
};

FlowNetwork makeFlowNetwork(int nodeCount) {
    FlowNetwork net;
    net.nodeCount = nodeCount;
    net.names.assign(nodeCount, "");
    net.capacity.assign(nodeCount, vector<long long>(nodeCount, 0));
    return net;
}

struct FlowResult {
    long long maxFlow = 0;
    vector<vector<long long>> flow;
    long long augmentingPaths = 0;
    long long operations = 0;
};

// Ford-Fulkerson method: repeatedly find ANY augmenting path (here: DFS) in the
// residual graph and push the bottleneck. O(E * f*) with integer capacities.
FlowResult fordFulkerson(const FlowNetwork &net, int source, int sink) {
    FlowResult result;
    int n = net.nodeCount;
    result.flow.assign(n, vector<long long>(n, 0));
    if (source < 0 || sink < 0 || source >= n || sink >= n || source == sink) return result;
    vector<vector<long long>> residual = net.capacity;
    while (true) {
        vector<int> parent(n, -1);
        vector<bool> visited(n, false);
        vector<int> stack = {source};
        visited[source] = true;
        while (!stack.empty() && !visited[sink]) {
            int u = stack.back();
            stack.pop_back();
            for (int v = 0; v < n; v++) {
                result.operations++;
                if (!visited[v] && residual[u][v] > 0) {
                    visited[v] = true;
                    parent[v] = u;
                    stack.push_back(v);
                }
            }
        }
        if (!visited[sink]) break;                         // no augmenting path -> flow is maximum
        long long bottleneck = INF;
        for (int v = sink; v != source; v = parent[v]) bottleneck = min(bottleneck, residual[parent[v]][v]);
        for (int v = sink; v != source; v = parent[v]) {
            residual[parent[v]][v] -= bottleneck;
            residual[v][parent[v]] += bottleneck;
        }
        result.maxFlow += bottleneck;
        result.augmentingPaths++;
    }
    for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++)
            result.flow[u][v] = max(0LL, net.capacity[u][v] - residual[u][v]);
    return result;
}

// Maximum bipartite matching by augmenting paths (Kuhn). O(V * E).
bool tryAssign(int left, const vector<vector<int>> &adjacent, vector<bool> &visitedRight, vector<int> &matchOfRight,
               long long &ops) {
    for (int right : adjacent[left]) {
        ops++;
        if (visitedRight[right]) continue;
        visitedRight[right] = true;
        if (matchOfRight[right] == -1 || tryAssign(matchOfRight[right], adjacent, visitedRight, matchOfRight, ops)) {
            matchOfRight[right] = left;
            return true;
        }
    }
    return false;
}

int maxBipartiteMatching(const vector<vector<int>> &adjacent, int rightCount, vector<int> &matchOfRight, long long &ops) {
    matchOfRight.assign(rightCount, -1);
    int matched = 0;
    for (int left = 0; left < (int)adjacent.size(); left++) {
        vector<bool> visitedRight(rightCount, false);
        if (tryAssign(left, adjacent, visitedRight, matchOfRight, ops)) matched++;
    }
    return matched;
}

// The same matching solved as a unit-capacity max-flow (cross-check).
int matchingViaFlow(const vector<vector<int>> &adjacent, int rightCount, long long &ops) {
    int leftCount = (int)adjacent.size();
    FlowNetwork net = makeFlowNetwork(leftCount + rightCount + 2);
    int source = 0, sink = leftCount + rightCount + 1;
    for (int l = 0; l < leftCount; l++) {
        net.capacity[source][1 + l] = 1;
        for (int r : adjacent[l]) net.capacity[1 + l][1 + leftCount + r] = 1;
    }
    for (int r = 0; r < rightCount; r++) net.capacity[1 + leftCount + r][sink] = 1;
    FlowResult flow = fordFulkerson(net, source, sink);
    ops += flow.operations;
    return (int)flow.maxFlow;
}

// ============================================================================
// SECTION 12: NUMBER THEORY - CHINESE REMAINDER THEOREM
// ============================================================================

long long gcdOf(long long a, long long b) {
    while (b != 0) { long long t = a % b; a = b; b = t; }
    return a < 0 ? -a : a;
}

long long extendedGcd(long long a, long long b, long long &x, long long &y) {
    if (b == 0) { x = 1; y = 0; return a; }
    long long x1 = 0, y1 = 0;
    long long g = extendedGcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

// Solves x = r_i (mod m_i) for pairwise-coprime moduli. Product limited to 2e9
// so every multiplication fits safely in 64-bit integers.
bool chineseRemainder(const vector<long long> &remainders, const vector<long long> &moduli, long long &answer,
                      long long &product, string &error) {
    if (moduli.empty() || remainders.size() != moduli.size()) { error = "need the same number of remainders and moduli"; return false; }
    product = 1;
    for (long long m : moduli) {
        if (m <= 1) { error = "every modulus must be >= 2"; return false; }
        if (product > 2000000000LL / m) { error = "product of moduli too large (limit 2e9)"; return false; }
        product *= m;
    }
    for (size_t i = 0; i < moduli.size(); i++)
        for (size_t j = i + 1; j < moduli.size(); j++)
            if (gcdOf(moduli[i], moduli[j]) != 1) {
                error = "moduli " + to_string(moduli[i]) + " and " + to_string(moduli[j]) + " are not coprime";
                return false;
            }
    long long result = 0;
    for (size_t i = 0; i < moduli.size(); i++) {
        long long m = moduli[i];
        long long partial = product / m;
        long long x = 0, y = 0;
        extendedGcd(partial % m, m, x, y);
        long long inverse = ((x % m) + m) % m;
        long long r = ((remainders[i] % m) + m) % m;
        long long term = (r * partial) % product * inverse % product;
        result = (result + term) % product;
    }
    answer = result;
    return true;
}

// ============================================================================
// SECTION 13: STRING MATCHING (Naive, Rabin-Karp, Finite Automaton, KMP)
// ============================================================================

struct MatchResult {
    string algorithm;
    vector<int> positions;
    long long comparisons = 0;     // character comparisons / transitions
    long long preprocessing = 0;
    long long spuriousHits = 0;    // Rabin-Karp hash matches that were false
    double millis = 0;
};

MatchResult naiveMatch(const string &text, const string &pattern) {
    MatchResult r;
    r.algorithm = "Naive";
    int n = (int)text.size(), m = (int)pattern.size();
    if (m == 0 || m > n) return r;
    for (int s = 0; s <= n - m; s++) {
        int j = 0;
        while (j < m) {
            r.comparisons++;
            if (text[s + j] != pattern[j]) break;
            j++;
        }
        if (j == m) r.positions.push_back(s);
    }
    return r;
}

MatchResult rabinKarpMatch(const string &text, const string &pattern) {
    MatchResult r;
    r.algorithm = "Rabin-Karp";
    int n = (int)text.size(), m = (int)pattern.size();
    if (m == 0 || m > n) return r;
    const long long BASE = 256, MOD = 1000000007LL;
    long long high = fastPowerIterative(BASE, m - 1, MOD, r.preprocessing);   // BASE^(m-1) mod MOD
    long long patternHash = 0, windowHash = 0;
    for (int i = 0; i < m; i++) {
        patternHash = (patternHash * BASE + (unsigned char)pattern[i]) % MOD;
        windowHash = (windowHash * BASE + (unsigned char)text[i]) % MOD;
        r.preprocessing++;
    }
    for (int s = 0; s <= n - m; s++) {
        r.comparisons++;                                   // one hash comparison
        if (patternHash == windowHash) {
            int j = 0;
            while (j < m) {
                r.comparisons++;
                if (text[s + j] != pattern[j]) break;
                j++;
            }
            if (j == m) r.positions.push_back(s);
            else r.spuriousHits++;
        }
        if (s < n - m) {                                   // roll the hash one character
            windowHash = (windowHash - (unsigned char)text[s] * high % MOD + MOD) % MOD;
            windowHash = (windowHash * BASE + (unsigned char)text[s + m]) % MOD;
        }
    }
    return r;
}

// KMP prefix function: lps[i] = length of the longest proper prefix of
// pattern[0..i] that is also a suffix of it. Theta(m).
vector<int> prefixFunction(const string &pattern, long long &ops) {
    int m = (int)pattern.size();
    vector<int> lps(m, 0);
    int k = 0;
    for (int i = 1; i < m; i++) {
        while (k > 0 && pattern[k] != pattern[i]) { k = lps[k - 1]; ops++; }
        ops++;
        if (pattern[k] == pattern[i]) k++;
        lps[i] = k;
    }
    return lps;
}

// Transition table delta[state][character], built in Theta(m * 256).
vector<vector<int>> buildAutomaton(const string &pattern, long long &ops) {
    int m = (int)pattern.size();
    vector<vector<int>> delta(m + 1, vector<int>(256, 0));
    vector<int> lps = prefixFunction(pattern, ops);
    for (int q = 0; q <= m; q++)
        for (int c = 0; c < 256; c++) {
            ops++;
            if (q < m && (unsigned char)pattern[q] == c) delta[q][c] = q + 1;
            else if (q == 0) delta[q][c] = 0;
            else delta[q][c] = delta[lps[q - 1]][c];
        }
    return delta;
}

MatchResult finiteAutomatonMatch(const string &text, const string &pattern) {
    MatchResult r;
    r.algorithm = "Finite Automaton";
    int n = (int)text.size(), m = (int)pattern.size();
    if (m == 0 || m > n) return r;
    vector<vector<int>> delta = buildAutomaton(pattern, r.preprocessing);
    int state = 0;
    for (int i = 0; i < n; i++) {
        r.comparisons++;                                   // exactly one transition per character
        state = delta[state][(unsigned char)text[i]];
        if (state == m) r.positions.push_back(i - m + 1);
    }
    return r;
}

MatchResult kmpMatch(const string &text, const string &pattern) {
    MatchResult r;
    r.algorithm = "KMP";
    int n = (int)text.size(), m = (int)pattern.size();
    if (m == 0 || m > n) return r;
    vector<int> lps = prefixFunction(pattern, r.preprocessing);
    int q = 0;
    for (int i = 0; i < n; i++) {
        while (true) {
            r.comparisons++;
            if (pattern[q] == text[i]) { q++; break; }
            if (q == 0) break;
            q = lps[q - 1];
        }
        if (q == m) { r.positions.push_back(i - m + 1); q = lps[q - 1]; }
    }
    return r;
}

MatchResult runMatcher(int which, const string &text, const string &pattern) {
    Timer t;
    t.start();
    MatchResult r;
    if (which == 0) r = naiveMatch(text, pattern);
    else if (which == 1) r = rabinKarpMatch(text, pattern);
    else if (which == 2) r = finiteAutomatonMatch(text, pattern);
    else r = kmpMatch(text, pattern);
    r.millis = t.millis();
    return r;
}

// ============================================================================
// SECTION 14: SYNTHETIC DATA GENERATOR AND VALIDATION
// ============================================================================

const string FIRST_NAMES[20] = {"Aarav", "Diya", "Kabir", "Ananya", "Vihaan", "Isha", "Arjun", "Meera", "Rohan", "Sara",
                                "Aditya", "Nisha", "Yash", "Priya", "Karan", "Tara", "Dev", "Riya", "Neel", "Zoya"};
const string LAST_NAMES[15] = {"Sharma", "Iyer", "Khan", "Patel", "Rao", "Nair", "Bose", "Gupta",
                               "Das", "Mehta", "Singh", "Reddy", "Joshi", "Kapoor", "Verma"};
const string REVIEWER_NAMES[6] = {"Dr. R. Mehta", "Prof. S. Iyer", "Ms. A. Khan", "Mr. V. Rao", "Dr. P. Nair", "Dr. K. Bose"};
const int REVIEWER_SKILLS[6] = {CAT_LOGIN | CAT_DEVICE, CAT_BROWSER | CAT_NETWORK, CAT_ALL,
                                CAT_BROWSER | CAT_DEVICE, CAT_LOGIN | CAT_NETWORK, CAT_ALL};

string describeEvent(int type, int deviceId, int extra) {
    switch (type) {
        case LOGIN_OK: return "Login successful on device " + to_string(deviceId);
        case LOGIN_FAILED: return "Login failed - wrong password on device " + to_string(deviceId);
        case TAB_SWITCH: return "Browser focus lost - tab switched";
        case DEVICE_CHANGE: return "Device fingerprint changed from " + to_string(extra) + " to " + to_string(deviceId);
        case NETWORK_DROP: return "Network connection lost on device " + to_string(deviceId);
        case COPY_PASTE: return "Clipboard paste detected in answer box";
        case IP_CHANGE: return "IP address changed during session on device " + to_string(deviceId);
        case ANSWER_SUBMIT: return extra > 0 ? "Answer saved for question " + to_string(extra) : "Final submission";
    }
    return "Unknown event";
}

void addEvent(Dataset &data, int candidateId, const string &examId, int type, int timestamp, int deviceId,
              const string &description) {
    ExamEvent e;
    e.eventId = (int)data.events.size() + 1;
    e.candidateId = candidateId;
    e.examId = examId;
    e.eventType = type;
    if (timestamp < 0) timestamp = 0;
    if (timestamp > data.examDurationSeconds) timestamp = data.examDurationSeconds;
    // Avoid ACCIDENTAL exact duplicates in synthetic data (same candidate, type,
    // second and device): shift by one second. Deliberate duplicates are added separately.
    bool clash = true;
    int step = 1;
    while (clash) {
        clash = false;
        for (int i = (int)data.events.size() - 1; i >= 0 && data.events[i].candidateId == candidateId; i--) {
            const ExamEvent &old = data.events[i];
            if (old.eventType == type && old.timestamp == timestamp && old.deviceId == deviceId) {
                if (timestamp >= data.examDurationSeconds) step = -1;   // walk backwards at the end of the exam
                timestamp += step;
                clash = true;
                break;
            }
        }
    }
    e.timestamp = timestamp;
    e.deviceId = deviceId;
    e.severity = DEFAULT_SEVERITY[type];
    e.description = description;
    data.events.push_back(e);
}

void generateNormalSession(Dataset &data, SimpleRandom &rng, const Candidate &c) {
    int duration = data.examDurationSeconds, device = c.primaryDeviceId;
    int loginTime = rng.range(0, 90);
    addEvent(data, c.candidateId, c.examId, LOGIN_OK, loginTime, device, describeEvent(LOGIN_OK, device, 0));
    int question = 1;
    int t = loginTime + rng.range(90, 200);
    while (t < duration - 120) {
        addEvent(data, c.candidateId, c.examId, ANSWER_SUBMIT, t, device, describeEvent(ANSWER_SUBMIT, device, question));
        question++;
        if (rng.chance(15)) addEvent(data, c.candidateId, c.examId, TAB_SWITCH, t + rng.range(5, 60), device, describeEvent(TAB_SWITCH, device, 0));
        t += rng.range(120, 300);
    }
    if (rng.chance(25)) {
        int drop = rng.range(300, duration - 400);
        addEvent(data, c.candidateId, c.examId, NETWORK_DROP, drop, device, describeEvent(NETWORK_DROP, device, 0));
        addEvent(data, c.candidateId, c.examId, LOGIN_OK, drop + rng.range(15, 60), device, describeEvent(LOGIN_OK, device, 0));
    }
    addEvent(data, c.candidateId, c.examId, ANSWER_SUBMIT, duration - rng.range(5, 60), device, describeEvent(ANSWER_SUBMIT, device, 0));
}

// Five synthetic behaviour patterns. They imitate what a proctoring client
// would log; they are test data, not real candidates.
void plantSuspiciousPattern(Dataset &data, SimpleRandom &rng, const Candidate &c, int pattern) {
    int duration = data.examDurationSeconds;
    int id = c.candidateId, primary = c.primaryDeviceId, alt = c.primaryDeviceId + 100000;
    const string &exam = c.examId;
    int base = rng.range(600, duration - 1500);
    if (pattern == 0) {                    // burst of failed logins from another device
        int t = rng.range(0, 30), tries = rng.range(4, 6);
        for (int k = 0; k < tries; k++) addEvent(data, id, exam, LOGIN_FAILED, t + k * rng.range(5, 10), alt, describeEvent(LOGIN_FAILED, alt, 0));
        for (int k = 0; k < 2; k++) addEvent(data, id, exam, LOGIN_FAILED, base + k * 20, alt, describeEvent(LOGIN_FAILED, alt, 0));
        addEvent(data, id, exam, IP_CHANGE, base + 60, primary, describeEvent(IP_CHANGE, primary, 0));
    } else if (pattern == 1) {             // tab-switching storm with clipboard use
        int switches = rng.range(15, 25), pastes = rng.range(3, 4);
        for (int k = 0; k < switches; k++) addEvent(data, id, exam, TAB_SWITCH, base + rng.range(0, 600), primary, describeEvent(TAB_SWITCH, primary, 0));
        for (int k = 0; k < pastes; k++) addEvent(data, id, exam, COPY_PASTE, base + rng.range(0, 600), primary, describeEvent(COPY_PASTE, primary, 0));
    } else if (pattern == 2) {             // hand-off to a second device right after a disconnect
        addEvent(data, id, exam, NETWORK_DROP, base, primary, describeEvent(NETWORK_DROP, primary, 0));
        addEvent(data, id, exam, LOGIN_OK, base + 12, alt, describeEvent(LOGIN_OK, alt, 0));
        addEvent(data, id, exam, DEVICE_CHANGE, base + 13, alt, describeEvent(DEVICE_CHANGE, alt, primary));
        addEvent(data, id, exam, IP_CHANGE, base + 14, alt, describeEvent(IP_CHANGE, alt, 0));
        for (int k = 0; k < 8; k++) addEvent(data, id, exam, TAB_SWITCH, base + 40 + k * 30, alt, describeEvent(TAB_SWITCH, alt, 0));
        for (int k = 0; k < 2; k++) addEvent(data, id, exam, COPY_PASTE, base + 100 + k * 90, alt, describeEvent(COPY_PASTE, alt, 0));
    } else if (pattern == 3) {             // unstable network (frequently innocent!)
        int drops = rng.range(5, 7);
        for (int k = 0; k < drops; k++) {
            int t = rng.range(200, duration - 200);
            addEvent(data, id, exam, NETWORK_DROP, t, primary, describeEvent(NETWORK_DROP, primary, 0));
            addEvent(data, id, exam, LOGIN_OK, t + rng.range(10, 40), primary, describeEvent(LOGIN_OK, primary, 0));
        }
        addEvent(data, id, exam, IP_CHANGE, rng.range(200, duration - 200), primary, describeEvent(IP_CHANGE, primary, 0));
    } else {                               // device swapping + clipboard + failed logins
        addEvent(data, id, exam, DEVICE_CHANGE, base, alt, describeEvent(DEVICE_CHANGE, alt, primary));
        addEvent(data, id, exam, DEVICE_CHANGE, base + 400, primary, describeEvent(DEVICE_CHANGE, primary, alt));
        for (int k = 0; k < 4; k++) addEvent(data, id, exam, COPY_PASTE, base + 30 + k * 60, alt, describeEvent(COPY_PASTE, alt, 0));
        for (int k = 0; k < 12; k++) addEvent(data, id, exam, TAB_SWITCH, base + rng.range(0, 380), alt, describeEvent(TAB_SWITCH, alt, 0));
        for (int k = 0; k < 4; k++) addEvent(data, id, exam, LOGIN_FAILED, base + 420 + k * 10, alt, describeEvent(LOGIN_FAILED, alt, 0));
    }
}

void addStandardReviewers(Dataset &data, int count, int minutes) {
    data.reviewers.clear();
    for (int i = 0; i < count && i < 6; i++) {
        Reviewer r;
        r.reviewerId = 501 + i;
        r.name = REVIEWER_NAMES[i];
        r.availableMinutes = minutes;
        r.expertiseMask = REVIEWER_SKILLS[i];
        data.reviewers.push_back(r);
    }
}

// Logs are stored in time order (as a real server would write them) and
// event IDs are renumbered in that order. Uses our stable merge sort.
void sortEventsByTimeAndRenumber(Dataset &data) {
    vector<SortItem> items;
    for (int i = 0; i < (int)data.events.size(); i++)
        items.push_back({(long long)data.events[i].timestamp * 10000000LL + data.events[i].candidateId, i});
    SortStats stats;
    mergeSort(items, stats);
    vector<ExamEvent> sorted;
    for (const SortItem &item : items) sorted.push_back(data.events[item.index]);
    for (int i = 0; i < (int)sorted.size(); i++) sorted[i].eventId = i + 1;
    data.events = sorted;
}

// scenario: 1 normal, 2 suspicious, 3 capacity shock, 4 large-data stress.
Dataset generateScenario(int scenario) {
    Dataset data;
    data.scenarioNumber = scenario;
    data.examDurationSeconds = 7200;
    int candidateCount = 30, plantPercent = 0;
    unsigned long long seed = 101;
    if (scenario == 1) {
        data.scenarioName = "Scenario 1 - Normal operation";
        addStandardReviewers(data, 5, 240);
        data.reviewRooms = 3;
    } else if (scenario == 2) {
        data.scenarioName = "Scenario 2 - Suspicious activity";
        plantPercent = 25; seed = 202;
        addStandardReviewers(data, 5, 240);
        data.reviewRooms = 3;
        data.relayPlanted = true;
    } else if (scenario == 3) {
        data.scenarioName = "Scenario 3 - Capacity / constraint shock";
        candidateCount = 40; plantPercent = 50; seed = 303;
        addStandardReviewers(data, 2, 120);
        data.reviewRooms = 1;
        data.relayPlanted = true;
    } else {
        data.scenarioName = "Scenario 4 - Large-data stress test";
        candidateCount = 5000; plantPercent = 10; seed = 404;
        addStandardReviewers(data, 6, 480);
        data.reviewRooms = 4;
    }
    SimpleRandom rng(seed);
    int planted = 0;
    for (int i = 0; i < candidateCount; i++) {
        Candidate c;
        c.candidateId = 1001 + i;
        c.name = FIRST_NAMES[i % 20] + " " + LAST_NAMES[(i / 20 + i) % 15];
        c.examId = (scenario == 4 && i % 2 == 1) ? "DAA-END-02" : "DAA-END-01";
        c.primaryDeviceId = 100000 + c.candidateId;
        data.candidates.push_back(c);
        generateNormalSession(data, rng, c);
        if (plantPercent > 0 && rng.chance(plantPercent)) {
            plantSuspiciousPattern(data, rng, c, planted % 5);   // cycle through all 5 patterns
            planted++;
        }
    }
    if (scenario == 2 || scenario == 3) {
        // Deliberate data-quality faults so validation can be demonstrated.
        ExamEvent duplicate = data.events[5];
        data.events.push_back(duplicate);
        addEvent(data, 9999, "DAA-END-01", TAB_SWITCH, 1200, 999999, "Event from unknown candidate (orphan)");
    }
    sortEventsByTimeAndRenumber(data);
    return data;
}

struct ValidationSummary {
    int invalidRemoved = 0;
    int duplicatesRemoved = 0;
    int duplicateCandidates = 0;
    int orphanEvents = 0;
    vector<string> notes;
};

// Defensive validation: drop invalid rows, remove exact duplicates (same
// candidate, type, timestamp and device), count events of unknown candidates.
ValidationSummary validateDataset(Dataset &data) {
    ValidationSummary summary;
    // 1. duplicate candidate IDs (keep first)
    vector<SortItem> candidateItems;
    for (int i = 0; i < (int)data.candidates.size(); i++) candidateItems.push_back({(long long)data.candidates[i].candidateId, i});
    SortStats stats;
    mergeSort(candidateItems, stats);
    vector<bool> dropCandidate(data.candidates.size(), false);
    for (size_t i = 1; i < candidateItems.size(); i++)
        if (candidateItems[i].key == candidateItems[i - 1].key) { dropCandidate[candidateItems[i].index] = true; summary.duplicateCandidates++; }
    if (summary.duplicateCandidates > 0) {
        vector<Candidate> kept;
        for (size_t i = 0; i < data.candidates.size(); i++) if (!dropCandidate[i]) kept.push_back(data.candidates[i]);
        data.candidates = kept;
    }
    // 2. invalid events
    vector<ExamEvent> valid;
    for (const ExamEvent &e : data.events) {
        bool ok = e.eventType >= 0 && e.eventType < EVENT_TYPE_COUNT && e.timestamp >= 0 &&
                  e.timestamp <= data.examDurationSeconds && e.severity >= 0 && e.severity <= 5 && e.candidateId > 0;
        if (ok) valid.push_back(e);
        else summary.invalidRemoved++;
    }
    // 3. exact duplicates (sort by composite key, compare neighbours inside equal-key runs)
    vector<SortItem> items;
    for (int i = 0; i < (int)valid.size(); i++)
        items.push_back({((long long)valid[i].candidateId * 10000000LL + valid[i].timestamp) * 10 + valid[i].eventType, i});
    SortStats eventStats;
    mergeSort(items, eventStats);
    vector<bool> removeEvent(valid.size(), false);
    size_t runStart = 0;
    while (runStart < items.size()) {
        size_t runEnd = runStart;
        while (runEnd + 1 < items.size() && items[runEnd + 1].key == items[runStart].key) runEnd++;
        for (size_t a = runStart; a <= runEnd; a++)
            for (size_t b = a + 1; b <= runEnd; b++) {
                int ia = items[a].index, ib = items[b].index;
                if (!removeEvent[ia] && !removeEvent[ib] && valid[ia].deviceId == valid[ib].deviceId) {
                    int later = (valid[ia].eventId > valid[ib].eventId) ? ia : ib;   // keep the earlier record
                    removeEvent[later] = true;
                }
            }
        runStart = runEnd + 1;
    }
    vector<ExamEvent> cleaned;
    for (size_t i = 0; i < valid.size(); i++) {
        if (removeEvent[i]) summary.duplicatesRemoved++;
        else cleaned.push_back(valid[i]);
    }
    data.events = cleaned;
    // 4. orphan events (candidate not registered)
    vector<SortItem> keptCandidates;
    for (int i = 0; i < (int)data.candidates.size(); i++) keptCandidates.push_back({(long long)data.candidates[i].candidateId, i});
    SortStats s2;
    mergeSort(keptCandidates, s2);
    vector<long long> candidateKeys = keysOf(keptCandidates);
    for (const ExamEvent &e : data.events) {
        long long cmp = 0;
        if (binarySearchIterative(candidateKeys, e.candidateId, cmp) == -1) summary.orphanEvents++;
    }
    if (summary.invalidRemoved) summary.notes.push_back(to_string(summary.invalidRemoved) + " invalid event(s) removed");
    if (summary.duplicatesRemoved) summary.notes.push_back(to_string(summary.duplicatesRemoved) + " duplicate event(s) removed");
    if (summary.duplicateCandidates) summary.notes.push_back(to_string(summary.duplicateCandidates) + " duplicate candidate ID(s) removed");
    if (summary.orphanEvents) summary.notes.push_back(to_string(summary.orphanEvents) + " event(s) reference unknown candidates (kept, excluded from scoring)");
    if (summary.notes.empty()) summary.notes.push_back("no data-quality problems found");
    return summary;
}

// Network contact matrix for celebrity identification.
// Endpoints: up to 10 candidate devices + ProctorServer (+ relay if planted).
vector<vector<int>> buildContactMatrix(const Dataset &data, vector<string> &names) {
    names.clear();
    int deviceCount = min(10, (int)data.candidates.size());
    for (int i = 0; i < deviceCount; i++) names.push_back("Dev" + to_string(data.candidates[i].primaryDeviceId));
    names.push_back("ProctorServer");
    if (data.relayPlanted) names.push_back("Relay-203.0.113.9");
    int n = (int)names.size();
    vector<vector<int>> knows(n, vector<int>(n, 0));
    SimpleRandom rng(77 + data.scenarioNumber);
    int server = deviceCount;
    for (int i = 0; i < deviceCount; i++) {
        knows[i][server] = 1;                 // every device sends heartbeats to the server
        knows[server][i] = 1;                 // server pings every device back
        for (int j = 0; j < deviceCount; j++)
            if (i != j && rng.chance(10)) knows[i][j] = 1;   // occasional LAN contacts
        if (data.relayPlanted) knows[i][n - 1] = 1;          // every device talks to the relay
    }
    if (data.relayPlanted) knows[server][n - 1] = 1;         // even the server's log shows the relay
    return knows;                                            // relay itself never initiates contact
}

// ============================================================================
// SECTION 15: RISK DETECTION ENGINE AND EVIDENCE TIMELINES
// ============================================================================

struct RiskRules {
    int limit[EVENT_TYPE_COUNT] = {-1, 3, 10, 0, 3, 2, 0, -1};    // -1 = no counting rule
    int weight[EVENT_TYPE_COUNT] = {0, 8, 2, 15, 4, 6, 10, 0};
    string knownPattern = "NLD";            // network drop -> login -> device change
    int patternBonus = 15;
    string violationTemplate = "NLDITTC";   // drop, login, device, IP, tab, tab, copy
    int similarityPercent = 80;
    int templateBonus = 10;
    int burstThreshold = 5;
    int burstBonus = 10;
    int highThreshold = 60;
    int mediumThreshold = 30;
};

struct CandidateRisk {
    int candidateId = 0;
    string name;
    int score = 0;
    string priority = "NONE";
    int counts[EVENT_TYPE_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
    vector<string> reasons;
    string behaviour;               // event codes in time order, e.g. "LSSTNLDS"
    vector<int> eventIndices;       // indices into Dataset::events, time order
    vector<int> patternPositions;   // where knownPattern starts in behaviour
    int patternLength = 0;
    double similarity = 0;
    string lcsText;
    int burstStartMinute = -1, burstEndMinute = -1;
    long long burstValue = 0;
    int category = 0;
    int reviewMinutes = 0;
};

struct AnalysisReport {
    vector<CandidateRisk> risks;    // sorted by score, highest first
    int orphanEvents = 0;
    long long sortComparisons = 0;
    double millis = 0;
    int highCount = 0, mediumCount = 0, lowCount = 0;
};

int categoryOfType(int type) {
    if (type == LOGIN_FAILED) return CAT_LOGIN;
    if (type == TAB_SWITCH || type == COPY_PASTE) return CAT_BROWSER;
    if (type == DEVICE_CHANGE || type == IP_CHANGE) return CAT_DEVICE;
    if (type == NETWORK_DROP) return CAT_NETWORK;
    return 0;
}

// Per-minute "suspicious activity minus baseline" series for max-subarray.
vector<long long> perMinuteSeries(const Dataset &data, const vector<int> &eventIndices, int baseline) {
    int minutes = data.examDurationSeconds / 60 + 1;
    vector<long long> series(minutes, -baseline);
    for (int idx : eventIndices) {
        const ExamEvent &e = data.events[idx];
        if (e.severity < 1) continue;
        int m = e.timestamp / 60;
        if (m < 0) m = 0;
        if (m >= minutes) m = minutes - 1;
        series[m]++;
    }
    return series;
}

void scoreCandidate(CandidateRisk &risk, const Dataset &data, const RiskRules &rules) {
    int contribution[EVENT_TYPE_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
    int score = 0;
    // Rule 1: configurable count limits (explainable: every point has a reason)
    for (int t = 0; t < EVENT_TYPE_COUNT; t++) {
        if (rules.limit[t] < 0 || risk.counts[t] <= rules.limit[t]) continue;
        int points = rules.weight[t] * (risk.counts[t] - rules.limit[t]);
        if (points <= 0) continue;
        contribution[t] = points;
        score += points;
        risk.reasons.push_back(EVENT_NAMES[t] + " x" + to_string(risk.counts[t]) + " (limit " +
                               to_string(rules.limit[t]) + ") -> +" + to_string(points));
    }
    // Rule 2: exact suspicious sequence (KMP string matching)
    if (!rules.knownPattern.empty()) {
        MatchResult m = kmpMatch(risk.behaviour, rules.knownPattern);
        risk.patternLength = (int)rules.knownPattern.size();
        if (!m.positions.empty()) {
            risk.patternPositions = m.positions;
            score += rules.patternBonus;
            risk.reasons.push_back("Sequence '" + rules.knownPattern + "' found " + to_string(m.positions.size()) +
                                   " time(s) (KMP) -> +" + to_string(rules.patternBonus));
        }
    }
    // Rule 3: similarity to a known violation template (LCS dynamic programming)
    if (!rules.violationTemplate.empty()) {
        long long ops = 0;
        int length = lcsLength(risk.behaviour, rules.violationTemplate, risk.lcsText, ops);
        risk.similarity = 100.0 * length / rules.violationTemplate.size();
        if (risk.similarity >= rules.similarityPercent) {
            score += rules.templateBonus;
            risk.reasons.push_back("Behaviour matches " + formatDouble(risk.similarity, 0) + "% of template '" +
                                   rules.violationTemplate + "' (LCS='" + risk.lcsText + "') -> +" + to_string(rules.templateBonus));
        }
    }
    // Rule 4: concentrated burst of suspicious minutes (maximum subarray, D&C)
    vector<long long> series = perMinuteSeries(data, risk.eventIndices, 1);
    long long ops = 0;
    SubarrayResult burst = maxSubarrayDC(series, 0, (int)series.size() - 1, ops);
    risk.burstValue = burst.sum;
    if (burst.sum >= rules.burstThreshold) {
        risk.burstStartMinute = burst.low;
        risk.burstEndMinute = burst.high;
        score += rules.burstBonus;
        risk.reasons.push_back("Burst: minutes " + to_string(burst.low) + "-" + to_string(burst.high) + " have +" +
                               to_string(burst.sum) + " suspicious events above baseline (max-subarray) -> +" + to_string(rules.burstBonus));
    }
    if (score > 100) score = 100;
    risk.score = score;
    if (score >= rules.highThreshold) risk.priority = "HIGH";
    else if (score >= rules.mediumThreshold) risk.priority = "MEDIUM";
    else if (score > 0) risk.priority = "LOW";
    else risk.priority = "NONE";
    int bestType = -1;
    for (int t = 0; t < EVENT_TYPE_COUNT; t++)
        if (contribution[t] > 0 && (bestType == -1 || contribution[t] > contribution[bestType])) bestType = t;
    risk.category = 0;
    if (bestType >= 0) risk.category = categoryOfType(bestType);   // category of the biggest contributor
    else if (score > 0) risk.category = CAT_DEVICE;                 // only sequence rules fired
    risk.reviewMinutes = (score == 0) ? 0 : 10 + score / 4;
}

AnalysisReport analyzeDataset(const Dataset &data, const RiskRules &rules) {
    AnalysisReport report;
    Timer timer;
    timer.start();
    // Candidate index sorted by ID (for binary search lookups)
    vector<SortItem> candidateItems;
    for (int i = 0; i < (int)data.candidates.size(); i++) candidateItems.push_back({(long long)data.candidates[i].candidateId, i});
    SortStats candidateStats;
    mergeSort(candidateItems, candidateStats);
    vector<long long> candidateKeys = keysOf(candidateItems);
    // Group events by candidate, then by time (stable merge sort keeps ID order on ties)
    vector<SortItem> eventItems;
    for (int i = 0; i < (int)data.events.size(); i++)
        eventItems.push_back({(long long)data.events[i].candidateId * 10000000LL + data.events[i].timestamp, i});
    SortStats eventStats;
    mergeSort(eventItems, eventStats);
    report.sortComparisons = candidateStats.comparisons + eventStats.comparisons;
    vector<bool> seen(data.candidates.size(), false);
    int i = 0, n = (int)eventItems.size();
    while (i < n) {
        int candidateId = data.events[eventItems[i].index].candidateId;
        int j = i;
        while (j < n && data.events[eventItems[j].index].candidateId == candidateId) j++;
        long long comparisons = 0;
        int position = binarySearchIterative(candidateKeys, candidateId, comparisons);
        if (position == -1) { report.orphanEvents += j - i; i = j; continue; }
        int candidateIndex = candidateItems[position].index;
        seen[candidateIndex] = true;
        CandidateRisk risk;
        risk.candidateId = candidateId;
        risk.name = data.candidates[candidateIndex].name;
        for (int k = i; k < j; k++) {
            int idx = eventItems[k].index;
            risk.eventIndices.push_back(idx);
            risk.counts[data.events[idx].eventType]++;
            risk.behaviour += EVENT_CODES[data.events[idx].eventType];
        }
        scoreCandidate(risk, data, rules);
        report.risks.push_back(risk);
        i = j;
    }
    for (size_t c = 0; c < data.candidates.size(); c++) {
        if (seen[c]) continue;
        CandidateRisk risk;
        risk.candidateId = data.candidates[c].candidateId;
        risk.name = data.candidates[c].name;
        risk.reasons.push_back("No events logged - verify attendance manually");
        report.risks.push_back(risk);
    }
    // Review queue: highest score first (quick sort, median-of-three pivot)
    vector<SortItem> order;
    for (int r = 0; r < (int)report.risks.size(); r++)
        order.push_back({-(long long)report.risks[r].score * 10000000LL + report.risks[r].candidateId, r});
    SortStats queueStats;
    quickSort(order, PIVOT_MEDIAN_OF_THREE, queueStats);
    report.sortComparisons += queueStats.comparisons;
    vector<CandidateRisk> sorted;
    for (const SortItem &item : order) sorted.push_back(report.risks[item.index]);
    report.risks = sorted;
    for (const CandidateRisk &r : report.risks) {
        if (r.priority == "HIGH") report.highCount++;
        else if (r.priority == "MEDIUM") report.mediumCount++;
        else if (r.priority == "LOW") report.lowCount++;
    }
    report.millis = timer.millis();
    return report;
}

// Returns the position of the candidate in report.risks, or -1 if not found.
int findRiskIndex(const AnalysisReport &report, int candidateId) {
    for (int i = 0; i < (int)report.risks.size(); i++)
        if (report.risks[i].candidateId == candidateId) return i;
    return -1;
}

void printRiskTable(const AnalysisReport &report, int maxRows) {
    cout << padRight("Rank", 5) << padRight("CandID", 8) << padRight("Name", 17) << padLeft("Score", 5) << "  "
         << padRight("Priority", 9) << padRight("Category", 9) << "Review(min)\n";
    printLine();
    int shown = 0;
    for (size_t i = 0; i < report.risks.size() && shown < maxRows; i++, shown++) {
        const CandidateRisk &r = report.risks[i];
        cout << padRight(to_string(i + 1), 5) << padRight(to_string(r.candidateId), 8) << padRight(r.name, 17)
             << padLeft(to_string(r.score), 5) << "  " << padRight(r.priority, 9) << padRight(categoryName(r.category), 9)
             << r.reviewMinutes << "\n";
        for (size_t k = 0; k < r.reasons.size(); k++) cout << "        - " << r.reasons[k] << "\n";   // every reason, so the points add up to the score
    }
    if ((int)report.risks.size() > maxRows) cout << "  ... " << report.risks.size() - maxRows << " more candidate(s) not shown\n";
}

void printAnalysisSummary(const Dataset &data, const AnalysisReport &report) {
    cout << "Candidates analysed: " << report.risks.size() << " | Events: " << data.events.size()
         << " | HIGH: " << report.highCount << " | MEDIUM: " << report.mediumCount << " | LOW: " << report.lowCount
         << " | Orphan events skipped: " << report.orphanEvents << "\n";
    cout << "Analysis time: " << formatDouble(report.millis, 2) << " ms, sort comparisons: " << report.sortComparisons << "\n";
    cout << "REMINDER: scores flag cases for HUMAN review. They do not prove misconduct.\n";
}

// Evidence fingerprint: sum of value_i * BASE^(i+1) mod P, each power by fast
// exponentiation. Editing, deleting or reordering any row changes the value.
long long evidenceFingerprint(const Dataset &data, const vector<int> &eventIndices, long long &multiplications) {
    const long long MOD = 1000000007LL, BASE = 131;
    long long hash = 0;
    for (int i = 0; i < (int)eventIndices.size(); i++) {
        const ExamEvent &e = data.events[eventIndices[i]];
        long long value = ((long long)e.eventId * 31 + e.eventType * 7 + e.timestamp) % MOD;
        long long weight = fastPowerIterative(BASE, i + 1, MOD, multiplications);
        hash = (hash + value * weight) % MOD;
    }
    return hash;
}

void printEvidenceTimeline(const Dataset &data, const CandidateRisk &risk) {
    printHeader("EVIDENCE TIMELINE - Candidate " + to_string(risk.candidateId) + " (" + risk.name + ")");
    cout << "Scenario: " << data.scenarioName << "\n";
    cout << "Risk score " << risk.score << "/100 [" << risk.priority << "]   Category: " << categoryName(risk.category)
         << "   Suggested review time: " << risk.reviewMinutes << " min\n";
    cout << "Behaviour code string: " << (risk.behaviour.size() > 60 ? risk.behaviour.substr(0, 60) + "..." : risk.behaviour) << "\n";
    cout << "Flags: ! = severity >= 2,  B = inside burst window,  P = part of the known suspicious sequence\n\n";
    vector<bool> inPattern(risk.eventIndices.size(), false);
    for (int start : risk.patternPositions)
        for (int k = 0; k < risk.patternLength; k++)
            if (start + k < (int)inPattern.size()) inPattern[start + k] = true;
    cout << padRight("#", 4) << padRight("EventID", 8) << padRight("Time", 10) << padRight("Event", 15) << padRight("Device", 8)
         << padRight("Sev", 4) << padRight("Flags", 6) << "Description\n";
    printLine();
    for (size_t i = 0; i < risk.eventIndices.size(); i++) {
        const ExamEvent &e = data.events[risk.eventIndices[i]];
        int minute = e.timestamp / 60;
        string flags;
        flags += (e.severity >= 2) ? '!' : ' ';
        flags += (risk.burstStartMinute >= 0 && minute >= risk.burstStartMinute && minute <= risk.burstEndMinute) ? 'B' : ' ';
        flags += inPattern[i] ? 'P' : ' ';
        cout << padRight(to_string(i + 1), 4) << padRight(to_string(e.eventId), 8) << padRight(formatClock(e.timestamp), 10)
             << padRight(EVENT_NAMES[e.eventType], 15) << padRight(to_string(e.deviceId), 8) << padRight(to_string(e.severity), 4)
             << padRight(flags, 6) << e.description << "\n";
    }
    printLine();
    cout << "Why this case is in the queue:\n";
    if (risk.reasons.empty()) cout << "  - No rule threshold crossed (no review needed).\n";
    for (const string &reason : risk.reasons) cout << "  - " << reason << "\n";
    long long multiplications = 0;
    long long fingerprint = evidenceFingerprint(data, risk.eventIndices, multiplications);
    cout << "Evidence fingerprint (mod 1e9+7): " << fingerprint << "  [" << risk.eventIndices.size()
         << " rows, " << multiplications << " modular multiplications via fast exponentiation]\n";
    cout << "Any edit, deletion or re-ordering of these rows changes the fingerprint (traceability).\n";
    cout << "DISCLAIMER: this timeline supports a HUMAN investigation; it is not a verdict.\n";
}

string buildLogText(const Dataset &data) {
    string text;
    for (const ExamEvent &e : data.events) {
        text += "[" + formatClock(e.timestamp) + "] cand=" + to_string(e.candidateId) + " dev=" + to_string(e.deviceId) +
                " " + EVENT_NAMES[e.eventType] + " | " + e.description + "\n";
    }
    return text;
}

string buildBehaviourStream(const AnalysisReport &report) {
    string text;
    for (const CandidateRisk &r : report.risks) text += r.behaviour + "|";
    return text;
}

// ============================================================================
// SECTION 16: PERSISTENCE (CSV import/export, JSON export)
// ============================================================================

string csvSafe(const string &text) {
    string out = text;
    for (char &ch : out) {
        if (ch == ',') ch = ';';
        if (ch == '\n' || ch == '\r') ch = ' ';
        if (ch == '"') ch = '\'';
    }
    return out;
}

string jsonEscape(const string &text) {
    string out;
    for (char ch : text) {
        if (ch == '"') out += "\\\"";
        else if (ch == '\\') out += "\\\\";
        else if (ch == '\n') out += "\\n";
        else out += ch;
    }
    return out;
}

vector<string> splitCSV(const string &line) {
    vector<string> fields;
    string current;
    for (char ch : line) {
        if (ch == ',') { fields.push_back(trim(current)); current.clear(); }
        else if (ch != '\r') current += ch;
    }
    fields.push_back(trim(current));
    return fields;
}

bool exportEventsCSV(const Dataset &data, const string &fileName) {
    ofstream out(fileName);
    if (!out) return false;
    out << "eventId,candidateId,examId,eventType,timestamp,deviceId,severity,description\n";
    for (const ExamEvent &e : data.events)
        out << e.eventId << "," << e.candidateId << "," << csvSafe(e.examId) << "," << EVENT_NAMES[e.eventType] << ","
            << e.timestamp << "," << e.deviceId << "," << e.severity << "," << csvSafe(e.description) << "\n";
    return true;
}

bool exportCandidatesCSV(const Dataset &data, const string &fileName) {
    ofstream out(fileName);
    if (!out) return false;
    out << "candidateId,name,examId,primaryDeviceId\n";
    for (const Candidate &c : data.candidates)
        out << c.candidateId << "," << csvSafe(c.name) << "," << csvSafe(c.examId) << "," << c.primaryDeviceId << "\n";
    return true;
}

bool exportRiskCSV(const AnalysisReport &report, const string &fileName) {
    ofstream out(fileName);
    if (!out) return false;
    out << "rank,candidateId,name,score,priority,category,reviewMinutes,lcsSimilarityPercent,burstStartMinute,burstEndMinute,reasons\n";
    for (size_t i = 0; i < report.risks.size(); i++) {
        const CandidateRisk &r = report.risks[i];
        string reasons;
        for (size_t k = 0; k < r.reasons.size(); k++) reasons += (k ? " | " : "") + r.reasons[k];
        out << i + 1 << "," << r.candidateId << "," << csvSafe(r.name) << "," << r.score << "," << r.priority << ","
            << categoryName(r.category) << "," << r.reviewMinutes << "," << formatDouble(r.similarity, 1) << ","
            << r.burstStartMinute << "," << r.burstEndMinute << "," << csvSafe(reasons) << "\n";
    }
    return true;
}

bool exportRiskJSON(const Dataset &data, const AnalysisReport &report, const string &fileName) {
    ofstream out(fileName);
    if (!out) return false;
    out << "{\n  \"scenario\": \"" << jsonEscape(data.scenarioName) << "\",\n";
    out << "  \"disclaimer\": \"Scores identify cases for human review; they are not proof of misconduct.\",\n";
    out << "  \"summary\": {\"candidates\": " << report.risks.size() << ", \"events\": " << data.events.size()
        << ", \"high\": " << report.highCount << ", \"medium\": " << report.mediumCount << ", \"low\": " << report.lowCount << "},\n";
    out << "  \"reviewQueue\": [\n";
    for (size_t i = 0; i < report.risks.size(); i++) {
        const CandidateRisk &r = report.risks[i];
        out << "    {\"rank\": " << i + 1 << ", \"candidateId\": " << r.candidateId << ", \"name\": \"" << jsonEscape(r.name)
            << "\", \"score\": " << r.score << ", \"priority\": \"" << r.priority << "\", \"category\": \"" << categoryName(r.category)
            << "\", \"reviewMinutes\": " << r.reviewMinutes << ", \"reasons\": [";
        for (size_t k = 0; k < r.reasons.size(); k++) out << (k ? ", " : "") << "\"" << jsonEscape(r.reasons[k]) << "\"";
        out << "]}" << (i + 1 < report.risks.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
    return true;
}

bool exportTimelineCSV(const Dataset &data, const CandidateRisk &risk, const string &fileName) {
    ofstream out(fileName);
    if (!out) return false;
    long long mults = 0;
    out << "# candidate," << risk.candidateId << ",score," << risk.score << ",priority," << risk.priority
        << ",fingerprint," << evidenceFingerprint(data, risk.eventIndices, mults) << "\n";
    out << "order,eventId,time,eventType,deviceId,severity,description\n";
    for (size_t i = 0; i < risk.eventIndices.size(); i++) {
        const ExamEvent &e = data.events[risk.eventIndices[i]];
        out << i + 1 << "," << e.eventId << "," << formatClock(e.timestamp) << "," << EVENT_NAMES[e.eventType] << ","
            << e.deviceId << "," << e.severity << "," << csvSafe(e.description) << "\n";
    }
    return true;
}

bool loadDatasetFromCSV(const string &candidateFile, const string &eventFile, Dataset &data, vector<string> &errors) {
    ifstream candidatesIn(candidateFile), eventsIn(eventFile);
    if (!candidatesIn) { errors.push_back("cannot open " + candidateFile); return false; }
    if (!eventsIn) { errors.push_back("cannot open " + eventFile); return false; }
    Dataset loaded;
    loaded.scenarioName = "Loaded from " + eventFile;
    loaded.scenarioNumber = 0;
    string line;
    int lineNumber = 0;
    while (getline(candidatesIn, line)) {
        lineNumber++;
        if (lineNumber == 1 || trim(line).empty()) continue;      // header
        vector<string> f = splitCSV(line);
        long long id = 0, device = 0;
        if (f.size() < 4 || !parseLongStrict(f[0], id) || !parseLongStrict(f[3], device) || id <= 0) {
            if (errors.size() < 20) errors.push_back(candidateFile + " line " + to_string(lineNumber) + ": invalid candidate row");
            continue;
        }
        Candidate c;
        c.candidateId = (int)id; c.name = f[1]; c.examId = f[2]; c.primaryDeviceId = (int)device;
        loaded.candidates.push_back(c);
    }
    lineNumber = 0;
    int maxTimestamp = 0;
    while (getline(eventsIn, line)) {
        lineNumber++;
        if (lineNumber == 1 || trim(line).empty()) continue;
        vector<string> f = splitCSV(line);
        long long eventId = 0, candidateId = 0, timestamp = 0, device = 0, severity = 0;
        int type = (f.size() >= 4) ? eventTypeFromText(f[3]) : -1;
        bool ok = f.size() >= 8 && parseLongStrict(f[0], eventId) && parseLongStrict(f[1], candidateId) && type >= 0 &&
                  parseLongStrict(f[4], timestamp) && parseLongStrict(f[5], device) && parseLongStrict(f[6], severity) &&
                  timestamp >= 0 && timestamp <= 864000 && severity >= 0 && severity <= 5 && candidateId > 0;
        if (!ok) {
            if (errors.size() < 20) errors.push_back(eventFile + " line " + to_string(lineNumber) + ": invalid event row (skipped)");
            continue;
        }
        ExamEvent e;
        e.eventId = (int)eventId; e.candidateId = (int)candidateId; e.examId = f[2]; e.eventType = type;
        e.timestamp = (int)timestamp; e.deviceId = (int)device; e.severity = (int)severity;
        e.description = f[7];
        for (size_t k = 8; k < f.size(); k++) e.description += "; " + f[k];
        loaded.events.push_back(e);
        if (e.timestamp > maxTimestamp) maxTimestamp = e.timestamp;
    }
    loaded.examDurationSeconds = max(3600, maxTimestamp + 60);
    addStandardReviewers(loaded, 5, 240);
    loaded.reviewRooms = 3;
    data = loaded;
    return true;
}

// ============================================================================
// SECTION 17: REVIEW PLANNING (integration of greedy, DP, B&B, flow, matching)
// ============================================================================

struct AppState {
    Dataset data;
    bool loaded = false;
    RiskRules rules;
    AnalysisReport report;
    bool analyzed = false;
    ValidationSummary validation;
};

void loadScenario(AppState &app, int scenario, bool verbose) {
    Timer t;
    t.start();
    app.data = generateScenario(scenario);
    app.validation = validateDataset(app.data);
    app.loaded = true;
    app.analyzed = false;
    if (verbose) {
        cout << "Loaded " << app.data.scenarioName << ": " << app.data.candidates.size() << " candidates, "
             << app.data.events.size() << " events, " << app.data.reviewers.size() << " reviewers, "
             << app.data.reviewRooms << " review room(s)  [" << formatDouble(t.millis(), 1) << " ms]\n";
        cout << "Validation: ";
        for (size_t i = 0; i < app.validation.notes.size(); i++) cout << (i ? "; " : "") << app.validation.notes[i];
        cout << "\n";
    }
}

void ensureLoaded(AppState &app) {
    if (!app.loaded) {
        cout << "(No dataset loaded - using Scenario 2: suspicious activity.)\n";
        loadScenario(app, 2, true);
    }
}

void ensureAnalyzed(AppState &app) {
    ensureLoaded(app);
    if (!app.analyzed) {
        app.report = analyzeDataset(app.data, app.rules);
        app.analyzed = true;
    }
}

struct ReviewCase {
    int candidateId = 0;
    string name;
    int score = 0;
    string priority;
    int category = 0;
    int minutes = 0;
};

vector<ReviewCase> buildReviewCases(const AnalysisReport &report) {
    vector<ReviewCase> cases;
    for (const CandidateRisk &r : report.risks) {
        if (r.score <= 0) continue;
        ReviewCase c;
        c.candidateId = r.candidateId; c.name = r.name; c.score = r.score; c.priority = r.priority;
        c.category = r.category; c.minutes = r.reviewMinutes;
        cases.push_back(c);
    }
    return cases;
}

// Small built-in case list used when the loaded scenario has no flagged cases.
vector<ReviewCase> sampleReviewCases() {
    int scores[8] = {72, 64, 55, 48, 40, 33, 26, 18};
    int categories[8] = {CAT_DEVICE, CAT_LOGIN, CAT_BROWSER, CAT_DEVICE, CAT_BROWSER, CAT_NETWORK, CAT_NETWORK, CAT_LOGIN};
    vector<ReviewCase> cases;
    for (int i = 0; i < 8; i++) {
        ReviewCase c;
        c.candidateId = 2001 + i; c.name = "Sample case " + to_string(i + 1); c.score = scores[i];
        if (scores[i] >= 60) c.priority = "HIGH";
        else if (scores[i] >= 30) c.priority = "MEDIUM";
        else c.priority = "LOW";
        c.category = categories[i]; c.minutes = 10 + scores[i] / 4;
        cases.push_back(c);
    }
    return cases;
}

vector<ReviewCase> getCasesOrSample(AppState &app) {
    ensureAnalyzed(app);
    vector<ReviewCase> cases = buildReviewCases(app.report);
    if (cases.empty()) {
        cout << "(No flagged cases in this scenario - using a built-in sample case list.)\n";
        cases = sampleReviewCases();
    }
    return cases;
}

vector<KnapsackItem> casesToItems(const vector<ReviewCase> &cases, int limit) {
    vector<KnapsackItem> items;
    for (int i = 0; i < (int)cases.size() && i < limit; i++) {
        KnapsackItem it;
        it.id = cases[i].candidateId;
        it.label = "C" + to_string(cases[i].candidateId);
        it.weight = max(1, cases[i].minutes);
        it.value = cases[i].score;
        items.push_back(it);
    }
    return items;
}

// Each flagged case requests a hearing slot (deterministic synthetic request).
vector<Interval> buildHearingRequests(const vector<ReviewCase> &cases) {
    vector<Interval> intervals;
    for (int i = 0; i < (int)cases.size(); i++) {
        Interval iv;
        iv.id = cases[i].candidateId;
        iv.start = (cases[i].candidateId * 37) % 300;          // minutes after 09:00
        iv.finish = iv.start + max(1, cases[i].minutes);
        iv.label = "C" + to_string(cases[i].candidateId);
        intervals.push_back(iv);
    }
    return intervals;
}

FlowNetwork buildReviewFlowNetwork(const vector<ReviewCase> &cases, const vector<Reviewer> &reviewers) {
    int c = (int)cases.size(), r = (int)reviewers.size();
    FlowNetwork net = makeFlowNetwork(c + r + 2);
    int source = 0, sink = c + r + 1;
    net.names[source] = "SOURCE";
    net.names[sink] = "SINK";
    for (int i = 0; i < c; i++) {
        net.names[1 + i] = "C" + to_string(cases[i].candidateId);
        net.capacity[source][1 + i] = cases[i].minutes;
        for (int k = 0; k < r; k++)
            if (reviewers[k].expertiseMask & cases[i].category) net.capacity[1 + i][1 + c + k] = cases[i].minutes;
    }
    for (int k = 0; k < r; k++) {
        net.names[1 + c + k] = reviewers[k].name;
        net.capacity[1 + c + k][sink] = reviewers[k].availableMinutes;
    }
    return net;
}

// Budget for "which complete cases can one reviewer take?": the reviewer's free
// minutes, capped at half of the total demand so that the choice is never trivial.
int selectionBudget(const Dataset &data, const vector<KnapsackItem> &items) {
    int demand = 0;
    for (const KnapsackItem &it : items) demand += it.weight;
    int reviewerMinutes = data.reviewers.empty() ? 240 : data.reviewers[0].availableMinutes;
    return max(1, min(reviewerMinutes, demand / 2));
}

void printKnapsackComparison(const vector<KnapsackItem> &items, int capacity) {
    vector<KnapsackResult> results;
    Timer t;
    t.start(); results.push_back(knapsackDP(items, capacity)); results.back().millis = t.millis();
    t.start(); results.push_back(knapsackBacktracking(items, capacity)); results.back().millis = t.millis();
    for (int s = 0; s < 3; s++) {
        t.start(); results.push_back(knapsackBranchAndBound(items, capacity, s, 2000000)); results.back().millis = t.millis();
    }
    t.start(); results.push_back(knapsackGreedyRatio(items, capacity)); results.back().millis = t.millis();
    FractionalResult fr = fractionalKnapsack(items, capacity);
    cout << padRight("Method", 28) << padLeft("Risk value", 11) << padLeft("Minutes", 9) << padLeft("Nodes/ops", 12) << padLeft("Time ms", 10) << "\n";
    printLine();
    for (const KnapsackResult &r : results)
        cout << padRight(r.method, 28) << padLeft(to_string(r.bestValue), 11) << padLeft(to_string(r.totalWeight), 9)
             << padLeft(to_string(r.nodesGenerated > 0 ? r.nodesGenerated : r.operations), 12)
             << padLeft(formatDouble(r.millis, 3), 10) << (r.aborted ? "  (node limit hit)" : "") << "\n";
    cout << padRight("Fractional greedy (bound)", 28) << padLeft(formatDouble(fr.totalValue, 1), 11) << padLeft(to_string(capacity), 9)
         << padLeft(to_string(fr.ops), 12) << "\n";
    cout << "Exact methods agree: " << yesNo(results[0].bestValue == results[1].bestValue && results[1].bestValue == results[2].bestValue &&
                                         results[2].bestValue == results[3].bestValue && results[3].bestValue == results[4].bestValue) << "\n";
    cout << "Selected cases (DP): ";
    for (size_t i = 0; i < items.size(); i++) if (results[0].chosen[i]) cout << items[i].label << " ";
    cout << "\n";
}

void runReviewPlanning(AppState &app, bool brief) {
    ensureAnalyzed(app);
    vector<ReviewCase> cases = buildReviewCases(app.report);
    printHeader("REVIEW PLANNING - " + app.data.scenarioName);
    if (cases.empty()) {
        cout << "No candidate crossed any rule threshold. No review hearings are needed.\n";
        return;
    }
    int demand = 0;
    for (const ReviewCase &c : cases) demand += c.minutes;
    int supply = 0;
    for (const Reviewer &r : app.data.reviewers) supply += r.availableMinutes;
    cout << cases.size() << " flagged case(s), review demand " << demand << " min, reviewer supply " << supply << " min, "
         << app.data.reviewers.size() << " reviewer(s), " << app.data.reviewRooms << " hearing room(s).\n";

    // 1. Activity selection: one senior reviewer, maximum number of hearings
    vector<Interval> hearings = buildHearingRequests(cases);
    long long ops = 0;
    vector<int> chosen = activitySelection(hearings, ops);
    cout << "\n[1] Activity selection (greedy earliest-finish): one senior reviewer can attend " << chosen.size()
         << " of " << hearings.size() << " requested hearings.\n";
    if (!brief)
        for (int idx : chosen)
            cout << "      " << hearings[idx].label << "  " << clockAfterNine(hearings[idx].start) << "-" << clockAfterNine(hearings[idx].finish) << "\n";

    // 2. Interval partitioning: rooms needed for ALL hearings
    ops = 0;
    PartitionResult rooms = intervalPartitioning(hearings, ops);
    cout << "\n[2] Interval partitioning: all hearings need " << rooms.roomsNeeded << " room(s); available "
         << app.data.reviewRooms << ".";
    if (rooms.roomsNeeded > app.data.reviewRooms)
        cout << "  ** CAPACITY ALERT: " << rooms.roomsNeeded - app.data.reviewRooms << " extra room(s) or rescheduling required **";
    cout << "\n";
    if (!brief)
        for (size_t i = 0; i < hearings.size() && i < 12; i++)
            cout << "      " << hearings[i].label << " " << clockAfterNine(hearings[i].start) << "-" << clockAfterNine(hearings[i].finish)
                 << " -> Room " << rooms.roomOf[i] + 1 << "\n";

    // 3. Knapsack family: which cases fit into one reviewer's day?
    vector<KnapsackItem> items = casesToItems(cases, 20);
    int budget = selectionBudget(app.data, items);
    cout << "\n[3] Case selection for one reviewer (" << budget << " min budget = min(reviewer minutes, half of demand), top "
         << items.size() << " cases, value = risk score):\n";
    printKnapsackComparison(items, budget);

    // 4. Ford-Fulkerson: allocate review minutes respecting expertise
    FlowNetwork net = buildReviewFlowNetwork(cases, app.data.reviewers);
    FlowResult flow = fordFulkerson(net, 0, net.nodeCount - 1);
    cout << "\n[4] Ford-Fulkerson allocation: " << flow.maxFlow << " of " << demand << " demanded review minutes can be assigned ("
         << formatDouble(demand ? 100.0 * flow.maxFlow / demand : 100.0, 1) << "% coverage, " << flow.augmentingPaths << " augmenting paths).\n";
    int c = (int)cases.size();
    for (int k = 0; k < (int)app.data.reviewers.size(); k++) {
        long long used = flow.flow[1 + c + k][net.nodeCount - 1];
        cout << "      " << padRight(app.data.reviewers[k].name, 15) << " skills " << padRight(skillsText(app.data.reviewers[k].expertiseMask), 30)
             << used << "/" << app.data.reviewers[k].availableMinutes << " min\n";
    }
    if (flow.maxFlow < demand) cout << "      ** " << demand - flow.maxFlow << " min of review demand cannot be covered today **\n";

    // 5. Bipartite matching: one live interview per reviewer for HIGH/MEDIUM cases
    vector<vector<int>> adjacent;
    vector<int> leftCases;
    for (int i = 0; i < c; i++) {
        if (cases[i].priority != "HIGH" && cases[i].priority != "MEDIUM") continue;
        vector<int> options;
        for (int k = 0; k < (int)app.data.reviewers.size(); k++)
            if (app.data.reviewers[k].expertiseMask & cases[i].category) options.push_back(k);
        adjacent.push_back(options);
        leftCases.push_back(i);
    }
    vector<int> matchOfRight;
    long long matchOps = 0;
    int matched = maxBipartiteMatching(adjacent, (int)app.data.reviewers.size(), matchOfRight, matchOps);
    cout << "\n[5] Maximum bipartite matching (one live interview slot per reviewer): " << matched << " of "
         << leftCases.size() << " HIGH/MEDIUM case(s) get a dedicated reviewer now.\n";
    for (int k = 0; k < (int)matchOfRight.size(); k++)
        if (matchOfRight[k] >= 0)
            cout << "      " << padRight(app.data.reviewers[k].name, 15) << " <- C" << cases[leftCases[matchOfRight[k]]].candidateId
                 << " (" << categoryName(cases[leftCases[matchOfRight[k]]].category) << ")\n";
}

// ============================================================================
// SECTION 18: ALGORITHM WORKBENCH DEMOS (each linked to the proctoring domain)
// ============================================================================

void printMapping(const string &mapping, const string &complexity) {
    cout << "Proctoring mapping: " << mapping << "\n";
    cout << "Complexity: " << complexity << "\n";
}

void demoMergeSort(AppState &app) {
    ensureLoaded(app);
    printHeader("A1. MERGE SORT - group the event log by candidate, then by time");
    vector<SortItem> items;
    for (int i = 0; i < (int)app.data.events.size(); i++)
        items.push_back({(long long)app.data.events[i].candidateId * 10000000LL + app.data.events[i].timestamp, i});
    SortStats stats;
    Timer t;
    t.start();
    mergeSort(items, stats);
    double ms = t.millis();
    cout << "Sorted " << items.size() << " events in " << formatDouble(ms, 2) << " ms, comparisons " << stats.comparisons
         << ", moves " << stats.moves << ", recursion depth " << stats.maxDepth << ". Sorted: " << yesNo(isSortedItems(items)) << "\n";
    cout << "First 8 rows after sorting:\n";
    for (size_t i = 0; i < items.size() && i < 8; i++) {
        const ExamEvent &e = app.data.events[items[i].index];
        cout << "  cand " << e.candidateId << "  " << formatClock(e.timestamp) << "  " << EVENT_NAMES[e.eventType] << "\n";
    }
    printMapping("builds the per-candidate evidence timelines; STABLE, so equal timestamps keep log order.",
                 "Theta(n log n) best/avg/worst, Theta(n) extra space; T(n)=2T(n/2)+n.");
}

void demoQuickSort(AppState &app) {
    ensureAnalyzed(app);
    printHeader("A2. QUICK SORT - review queue ordering and the sorted-log trap");
    int n = min(20000, (int)app.data.events.size());
    vector<SortItem> base;
    for (int i = 0; i < n; i++) base.push_back({(long long)app.data.events[i].timestamp * 10000000LL + app.data.events[i].candidateId, i});
    cout << "Input: first " << n << " events, keyed by timestamp (logs arrive ALREADY in time order).\n";
    for (int rule = 0; rule < 2; rule++) {
        vector<SortItem> copy = base;
        SortStats stats;
        Timer t;
        t.start();
        quickSort(copy, rule, stats);
        cout << "  Pivot " << padRight(rule == PIVOT_LAST ? "last element" : "median-of-three", 16) << ": comparisons "
             << padLeft(to_string(stats.comparisons), 10) << ", time " << formatDouble(t.millis(), 2) << " ms, sorted "
             << yesNo(isSortedItems(copy)) << "\n";
    }
    vector<SortItem> mergeCopy = base;
    SortStats mergeStats;
    mergeSort(mergeCopy, mergeStats);
    cout << "  Merge sort for the same input   : comparisons " << padLeft(to_string(mergeStats.comparisons), 10) << "\n";
    cout << "Review queue built with quick sort (median-of-three), top 5:\n";
    for (size_t i = 0; i < app.report.risks.size() && i < 5; i++)
        cout << "  " << i + 1 << ". C" << app.report.risks[i].candidateId << " score " << app.report.risks[i].score << " " << app.report.risks[i].priority << "\n";
    printMapping("orders the human-review queue by risk; shows why a naive pivot is dangerous on time-ordered logs.",
                 "best/avg Theta(n log n), worst Theta(n^2) [T(n)=T(n-1)+n]; O(log n) stack (smaller side first).");
}

void demoBinarySearch(AppState &app) {
    ensureLoaded(app);
    printHeader("A3/A4. BINARY SEARCH - iterative and recursive event lookup");
    vector<SortItem> items;
    for (int i = 0; i < (int)app.data.events.size(); i++) items.push_back({(long long)app.data.events[i].eventId, i});
    SortStats stats;
    mergeSort(items, stats);
    vector<long long> keys = keysOf(items);
    if (keys.empty()) { cout << "No events.\n"; return; }
    long long targets[4] = {keys[0], keys[keys.size() / 2], keys.back(), keys.back() + 50};
    cout << padRight("Target ID", 12) << padRight("Iterative", 22) << padRight("Recursive", 22) << "Linear\n";
    for (long long target : targets) {
        long long c1 = 0, c2 = 0, c3 = 0;
        int p1 = binarySearchIterative(keys, target, c1);
        int p2 = binarySearchRecursive(keys, 0, (int)keys.size() - 1, target, c2);
        int p3 = linearSearch(keys, target, c3);
        cout << padRight(to_string(target), 12) << padRight((p1 >= 0 ? "found" : "absent") + string(", ") + to_string(c1) + " cmp", 22)
             << padRight((p2 >= 0 ? "found" : "absent") + string(", ") + to_string(c2) + " cmp", 22)
             << (p3 >= 0 ? "found" : "absent") << ", " << c3 << " cmp\n";
    }
    printMapping("an investigator types an Event ID from a complaint and retrieves the record instantly.",
                 "Theta(log n) worst; iterative O(1) space, recursive O(log n) stack; T(n)=T(n/2)+1.");
}

void demoMaxSubarray(AppState &app) {
    ensureLoaded(app);
    printHeader("B1. MAXIMUM SUBARRAY - most concentrated suspicious period of the whole exam");
    int minutes = app.data.examDurationSeconds / 60 + 1;
    vector<long long> counts(minutes, 0);
    long long total = 0;
    for (const ExamEvent &e : app.data.events)
        if (e.severity >= 1) { counts[min(minutes - 1, e.timestamp / 60)]++; total++; }
    long long baseline = (total + minutes - 1) / minutes;    // average per minute, rounded up
    vector<long long> series(minutes);
    for (int m = 0; m < minutes; m++) series[m] = counts[m] - baseline;
    long long ops1 = 0, ops2 = 0, ops3 = 0;
    SubarrayResult dc = maxSubarrayDC(series, 0, minutes - 1, ops1);
    SubarrayResult kadane = maxSubarrayKadane(series, ops2);
    SubarrayResult brute = maxSubarrayBrute(series, ops3);
    cout << "Series: suspicious events per minute minus hall baseline (" << baseline << "/min), " << minutes << " minutes.\n";
    cout << "  Divide & conquer : minutes " << dc.low << "-" << dc.high << ", excess " << dc.sum << ", ops " << ops1 << "\n";
    cout << "  Kadane (DP)      : minutes " << kadane.low << "-" << kadane.high << ", excess " << kadane.sum << ", ops " << ops2 << "\n";
    cout << "  Brute force      : minutes " << brute.low << "-" << brute.high << ", excess " << brute.sum << ", ops " << ops3 << "\n";
    cout << "Same best sum: " << yesNo(dc.sum == kadane.sum && kadane.sum == brute.sum) << "\n";
    printMapping("finds the window where suspicious activity spiked (e.g. a leaked question); also used per candidate as the 'Burst' rule.",
                 "D&C Theta(n log n) [T(n)=2T(n/2)+n], Kadane Theta(n), brute Theta(n^2).");
}

void demoFastPower(AppState &app) {
    ensureAnalyzed(app);
    printHeader("B2. FAST EXPONENTIATION - evidence fingerprints and Rabin-Karp powers");
    long long exponents[3] = {(long long)app.data.events.size(), 1000000LL, 1000000000000000000LL};
    const long long MOD = 1000000007LL;
    for (long long e : exponents) {
        long long m1 = 0, m2 = 0, m3 = 0;
        long long r1 = fastPowerIterative(131, e, MOD, m1);
        long long r2 = fastPowerRecursive(131, e, MOD, m2);
        cout << "  131^" << e << " mod 1e9+7 = " << r1 << "  iterative mults " << m1 << ", recursive mults " << m2;
        if (e <= 1000000) { long long r3 = naivePower(131, e, MOD, m3); cout << ", naive mults " << m3 << (r3 == r1 ? " (same)" : " (MISMATCH)"); }
        else cout << ", naive would need " << e << " mults";
        cout << (r1 == r2 ? "" : " MISMATCH") << "\n";
    }
    if (!app.report.risks.empty()) {
        long long mults = 0;
        cout << "Fingerprint of top case timeline (C" << app.report.risks[0].candidateId << "): "
             << evidenceFingerprint(app.data, app.report.risks[0].eventIndices, mults) << "\n";
    }
    printMapping("evidence-timeline fingerprints (tamper detection) and BASE^(m-1) in Rabin-Karp.",
                 "Theta(log e) multiplications; T(e)=T(e/2)+1; naive Theta(e).");
}

void demoCelebrity(AppState &app) {
    ensureLoaded(app);
    printHeader("B3. CELEBRITY IDENTIFICATION - shared relay endpoint in the contact network");
    vector<string> names;
    vector<vector<int>> knows = buildContactMatrix(app.data, names);
    int n = (int)names.size();
    cout << "knows[i][j] = 1 if endpoint i initiated contact with endpoint j (" << n << " endpoints):\n      ";
    for (int j = 0; j < n; j++) cout << padLeft(to_string(j), 3);
    cout << "\n";
    for (int i = 0; i < n; i++) {
        cout << padLeft(to_string(i), 4) << "  ";
        for (int j = 0; j < n; j++) cout << padLeft(to_string(knows[i][j]), 3);
        cout << "   " << names[i] << "\n";
    }
    long long q1 = 0, q2 = 0;
    int c1 = findCelebrity(knows, q1);
    int c2 = findCelebrityBrute(knows, q2);
    cout << "Divide & conquer: " << (c1 >= 0 ? names[c1] : "no celebrity") << " (" << q1 << " questions)\n";
    cout << "Brute force     : " << (c2 >= 0 ? names[c2] : "no celebrity") << " (" << q2 << " questions)\n";
    if (c1 >= 0) cout << "Interpretation: every device contacts '" << names[c1] << "' but it contacts no one -> possible shared answer relay. Investigate.\n";
    else cout << "Interpretation: no endpoint is contacted by all devices while staying silent -> no relay indicator.\n";
    printMapping("detects one hidden endpoint that all exam devices talk to (relay/answer server).",
                 "Theta(n) questions [T(n)=2T(n/2)+1, Master case 1] vs brute O(n^2).");
}

void demoMatrix(AppState &app) {
    ensureAnalyzed(app);
    printHeader("B4/B5. CLASSICAL vs STRASSEN - risk scores under 4 rule profiles at once");
    int rows = min(16, (int)app.report.risks.size());
    Matrix A = makeMatrix(rows, EVENT_TYPE_COUNT);
    for (int i = 0; i < rows; i++)
        for (int t = 0; t < EVENT_TYPE_COUNT; t++) A[i][t] = app.report.risks[i].counts[t];
    // Columns: Strict, Standard, Lenient, Network-tolerant weight profiles
    long long profile[EVENT_TYPE_COUNT][4] = {{0, 0, 0, 0}, {10, 8, 4, 8}, {3, 2, 1, 2}, {20, 15, 8, 15},
                                              {6, 4, 2, 0}, {8, 6, 3, 6}, {12, 10, 5, 4}, {0, 0, 0, 0}};
    Matrix B = makeMatrix(EVENT_TYPE_COUNT, 4);
    for (int t = 0; t < EVENT_TYPE_COUNT; t++)
        for (int p = 0; p < 4; p++) B[t][p] = profile[t][p];
    long long m1 = 0, m2 = 0, m3 = 0;
    Matrix C1 = multiplyClassical(A, B, m1);
    Matrix C2 = multiplyStrassen(A, B, 1, m2);
    Matrix C3 = multiplyStrassen(A, B, 4, m3);
    cout << "A = " << rows << " candidates x 8 event counts, B = 8 event types x 4 rule profiles.\n";
    cout << padRight("Candidate", 11) << padLeft("Strict", 8) << padLeft("Standard", 10) << padLeft("Lenient", 9) << padLeft("NetTol", 8) << "\n";
    for (int i = 0; i < rows && i < 8; i++) {
        cout << padRight("C" + to_string(app.report.risks[i].candidateId), 11);
        cout << padLeft(to_string(C1[i][0]), 8) << padLeft(to_string(C1[i][1]), 10)
             << padLeft(to_string(C1[i][2]), 9) << padLeft(to_string(C1[i][3]), 8);
        cout << "\n";
    }
    cout << "Scalar multiplications: classical " << m1 << ", Strassen pure (padded to 16x16) " << m2 << ", Strassen leaf=4 " << m3 << "\n";
    cout << "Results identical: " << yesNo(C1 == C2 && C1 == C3) << "  (padding makes Strassen slower on tiny matrices - see benchmarks)\n";
    printMapping("one matrix product scores every candidate under several rule profiles for sensitivity analysis.",
                 "classical Theta(n^3); Strassen Theta(n^2.807) [T(n)=7T(n/2)+n^2].");
}

void demoActivitySelection(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("C1. ACTIVITY SELECTION - maximum hearings for one senior reviewer");
    vector<Interval> hearings = buildHearingRequests(cases);
    long long ops = 0;
    vector<int> chosen = activitySelection(hearings, ops);
    for (const Interval &iv : hearings) {
        bool picked = false;
        for (int idx : chosen) if (hearings[idx].id == iv.id) picked = true;
        cout << "  " << padRight(iv.label, 8) << clockAfterNine(iv.start) << "-" << clockAfterNine(iv.finish) << (picked ? "   <- selected" : "") << "\n";
    }
    cout << "Selected " << chosen.size() << " non-overlapping hearings (" << ops << " operations).\n";
    printMapping("a single reviewer's day: attend as many case hearings as possible.",
                 "Theta(n log n) sort + Theta(n) greedy scan; exchange argument proves optimality.");
}

void demoIntervalPartitioning(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("C2. INTERVAL PARTITIONING - minimum hearing rooms for all cases");
    vector<Interval> hearings = buildHearingRequests(cases);
    long long ops = 0;
    PartitionResult result = intervalPartitioning(hearings, ops);
    for (size_t i = 0; i < hearings.size() && i < 20; i++)
        cout << "  " << padRight(hearings[i].label, 8) << clockAfterNine(hearings[i].start) << "-" << clockAfterNine(hearings[i].finish)
             << "  Room " << result.roomOf[i] + 1 << "\n";
    cout << "Rooms needed: " << result.roomsNeeded << " (available " << app.data.reviewRooms << ")"
         << (result.roomsNeeded > app.data.reviewRooms ? "  -> CAPACITY SHOCK" : "") << "\n";
    printMapping("how many parallel review rooms/panels are needed so no hearing is postponed.",
                 "Theta(n log n) with a min-heap; rooms = maximum overlap depth (lower bound reached).");
}

void demoFractionalKnapsack(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("C3. FRACTIONAL KNAPSACK - partial evidence review within a time budget");
    vector<KnapsackItem> items = casesToItems(cases, 12);
    int budget = 120;
    FractionalResult fr = fractionalKnapsack(items, budget);
    for (size_t i = 0; i < items.size(); i++)
        cout << "  " << padRight(items[i].label, 8) << "risk " << padLeft(to_string(items[i].value), 3) << ", " << padLeft(to_string(items[i].weight), 3)
             << " min, ratio " << formatDouble((double)items[i].value / items[i].weight, 2) << "  taken " << formatDouble(fr.fraction[i] * 100, 0) << "%\n";
    KnapsackResult dp = knapsackDP(items, budget);
    cout << "Fractional value " << formatDouble(fr.totalValue, 2) << " vs 0/1 optimum (DP) " << dp.bestValue
         << " -> fractional is an upper bound (used by branch & bound).\n";
    printMapping("a reviewer may skim part of a case's evidence; maximise risk covered in " + to_string(budget) + " minutes.",
                 "Theta(n log n); greedy by ratio is optimal only for the fractional version.");
}

void demoHuffman(AppState &app) {
    ensureAnalyzed(app);
    printHeader("C4. HUFFMAN CODING - compress the behaviour log for archival");
    string text = buildBehaviourStream(app.report);
    long long ops = 0;
    HuffmanCode h = buildHuffman(text, ops);
    string bits = huffmanEncode(text, h);
    string decoded = huffmanDecode(bits, h);
    cout << "Symbol  Frequency  Code\n";
    for (int s = 0; s < 256; s++) {
        if (h.codes[s].empty()) continue;
        long long f = 0;
        for (char ch : text) if ((unsigned char)ch == s) f++;
        cout << "  " << (char)s << "     " << padLeft(to_string(f), 9) << "  " << h.codes[s] << "\n";
    }
    cout << "Original: " << text.size() << " symbols = " << text.size() * 8 << " bits (8-bit), "
         << text.size() * 4 << " bits (fixed 4-bit for 9 symbols)\n";
    cout << "Huffman : " << bits.size() << " bits (" << formatDouble(text.empty() ? 0 : 100.0 * bits.size() / (text.size() * 8.0), 1)
         << "% of 8-bit size). Decoded correctly: " << yesNo(decoded == text) << "\n";
    printMapping("long-term storage of per-candidate behaviour streams with lossless compression.",
                 "O(n + k log k), k = distinct symbols; optimal prefix code (greedy choice property).");
}

void demoDijkstra(AppState &) {
    printHeader("C5. DIJKSTRA - lowest-latency evidence upload routes");
    for (int variant = 0; variant < 2; variant++) {
        Graph g = buildInfrastructureGraph(variant == 1, false);
        PathResult r = dijkstra(g, 0);
        cout << (variant == 0 ? "Normal network" : "After link failure (ProctorServer-NorthHub and NorthHub-BackupDC down)") << ":\n";
        for (int v = 1; v < g.vertexCount; v++)
            cout << "  " << padRight(g.names[v], 10) << padLeft(r.distance[v] >= INF ? "unreach" : to_string(r.distance[v]) + " ms", 8)
                 << "   " << pathFromParents(r.parent, v, g.names) << "\n";
    }
    Graph isolated = buildInfrastructureGraph(false, true);
    PathResult r2 = dijkstra(isolated, 0);
    cout << "Centre E isolated: distance = " << (r2.distance[8] >= INF ? "UNREACHABLE (reported, not crashed)" : to_string(r2.distance[8])) << "\n";
    printMapping("routes evidence uploads from exam centres; shows re-routing when links fail.",
                 "array version Theta(V^2 + E); heap version O((V+E) log V); needs non-negative weights.");
}

void printMST(const string &title, const MSTResult &r, const Graph &g) {
    cout << title << ": total cost " << r.totalWeight << ", spanning " << yesNo(r.spanning) << ", operations " << r.operations << "\n";
    for (const Edge &e : r.chosen) cout << "    " << padRight(g.names[e.from], 14) << " -- " << padRight(g.names[e.to], 14) << e.weight << "\n";
}

void demoPrim(AppState &) {
    printHeader("C6. PRIM - cheapest secure network connecting all exam centres");
    Graph g = buildInfrastructureGraph(false, false);
    printMST("Prim MST", primMST(g), g);
    Graph broken = buildInfrastructureGraph(false, true);
    MSTResult forest = primMST(broken);
    cout << "Disconnected graph (Centre E isolated): spanning " << yesNo(forest.spanning) << ", " << forest.components << " component(s).\n";
    printMapping("dedicated proctoring network: connect every centre at minimum cabling cost.", "Theta(V^2) with adjacency matrix.");
}

void demoKruskal(AppState &) {
    printHeader("C7. KRUSKAL - same network, edge-by-edge greedy with union-find");
    Graph g = buildInfrastructureGraph(false, false);
    MSTResult k = kruskalMST(g);
    printMST("Kruskal MST", k, g);
    MSTResult p = primMST(g);
    cout << "Same total as Prim: " << yesNo(k.totalWeight == p.totalWeight) << " (" << k.totalWeight << ")\n";
    printMapping("same infrastructure decision; better for sparse edge lists.", "Theta(E log E) sorting + near-linear union-find.");
}

void demoKnapsackDP(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("D1. 0/1 KNAPSACK (DP) - which whole cases fit in a reviewer's day?");
    vector<KnapsackItem> items = casesToItems(cases, 20);
    int budget = selectionBudget(app.data, items);
    KnapsackResult dp = knapsackDP(items, budget);
    KnapsackResult greedy = knapsackGreedyRatio(items, budget);
    cout << "Budget " << budget << " min, " << items.size() << " cases. DP optimum risk value " << dp.bestValue << " using "
         << dp.totalWeight << " min (table cells " << dp.operations << ").\n";
    cout << "Greedy-by-ratio heuristic gets " << greedy.bestValue << " -> greedy is " << (greedy.bestValue == dp.bestValue ? "optimal here" : "SUBOPTIMAL here") << ".\n";
    cout << "Chosen: ";
    for (size_t i = 0; i < items.size(); i++) if (dp.chosen[i]) cout << items[i].label << "(" << items[i].weight << "m) ";
    cout << "\n";
    printMapping("select complete cases (no partial reviews) maximising total risk reviewed.",
                 "Theta(nW) time/space - pseudo-polynomial; decision version is NP-complete.");
}

void demoMCM(AppState &app) {
    ensureLoaded(app);
    printHeader("D2. MATRIX CHAIN MULTIPLICATION - cheapest order for the scoring pipeline");
    long long n = (long long)app.data.candidates.size();
    vector<long long> dims = {n, 8, 6, 4, 3, 1};
    vector<string> names = {"Counts(Nx8)", "Feature(8x6)", "Rule(6x4)", "Category(4x3)", "Score(3x1)"};
    MCMResult r = matrixChainOrder(dims, names);
    cout << "Pipeline: candidate counts x event-to-feature x feature-to-rule x rule-to-category x category-to-score\n";
    cout << "N = " << n << " candidates\n";
    cout << "Optimal order : " << r.order << "\n";
    cout << "Optimal cost  : " << r.minCost << " scalar multiplications\n";
    cout << "Left-to-right : " << r.leftToRightCost << " scalar multiplications (" << formatDouble(r.minCost ? (double)r.leftToRightCost / r.minCost : 0, 2) << "x more)\n";
    printMapping("chooses how to combine scoring matrices before running them on thousands of candidates.",
                 "Theta(n^3) time, Theta(n^2) space for n matrices.");
}

void demoLCS(AppState &app) {
    ensureAnalyzed(app);
    printHeader("D3. LONGEST COMMON SUBSEQUENCE - similarity to a known violation template");
    cout << "Template '" << app.rules.violationTemplate << "' = drop, login, device change, IP change, tab, tab, copy-paste\n";
    for (size_t i = 0; i < app.report.risks.size() && i < 6; i++) {
        const CandidateRisk &r = app.report.risks[i];
        string common;
        long long ops = 0;
        int length = lcsLength(r.behaviour, app.rules.violationTemplate, common, ops);
        cout << "  C" << r.candidateId << ": LCS length " << length << "/" << app.rules.violationTemplate.size() << " = "
             << formatDouble(100.0 * length / app.rules.violationTemplate.size(), 0) << "%  LCS='" << common << "'  (" << ops << " cells)\n";
    }
    printMapping("tolerant pattern matching: the template events may be separated by normal events.",
                 "Theta(nm) time and space.");
}

void demoOBST(AppState &app) {
    ensureAnalyzed(app);
    printHeader("D4. OPTIMAL BINARY SEARCH TREE - event-type lookup dictionary");
    int alphabetical[EVENT_TYPE_COUNT] = {ANSWER_SUBMIT, COPY_PASTE, DEVICE_CHANGE, IP_CHANGE, LOGIN_FAILED, LOGIN_OK, NETWORK_DROP, TAB_SWITCH};
    long long totals[EVENT_TYPE_COUNT] = {0};
    for (const ExamEvent &e : app.data.events) totals[e.eventType]++;
    vector<long long> freq;
    vector<string> keys;
    for (int i = 0; i < EVENT_TYPE_COUNT; i++) {
        keys.push_back(EVENT_NAMES[alphabetical[i]]);
        freq.push_back(totals[alphabetical[i]] + 1);       // +1 so unseen types still have a node weight
        cout << "  " << padRight(keys.back(), 15) << " lookups " << freq.back() << "\n";
    }
    OBSTResult r = optimalBST(freq);
    long long balanced = balancedBSTCost(freq, 0, (int)freq.size() - 1, 1);
    cout << "Optimal BST (root first):\n";
    printOBSTTree(r.root, keys, 0, (int)keys.size() - 1, 0, "ROOT: ");
    cout << "Total comparisons for all lookups: optimal " << r.cost << " vs balanced " << balanced << " ("
         << formatDouble(balanced ? 100.0 * (balanced - r.cost) / balanced : 0, 1) << "% fewer)\n";
    printMapping("the log parser looks up the event type of every record; frequent types should sit near the root.",
                 "Theta(n^3) time, Theta(n^2) space.");
}

Graph buildTimelineConstraints(bool inconsistent) {
    // Difference constraints x_j - x_i <= w  ->  edge i -> j with weight w.
    // Vertices 0..3 are true times of: drop, re-login, device change, IP change; 4 = virtual source.
    Graph g;
    g.directed = true;
    g.names = {"E0:NETWORK_DROP", "E1:LOGIN_OK", "E2:DEVICE_CHANGE", "E3:IP_CHANGE", "S"};
    g.vertexCount = 5;
    g.edges.push_back({1, 0, -2});     // x0 - x1 <= -2 : login at least 2 s after drop
    g.edges.push_back({0, 1, 30});     // x1 - x0 <= 30 : login within 30 s of drop
    g.edges.push_back({1, 2, 5});      // x2 - x1 <= 5  : device change within 5 s of login
    g.edges.push_back({2, 1, -1});     // x1 - x2 <= -1 : device change after login
    g.edges.push_back({3, 2, -3});     // x2 - x3 <= -3 : IP change at least 3 s after device change
    if (inconsistent) g.edges.push_back({0, 3, -10});   // tampered server log: IP change 10 s BEFORE the drop
    else g.edges.push_back({0, 3, 60});                 // IP change within 60 s of the drop
    for (int v = 0; v < 4; v++) g.edges.push_back({4, v, 0});
    return g;
}

void demoBellmanFord(AppState &) {
    printHeader("D5. BELLMAN-FORD - is an evidence timeline physically consistent?");
    cout << "Each device clock can drift, so the logs give difference constraints (x_j - x_i <= w).\n";
    cout << "A negative cycle means the constraints contradict each other -> the timeline was tampered or a clock is broken.\n";
    for (int variant = 0; variant < 2; variant++) {
        Graph g = buildTimelineConstraints(variant == 1);
        PathResult r = bellmanFord(g, 4);
        cout << "\n" << (variant == 0 ? "Consistent logs" : "Logs with a contradictory server entry") << ": "
             << (r.negativeCycle ? "NEGATIVE CYCLE -> INCONSISTENT, flag for manual audit" : "feasible") << " (" << r.operations << " relaxations)\n";
        if (!r.negativeCycle) {
            long long minimum = 0;
            for (int v = 0; v < 4; v++) minimum = min(minimum, r.distance[v]);
            for (int v = 0; v < 4; v++) cout << "    " << padRight(g.names[v], 18) << " reconstructed offset +" << r.distance[v] - minimum << " s\n";
        }
    }
    Graph infra = buildInfrastructureGraph(false, false);
    PathResult bf = bellmanFord(infra, 0), dj = dijkstra(infra, 0);
    cout << "\nOn the (non-negative) infrastructure network Bellman-Ford equals Dijkstra: " << yesNo(bf.distance == dj.distance)
         << "  (ops BF " << bf.operations << " vs Dijkstra " << dj.operations << ")\n";
    printMapping("timeline-consistency audit using negative edge weights (Dijkstra cannot do this).",
                 "O(VE) worst, Theta(E) best with early exit; detects negative cycles.");
}

void demoFloyd(AppState &) {
    printHeader("D6. FLOYD-WARSHALL - all-pairs latency table of the infrastructure");
    Graph g = buildInfrastructureGraph(false, false);
    vector<vector<int>> next;
    long long ops = 0;
    bool negative = false;
    vector<vector<long long>> dist = floydWarshall(g, next, ops, negative);
    cout << padRight("", 14);
    for (int j = 0; j < g.vertexCount; j++) cout << padLeft(g.names[j].substr(0, 6), 7);
    cout << "\n";
    for (int i = 0; i < g.vertexCount; i++) {
        cout << padRight(g.names[i], 14);
        for (int j = 0; j < g.vertexCount; j++) cout << padLeft(dist[i][j] >= INF ? "INF" : to_string(dist[i][j]), 7);
        cout << "\n";
    }
    bool same = true;
    for (int s = 0; s < g.vertexCount; s++) if (dijkstra(g, s).distance != dist[s]) same = false;
    cout << "Matches V runs of Dijkstra: " << yesNo(same) << ", operations " << ops << ", negative cycle " << yesNo(negative) << "\n";
    printMapping("latency between every pair of sites; feeds the TSP audit-route planner.", "Theta(V^3) time, Theta(V^2) space.");
}

void demoTSP(AppState &) {
    printHeader("D7. TRAVELLING SALESPERSON - on-site audit route through flagged centres");
    Graph g = buildInfrastructureGraph(false, false);
    vector<vector<int>> next;
    long long ops = 0;
    bool negative = false;
    vector<vector<long long>> all = floydWarshall(g, next, ops, negative);
    vector<int> sites = {0, 4, 5, 6, 7, 8};
    vector<vector<long long>> dist(sites.size(), vector<long long>(sites.size()));
    for (size_t i = 0; i < sites.size(); i++)
        for (size_t j = 0; j < sites.size(); j++) dist[i][j] = all[sites[i]][sites[j]];
    TSPResult hk = tspHeldKarp(dist);
    TSPResult brute = tspBruteForce(dist);
    cout << "Officer starts at ProctorServer and must visit Centres A-E (distances from Floyd-Warshall).\n";
    cout << "Held-Karp DP : cost " << hk.cost << ", operations " << hk.operations << ", tour: ";
    for (size_t i = 0; i < hk.tour.size(); i++) cout << g.names[sites[hk.tour[i]]] << (i + 1 < hk.tour.size() ? " -> " : "");
    cout << "\nBrute force  : cost " << brute.cost << ", operations " << brute.operations << "  Same optimum: " << yesNo(hk.cost == brute.cost) << "\n";
    printMapping("small exact instance (<= 16 sites); decision version is NP-complete, so no polynomial algorithm is known.",
                 "Held-Karp Theta(n^2 2^n) time, Theta(n 2^n) space; brute force Theta(n!).");
}

void demoNQueens(AppState &) {
    printHeader("E1. N-QUEENS - line-of-sight-free seating for flagged candidates");
    NQueensResult r = solveNQueens(8);
    cout << "8x8 exam hall: place 8 flagged candidates so no two share a row, column or diagonal (no screen visibility).\n";
    for (int row = 0; row < 8; row++) {
        cout << "    ";
        for (int col = 0; col < 8; col++) cout << (r.firstSolution[row] == col ? "Q " : ". ");
        cout << "\n";
    }
    cout << "Total valid seatings: " << r.solutions << " (nodes explored " << r.nodes << ")\n";
    cout << "n:         ";
    for (int n = 1; n <= 10; n++) cout << padLeft(to_string(n), 6);
    cout << "\nsolutions: ";
    for (int n = 1; n <= 10; n++) cout << padLeft(to_string(solveNQueens(n).solutions), 6);
    cout << "\n";
    printMapping("constraint-solving validation exercise mapped to anti-collusion seating.",
                 "O(n!) search tree in the worst case, Theta(n) space; pruning removes most branches.");
}

void demoSubsets(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("E2. SUM OF SUBSETS - fill an exact 60-minute review slot");
    vector<int> minutes;
    for (int i = 0; i < (int)cases.size() && i < 14; i++) minutes.push_back(max(1, cases[i].minutes));
    cout << "Case minutes: ";
    for (size_t i = 0; i < minutes.size(); i++) cout << "C" << cases[i].candidateId << "=" << minutes[i] << " ";
    cout << "\n";
    SubsetResult r = sumOfSubsets(minutes, 60, 8);
    cout << "Combinations that fill exactly 60 minutes: " << r.totalSolutions << " (nodes " << r.nodes << ")\n";
    for (const vector<int> &solution : r.solutions) {
        cout << "    { ";
        for (int idx : solution) cout << "C" << cases[idx].candidateId << "(" << minutes[idx] << ") ";
        cout << "}\n";
    }
    printMapping("pack a fixed free slot in a reviewer's calendar with no idle time.",
                 "O(2^n) worst case, Theta(n) recursion depth; bounding prunes overshooting branches.");
}

void demoKnapsackBT(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("E3. 0/1 KNAPSACK BY BACKTRACKING - same decision, explicit search tree");
    vector<KnapsackItem> items = casesToItems(cases, 20);
    int budget = selectionBudget(app.data, items);
    KnapsackResult bt = knapsackBacktracking(items, budget);
    KnapsackResult dp = knapsackDP(items, budget);
    cout << "Backtracking value " << bt.bestValue << " (nodes " << bt.nodesGenerated << ", full tree would have up to "
         << (items.size() < 31 ? to_string((1LL << (items.size() + 1)) - 1) : string("2^(n+1)")) << ")\n";
    cout << "DP value " << dp.bestValue << " -> same: " << yesNo(bt.bestValue == dp.bestValue) << "\n";
    printMapping("exact case selection without a W-sized table (good when minutes are large numbers).",
                 "O(2^n) worst case; fractional bound prunes; Theta(n) space.");
}

void demoBranchBound(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("F. BRANCH AND BOUND - FIFO vs LIFO vs Least-Cost on case selection");
    vector<KnapsackItem> items = casesToItems(cases, 20);
    int budget = selectionBudget(app.data, items);
    printKnapsackComparison(items, budget);
    cout << "FIFO explores level by level (largest frontier), LIFO dives deep first (small memory),\n"
         << "Least-Cost always expands the node with the best fractional bound (usually fewest nodes).\n";
    printMapping("exact small-instance decision: which complete cases a reviewer takes today.",
                 "O(2^n) nodes worst case for all three; bound quality decides practical speed.");
}

void demoFordFulkerson(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("G1. FORD-FULKERSON - capacity-limited reviewer allocation");
    FlowNetwork net = buildReviewFlowNetwork(cases, app.data.reviewers);
    FlowResult flow = fordFulkerson(net, 0, net.nodeCount - 1);
    int demand = 0;
    for (const ReviewCase &c : cases) demand += c.minutes;
    cout << "Network: SOURCE -> case (minutes needed) -> qualified reviewer -> SINK (reviewer minutes).\n";
    cout << "Max flow = " << flow.maxFlow << " of " << demand << " minutes (" << flow.augmentingPaths << " augmenting paths)\n";
    int c = (int)cases.size();
    for (int i = 0; i < c && i < 12; i++) {
        cout << "  " << padRight(net.names[1 + i], 8) << " needs " << padLeft(to_string(cases[i].minutes), 3) << " ->";
        for (int k = 0; k < (int)app.data.reviewers.size(); k++)
            if (flow.flow[1 + i][1 + c + k] > 0) cout << " " << app.data.reviewers[k].name << ":" << flow.flow[1 + i][1 + c + k];
        cout << "\n";
    }
    printMapping("reviewers have limited minutes and expertise; flow shows the maximum review work that can be done.",
                 "O(E * f) with integer capacities (DFS augmenting paths).");
}

void demoMatching(AppState &app) {
    vector<ReviewCase> cases = getCasesOrSample(app);
    printHeader("G2. MAXIMUM BIPARTITE MATCHING - one interview per reviewer");
    vector<vector<int>> adjacent;
    int rightCount = (int)app.data.reviewers.size();
    for (const ReviewCase &c : cases) {
        vector<int> options;
        for (int k = 0; k < rightCount; k++) if (app.data.reviewers[k].expertiseMask & c.category) options.push_back(k);
        adjacent.push_back(options);
    }
    vector<int> matchOfRight;
    long long ops1 = 0, ops2 = 0;
    int kuhn = maxBipartiteMatching(adjacent, rightCount, matchOfRight, ops1);
    int viaFlow = matchingViaFlow(adjacent, rightCount, ops2);
    for (int k = 0; k < rightCount; k++)
        cout << "  " << padRight(app.data.reviewers[k].name, 15) << (matchOfRight[k] >= 0 ? "<- C" + to_string(cases[matchOfRight[k]].candidateId) + " (" + categoryName(cases[matchOfRight[k]].category) + ")" : "(free)") << "\n";
    cout << "Augmenting-path matching: " << kuhn << " pairs (" << ops1 << " ops); same via Ford-Fulkerson: " << viaFlow
         << " (" << ops2 << " ops). Equal: " << yesNo(kuhn == viaFlow) << "\n";
    printMapping("two-sided assignment: flagged cases <-> qualified reviewers, at most one each.",
                 "O(V*E) augmenting paths; equals max-flow on a unit-capacity network.");
}

void demoCRT(AppState &app) {
    ensureAnalyzed(app);
    printHeader("H. CHINESE REMAINDER THEOREM - reconstruct a partitioned counter");
    long long total = 0;
    for (const ExamEvent &e : app.data.events) if (e.severity >= 1) total++;
    vector<long long> moduli = {101, 103, 107, 109};
    vector<long long> residues;
    cout << "Four log shards each keep only (suspicious-event count mod p), a tiny 7-bit counter:\n";
    for (long long m : moduli) { residues.push_back(total % m); cout << "  shard mod " << m << " -> " << total % m << "\n"; }
    long long answer = 0, product = 0;
    string error;
    bool ok = chineseRemainder(residues, moduli, answer, product, error);
    cout << "CRT reconstruction: " << (ok ? to_string(answer) : "ERROR " + error) << " (valid up to " << product - 1 << "), direct count "
         << total << " -> " << (answer == total ? "VERIFIED" : "MISMATCH") << "\n";
    residues[2] = (residues[2] + 1) % moduli[2];
    long long tampered = 0;
    chineseRemainder(residues, moduli, tampered, product, error);
    cout << "If shard 3 is tampered (+1): reconstruction " << tampered << " != " << total << " -> tampering detected.\n";
    vector<long long> bad = {6, 9};
    vector<long long> badResidues = {1, 2};
    long long dummy = 0;
    cout << "Non-coprime moduli {6, 9}: " << (chineseRemainder(badResidues, bad, dummy, product, error) ? "accepted" : "rejected - " + error) << "\n";
    printMapping("distributed counters and modular verification of partitioned event results.",
                 "O(k log M) using the extended Euclidean algorithm.");
}

void demoStringMatching(AppState &app) {
    ensureAnalyzed(app);
    printHeader("I. STRING MATCHING - Naive vs Rabin-Karp vs Finite Automaton vs KMP");
    string logText = buildLogText(app.data);
    string stream = buildBehaviourStream(app.report);
    string patterns[4] = {"Clipboard paste detected", "LOGIN_FAILED", "FFF", "NLD"};
    for (int qi = 0; qi < 4; qi++) {
        string pattern = patterns[qi];
        string text = (qi < 2) ? logText : stream;            // first two search the log, last two the code stream
        string label = (qi < 2) ? "log text" : "behaviour codes";
        cout << "\nPattern '" << pattern << "' in " << label << " (" << text.size() << " chars):\n";
        vector<int> reference;
        bool agree = true;
        for (int a = 0; a < 4; a++) {
            MatchResult r = runMatcher(a, text, pattern);
            if (a == 0) reference = r.positions;
            else if (r.positions != reference) agree = false;
            cout << "  " << padRight(r.algorithm, 17) << "matches " << padLeft(to_string(r.positions.size()), 5) << "  comparisons "
                 << padLeft(to_string(r.comparisons), 9) << "  preprocessing " << padLeft(to_string(r.preprocessing), 6)
                 << "  " << formatDouble(r.millis, 3) << " ms";
            if (a == 1) cout << "  spurious " << r.spuriousHits;
            cout << "\n";
        }
        cout << "  All four agree: " << yesNo(agree) << "\n";
    }
    printMapping("searching raw exam logs and behaviour code streams (e.g. 'FFF' = three failed logins in a row).",
                 "Naive O(nm); Rabin-Karp O(n+m) avg, O(nm) worst; FA Theta(n) + Theta(m*256); KMP Theta(n+m).");
}

void runWorkbenchChoice(AppState &app, int choice) {
    switch (choice) {
        case 1: demoMergeSort(app); break;
        case 2: demoQuickSort(app); break;
        case 3: demoBinarySearch(app); break;
        case 4: demoMaxSubarray(app); break;
        case 5: demoFastPower(app); break;
        case 6: demoCelebrity(app); break;
        case 7: demoMatrix(app); break;
        case 8: demoActivitySelection(app); break;
        case 9: demoIntervalPartitioning(app); break;
        case 10: demoFractionalKnapsack(app); break;
        case 11: demoHuffman(app); break;
        case 12: demoDijkstra(app); break;
        case 13: demoPrim(app); break;
        case 14: demoKruskal(app); break;
        case 15: demoKnapsackDP(app); break;
        case 16: demoMCM(app); break;
        case 17: demoLCS(app); break;
        case 18: demoOBST(app); break;
        case 19: demoBellmanFord(app); break;
        case 20: demoFloyd(app); break;
        case 21: demoTSP(app); break;
        case 22: demoNQueens(app); break;
        case 23: demoSubsets(app); break;
        case 24: demoKnapsackBT(app); break;
        case 25: demoBranchBound(app); break;
        case 26: demoFordFulkerson(app); break;
        case 27: demoMatching(app); break;
        case 28: demoCRT(app); break;
        case 29: demoStringMatching(app); break;
        default: break;
    }
}

// ============================================================================
// SECTION 19: BENCHMARK RUNNER (identical inputs, increasing sizes)
// ============================================================================

struct BenchRow {
    string group;
    string algorithm;
    string input;
    long long n = 0;
    double millis = 0;
    long long operations = 0;
    long long memoryBytes = 0;   // estimated auxiliary memory (0 = not estimated)
    string result;
};

void addBench(vector<BenchRow> &rows, const string &group, const string &algorithm, const string &input, long long n,
              double millis, long long ops, long long memory, const string &result) {
    BenchRow row;
    row.group = group; row.algorithm = algorithm; row.input = input; row.n = n; row.millis = millis;
    row.operations = ops; row.memoryBytes = memory; row.result = result;
    rows.push_back(row);
    cout << padRight(group, 10) << padRight(algorithm, 27) << padRight(input, 17) << padLeft(to_string(n), 9)
         << padLeft(formatDouble(millis, 3), 11) << padLeft(to_string(ops), 13)
         << padLeft(memory > 0 ? formatDouble(memory / 1024.0, 1) : "-", 10) << "  " << result << "\n";
}

void benchSorting(vector<BenchRow> &rows, bool quick) {
    vector<int> randomSizes = {1000, 10000, 100000, 300000};
    if (quick) randomSizes = {1000, 10000, 50000};
    vector<int> sortedSizes = {1000, 5000, 20000};
    if (quick) sortedSizes = {1000, 5000};
    for (int pass = 0; pass < 3; pass++) {
        vector<int> sizes = randomSizes;
        string label = "random";
        if (pass == 1) { sizes = sortedSizes; label = "sorted(adv)"; }
        if (pass == 2) { sizes = {1000, 5000}; label = "all-equal(adv)"; }
        for (int n : sizes) {
            vector<SortItem> base;
            SimpleRandom rng(n + pass);
            for (int i = 0; i < n; i++) {
                long long key = rng.next();          // pass 0: random keys
                if (pass == 1) key = i;              // pass 1: already sorted
                if (pass == 2) key = 7;              // pass 2: every key equal
                base.push_back({key, i});
            }
            for (int algo = 0; algo < 3; algo++) {
                if (pass == 2 && algo == 2) continue;
                vector<SortItem> copy = base;              // identical input for every algorithm
                SortStats stats;
                Timer t;
                t.start();
                if (algo == 0) mergeSort(copy, stats);
                else quickSort(copy, algo == 1 ? PIVOT_MEDIAN_OF_THREE : PIVOT_LAST, stats);
                double ms = t.millis();
                long long memory = (long long)stats.maxDepth * 64;          // quick sort: stack frames
                string name = "Quick sort (last)";
                if (algo == 1) name = "Quick sort (median3)";
                if (algo == 0) { name = "Merge sort"; memory = (long long)n * (long long)sizeof(SortItem); }   // merge buffer
                string result = "NOT SORTED";
                if (isSortedItems(copy)) result = "sorted OK";
                addBench(rows, "Sorting", name, label, n, ms, stats.comparisons, memory, result);
            }
        }
    }
}

void benchSearching(vector<BenchRow> &rows, bool quick) {
    int n = quick ? 100000 : 1000000;
    vector<long long> keys(n);
    for (int i = 0; i < n; i++) keys[i] = 2LL * i;
    SimpleRandom rng(5);
    int queries = 10000;
    vector<long long> targets;
    for (int q = 0; q < queries; q++) targets.push_back(rng.range(0, 2 * n));
    for (int algo = 0; algo < 3; algo++) {
        int count = (algo == 2) ? 100 : queries;
        long long comparisons = 0, found = 0;
        Timer t;
        t.start();
        for (int q = 0; q < count; q++) {
            int position;
            if (algo == 0) position = binarySearchIterative(keys, targets[q], comparisons);
            else if (algo == 1) position = binarySearchRecursive(keys, 0, n - 1, targets[q], comparisons);
            else position = linearSearch(keys, targets[q], comparisons);
            if (position >= 0) found++;
        }
        double ms = t.millis();
        string name = "Linear (baseline)";
        if (algo == 0) name = "Binary iterative";
        if (algo == 1) name = "Binary recursive";
        addBench(rows, "Search", name,
                 to_string(count) + " queries", n, ms, comparisons / count, algo == 1 ? (long long)(log2(n) + 1) * 48 : 0,
                 "avg cmp/query, found " + to_string(found));
    }
}

void benchDivideConquer(vector<BenchRow> &rows, bool quick) {
    vector<int> sizes = {1000, 10000, 100000};
    if (quick) sizes = {1000, 10000};
    for (int n : sizes) {
        SimpleRandom rng(n);
        vector<long long> a(n);
        for (int i = 0; i < n; i++) a[i] = rng.range(-50, 50);
        for (int algo = 0; algo < 3; algo++) {
            if (algo == 2 && n > 10000) continue;
            long long ops = 0;
            Timer t;
            t.start();
            SubarrayResult r;
            string name;
            if (algo == 0) { r = maxSubarrayDC(a, 0, n - 1, ops); name = "D&C"; }
            else if (algo == 1) { r = maxSubarrayKadane(a, ops); name = "Kadane (DP)"; }
            else { r = maxSubarrayBrute(a, ops); name = "Brute force"; }
            addBench(rows, "MaxSub", name, "random", n, t.millis(), ops,
                     algo == 0 ? (long long)(log2(n) + 1) * 64 : 0, "sum " + to_string(r.sum));
        }
    }
    long long exponents[4] = {1000, 1000000, 1000000000LL, 1000000000000000000LL};
    for (long long e : exponents) {
        for (int algo = 0; algo < 3; algo++) {
            if (algo == 2 && e > 1000000) continue;
            long long mults = 0;
            Timer t;
            t.start();
            long long r;
            string name;
            if (algo == 0) { r = fastPowerIterative(7, e, 1000000007LL, mults); name = "Fast iterative"; }
            else if (algo == 1) { r = fastPowerRecursive(7, e, 1000000007LL, mults); name = "Fast recursive"; }
            else { r = naivePower(7, e, 1000000007LL, mults); name = "Naive repeated"; }
            addBench(rows, "Power", name, "7^e mod p", e, t.millis(), mults, 0, "= " + to_string(r));
        }
    }
    vector<int> matrixSizes = {32, 64, 128, 256};
    if (quick) matrixSizes = {16, 32, 64};
    for (int n : matrixSizes) {
        SimpleRandom rng(n * 3);
        Matrix A = makeMatrix(n, n), B = makeMatrix(n, n);
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) { A[i][j] = rng.range(0, 9); B[i][j] = rng.range(0, 9); }
        long long m1 = 0;
        Timer t;
        t.start();
        Matrix C1 = multiplyClassical(A, B, m1);
        addBench(rows, "Matrix", "Classical", "random", n, t.millis(), m1, 3LL * n * n * 8, "baseline");
        if (n <= 128) {
            long long m2 = 0;
            t.start();
            Matrix C2 = multiplyStrassen(A, B, 1, m2);
            addBench(rows, "Matrix", "Strassen (pure)", "random", n, t.millis(), m2, 9LL * n * n * 8, C2 == C1 ? "equal" : "DIFFERENT");
        }
        long long m3 = 0;
        t.start();
        Matrix C3 = multiplyStrassen(A, B, 32, m3);
        addBench(rows, "Matrix", "Strassen (leaf 32)", "random", n, t.millis(), m3, 9LL * n * n * 8, C3 == C1 ? "equal" : "DIFFERENT");
    }
}

void benchStrings(vector<BenchRow> &rows, bool quick) {
    Dataset data = generateScenario(quick ? 2 : 4);
    string logText = buildLogText(data);
    if (quick) { string once = logText; for (int k = 0; k < 20; k++) logText += once; }
    string adversarial(quick ? 50000 : 300000, 'A');
    string adversarialPattern = string(29, 'A') + "B";
    for (int test = 0; test < 2; test++) {
        const string &text = (test == 0) ? logText : adversarial;
        string pattern = (test == 0) ? "Clipboard paste detected" : adversarialPattern;
        for (int a = 0; a < 4; a++) {
            MatchResult r = runMatcher(a, text, pattern);
            long long memory = 0;
            if (a == 2) memory = (long long)(pattern.size() + 1) * 256 * 4;   // automaton table
            if (a == 3) memory = (long long)pattern.size() * 4;               // KMP prefix array
            addBench(rows, "Strings", r.algorithm, test == 0 ? "exam log" : "AAA..AB (adv)", (long long)text.size(), r.millis,
                     r.comparisons + r.preprocessing, memory, to_string(r.positions.size()) + " matches");
        }
    }
}

void benchKnapsack(vector<BenchRow> &rows, bool quick) {
    vector<int> sizes = {10, 15, 20, 25};
    if (quick) sizes = {10, 15, 20};
    for (int round = 0; round < 2; round++) {
        vector<int> list = sizes;                 // round 0: random instances of growing size
        if (round == 1) {                         // round 1: one adversarial instance
            if (quick) list = {16};
            else list = {20};
        }
        for (int n : list) {
            SimpleRandom rng(n * 11 + round);
            vector<KnapsackItem> items;
            int total = 0;
            for (int i = 0; i < n; i++) {
                KnapsackItem it;
                it.id = i; it.label = "I" + to_string(i);
                if (round == 0) {
                    it.weight = rng.range(5, 60);
                    it.value = rng.range(1, 100);
                } else {
                    it.weight = rng.range(10, 99);
                    it.value = it.weight;          // every ratio equal = adversarial for the bounds
                }
                total += it.weight;
                items.push_back(it);
            }
            int capacity = total / 2;
            string label = (round == 0) ? "random" : "equal-ratio(adv)";
            int reference = -1;
            for (int method = 0; method < 6; method++) {
                Timer t;
                t.start();
                KnapsackResult r;
                if (method == 0) r = knapsackDP(items, capacity);
                else if (method == 1) r = knapsackBacktracking(items, capacity);
                else if (method == 5) r = knapsackGreedyRatio(items, capacity);
                else r = knapsackBranchAndBound(items, capacity, method - 2, 3000000);   // methods 2,3,4 = FIFO, LIFO, LC
                double ms = t.millis();
                if (method == 0) reference = r.bestValue;
                long long memory = (long long)n * 16;                                      // recursion / greedy
                if (method == 0) memory = (long long)(n + 1) * (capacity + 1) * 4;          // DP table
                if (method >= 2 && method <= 4) memory = r.nodesGenerated * (long long)sizeof(BBNode);   // node pool
                string quality = "value " + to_string(r.bestValue);
                if (r.bestValue == reference) quality += " (optimal)";
                else quality += " (" + formatDouble(100.0 * r.bestValue / max(1, reference), 1) + "% of opt)";
                if (r.aborted) quality += " ABORTED";
                addBench(rows, "Knapsack", r.method, label, n, ms, r.nodesGenerated > 0 ? r.nodesGenerated : r.operations, memory, quality);
            }
        }
    }
}

void benchGraphs(vector<BenchRow> &rows, bool quick) {
    vector<int> sizes = {100, 300, 600};
    if (quick) sizes = {100, 200};
    for (int v : sizes) {
        Graph g = randomConnectedGraph(v, 4, v * 13, false);
        long long e = (long long)g.edges.size();
        Timer t;
        t.start(); PathResult d = dijkstra(g, 0); double dms = t.millis();
        t.start(); PathResult b = bellmanFord(g, 0); double bms = t.millis();
        vector<vector<int>> next;
        long long fops = 0;
        bool negative = false;
        t.start(); vector<vector<long long>> all = floydWarshall(g, next, fops, negative); double fms = t.millis();
        addBench(rows, "Paths", "Dijkstra (array)", "E=" + to_string(e), v, dms, d.operations, (long long)v * 24, "single source");
        addBench(rows, "Paths", "Bellman-Ford", "E=" + to_string(e), v, bms, b.operations, (long long)v * 24, d.distance == b.distance ? "same as Dijkstra" : "DIFFERENT");
        addBench(rows, "Paths", "Floyd-Warshall", "E=" + to_string(e), v, fms, fops, (long long)v * v * 12, all[0] == d.distance ? "all pairs, row 0 same" : "DIFFERENT");
        t.start(); MSTResult p = primMST(g); double pms = t.millis();
        t.start(); MSTResult k = kruskalMST(g); double kms = t.millis();
        addBench(rows, "MST", "Prim (matrix)", "E=" + to_string(e), v, pms, p.operations, (long long)v * v * 8, "cost " + to_string(p.totalWeight));
        addBench(rows, "MST", "Kruskal", "E=" + to_string(e), v, kms, k.operations, e * 32, p.totalWeight == k.totalWeight ? "cost equal" : "DIFFERENT");
    }
    vector<int> tspSizes = {6, 8, 10, 12, 14, 16};
    if (quick) tspSizes = {6, 8, 10, 12};
    for (int n : tspSizes) {
        SimpleRandom rng(n * 17);
        vector<vector<long long>> dist(n, vector<long long>(n, 0));
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++) dist[i][j] = dist[j][i] = rng.range(10, 99);
        Timer t;
        t.start(); TSPResult hk = tspHeldKarp(dist); double hms = t.millis();
        addBench(rows, "TSP", "Held-Karp DP", "random", n, hms, hk.operations, (1LL << n) * n * 12, "cost " + to_string(hk.cost));
        if (n <= 10) {
            t.start(); TSPResult bf = tspBruteForce(dist); double bms2 = t.millis();
            addBench(rows, "TSP", "Brute force", "random", n, bms2, bf.operations, (long long)n * 16, bf.cost == hk.cost ? "same optimum" : "DIFFERENT");
        }
    }
}

void benchFlowAndBacktracking(vector<BenchRow> &rows, bool quick) {
    vector<int> sizes = {50, 100, 200};
    if (quick) sizes = {50, 100};
    for (int n : sizes) {
        SimpleRandom rng(n * 19);
        vector<vector<int>> adjacent(n);
        for (int l = 0; l < n; l++)
            for (int k = 0; k < 3; k++) adjacent[l].push_back(rng.range(0, n - 1));
        vector<int> match;
        long long ops1 = 0, ops2 = 0;
        Timer t;
        t.start(); int kuhn = maxBipartiteMatching(adjacent, n, match, ops1); double kms = t.millis();
        t.start(); int flow = matchingViaFlow(adjacent, n, ops2); double fms = t.millis();
        addBench(rows, "Matching", "Augmenting paths", "3 edges/left", n, kms, ops1, (long long)n * 8, to_string(kuhn) + " pairs");
        addBench(rows, "Matching", "Ford-Fulkerson unit", "3 edges/left", n, fms, ops2, (long long)(2 * n + 2) * (2 * n + 2) * 8, to_string(flow) + (flow == kuhn ? " pairs (equal)" : " DIFFERENT"));
    }
    int maxQueens = quick ? 10 : 12;
    for (int n = 4; n <= maxQueens; n += 2) {
        Timer t;
        t.start();
        NQueensResult r = solveNQueens(n);
        addBench(rows, "N-Queens", "Backtracking", "board", n, t.millis(), r.nodes, (long long)n * 12, to_string(r.solutions) + " solutions");
    }
}

void benchPipeline(vector<BenchRow> &rows, bool quick) {
    RiskRules rules;
    vector<int> scenarios = {1, 2, 3, 4};
    if (quick) scenarios = {1, 2, 3};
    for (int s : scenarios) {
        Dataset data = generateScenario(s);
        validateDataset(data);
        AnalysisReport report = analyzeDataset(data, rules);
        addBench(rows, "Pipeline", "Full risk analysis", "scenario " + to_string(s), (long long)data.events.size(), report.millis,
                 report.sortComparisons, 0, to_string(report.highCount) + "H/" + to_string(report.mediumCount) + "M/" + to_string(report.lowCount) + "L");
    }
}

bool exportBenchCSV(const vector<BenchRow> &rows, const string &fileName) {
    ofstream out(fileName);
    if (!out) return false;
    out << "group,algorithm,input,n,time_ms,operations,est_memory_bytes,result\n";
    for (const BenchRow &r : rows)
        out << r.group << "," << csvSafe(r.algorithm) << "," << csvSafe(r.input) << "," << r.n << "," << formatDouble(r.millis, 4) << ","
            << r.operations << "," << r.memoryBytes << "," << csvSafe(r.result) << "\n";
    return true;
}

void runBenchmarkSuite(bool quick) {
    printHeader(string("BENCHMARK RUNNER (") + (quick ? "quick" : "full") + " mode) - identical inputs per comparison");
    cout << "C++ standard value: " << __cplusplus << " (201703 means C++17)\n";
    cout << "Times are wall-clock on THIS machine; memory is an analytical estimate of auxiliary storage.\n\n";
    cout << padRight("Group", 10) << padRight("Algorithm", 27) << padRight("Input", 17) << padLeft("n", 9) << padLeft("Time(ms)", 11)
         << padLeft("Ops", 13) << padLeft("Mem(KB)", 10) << "  Result\n";
    printLine('-', 116);
    vector<BenchRow> rows;
    Timer total;
    total.start();
    benchSorting(rows, quick);
    benchSearching(rows, quick);
    benchDivideConquer(rows, quick);
    benchStrings(rows, quick);
    benchKnapsack(rows, quick);
    benchGraphs(rows, quick);
    benchFlowAndBacktracking(rows, quick);
    benchPipeline(rows, quick);
    printLine('-', 116);
    cout << rows.size() << " measurements in " << formatDouble(total.millis() / 1000.0, 2) << " s.\n";
    if (exportBenchCSV(rows, "benchmark_results.csv")) cout << "Saved: benchmark_results.csv (open in Excel to draw graphs)\n";
    cout << "Observations to look for:\n"
         << "  * Quick sort (last pivot) explodes on sorted logs; median-of-three fixes sorted input but not all-equal keys.\n"
         << "  * Strassen does fewer multiplications but pure Strassen is slower at these sizes (allocation overhead).\n"
         << "  * KMP / automaton stay linear on the adversarial text while naive does ~n*m comparisons.\n"
         << "  * Least-cost branch & bound usually generates the fewest nodes; equal-ratio items weaken every bound.\n"
         << "  * Held-Karp beats brute force from n = 8 onwards; both are exponential.\n";
}

// ============================================================================
// SECTION 20: AUTOMATED TESTS (unit, edge cases, integration)
// ============================================================================

int testsPassed = 0, testsFailed = 0;

void check(bool condition, const string &name) {
    if (condition) { testsPassed++; cout << "  [PASS] " << name << "\n"; }
    else { testsFailed++; cout << "  [FAIL] " << name << "\n"; }
}

vector<SortItem> randomItems(int n, unsigned long long seed, int maxKey) {
    SimpleRandom rng(seed);
    vector<SortItem> items;
    for (int i = 0; i < n; i++) items.push_back({(long long)rng.range(0, maxKey), i});
    return items;
}

void testSortingAndSearching() {
    cout << "\n-- Sorting and searching --\n";
    vector<SortItem> base = randomItems(500, 1, 50);
    vector<SortItem> a = base, b = base, c = base;
    SortStats s1, s2, s3;
    mergeSort(a, s1);
    quickSort(b, PIVOT_LAST, s2);
    quickSort(c, PIVOT_MEDIAN_OF_THREE, s3);
    check(isSortedItems(a) && isSortedItems(b) && isSortedItems(c), "merge/quick (both pivots) sort 500 random items with duplicates");
    bool stable = true;
    for (size_t i = 1; i < a.size(); i++) if (a[i].key == a[i - 1].key && a[i].index < a[i - 1].index) stable = false;
    check(stable, "merge sort is stable");
    vector<SortItem> empty, single = {{5, 0}};
    SortStats s4;
    mergeSort(empty, s4); quickSort(empty, PIVOT_LAST, s4); mergeSort(single, s4); quickSort(single, PIVOT_LAST, s4);
    check(empty.empty() && single.size() == 1, "empty and single-element inputs");
    vector<SortItem> sorted;
    for (int i = 0; i < 3000; i++) sorted.push_back({i, i});
    SortStats s5;
    quickSort(sorted, PIVOT_LAST, s5);
    check(isSortedItems(sorted) && s5.maxDepth < 40, "quick sort on sorted input keeps recursion depth O(log n)");
    vector<long long> keys = {1, 3, 5, 7, 9, 11};
    long long cmp = 0;
    bool allFound = true;
    for (int i = 0; i < 6; i++)
        if (binarySearchIterative(keys, keys[i], cmp) != i || binarySearchRecursive(keys, 0, 5, keys[i], cmp) != i) allFound = false;
    check(allFound, "binary search (iterative + recursive) finds every key");
    check(binarySearchIterative(keys, 0, cmp) == -1 && binarySearchIterative(keys, 12, cmp) == -1 &&
          binarySearchRecursive(keys, 0, 5, 4, cmp) == -1, "binary search reports absent keys (below, above, between)");
    vector<long long> none;
    check(binarySearchIterative(none, 3, cmp) == -1 && binarySearchRecursive(none, 0, -1, 3, cmp) == -1, "binary search on empty array");
    check(lowerBoundIterative(keys, 6, cmp) == 3 && lowerBoundIterative(keys, 100, cmp) == 6 && lowerBoundIterative(keys, -5, cmp) == 0,
          "lower bound for time-window queries");
}

void testDivideAndConquer() {
    cout << "\n-- Divide and conquer --\n";
    vector<long long> clrs = {13, -3, -25, 20, -3, -16, -23, 18, 20, -7, 12, -5, -22, 15, -4, 7};
    long long ops = 0;
    SubarrayResult r = maxSubarrayDC(clrs, 0, 15, ops);
    check(r.sum == 43 && r.low == 7 && r.high == 10, "max subarray CLRS example = 43 at [7..10]");
    vector<long long> negatives = {-5, -2, -8};
    check(maxSubarrayDC(negatives, 0, 2, ops).sum == -2 && maxSubarrayKadane(negatives, ops).sum == -2, "max subarray all-negative input");
    bool agree = true;
    for (int t = 0; t < 40; t++) {
        SimpleRandom rng(t + 10);
        int n = rng.range(1, 60);
        vector<long long> a(n);
        for (int i = 0; i < n; i++) a[i] = rng.range(-20, 20);
        long long x = 0;
        long long s1 = maxSubarrayDC(a, 0, n - 1, x).sum, s2 = maxSubarrayKadane(a, x).sum, s3 = maxSubarrayBrute(a, x).sum;
        if (s1 != s2 || s2 != s3) agree = false;
    }
    check(agree, "D&C, Kadane and brute force agree on 40 random arrays");
    vector<long long> emptyArray;
    check(maxSubarrayDC(emptyArray, 0, -1, ops).low == -1, "max subarray on empty input returns empty result");
    long long m = 0;
    check(fastPowerIterative(2, 10, 1000, m) == 24 && fastPowerRecursive(2, 10, 1000, m) == 24, "2^10 mod 1000 = 24");
    check(fastPowerIterative(3, 0, 7, m) == 1 && fastPowerIterative(5, 3, 1, m) == 0, "exponent 0 and modulus 1 edge cases");
    check(fastPowerIterative(-2, 3, 7, m) == naivePower(-2, 3, 7, m), "negative base handled");
    check(fastPowerIterative(131, 1000000000000000000LL, 1000000007LL, m) == fastPowerRecursive(131, 1000000000000000000LL, 1000000007LL, m),
          "iterative = recursive for exponent 1e18");
    bool powAgree = true;
    for (int e = 0; e < 200; e++) if (fastPowerIterative(7, e, 1000003, m) != naivePower(7, e, 1000003, m)) powAgree = false;
    check(powAgree, "fast power = naive power for exponents 0..199");
    vector<vector<int>> knows = {{0, 0, 1, 0}, {1, 0, 1, 1}, {0, 0, 0, 0}, {0, 1, 1, 0}};
    long long q = 0;
    check(findCelebrity(knows, q) == 2 && findCelebrityBrute(knows, q) == 2, "celebrity found (person 2)");
    vector<vector<int>> noCeleb = {{0, 1}, {1, 0}};
    check(findCelebrity(noCeleb, q) == -1, "no celebrity when everybody knows somebody");
    vector<vector<int>> one = {{0}}, zero;
    check(findCelebrity(one, q) == 0 && findCelebrity(zero, q) == -1, "celebrity with n = 1 and n = 0");
    bool matrixAgree = true;
    for (int t = 0; t < 10; t++) {
        SimpleRandom rng(t + 99);
        int n = rng.range(1, 9), k = rng.range(1, 9), p = rng.range(1, 9);
        Matrix A = makeMatrix(n, k), B = makeMatrix(k, p);
        for (int i = 0; i < n; i++) for (int j = 0; j < k; j++) A[i][j] = rng.range(-9, 9);
        for (int i = 0; i < k; i++) for (int j = 0; j < p; j++) B[i][j] = rng.range(-9, 9);
        long long x = 0;
        if (multiplyClassical(A, B, x) != multiplyStrassen(A, B, 1, x) || multiplyClassical(A, B, x) != multiplyStrassen(A, B, 2, x)) matrixAgree = false;
    }
    check(matrixAgree, "Strassen (pure and hybrid) = classical on 10 random rectangular pairs");
    Matrix bad1 = makeMatrix(2, 3), bad2 = makeMatrix(2, 2);
    long long x = 0;
    check(multiplyClassical(bad1, bad2, x).empty() && multiplyStrassen(bad1, bad2, 1, x).empty(), "dimension mismatch rejected");
}

void testGreedy() {
    cout << "\n-- Greedy --\n";
    int raw[11][2] = {{1, 4}, {3, 5}, {0, 6}, {5, 7}, {3, 9}, {5, 9}, {6, 10}, {8, 11}, {8, 12}, {2, 14}, {12, 16}};
    vector<Interval> intervals;
    for (int i = 0; i < 11; i++) { Interval iv; iv.id = i; iv.start = raw[i][0]; iv.finish = raw[i][1]; intervals.push_back(iv); }
    long long ops = 0;
    check(activitySelection(intervals, ops).size() == 4, "activity selection CLRS example selects 4");
    vector<Interval> none;
    check(activitySelection(none, ops).empty() && intervalPartitioning(none, ops).roomsNeeded == 0, "empty interval list");
    vector<Interval> overlap;
    int raw2[5][2] = {{0, 10}, {1, 5}, {2, 6}, {6, 9}, {10, 12}};
    for (int i = 0; i < 5; i++) { Interval iv; iv.id = i; iv.start = raw2[i][0]; iv.finish = raw2[i][1]; overlap.push_back(iv); }
    check(intervalPartitioning(overlap, ops).roomsNeeded == 3, "interval partitioning: overlap depth 3 -> 3 rooms");
    vector<Interval> touching;
    Interval t1; t1.start = 1; t1.finish = 2; touching.push_back(t1);
    Interval t2; t2.start = 2; t2.finish = 3; touching.push_back(t2);
    check(intervalPartitioning(touching, ops).roomsNeeded == 1, "touching intervals share one room");
    vector<KnapsackItem> items(3);
    items[0].weight = 10; items[0].value = 60; items[1].weight = 20; items[1].value = 100; items[2].weight = 30; items[2].value = 120;
    check(fabs(fractionalKnapsack(items, 50).totalValue - 240.0) < 1e-9, "fractional knapsack classic example = 240");
    check(fractionalKnapsack(items, 0).totalValue == 0, "fractional knapsack with zero capacity");
    long long hops = 0;
    HuffmanCode h = buildHuffman("aaaabbc", hops);
    string bits = huffmanEncode("aaaabbc", h);
    check(bits.size() == 10 && huffmanDecode(bits, h) == "aaaabbc", "Huffman 'aaaabbc' -> 10 bits, decodes back");
    HuffmanCode single = buildHuffman("TTTT", hops);
    check(huffmanDecode(huffmanEncode("TTTT", single), single) == "TTTT", "Huffman single distinct symbol");
    HuffmanCode emptyCode = buildHuffman("", hops);
    check(emptyCode.root == -1 && huffmanEncode("", emptyCode).empty(), "Huffman empty text");
    SimpleRandom rng(3);
    string text;
    for (int i = 0; i < 2000; i++) text += (char)('A' + rng.range(0, 7) * rng.range(0, 1));
    HuffmanCode big = buildHuffman(text, hops);
    check(huffmanDecode(huffmanEncode(text, big), big) == text, "Huffman round trip on 2000 skewed symbols");
}

void testGraphs() {
    cout << "\n-- Graphs --\n";
    Graph g = buildInfrastructureGraph(false, false);
    PathResult d = dijkstra(g, 0);
    check(d.distance[4] == 17 && d.distance[8] == 29, "Dijkstra known distances (CentreA 17, CentreE 29)");
    Graph isolated = buildInfrastructureGraph(false, true);
    check(dijkstra(isolated, 0).distance[8] >= INF, "unreachable node reported as INF");
    Graph negative;
    negative.vertexCount = 2; negative.directed = true; negative.names = {"a", "b"}; negative.edges.push_back({0, 1, -1});
    check(dijkstra(negative, 0).invalidInput, "Dijkstra rejects negative weights");
    check(dijkstra(g, 99).invalidInput, "Dijkstra rejects invalid source");
    check(bellmanFord(g, 0).distance == d.distance, "Bellman-Ford = Dijkstra on non-negative graph");
    check(bellmanFord(buildTimelineConstraints(true), 4).negativeCycle, "Bellman-Ford detects contradictory timeline (negative cycle)");
    check(!bellmanFord(buildTimelineConstraints(false), 4).negativeCycle, "consistent timeline has no negative cycle");
    Graph withNeg;
    withNeg.vertexCount = 4; withNeg.directed = true; withNeg.names = {"s", "a", "b", "c"};
    withNeg.edges = {{0, 1, 4}, {0, 2, 5}, {2, 1, -3}, {1, 3, 2}};
    PathResult bf = bellmanFord(withNeg, 0);
    check(bf.distance[1] == 2 && bf.distance[3] == 4 && !bf.negativeCycle, "Bellman-Ford with negative edge (no cycle)");
    vector<vector<int>> next;
    long long ops = 0;
    bool neg = false;
    vector<vector<long long>> all = floydWarshall(g, next, ops, neg);
    bool same = true;
    for (int s = 0; s < g.vertexCount; s++) if (dijkstra(g, s).distance != all[s]) same = false;
    check(same && !neg, "Floyd-Warshall = Dijkstra from every source");
    MSTResult p = primMST(g), k = kruskalMST(g);
    check(p.totalWeight == k.totalWeight && p.spanning && k.spanning && p.chosen.size() == 8, "Prim = Kruskal total, 8 edges for 9 vertices");
    MSTResult pf = primMST(isolated), kf = kruskalMST(isolated);
    check(!pf.spanning && !kf.spanning && pf.totalWeight == kf.totalWeight, "disconnected graph -> spanning forest reported");
    bool randomAgree = true;
    for (int t = 0; t < 5; t++) {
        Graph rg = randomConnectedGraph(40, 3, 500 + t, false);
        if (primMST(rg).totalWeight != kruskalMST(rg).totalWeight || dijkstra(rg, 0).distance != bellmanFord(rg, 0).distance) randomAgree = false;
    }
    check(randomAgree, "Prim/Kruskal and Dijkstra/Bellman-Ford agree on 5 random graphs");
}

int bruteKnapsack(const vector<KnapsackItem> &items, int capacity) {
    int n = (int)items.size(), best = 0;
    for (int mask = 0; mask < (1 << n); mask++) {
        int w = 0, v = 0;
        for (int i = 0; i < n; i++) if (mask & (1 << i)) { w += items[i].weight; v += items[i].value; }
        if (w <= capacity && v > best) best = v;
    }
    return best;
}

void testDynamicProgrammingAndSearch() {
    cout << "\n-- Dynamic programming, backtracking, branch and bound --\n";
    bool allAgree = true;
    for (int t = 0; t < 30; t++) {
        SimpleRandom rng(t + 1000);
        int n = rng.range(1, 12);
        vector<KnapsackItem> items;
        int total = 0;
        for (int i = 0; i < n; i++) {
            KnapsackItem it;
            it.id = i; it.weight = rng.range(1, 30); it.value = rng.range(0, 50);
            total += it.weight;
            items.push_back(it);
        }
        int capacity = rng.range(0, total);
        int oracle = bruteKnapsack(items, capacity);
        KnapsackResult results[5] = {knapsackDP(items, capacity), knapsackBacktracking(items, capacity),
                                     knapsackBranchAndBound(items, capacity, BB_FIFO, 1000000),
                                     knapsackBranchAndBound(items, capacity, BB_LIFO, 1000000),
                                     knapsackBranchAndBound(items, capacity, BB_LEAST_COST, 1000000)};
        for (const KnapsackResult &r : results)
            if (r.bestValue != oracle || r.totalWeight > capacity) allAgree = false;
    }
    check(allAgree, "DP, backtracking, FIFO/LIFO/LC B&B = brute force on 30 random instances (and respect capacity)");
    vector<KnapsackItem> heavy(1);
    heavy[0].weight = 50; heavy[0].value = 10;
    check(knapsackDP(heavy, 10).bestValue == 0 && knapsackBranchAndBound(heavy, 10, BB_LEAST_COST, 100).bestValue == 0, "item heavier than capacity");
    vector<KnapsackItem> noItems;
    check(knapsackDP(noItems, 100).bestValue == 0 && knapsackBacktracking(noItems, 100).bestValue == 0, "knapsack with no items");
    string error;
    vector<KnapsackItem> zeroWeight(1);
    zeroWeight[0].weight = 0;
    check(!validateKnapsack(zeroWeight, 10, error), "knapsack validation rejects zero-minute case");
    vector<string> names = {"A1", "A2", "A3", "A4", "A5", "A6"};
    check(matrixChainOrder({30, 35, 15, 5, 10, 20, 25}, names).minCost == 15125, "matrix chain CLRS example = 15125");
    check(matrixChainOrder({10, 20}, names).minCost == 0 && !matrixChainOrder({10}, names).valid, "matrix chain with 1 and 0 matrices");
    string common;
    long long ops = 0;
    check(lcsLength("ABCBDAB", "BDCABA", common, ops) == 4 && common.size() == 4, "LCS('ABCBDAB','BDCABA') = 4");
    check(lcsLength("", "ABC", common, ops) == 0, "LCS with empty string");
    check(optimalBST({34, 8, 50}).cost == 142, "optimal BST {34, 8, 50} = 142");
    check(optimalBST({}).cost == 0, "optimal BST with no keys");
    vector<vector<long long>> four = {{0, 10, 15, 20}, {10, 0, 35, 25}, {15, 35, 0, 30}, {20, 25, 30, 0}};
    check(tspHeldKarp(four).cost == 80 && tspBruteForce(four).cost == 80, "TSP 4-city example = 80");
    bool tspAgree = true;
    for (int t = 0; t < 5; t++) {
        SimpleRandom rng(t + 50);
        int n = 7;
        vector<vector<long long>> dist(n, vector<long long>(n, 0));
        for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) if (i != j) dist[i][j] = rng.range(1, 50);   // asymmetric too
        if (tspHeldKarp(dist).cost != tspBruteForce(dist).cost) tspAgree = false;
    }
    check(tspAgree, "Held-Karp = brute force on 5 random asymmetric 7-city instances");
    vector<vector<long long>> disconnected = {{0, INF, 1}, {INF, 0, INF}, {1, INF, 0}};
    check(!tspHeldKarp(disconnected).valid, "TSP with no Hamiltonian cycle reported invalid");
    check(solveNQueens(1).solutions == 1 && solveNQueens(2).solutions == 0 && solveNQueens(3).solutions == 0 &&
          solveNQueens(4).solutions == 2 && solveNQueens(6).solutions == 4 && solveNQueens(8).solutions == 92,
          "N-Queens counts 1,0,0,2,4,92 for n = 1,2,3,4,6,8");
    SubsetResult subsets = sumOfSubsets({1, 2, 3, 4, 5}, 5, 10);
    check(subsets.totalSolutions == 3, "sum of subsets {1..5} target 5 -> 3 subsets");
    check(sumOfSubsets({1, 2}, 10, 10).totalSolutions == 0 && sumOfSubsets({1, 2}, 0, 10).totalSolutions == 0 &&
          sumOfSubsets({-1, 2}, 1, 10).totalSolutions == 0, "sum of subsets: unreachable, zero target, negative values");
}

void testFlowCrtStrings() {
    cout << "\n-- Flow, matching, CRT, string matching --\n";
    FlowNetwork net = makeFlowNetwork(6);
    long long caps[10][3] = {{0, 1, 16}, {0, 2, 13}, {1, 3, 12}, {2, 1, 4}, {2, 4, 14}, {3, 2, 9}, {3, 5, 20}, {4, 3, 7}, {4, 5, 4}, {1, 2, 0}};
    for (int i = 0; i < 10; i++) net.capacity[caps[i][0]][caps[i][1]] += caps[i][2];
    check(fordFulkerson(net, 0, 5).maxFlow == 23, "Ford-Fulkerson CLRS network = 23");
    check(fordFulkerson(net, 0, 0).maxFlow == 0 && fordFulkerson(net, 0, 99).maxFlow == 0, "flow with source = sink / invalid sink");
    FlowNetwork cut = makeFlowNetwork(3);
    cut.capacity[0][1] = 5;
    check(fordFulkerson(cut, 0, 2).maxFlow == 0, "sink unreachable -> flow 0");
    vector<vector<int>> adjacent = {{0, 1}, {0}, {2}};
    vector<int> match;
    long long ops = 0;
    check(maxBipartiteMatching(adjacent, 3, match, ops) == 3, "bipartite matching small example = 3");
    bool same = true;
    for (int t = 0; t < 10; t++) {
        SimpleRandom rng(t + 70);
        int l = rng.range(1, 12), r = rng.range(1, 12);
        vector<vector<int>> adj(l);
        for (int i = 0; i < l; i++) for (int k = 0; k < 3; k++) if (rng.chance(60)) adj[i].push_back(rng.range(0, r - 1));
        vector<int> m2;
        long long o = 0;
        if (maxBipartiteMatching(adj, r, m2, o) != matchingViaFlow(adj, r, o)) same = false;
    }
    check(same, "augmenting-path matching = max-flow matching on 10 random graphs");
    long long answer = 0, product = 0;
    string error;
    check(chineseRemainder({2, 3, 2}, {3, 5, 7}, answer, product, error) && answer == 23 && product == 105, "CRT x=2(3), 3(5), 2(7) -> 23");
    check(!chineseRemainder({1, 2}, {6, 9}, answer, product, error), "CRT rejects non-coprime moduli");
    check(!chineseRemainder({1}, {1}, answer, product, error) && !chineseRemainder({}, {}, answer, product, error), "CRT rejects modulus 1 / empty input");
    check(chineseRemainder({123456}, {1000003}, answer, product, error) && answer == 123456, "CRT with a single modulus");
    string text = "ABABDABACDABABCABAB", pattern = "ABABCABAB";
    bool basic = true;
    for (int a = 0; a < 4; a++) { MatchResult r = runMatcher(a, text, pattern); if (r.positions.size() != 1 || r.positions[0] != 10) basic = false; }
    check(basic, "all 4 matchers find 'ABABCABAB' at index 10");
    bool overlap = true;
    for (int a = 0; a < 4; a++) { MatchResult r = runMatcher(a, "AAAAAA", "AAAA"); if (r.positions != vector<int>({0, 1, 2})) overlap = false; }
    check(overlap, "overlapping matches 'AAAA' in 'AAAAAA' -> 0,1,2");
    bool edge = true;
    for (int a = 0; a < 4; a++) if (!runMatcher(a, "ABC", "").positions.empty() || !runMatcher(a, "AB", "ABC").positions.empty()) edge = false;
    check(edge, "empty pattern and pattern longer than text -> no matches");
    bool randomAgree = true;
    for (int t = 0; t < 50; t++) {
        SimpleRandom rng(t + 300);
        string tx, pt;
        int n = rng.range(0, 200), m = rng.range(1, 5);
        for (int i = 0; i < n; i++) tx += (char)('a' + rng.range(0, 1));
        for (int i = 0; i < m; i++) pt += (char)('a' + rng.range(0, 1));
        vector<int> ref = runMatcher(0, tx, pt).positions;
        for (int a = 1; a < 4; a++) if (runMatcher(a, tx, pt).positions != ref) randomAgree = false;
    }
    check(randomAgree, "all 4 matchers agree on 50 random binary texts");
}

void testDomainIntegration() {
    cout << "\n-- Domain model and integration --\n";
    RiskRules rules;
    Dataset normal = generateScenario(1);
    ValidationSummary v1 = validateDataset(normal);
    AnalysisReport r1 = analyzeDataset(normal, rules);
    check(v1.duplicatesRemoved == 0 && v1.orphanEvents == 0, "normal scenario has no data faults");
    check(r1.highCount == 0 && r1.mediumCount == 0, "normal scenario: no HIGH or MEDIUM cases (low false-positive rate)");
    check((int)r1.risks.size() == (int)normal.candidates.size(), "every candidate appears in the report");
    Dataset suspicious = generateScenario(2);
    size_t before = suspicious.events.size();
    ValidationSummary v2 = validateDataset(suspicious);
    check(v2.duplicatesRemoved == 1 && suspicious.events.size() == before - 1, "duplicate event detected and removed");
    check(v2.orphanEvents == 1, "event of a missing candidate (9999) detected");
    AnalysisReport r2 = analyzeDataset(suspicious, rules);
    check(r2.highCount + r2.mediumCount >= 3, "suspicious scenario produces HIGH/MEDIUM cases");
    check(r2.orphanEvents == 1, "orphan event excluded from scoring");
    bool ordered = true;
    for (size_t i = 1; i < r2.risks.size(); i++) if (r2.risks[i - 1].score < r2.risks[i].score) ordered = false;
    check(ordered, "review queue is sorted by score (highest first)");
    bool explained = true;
    for (const CandidateRisk &r : r2.risks) if (r.score > 0 && r.reasons.empty()) explained = false;
    check(explained, "every non-zero score has at least one explanation");
    bool patternFound = false;
    for (const CandidateRisk &r : r2.risks) if (!r.patternPositions.empty()) patternFound = true;
    check(patternFound, "hand-off sequence 'NLD' detected by KMP in suspicious scenario");
    vector<string> names;
    long long q = 0;
    vector<vector<int>> contacts2 = buildContactMatrix(suspicious, names);
    check(findCelebrity(contacts2, q) == (int)names.size() - 1, "planted relay endpoint identified as celebrity");
    vector<vector<int>> contacts1 = buildContactMatrix(normal, names);
    check(findCelebrity(contacts1, q) == -1, "no relay in normal scenario");
    const CandidateRisk &top = r2.risks[0];
    long long m1 = 0, m2 = 0;
    long long fp1 = evidenceFingerprint(suspicious, top.eventIndices, m1);
    Dataset tampered = suspicious;
    tampered.events[top.eventIndices[0]].timestamp += 1;
    long long fp2 = evidenceFingerprint(tampered, top.eventIndices, m2);
    check(fp1 != fp2, "editing one evidence row changes the timeline fingerprint");
    Dataset shock = generateScenario(3);
    validateDataset(shock);
    AnalysisReport r3 = analyzeDataset(shock, rules);
    vector<ReviewCase> cases = buildReviewCases(r3);
    long long ops = 0;
    PartitionResult rooms = intervalPartitioning(buildHearingRequests(cases), ops);
    check(rooms.roomsNeeded > shock.reviewRooms, "capacity shock: rooms needed exceed rooms available");
    FlowNetwork net = buildReviewFlowNetwork(cases, shock.reviewers);
    int demand = 0;
    for (const ReviewCase &c : cases) demand += c.minutes;
    long long flow = fordFulkerson(net, 0, net.nodeCount - 1).maxFlow;
    check(flow < demand && flow <= 240, "capacity shock: flow < demand and <= reviewer supply (240 min)");
    check(exportEventsCSV(suspicious, "test_tmp_events.csv") && exportCandidatesCSV(suspicious, "test_tmp_candidates.csv"), "CSV export");
    Dataset reloaded;
    vector<string> errors;
    bool loadedOk = loadDatasetFromCSV("test_tmp_candidates.csv", "test_tmp_events.csv", reloaded, errors);
    check(loadedOk && errors.empty() && reloaded.events.size() == suspicious.events.size() &&
          reloaded.candidates.size() == suspicious.candidates.size(), "CSV round trip keeps every candidate and event");
    {
        ofstream bad("test_tmp_bad.csv");
        bad << "eventId,candidateId,examId,eventType,timestamp,deviceId,severity,description\n";
        bad << "1,1001,X,LOGIN_OK,10,5,0,ok row\n2,abc,X,LOGIN_OK,10,5,0,bad id\n3,1001,X,NOT_A_TYPE,10,5,0,bad type\n4,1001,X,TAB_SWITCH,-5,5,1,bad time\n5,1001\n";
    }
    Dataset partial;
    vector<string> errors2;
    loadDatasetFromCSV("test_tmp_candidates.csv", "test_tmp_bad.csv", partial, errors2);
    check(partial.events.size() == 1 && errors2.size() == 4, "invalid CSV rows are rejected with line-numbered errors");
    vector<string> errors3;
    check(!loadDatasetFromCSV("missing_file_xyz.csv", "test_tmp_bad.csv", partial, errors3), "missing file reported");
    check(exportRiskJSON(suspicious, r2, "test_tmp_report.json") && exportRiskCSV(r2, "test_tmp_report.csv"), "JSON and CSV report export");
    remove("test_tmp_events.csv"); remove("test_tmp_candidates.csv"); remove("test_tmp_bad.csv");
    remove("test_tmp_report.json"); remove("test_tmp_report.csv");
    long long parsed = 0;
    check(!parseLongStrict("12abc", parsed) && !parseLongStrict("", parsed) && !parseLongStrict("99999999999999999999", parsed) &&
          parseLongStrict(" 42 ", parsed) && parsed == 42, "strict numeric input validation");
    Dataset empty;
    AnalysisReport r0 = analyzeDataset(empty, rules);
    check(r0.risks.empty() && r0.highCount == 0, "analysis of an empty dataset does not crash");
}

int runAllTests() {
    testsPassed = testsFailed = 0;
    printHeader("AUTOMATED TEST SUITE");
    testSortingAndSearching();
    testDivideAndConquer();
    testGreedy();
    testGraphs();
    testDynamicProgrammingAndSearch();
    testFlowCrtStrings();
    testDomainIntegration();
    printLine();
    cout << "RESULT: " << testsPassed << " passed, " << testsFailed << " failed.\n";
    return testsFailed;
}

// ============================================================================
// SECTION 21: COMPLEXITY REPORT, RECURRENCES AND P/NP REDUCTION
// ============================================================================

void demonstrateReduction() {
    cout << "\nREDUCTION DEMO: SUBSET-SUM  <=p  REVIEW-SELECT (decision 0/1 knapsack)\n";
    cout << "Mapping (O(n) time): each number s_i -> review case with minutes w_i = s_i and risk v_i = s_i;\n"
         << "budget W = t and goal K = t. Some subset sums to t  <=>  some case set has minutes <= t and risk >= t\n"
         << "(because risk = minutes, both conditions force the sum to be exactly t).\n";
    vector<int> numbers = {3, 34, 4, 12, 5, 2};
    int targets[2] = {9, 30};
    for (int t : targets) {
        vector<KnapsackItem> items;
        for (int i = 0; i < (int)numbers.size(); i++) {
            KnapsackItem it;
            it.id = i; it.label = to_string(numbers[i]); it.weight = numbers[i]; it.value = numbers[i];
            items.push_back(it);
        }
        KnapsackResult r = knapsackDP(items, t);
        bool viaKnapsack = r.bestValue >= t;
        bool viaSubset = sumOfSubsets(numbers, t, 1).totalSolutions > 0;
        cout << "  set {3,34,4,12,5,2}, t = " << t << ": REVIEW-SELECT says " << yesNo(viaKnapsack) << ", direct SUBSET-SUM says "
             << yesNo(viaSubset) << (viaKnapsack == viaSubset ? "  (answers agree)" : "  (MISMATCH)") << "\n";
    }
}

struct ComplexityRow {
    string name, best, average, worst, space;
};

void printComplexityReport() {
    printHeader("COMPLEXITY REPORT");
    ComplexityRow rows[] = {
        {"Merge sort", "n log n", "n log n", "n log n", "n"},
        {"Quick sort (smaller side first)", "n log n", "n log n", "n^2", "log n"},
        {"Binary search iterative", "1", "log n", "log n", "1"},
        {"Binary search recursive", "1", "log n", "log n", "log n"},
        {"Max subarray (D&C)", "n log n", "n log n", "n log n", "log n"},
        {"Fast exponentiation", "log e", "log e", "log e", "1 / log e"},
        {"Celebrity (D&C)", "n", "n", "n", "log n"},
        {"Classical matrix mult.", "n^3", "n^3", "n^3", "n^2"},
        {"Strassen", "n^2.807", "n^2.807", "n^2.807", "n^2"},
        {"Activity selection", "n log n", "n log n", "n log n", "n"},
        {"Interval partitioning", "n log n", "n log n", "n log n", "n"},
        {"Fractional knapsack", "n log n", "n log n", "n log n", "n"},
        {"Huffman coding", "n+k log k", "n+k log k", "n+k log k", "k"},
        {"Dijkstra (array)", "V^2+E", "V^2+E", "V^2+E", "V+E"},
        {"Prim (matrix)", "V^2", "V^2", "V^2", "V^2"},
        {"Kruskal", "E log E", "E log E", "E log E", "V+E"},
        {"0/1 knapsack DP", "nW", "nW", "nW", "nW"},
        {"Matrix chain DP", "n^3", "n^3", "n^3", "n^2"},
        {"LCS", "nm", "nm", "nm", "nm"},
        {"Optimal BST", "n^3", "n^3", "n^3", "n^2"},
        {"Bellman-Ford", "E", "VE", "VE", "V"},
        {"Floyd-Warshall", "V^3", "V^3", "V^3", "V^2"},
        {"TSP Held-Karp", "n^2 2^n", "n^2 2^n", "n^2 2^n", "n 2^n"},
        {"TSP brute force", "n!", "n!", "n!", "n"},
        {"N-Queens backtracking", "-", "-", "O(n!)", "n"},
        {"Sum of subsets", "-", "-", "O(2^n)", "n"},
        {"Knapsack backtracking", "-", "-", "O(n 2^n)", "n"},
        {"B&B FIFO / LIFO / LC", "-", "-", "O(n 2^n)", "nodes"},
        {"Ford-Fulkerson (DFS)", "-", "-", "O(E f*)", "V^2"},
        {"Bipartite matching", "-", "-", "O(VE)", "V"},
        {"Chinese Remainder", "k log M", "k log M", "k log M", "k"},
        {"Naive matching", "n", "nm", "nm", "1"},
        {"Rabin-Karp", "n+m", "n+m", "nm", "1"},
        {"Finite automaton", "n+m*256", "n+m*256", "n+m*256", "m*256"},
        {"KMP", "n+m", "n+m", "n+m", "m"},
    };
    cout << padRight("Algorithm", 33) << padRight("Best", 12) << padRight("Average", 12) << padRight("Worst", 12) << "Space\n";
    printLine();
    for (const ComplexityRow &r : rows) cout << padRight(r.name, 33) << padRight(r.best, 12) << padRight(r.average, 12) << padRight(r.worst, 12) << r.space << "\n";
    cout << "(All bounds are Theta unless written as O. Space = auxiliary space.)\n";

    printHeader("RECURRENCES");
    cout << "1. Merge sort / max subarray: T(n) = 2T(n/2) + cn\n"
         << "   Iteration    : T(n) = 2^k T(n/2^k) + k*cn; with k = log2 n -> cn log n + n*T(1) = Theta(n log n)\n"
         << "   Substitution : guess T(n) <= d n log n; 2d(n/2)log(n/2) + cn = dn log n - dn + cn <= dn log n for d >= c\n"
         << "   Recursion tree: log2 n + 1 levels, each level costs cn -> Theta(n log n)\n"
         << "   Master       : a=2, b=2, n^(log_b a) = n, f(n) = Theta(n) -> case 2 -> Theta(n log n)\n"
         << "2. Binary search / fast power: T(n) = T(n/2) + c -> a=1, b=2, n^0 = 1 = f(n) -> case 2 -> Theta(log n)\n"
         << "3. Strassen: T(n) = 7T(n/2) + cn^2 -> n^(log2 7) = n^2.807 dominates n^2 -> case 1 -> Theta(n^2.807)\n"
         << "   (8 recursive products, as in naive D&C, give 8T(n/2)+cn^2 = Theta(n^3).)\n"
         << "4. Celebrity D&C: T(n) = 2T(n/2) + 1 -> case 1 -> Theta(n)\n"
         << "5. Quick sort worst case: T(n) = T(n-1) + cn. MASTER THEOREM NOT APPLICABLE (subtracts 1, not of form aT(n/b)).\n"
         << "   Iteration: c(n + (n-1) + ... + 1) = cn(n+1)/2 = Theta(n^2).\n"
         << "6. TSP brute force: T(n) = n T(n-1) -> Theta(n!). Master not applicable.\n"
         << "7. T(n) = 2T(n/2) + n/log n fits the form but f(n) is not polynomially smaller than n -> falls in the gap\n"
         << "   between cases 1 and 2, so the basic Master Theorem does not apply (answer Theta(n log log n) by recursion tree).\n";

    printHeader("COMPLEXITY CLASSES (decision versions)");
    cout << "P   : sorting, searching, shortest paths, MST, max-flow, bipartite matching, LCS, matrix chain, string matching.\n"
         << "NP-complete: REVIEW-SELECT (0/1 knapsack decision: cases with minutes w_i, risk v_i, budget W, goal K -\n"
         << "      is there a set with total minutes <= W and total risk >= K?). In NP: a chosen set is checked in O(n).\n"
         << "      NP-hard: SUBSET-SUM (known NP-complete) reduces to it in polynomial time (demo below).\n"
         << "      The O(nW) DP does not contradict this: it is pseudo-polynomial (W is exponential in its bit length).\n"
         << "NP-complete: AUDIT-ROUTE (TSP decision: is there a tour of length <= L?), via Hamiltonian cycle (known result).\n"
         << "NP-hard    : the optimisation versions of both problems.\n"
         << "N-Queens   : one solution for a given n can be built in polynomial time; we use backtracking only as a\n"
         << "             constraint-solving exercise. (The n-queens COMPLETION variant has been shown NP-complete.)\n";
    demonstrateReduction();

    printHeader("LIMITATIONS AND ETHICS");
    cout << "* Synthetic data only; thresholds and weights are configurable assumptions, not validated detectors.\n"
         << "* Innocent causes (poor network, shared family devices, accessibility tools) can trigger indicators.\n"
         << "* Output is a prioritised list for HUMAN reviewers, with reasons and an evidence trail - never an automatic verdict.\n"
         << "* Memory figures in the benchmark are analytical estimates of auxiliary storage, not OS measurements.\n";
}

// ============================================================================
// SECTION 22: MENUS, DEMONSTRATION SCRIPT AND main()
// ============================================================================

void writeSampleDatasets() {
    printHeader("WRITING SAMPLE DATASETS");
    for (int s = 1; s <= 4; s++) {
        Dataset data = generateScenario(s);
        string prefix = "sample_scenario" + to_string(s);
        bool ok = exportEventsCSV(data, prefix + "_events.csv") && exportCandidatesCSV(data, prefix + "_candidates.csv");
        cout << "  " << prefix << "_events.csv / _candidates.csv  (" << data.candidates.size() << " candidates, "
             << data.events.size() << " events) " << (ok ? "OK" : "FAILED") << "\n";
    }
    cout << "Scenarios 2 and 3 deliberately contain one duplicate event and one orphan event for validation.\n";
}

void menuLoadData(AppState &app) {
    printHeader("LOAD / GENERATE DATASET");
    cout << " 1. Scenario 1 - Normal operation (30 candidates)\n"
         << " 2. Scenario 2 - Suspicious activity (30 candidates, planted patterns)\n"
         << " 3. Scenario 3 - Capacity / constraint shock (40 candidates, 2 reviewers, 1 room)\n"
         << " 4. Scenario 4 - Large-data stress test (5000 candidates)\n"
         << " 5. Load from CSV files\n"
         << " 6. Write sample dataset CSV files for all scenarios\n"
         << " 0. Back\n";
    int choice = readInt("Choice: ", 0, 6);
    if (choice >= 1 && choice <= 4) loadScenario(app, choice, true);
    else if (choice == 5) {
        string candidateFile = readText("Candidates CSV file (e.g. sample_scenario2_candidates.csv): ");
        string eventFile = readText("Events CSV file (e.g. sample_scenario2_events.csv): ");
        Dataset data;
        vector<string> errors;
        if (loadDatasetFromCSV(candidateFile, eventFile, data, errors)) {
            app.data = data;
            app.validation = validateDataset(app.data);
            app.loaded = true;
            app.analyzed = false;
            cout << "Loaded " << app.data.candidates.size() << " candidates and " << app.data.events.size() << " events.\n";
            for (const string &note : app.validation.notes) cout << "  Validation: " << note << "\n";
        }
        for (const string &e : errors) cout << "  Error: " << e << "\n";
    } else if (choice == 6) writeSampleDatasets();
}

void menuViewCandidates(AppState &app) {
    ensureLoaded(app);
    printHeader("CANDIDATES - " + app.data.scenarioName);
    cout << padRight("CandID", 8) << padRight("Name", 18) << padRight("Exam", 12) << "Primary device\n";
    printLine();
    for (size_t i = 0; i < app.data.candidates.size() && i < 40; i++) {
        const Candidate &c = app.data.candidates[i];
        cout << padRight(to_string(c.candidateId), 8) << padRight(c.name, 18) << padRight(c.examId, 12) << c.primaryDeviceId << "\n";
    }
    if (app.data.candidates.size() > 40) cout << "... " << app.data.candidates.size() - 40 << " more\n";
    cout << "Reviewers:\n";
    for (const Reviewer &r : app.data.reviewers)
        cout << "  " << r.reviewerId << " " << padRight(r.name, 15) << r.availableMinutes << " min, skills " << skillsText(r.expertiseMask) << "\n";
}

void menuRecordEvent(AppState &app) {
    ensureLoaded(app);
    printHeader("RECORD A NEW EVENT");
    vector<SortItem> items;
    for (int i = 0; i < (int)app.data.candidates.size(); i++) items.push_back({(long long)app.data.candidates[i].candidateId, i});
    SortStats stats;
    mergeSort(items, stats);
    vector<long long> keys = keysOf(items);
    long long comparisons = 0;
    int candidateId = readInt("Candidate ID: ", 1, 99999999);
    int position = binarySearchIterative(keys, candidateId, comparisons);
    if (position == -1) { cout << "Candidate " << candidateId << " is not registered - event rejected (missing candidate).\n"; return; }
    const Candidate &c = app.data.candidates[items[position].index];
    for (int t = 0; t < EVENT_TYPE_COUNT; t++) cout << "  " << t << ". " << EVENT_NAMES[t] << "\n";
    int type = readInt("Event type number: ", 0, EVENT_TYPE_COUNT - 1);
    int timestamp = readInt("Seconds since exam start (0-" + to_string(app.data.examDurationSeconds) + "): ", 0, app.data.examDurationSeconds);
    int device = readInt("Device ID (ENTER a number, primary is " + to_string(c.primaryDeviceId) + "): ", 1, 999999999);
    string description = readText("Description (ENTER for default): ");
    if (description.empty()) description = describeEvent(type, device, c.primaryDeviceId);
    for (const ExamEvent &e : app.data.events)
        if (e.candidateId == candidateId && e.eventType == type && e.timestamp == timestamp && e.deviceId == device) {
            cout << "Duplicate of event " << e.eventId << " - not recorded.\n";
            return;
        }
    int maxId = 0;
    for (const ExamEvent &e : app.data.events) maxId = max(maxId, e.eventId);
    ExamEvent e;
    e.eventId = maxId + 1; e.candidateId = candidateId; e.examId = c.examId; e.eventType = type; e.timestamp = timestamp;
    e.deviceId = device; e.severity = DEFAULT_SEVERITY[type]; e.description = csvSafe(description);
    app.data.events.push_back(e);
    app.analyzed = false;
    cout << "Recorded event " << e.eventId << " for candidate " << candidateId << " (found by binary search in " << comparisons << " comparisons).\n";
}

void menuSearch(AppState &app) {
    ensureLoaded(app);
    printHeader("SEARCH EVENTS");
    cout << " 1. Find event by Event ID (iterative vs recursive binary search)\n"
         << " 2. List events in a time window (binary-search lower bound)\n"
         << " 3. List all events of one candidate\n 0. Back\n";
    int choice = readInt("Choice: ", 0, 3);
    if (choice == 1) {
        vector<SortItem> items;
        for (int i = 0; i < (int)app.data.events.size(); i++) items.push_back({(long long)app.data.events[i].eventId, i});
        SortStats stats;
        mergeSort(items, stats);
        vector<long long> keys = keysOf(items);
        long long target = readNumber("Event ID: ", 0, 2000000000);
        long long c1 = 0, c2 = 0, c3 = 0;
        int p1 = binarySearchIterative(keys, target, c1);
        int p2 = binarySearchRecursive(keys, 0, (int)keys.size() - 1, target, c2);
        linearSearch(keys, target, c3);
        cout << "Iterative: " << c1 << " comparisons | Recursive: " << c2 << " comparisons | Linear: " << c3 << " comparisons\n";
        if (p1 == -1 || p2 == -1) { cout << "Event " << target << " not found.\n"; return; }
        const ExamEvent &e = app.data.events[items[p1].index];
        cout << "Event " << e.eventId << ": candidate " << e.candidateId << ", " << formatClock(e.timestamp) << ", " << EVENT_NAMES[e.eventType]
             << ", device " << e.deviceId << ", severity " << e.severity << ", \"" << e.description << "\"\n";
    } else if (choice == 2) {
        vector<SortItem> items;
        for (int i = 0; i < (int)app.data.events.size(); i++) items.push_back({(long long)app.data.events[i].timestamp, i});
        SortStats stats;
        mergeSort(items, stats);
        vector<long long> keys = keysOf(items);
        int from = readInt("From second: ", 0, app.data.examDurationSeconds);
        int to = readInt("To second: ", from, app.data.examDurationSeconds);
        long long comparisons = 0;
        int start = lowerBoundIterative(keys, from, comparisons);
        int shown = 0, total = 0;
        for (int i = start; i < (int)items.size() && keys[i] <= to; i++) {
            total++;
            if (shown >= 40) continue;
            const ExamEvent &e = app.data.events[items[i].index];
            cout << "  " << formatClock(e.timestamp) << "  cand " << e.candidateId << "  " << padRight(EVENT_NAMES[e.eventType], 14) << e.description << "\n";
            shown++;
        }
        cout << total << " event(s) in window (start located with " << comparisons << " comparisons)" << (total > 40 ? ", first 40 shown" : "") << ".\n";
    } else if (choice == 3) {
        ensureAnalyzed(app);
        int candidateId = readInt("Candidate ID: ", 1, 99999999);
        int position = findRiskIndex(app.report, candidateId);
        if (position == -1) { cout << "Candidate not found.\n"; return; }
        const CandidateRisk &risk = app.report.risks[position];
        for (int idx : risk.eventIndices) {
            const ExamEvent &e = app.data.events[idx];
            cout << "  " << e.eventId << "  " << formatClock(e.timestamp) << "  " << padRight(EVENT_NAMES[e.eventType], 14) << e.description << "\n";
        }
        if (risk.eventIndices.empty()) cout << "  (no events logged)\n";
    }
}

void menuDetection(AppState &app) {
    ensureLoaded(app);
    app.report = analyzeDataset(app.data, app.rules);
    app.analyzed = true;
    printHeader("SUSPICIOUS-PATTERN DETECTION - REVIEW QUEUE (" + app.data.scenarioName + ")");
    printAnalysisSummary(app.data, app.report);
    cout << "\n";
    printRiskTable(app.report, 15);
}

void menuTimeline(AppState &app) {
    ensureAnalyzed(app);
    if (!app.report.risks.empty())
        cout << "Highest-priority candidate is " << app.report.risks[0].candidateId << " (score " << app.report.risks[0].score << ").\n";
    int candidateId = readInt("Candidate ID for timeline: ", 1, 99999999);
    int position = findRiskIndex(app.report, candidateId);
    if (position == -1) { cout << "Candidate " << candidateId << " not found in this dataset.\n"; return; }
    const CandidateRisk &risk = app.report.risks[position];
    printEvidenceTimeline(app.data, risk);
    if (readInt("Export this timeline to CSV? (1 = yes, 0 = no): ", 0, 1) == 1) {
        string file = "timeline_" + to_string(candidateId) + ".csv";
        if (exportTimelineCSV(app.data, risk, file)) cout << "Saved " << file << "\n";
        else cout << "Could not write " << file << "\n";
    }
}

void menuRules(AppState &app) {
    while (true) {
        printHeader("RISK RULES (configurable)");
        RiskRules &r = app.rules;
        cout << " 1. Failed-login limit      : " << r.limit[LOGIN_FAILED] << "\n"
             << " 2. Tab-switch limit        : " << r.limit[TAB_SWITCH] << "\n"
             << " 3. Device-change limit     : " << r.limit[DEVICE_CHANGE] << "\n"
             << " 4. Network-drop limit      : " << r.limit[NETWORK_DROP] << "\n"
             << " 5. Copy-paste limit        : " << r.limit[COPY_PASTE] << "\n"
             << " 6. IP-change limit         : " << r.limit[IP_CHANGE] << "\n"
             << " 7. Known sequence pattern  : " << r.knownPattern << "  (codes: L F T D N C I S)\n"
             << " 8. HIGH / MEDIUM thresholds: " << r.highThreshold << " / " << r.mediumThreshold << "\n"
             << " 9. Reset to defaults\n 0. Back\n";
        int choice = readInt("Choice: ", 0, 9);
        if (choice == 0) return;
        int types[7] = {0, LOGIN_FAILED, TAB_SWITCH, DEVICE_CHANGE, NETWORK_DROP, COPY_PASTE, IP_CHANGE};
        if (choice >= 1 && choice <= 6) r.limit[types[choice]] = readInt("New limit (0-1000): ", 0, 1000);
        else if (choice == 7) {
            string pattern = readText("New pattern using codes L F T D N C I S: ");
            bool ok = !pattern.empty() && pattern.size() <= 20;
            for (char ch : pattern) if (string("LFTDNCIS").find(ch) == string::npos) ok = false;
            if (ok) r.knownPattern = pattern;
            else cout << "Invalid pattern - unchanged.\n";
        } else if (choice == 8) {
            r.highThreshold = readInt("HIGH threshold (1-100): ", 1, 100);
            r.mediumThreshold = readInt("MEDIUM threshold (1-" + to_string(r.highThreshold) + "): ", 1, r.highThreshold);
        } else if (choice == 9) r = RiskRules();
        app.analyzed = false;
    }
}

void menuWorkbench(AppState &app) {
    while (true) {
        printHeader("ALGORITHM WORKBENCH - dataset: " + (app.loaded ? app.data.scenarioName : string("(default)")));
        cout << " A. Sorting & searching        1 Merge sort  2 Quick sort  3 Binary search (iter+rec)\n"
             << " B. Divide & conquer           4 Max subarray  5 Fast power  6 Celebrity  7 Classical vs Strassen\n"
             << " C. Greedy                     8 Activity sel.  9 Interval partitioning  10 Fractional knapsack\n"
             << "                              11 Huffman  12 Dijkstra  13 Prim  14 Kruskal\n"
             << " D. Dynamic programming       15 0/1 knapsack  16 Matrix chain  17 LCS  18 Optimal BST\n"
             << "                              19 Bellman-Ford  20 Floyd-Warshall  21 TSP\n"
             << " E. Backtracking              22 N-Queens  23 Sum of subsets  24 Knapsack backtracking\n"
             << " F. Branch & bound            25 FIFO vs LIFO vs Least-cost\n"
             << " G. Flow & matching           26 Ford-Fulkerson  27 Bipartite matching\n"
             << " H. Number theory             28 Chinese Remainder Theorem\n"
             << " I. String matching           29 Naive / Rabin-Karp / Automaton / KMP\n"
             << "                              30 RUN ALL      0 Back\n";
        int choice = readInt("Choice: ", 0, 30);
        if (choice == 0) return;
        if (choice == 30) for (int c = 1; c <= 29; c++) runWorkbenchChoice(app, c);
        else runWorkbenchChoice(app, choice);
        pauseForEnter();
    }
}

void menuExport(AppState &app) {
    ensureAnalyzed(app);
    printHeader("EXPORT REPORTS");
    cout << (exportRiskCSV(app.report, "risk_report.csv") ? "Saved risk_report.csv\n" : "Failed risk_report.csv\n");
    cout << (exportRiskJSON(app.data, app.report, "risk_report.json") ? "Saved risk_report.json\n" : "Failed risk_report.json\n");
    cout << (exportEventsCSV(app.data, "current_events.csv") ? "Saved current_events.csv\n" : "Failed current_events.csv\n");
    cout << (exportCandidatesCSV(app.data, "current_candidates.csv") ? "Saved current_candidates.csv\n" : "Failed current_candidates.csv\n");
    cout << "Files are written to the folder you ran the program from.\n";
}

void runDemoScript(AppState &app) {
    printHeader("DEMONSTRATION SCRIPT - ONLINE EXAMINATION PROCTORING EVENT ANALYZER");
    cout << "Shows: normal operation, suspicious activity, a capacity shock, a large-data stress case,\n"
         << "and on each identical input at least two algorithmic strategies compared.\n";
    for (int scenario = 1; scenario <= 3; scenario++) {
        printHeader("DEMO STEP " + to_string(scenario) + ": " + generateScenario(scenario).scenarioName);
        loadScenario(app, scenario, true);
        ensureAnalyzed(app);
        printAnalysisSummary(app.data, app.report);
        cout << "\n";
        printRiskTable(app.report, scenario == 1 ? 5 : 8);
        if (!app.report.risks.empty() && app.report.risks[0].score > 0) printEvidenceTimeline(app.data, app.report.risks[0]);
        runReviewPlanning(app, true);
        if (scenario == 2) { demoCelebrity(app); demoBellmanFord(app); }
        if (scenario == 3) demoDijkstra(app);
    }
    printHeader("DEMO STEP 4: Scenario 4 - Large-data stress test");
    loadScenario(app, 4, true);
    ensureAnalyzed(app);
    printAnalysisSummary(app.data, app.report);
    printRiskTable(app.report, 3);
    demoQuickSort(app);
    vector<SortItem> base;
    for (int i = 0; i < (int)app.data.events.size(); i++) base.push_back({(long long)app.data.events[i].candidateId * 10000000LL + app.data.events[i].timestamp, i});
    for (int algo = 0; algo < 2; algo++) {
        vector<SortItem> copy = base;
        SortStats stats;
        Timer t;
        t.start();
        if (algo == 0) mergeSort(copy, stats); else quickSort(copy, PIVOT_MEDIAN_OF_THREE, stats);
        cout << (algo == 0 ? "Merge sort " : "Quick sort ") << "on all " << copy.size() << " events (grouping key): "
             << stats.comparisons << " comparisons, " << formatDouble(t.millis(), 2) << " ms\n";
    }
    string logText = buildLogText(app.data);
    cout << "\nString search on the full " << logText.size() << "-character log:\n";
    for (int a = 0; a < 4; a++) {
        MatchResult r = runMatcher(a, logText, "Clipboard paste detected");
        cout << "  " << padRight(r.algorithm, 17) << r.positions.size() << " matches, " << r.comparisons << " comparisons, " << formatDouble(r.millis, 2) << " ms\n";
    }
    printHeader("DEMO COMPLETE");
    cout << "Next: run '--bench' for scalability tables and '--test' for the automated test suite.\n";
}

void mainMenu(AppState &app) {
    while (true) {
        printHeader("ONLINE EXAMINATION PROCTORING EVENT ANALYZER  (DAA Project 26)");
        cout << " Dataset: " << (app.loaded ? app.data.scenarioName + " | " + to_string(app.data.candidates.size()) + " candidates | " +
                                              to_string(app.data.events.size()) + " events" : string("none loaded (Scenario 2 is used by default)")) << "\n";
        printLine();
        cout << "  1. Load / generate dataset          8. Review planning (schedule, allocate, match)\n"
             << "  2. View candidates & reviewers      9. Algorithm workbench (all syllabus algorithms)\n"
             << "  3. Record a new event              10. Benchmark runner\n"
             << "  4. Search events                   11. Run automated tests\n"
             << "  5. Detect suspicious patterns      12. Export reports (CSV / JSON)\n"
             << "  6. Evidence timeline               13. Complexity report & P/NP reduction\n"
             << "  7. Edit risk rules                 14. Run full demonstration script\n"
             << "  0. Exit\n";
        int choice = readInt("Choice: ", 0, 14);
        switch (choice) {
            case 0: cout << "Goodbye.\n"; return;
            case 1: menuLoadData(app); break;
            case 2: menuViewCandidates(app); break;
            case 3: menuRecordEvent(app); break;
            case 4: menuSearch(app); break;
            case 5: menuDetection(app); break;
            case 6: menuTimeline(app); break;
            case 7: menuRules(app); break;
            case 8: runReviewPlanning(app, false); break;
            case 9: menuWorkbench(app); break;
            case 10: runBenchmarkSuite(readInt("1 = quick (~10 s), 2 = full: ", 1, 2) == 1); break;
            case 11: runAllTests(); break;
            case 12: menuExport(app); break;
            case 13: printComplexityReport(); break;
            case 14: runDemoScript(app); break;
        }
        if (choice != 7 && choice != 9) pauseForEnter();
    }
}

int main(int argc, char *argv[]) {
    AppState app;
    if (argc > 1) {
        string mode = argv[1];
        if (mode == "--test") return runAllTests() == 0 ? 0 : 1;
        if (mode == "--demo") { runDemoScript(app); return 0; }
        if (mode == "--bench") { runBenchmarkSuite(false); return 0; }
        if (mode == "--bench-quick") { runBenchmarkSuite(true); return 0; }
        if (mode == "--samples") { writeSampleDatasets(); return 0; }
        if (mode == "--report") { printComplexityReport(); return 0; }
        if (mode == "--workbench") { for (int c = 1; c <= 29; c++) runWorkbenchChoice(app, c); return 0; }
        cout << "Unknown option '" << mode << "'. Options: --demo --test --bench --bench-quick --samples --report --workbench\n";
        return 1;
    }
    mainMenu(app);
    return 0;
}
