#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <chrono>

using namespace std;

/* -------------CONSTANTS---------- */

// If N >= this value, do NOT enumerate all solutions
const int FIND_ALL_LIMIT = 21;

// Maximum solutions to print for large N
const long long MAX_SOLUTIONS_LARGE_N = 1000;

/* -------------GLOBAL STATE---------- */

int N;                       // Board size
uint64_t LIMIT;              // Bitmask with lowest N bits set
long long solutionCount = 0; // Number of solutions found
bool stopSearch = false;     // Stop recursion when limit reached

FILE *tempFile; // Temporary file for solutions

/* --------------------------------------------------
   FAST BUFFERED OUTPUT
-------------------------------------------------- */

char outBuffer[65536];
int bufferPos = 0;

void flushBuffer()
{
    if (bufferPos > 0)
    {
        fwrite(outBuffer, 1, bufferPos, tempFile);
        bufferPos = 0;
    }
}

inline void writeInt(int x)
{
    char temp[16];
    int len = 0;

    do
    {
        temp[len++] = char('0' + (x % 10));
        x /= 10;
    } while (x);

    if (bufferPos + len + 1 >= 65536)
        flushBuffer();

    while (len--)
        outBuffer[bufferPos++] = temp[len];
}

inline void writeSpace()
{
    if (bufferPos >= 65536)
        flushBuffer();
    outBuffer[bufferPos++] = ' ';
}

inline void writeNewLine()
{
    if (bufferPos >= 65536)
        flushBuffer();
    outBuffer[bufferPos++] = '\n';
}

/* --------------------------------------------------
   SOLUTION PRINTING
-------------------------------------------------- */

void printSolution(const vector<int> &path)
{
    for (int i = 0; i < N; i++)
    {
        writeInt(path[i]);
        if (i < N - 1)
            writeSpace();
    }
    writeNewLine();
}

void printMirrorSolution(const vector<int> &path)
{
    for (int i = 0; i < N; i++)
    {
        writeInt((N + 1) - path[i]);
        if (i < N - 1)
            writeSpace();
    }
    writeNewLine();
}

/* ---------- BACKTRACKING SOLVER------------------------- */

void solve(uint64_t cols, uint64_t diagLeft, uint64_t diagRight,
           vector<int> &path)
{
    if (stopSearch)
        return;

    // All columns filled → valid solution
    if (cols == LIMIT)
    {
        solutionCount++;
        printSolution(path);

        if (N >= FIND_ALL_LIMIT &&
            solutionCount >= MAX_SOLUTIONS_LARGE_N)
        {
            stopSearch = true;
        }
        return;
    }

    // Compute valid positions
    uint64_t possible =
        ~(cols | diagLeft | diagRight) & LIMIT;

    while (possible)
    {
        if (stopSearch)
            return;

        // Pick lowest available column
        uint64_t bit = possible & -possible;
        possible -= bit;

        int column = __builtin_ctzll(bit);

        path.push_back(column + 1);

        solve(cols | bit,
              (diagLeft | bit) << 1,
              (diagRight | bit) >> 1,
              path);

        path.pop_back(); // backtrack
    }
}

/* -------------  SYMMETRY OPTIMIZED SOLVER (SMALL N)--------------------------------- */

void solveWithSymmetry()
{
    // Large N → normal solver only
    if (N >= FIND_ALL_LIMIT)
    {
        vector<int> path;
        solve(0, 0, 0, path);
        return;
    }

    vector<int> path;
    int half = N / 2;

    // First half columns (mirror will cover the rest)
    for (int col = 0; col < half; col++)
    {
        uint64_t bit = 1ULL << col;
        path.push_back(col + 1);

        auto symmetricSolve =
            [&](auto &&self, uint64_t c, uint64_t l, uint64_t r) -> void
        {
            if (c == LIMIT)
            {
                solutionCount++;
                printSolution(path);

                solutionCount++;
                printMirrorSolution(path);
                return;
            }

            uint64_t p = ~(c | l | r) & LIMIT;
            while (p)
            {
                uint64_t b = p & -p;
                p -= b;

                path.push_back(__builtin_ctzll(b) + 1);
                self(self, c | b, (l | b) << 1, (r | b) >> 1);
                path.pop_back();
            }
        };

        symmetricSolve(symmetricSolve, bit, bit << 1, bit >> 1);
        path.pop_back();
    }

    // Middle column for odd N (no mirror)
    if (N % 2 == 1)
    {
        int mid = N / 2;
        uint64_t bit = 1ULL << mid;
        path.push_back(mid + 1);
        solve(bit, bit << 1, bit >> 1, path);
        path.pop_back();
    }
}

/* --------------------------------------------------
   MAIN FUNCTION
-------------------------------------------------- */

int main(int argc, char *argv[])
{
    auto start = chrono::high_resolution_clock::now();
    if (argc < 2)
    {
        cerr << "Usage: ./nqueens_solver <input_file>\n";
        return 1;
    }

    ifstream input(argv[1]);
    if (!input || !(input >> N))
    {
        cerr << "Invalid input file\n";
        return 1;
    }

    string outputFile =
        string(argv[1]).substr(0, string(argv[1]).find_last_of('.')) + "_output.txt";

    // Handle impossible cases
    if (N < 4 && N != 1)
    {
        ofstream out(outputFile);
        out << "No Solution";
        return 0;
    }

    LIMIT = (1ULL << N) - 1;

    tempFile = tmpfile();
    if (!tempFile)
    {
        cerr << "Failed to create temp file\n";
        return 1;
    }

    solveWithSymmetry();

    flushBuffer();

    ofstream out(outputFile);
    out << N << "\n";
    out << solutionCount << "\n";

    rewind(tempFile);
    char copyBuf[4096];
    size_t bytes;
    while ((bytes = fread(copyBuf, 1, sizeof(copyBuf), tempFile)) > 0)
        out.write(copyBuf, bytes);

    fclose(tempFile);
    auto end = chrono::high_resolution_clock::now();

    cout << "Done. N=" << N
         << ", Solutions=" << solutionCount
         << ", Time="
         << chrono::duration_cast<chrono::milliseconds>(end - start).count()
         << " ms\n";

    return 0;
}

// g++ -O3 -march=native .\nqueens_final.cpp -o nqueens_final
// ./nqueens_final input.txt