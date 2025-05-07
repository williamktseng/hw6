#ifndef RECCHECK
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <random>
#include <iomanip>
#include <fstream>
#include <exception>
#endif

#include "boggle.h"

// Generate an n x n board using Scrabble frequencies and seed
std::vector<std::vector<char>> genBoard(unsigned int n, int seed) {
    std::mt19937 rng(seed);
    int freq[26] = {9,2,2,4,12,2,3,2,9,1,1,4,2,6,8,2,1,6,4,6,4,2,2,1,2,1};
    std::vector<char> letters;
    for(char c = 'A'; c <= 'Z'; ++c) {
        for(int i = 0; i < freq[c - 'A']; ++i) {
            letters.push_back(c);
        }
    }
    std::vector<std::vector<char>> board(n, std::vector<char>(n));
    for(unsigned int i = 0; i < n; ++i) {
        for(unsigned int j = 0; j < n; ++j) {
            board[i][j] = letters[rng() % letters.size()];
        }
    }
    return board;
}

// Print the board nicely
void printBoard(const std::vector<std::vector<char>>& board) {
    unsigned int n = board.size();
    for(unsigned int i = 0; i < n; ++i) {
        for(unsigned int j = 0; j < n; ++j) {
            std::cout << std::setw(2) << board[i][j];
        }
        std::cout << std::endl;
    }
}

// Parse dictionary into full words set and prefix set
std::pair<std::set<std::string>, std::set<std::string>> parseDict(std::string fname) {
    std::ifstream dictfs(fname.c_str());
    if(dictfs.fail()) {
        throw std::invalid_argument("unable to open dictionary file");
    }
    std::set<std::string> dict;
    std::set<std::string> prefix;
    std::string word;
    while(dictfs >> word) {
        dict.insert(word);
        // insert all strict prefixes
        for(size_t i = 1; i < word.size(); ++i) {
            prefix.insert(word.substr(0, i));
        }
    }
    // include empty string
    prefix.insert("");
    return std::make_pair(dict, prefix);
}

// Top-level boggle search: initiate three directions from each cell
std::set<std::string> boggle(
    const std::set<std::string>& dict,
    const std::set<std::string>& prefix,
    const std::vector<std::vector<char>>& board)
{
    std::set<std::string> result;
    unsigned int n = board.size();
    for(unsigned int i = 0; i < n; ++i) {
        for(unsigned int j = 0; j < n; ++j) {
            boggleHelper(dict, prefix, board, "", result, i, j, 1, 0);
            boggleHelper(dict, prefix, board, "", result, i, j, 0, 1);
            boggleHelper(dict, prefix, board, "", result, i, j, 1, 1);
        }
    }
    return result;
}

// Recursive helper: builds word along (dr,dc), backtracks using prefix set
bool boggleHelper(
    const std::set<std::string>& dict,
    const std::set<std::string>& prefix,
    const std::vector<std::vector<char>>& board,
    std::string word,
    std::set<std::string>& result,
    unsigned int r,
    unsigned int c,
    int dr,
    int dc)
{
    unsigned int n = board.size();
    // 1) stop if out of bounds
    if (r >= n || c >= n) return false;

    // 2) extend the current word
    word.push_back(board[r][c]);

    bool inDict   = (dict.find(word) != dict.end());
    bool canExtend = (prefix.find(word) != prefix.end());

    // 3) if we cannot extend further
    if (!canExtend) {
        // if current word is valid, record it
        if (inDict) {
            result.insert(word);
            return true;
        }
        return false;
    }

    // 4) else, we can extend: recurse deeper
    bool deeper = boggleHelper(
        dict, prefix, board,
        word, result,
        r + dr, c + dc, dr, dc
    );
    if (deeper) {
        return true;
    }

    // 5) no deeper word found, but if this is valid, record it
    if (inDict) {
        result.insert(word);
        return true;
    }
    return false;
}