#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <chrono>

using namespace std;

/* ---------------- CONFIGURATION ---------------- */

// If N is large, do not enumerate all solutions
const int ENUMERATION_LIMIT = 21;


/* ---------------- GLOBAL STATE ---------------- */

int boardSize;                  // Size of the board (N)
uint64_t fullMask;              // Mask with N lowest bits set
long long totalSolutions = 0;   // Count of solutions found
bool terminateSearch = false;   // Stops recursion when limit reached

FILE *solutionTempFile;         // Temporary file for solution storage

/* ---------------- BUFFERED OUTPUT ---------------- */

char outputBuffer[65536];
int bufferIndex = 0;

void flushOutput()
{
    if (bufferIndex > 0)
    {
        fwrite(outputBuffer, 1, bufferIndex, solutionTempFile);
        bufferIndex = 0;
    }
}

inline void writeNumber(int value)
{
    char temp[16];
    int length = 0;

    do
    {
        temp[length++] = char('0' + (value % 10));
        value /= 10;
    } while (value);

    if (bufferIndex + length + 1 >= 65536)
        flushOutput();

    while (length--)
        outputBuffer[bufferIndex++] = temp[length];
}

inline void writeSpace()
{
    if (bufferIndex >= 65536)
        flushOutput();
    outputBuffer[bufferIndex++] = ' ';
}

inline void writeLineBreak()
{
    if (bufferIndex >= 65536)
        flushOutput();
    outputBuffer[bufferIndex++] = '\n';
}

/* ---------------- SOLUTION OUTPUT ---------------- */

void writeSolution(const vector<int> &placement)
{
    for (int i = 0; i < boardSize; i++)
    {
        writeNumber(placement[i]);
        if (i < boardSize - 1)
            writeSpace();
    }
    writeLineBreak();
}

void writeMirroredSolution(const vector<int> &placement)
{
    for (int i = 0; i < boardSize; i++)
    {
        writeNumber((boardSize + 1) - placement[i]);
        if (i < boardSize - 1)
            writeSpace();
    }
    writeLineBreak();
}

/* ---------------- BACKTRACKING SOLVER ---------------- */

void backtrack(uint64_t columns, uint64_t diagLeft, uint64_t diagRight,
               vector<int> &placement)
{
    if (terminateSearch)
        return;

    // All columns occupied -> valid solution
    if (columns == fullMask)
    {
        totalSolutions++;
        writeSolution(placement);

        
        return;
    }

    // Calculate available positions
    uint64_t available =
        ~(columns | diagLeft | diagRight) & fullMask;

    while (available)
    {
        if (terminateSearch)
            return;

        // Select the lowest available column
        uint64_t bit = available & -available;
        available -= bit;

        int colIndex = __builtin_ctzll(bit);

        placement.push_back(colIndex + 1);

        backtrack(columns | bit,
                  (diagLeft | bit) << 1,
                  (diagRight | bit) >> 1,
                  placement);

        placement.pop_back(); // undo placement
    }
}

/* ---------------- SYMMETRY OPTIMIZED SOLVER ---------------- */

void solveWithSymmetry()
{
    // For large N, skip symmetry optimization
    if (boardSize >= ENUMERATION_LIMIT)
    {
        vector<int> placement;
        backtrack(0, 0, 0, placement);
        return;
    }

    vector<int> placement;
    int halfColumns = boardSize / 2;

    // Explore only half the first row (mirrors cover the rest)
    for (int col = 0; col < halfColumns; col++)
    {
        uint64_t bit = 1ULL << col;
        placement.push_back(col + 1);

        auto symmetricDFS =
            [&](auto &&self, uint64_t c, uint64_t l, uint64_t r) -> void
        {
            if (c == fullMask)
            {
                totalSolutions++;
                writeSolution(placement);

                totalSolutions++;
                writeMirroredSolution(placement);
                return;
            }

            uint64_t possible = ~(c | l | r) & fullMask;
            while (possible)
            {
                uint64_t b = possible & -possible;
                possible -= b;

                placement.push_back(__builtin_ctzll(b) + 1);
                self(self, c | b, (l | b) << 1, (r | b) >> 1);
                placement.pop_back();
            }
        };

        symmetricDFS(symmetricDFS, bit, bit << 1, bit >> 1);
        placement.pop_back();
    }

    // Handle middle column separately for odd N
    if (boardSize % 2 == 1)
    {
        int mid = boardSize / 2;
        uint64_t bit = 1ULL << mid;
        placement.push_back(mid + 1);
        backtrack(bit, bit << 1, bit >> 1, placement);
        placement.pop_back();
    }
}

/* ---------------- MAIN ---------------- */

int main(int argc, char *argv[])
{
    auto startTime = chrono::high_resolution_clock::now();

    if (argc < 2)
    {
        cerr << "Usage: ./nqueens_solver <input_file>\n";
        return 1;
    }

    ifstream input(argv[1]);
    if (!input || !(input >> boardSize))
    {
        cerr << "Invalid input file\n";
        return 1;
    }

    string outputFile =
        string(argv[1]).substr(0, string(argv[1]).find_last_of('.')) + "_output.txt";

    // No solution cases
    if (boardSize == 2 || boardSize == 3)
    {
        ofstream out(outputFile);
        out << "No Solution";
        cout << "No Solution";
        return 0;
    }

    fullMask = (1ULL << boardSize) - 1;

    solutionTempFile = tmpfile();
    if (!solutionTempFile)
    {
        cerr << "Failed to create temp file\n";
        return 1;
    }

    solveWithSymmetry();
    flushOutput();

    ofstream out(outputFile);
    out << boardSize << "\n";
    out << totalSolutions << "\n";

    rewind(solutionTempFile);
    char copyBuffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(copyBuffer, 1, sizeof(copyBuffer), solutionTempFile)) > 0)
        out.write(copyBuffer, bytesRead);

    fclose(solutionTempFile);

    auto endTime = chrono::high_resolution_clock::now();

    cout << "N = " << boardSize << "\n";
    cout << "Solutions = " << totalSolutions << "\n";
    cout << "Time = "
         << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count()
         << " ms\n";

    return 0;
}