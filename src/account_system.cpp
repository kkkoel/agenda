#include "account_system.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

using namespace std;

const string USER_FILE = "data/users.txt";

static const unsigned int K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static inline unsigned int rotr(unsigned int x, int n) { return (x >> n) | (x << (32 - n)); }
static inline unsigned int ch(unsigned int x, unsigned int y, unsigned int z) { return (x & y) ^ (~x & z); }
static inline unsigned int maj(unsigned int x, unsigned int y, unsigned int z) { return (x & y) ^ (x & z) ^ (y & z); }
static inline unsigned int sigma0(unsigned int x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
static inline unsigned int sigma1(unsigned int x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
static inline unsigned int gamma0(unsigned int x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
static inline unsigned int gamma1(unsigned int x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

class SHA256 {
public:
    SHA256() {
        m_h[0] = 0x6a09e667; m_h[1] = 0xbb67ae85; m_h[2] = 0x3c6ef372; m_h[3] = 0xa54ff53a;
        m_h[4] = 0x510e527f; m_h[5] = 0x9b05688c; m_h[6] = 0x1f83d9ab; m_h[7] = 0x5be0cd19;
        m_blockSize = 0; m_totalLen = 0; m_finalized = false;
    }

    void update(const string& str) {
        for (unsigned char c : str) {
            m_block[m_blockSize++] = c;
            if (m_blockSize == 64) { transform(m_block); m_totalLen += 512; m_blockSize = 0; }
        }
    }

    string final() {
        if (!m_finalized) {
            m_totalLen += m_blockSize * 8;
            unsigned char pad[64];
            pad[0] = 0x80;
            for (int i = 1; i < 64; i++) pad[i] = 0;
            size_t padLen = (m_blockSize < 56) ? (56 - m_blockSize) : (120 - m_blockSize);
            for (size_t i = 0; i < padLen; i++) {
                m_block[m_blockSize++] = pad[i];
                if (m_blockSize == 64) { transform(m_block); m_totalLen += 512; m_blockSize = 0; }
            }
            unsigned char lenBuf[8];
            for (int i = 0; i < 8; i++) lenBuf[7 - i] = (m_totalLen >> (i * 8)) & 0xFF;
            for (int i = 0; i < 8; i++) {
                m_block[m_blockSize++] = lenBuf[i];
                if (m_blockSize == 64) { transform(m_block); m_totalLen += 512; m_blockSize = 0; }
            }
            m_finalized = true;
        }
        stringstream ss;
        for (int i = 0; i < 8; i++) ss << hex << setw(8) << setfill('0') << m_h[i];
        return ss.str();
    }

private:
    unsigned int m_h[8], m_blockSize;
    unsigned char m_block[64];
    unsigned long long m_totalLen;
    bool m_finalized;

    void transform(const unsigned char* block) {
        unsigned int w[64];
        for (int i = 0; i < 16; i++) w[i] = (block[i*4]<<24) | (block[i*4+1]<<16) | (block[i*4+2]<<8) | block[i*4+3];
        for (int i = 16; i < 64; i++) w[i] = gamma1(w[i-2]) + w[i-7] + gamma0(w[i-15]) + w[i-16];
        unsigned int a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3];
        unsigned int e = m_h[4], f = m_h[5], g = m_h[6], h = m_h[7];
        for (int i = 0; i < 64; i++) {
            unsigned int t1 = h + sigma1(e) + ch(e,f,g) + K[i] + w[i];
            unsigned int t2 = sigma0(a) + maj(a,b,c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        m_h[0] += a; m_h[1] += b; m_h[2] += c; m_h[3] += d;
        m_h[4] += e; m_h[5] += f; m_h[6] += g; m_h[7] += h;
    }
};

string sha256(const string& input) { SHA256 sha; sha.update(input); return sha.final(); }

static bool findUser(const string& username, string& outHash) {
    ifstream fin(USER_FILE);
    if (!fin.is_open()) return false;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        size_t pos = line.find(':');
        if (pos == string::npos) continue;
        string name = line.substr(0, pos);
        string hash = line.substr(pos + 1);
        if (name == username) { outHash = hash; return true; }
    }
    return false;
}

bool registerUser(const string& username, const string& password) {
    if (username.empty() || password.empty()) {
        cerr << "[ERROR] Username or password cannot be empty\n";
        return false;
    }
    if (username.find(':') != string::npos) {
        cerr << "[ERROR] Username cannot contain colon ':'\n";
        return false;
    }
    string existingHash;
    if (findUser(username, existingHash)) {
        cerr << "[ERROR] Username already exists\n";
        return false;
    }
    string hash = sha256(password);
    ofstream fout(USER_FILE, ios::app);
    if (!fout.is_open()) {
        cerr << "[ERROR] Cannot open users.txt\n";
        return false;
    }
    fout << username << ":" << hash << "\n";
    return true;
}

bool loginUser(const string& username, const string& password) {
    if (username.empty() || password.empty()) {
        cerr << "[ERROR] Username or password cannot be empty\n";
        return false;
    }
    string storedHash;
    if (!findUser(username, storedHash)) {
        cerr << "[LOGIN FAILED] User does not exist\n";
        return false;
    }
    string inputHash = sha256(password);
    if (inputHash == storedHash) return true;
    cerr << "[LOGIN FAILED] Wrong password\n";
    return false;
}