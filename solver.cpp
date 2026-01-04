#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <chrono>

using namespace std;

int boardSize;                  // N
uint64_t fullBoardMask;         // Bitmask representing all columns (lowest N bits set)
long long totalSolutionsFound = 0; // Total valid solutions

FILE *temporarySolutionFile;    // Temporary file storage

/* --------------------------------------------------
   BUFFERED OUTPUT SYSTEM
-------------------------------------------------- */
char outputBuffer[65536];
int currentBufferPosition = 0;

void flushBuffer() {
    if (currentBufferPosition > 0) {
        fwrite(outputBuffer, 1, currentBufferPosition, temporarySolutionFile);
        currentBufferPosition = 0;
    }
}

inline void writeInt(int number) {
    char digitBuffer[16];
    int digitCount = 0;
    do {
        digitBuffer[digitCount++] = char('0' + (number % 10));
        number /= 10;
    } while (number);

    if (currentBufferPosition + digitCount + 1 >= 65536)
        flushBuffer();

    while (digitCount--)
        outputBuffer[currentBufferPosition++] = digitBuffer[digitCount];
}

inline void writeSpace() {
    if (currentBufferPosition >= 65536)
        flushBuffer();
    outputBuffer[currentBufferPosition++] = ' ';
}

inline void writeNewLine() {
    if (currentBufferPosition >= 65536)
        flushBuffer();
    outputBuffer[currentBufferPosition++] = '\n';
}

/* --------------------------------------------------
   SOLUTION OUTPUT FUNCTIONS
-------------------------------------------------- */
void printSolution(const vector<int> &queenPositions) {
    for (int row = 0; row < boardSize; row++) {
        writeInt(queenPositions[row]);
        if (row < boardSize - 1) writeSpace();
    }
    writeNewLine();
}

/* ---------- RECURSIVE BACKTRACKING SOLVER ------------------------- */
void solve(uint64_t occupiedColumns, uint64_t occupiedLeftDiagonals, uint64_t occupiedRightDiagonals,
           vector<int> &queenPositions) {
    // Base case: all rows filled → solution found
    if (occupiedColumns == fullBoardMask) {
        totalSolutionsFound++;
        printSolution(queenPositions);
        return;
    }

    // Available positions (not attacked)
    uint64_t availablePositions = ~(occupiedColumns | occupiedLeftDiagonals | occupiedRightDiagonals) & fullBoardMask;

    while (availablePositions) {
        uint64_t selectedPosition = availablePositions & -availablePositions;
        availablePositions -= selectedPosition;
        int columnIndex = __builtin_ctzll(selectedPosition);

        queenPositions.push_back(columnIndex + 1);

        solve(occupiedColumns | selectedPosition,
              (occupiedLeftDiagonals | selectedPosition) << 1,
              (occupiedRightDiagonals | selectedPosition) >> 1,
              queenPositions);

        queenPositions.pop_back(); // backtrack
    }
}

/* ------------- SYMMETRY-OPTIMIZED SOLVER (SMALL BOARDS) ------------- */
void solveWithSymmetry() {
    if (boardSize <= 1 || boardSize >= 21) {
        // For N=1 or very large boards, just use standard solver
        vector<int> queenPositions;
        solve(0, 0, 0, queenPositions);
        return;
    }

    vector<int> queenPositions;
    int halfBoard = boardSize / 2;

    // Process first half columns (mirror handled later)
    for (int column = 0; column < halfBoard; column++) {
        uint64_t selected = 1ULL << column;
        queenPositions.push_back(column + 1);

        auto recursiveSolver = [&](auto &&self, uint64_t cols, uint64_t leftDiag, uint64_t rightDiag) -> void {
            if (cols == fullBoardMask) {
                totalSolutionsFound++;
                printSolution(queenPositions);

                // Mirror solution
                totalSolutionsFound++;
                vector<int> mirrored = queenPositions;
                for (auto &pos : mirrored)
                    pos = boardSize + 1 - pos;
                printSolution(mirrored);

                return;
            }

            uint64_t available = ~(cols | leftDiag | rightDiag) & fullBoardMask;
            while (available) {
                uint64_t pos = available & -available;
                available -= pos;
                queenPositions.push_back(__builtin_ctzll(pos) + 1);
                self(self, cols | pos, (leftDiag | pos) << 1, (rightDiag | pos) >> 1);
                queenPositions.pop_back();
            }
        };

        recursiveSolver(recursiveSolver, selected, selected << 1, selected >> 1);
        queenPositions.pop_back();
    }

    // Middle column for odd boards
    if (boardSize % 2 == 1) {
        int middle = boardSize / 2;
        uint64_t pos = 1ULL << middle;
        queenPositions.push_back(middle + 1);
        solve(pos, pos << 1, pos >> 1, queenPositions);
        queenPositions.pop_back();
    }
}

/* --------------------------------------------------
   MAIN FUNCTION
-------------------------------------------------- */
int main(int argc, char *argv[]) {
    auto startTime = chrono::high_resolution_clock::now();

    if (argc < 2) {
        cerr << "Usage: ./nqueens_solver <input_file>\n";
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile || !(inputFile >> boardSize)) {
        cerr << "Invalid input file\n";
        return 1;
    }

    string outputFilePath = string(argv[1]).substr(0, string(argv[1]).find_last_of('.')) + "_output.txt";

    // Handle unsolvable cases
    if ((boardSize < 4 && boardSize != 1)) {
        ofstream out(outputFilePath);
        out << "No Solution";
        return 0;
    }

    fullBoardMask = (1ULL << boardSize) - 1;

    temporarySolutionFile = tmpfile();
    if (!temporarySolutionFile) {
        cerr << "Failed to create temp file\n";
        return 1;
    }

    // Solve
    solveWithSymmetry();

    flushBuffer();

    // Write final output
    ofstream outputFile(outputFilePath);
    outputFile << boardSize << "\n";
    outputFile << totalSolutionsFound << "\n";

    rewind(temporarySolutionFile);
    char transferBuffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(transferBuffer, 1, sizeof(transferBuffer), temporarySolutionFile)) > 0)
        outputFile.write(transferBuffer, bytesRead);

    fclose(temporarySolutionFile);

    auto endTime = chrono::high_resolution_clock::now();
    cout << "Done. N=" << boardSize
         << ", Solutions=" << totalSolutionsFound
         << ", Time=" << chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count()
         << " ms\n";

    return 0;
}
