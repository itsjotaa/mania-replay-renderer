#pragma once
#include <string>
#include <cstring>
#include <cstdint>

// single-header MD5 implementation
// based on RFC 1321

class MD5 {
public:
    // compute MD5 hash of a buffer and return hex string
    static std::string hash(const uint8_t* data, size_t length) {
        MD5 md5;
        md5.update(data, length);
        return md5.finalize();
    }

    // compute MD5 hash of a string
    static std::string hash(const std::string& str) {
        return hash(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }

private:
    uint32_t state[4];
    uint32_t count[2];
    uint8_t  buffer[64];

    static const uint32_t S[64];
    static const uint32_t K[64];

    void init() {
        count[0] = count[1] = 0;
        state[0] = 0x67452301;
        state[1] = 0xefcdab89;
        state[2] = 0x98badcfe;
        state[3] = 0x10325476;
    }

    static uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    static uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    static uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    static uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }
    static uint32_t rotateLeft(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

    void transform(const uint8_t block[64]) {
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
        for (int i = 0, j = 0; i < 16; i++, j += 4)
            x[i] = ((uint32_t)block[j]) | (((uint32_t)block[j+1]) << 8) |
                   (((uint32_t)block[j+2]) << 16) | (((uint32_t)block[j+3]) << 24);

        // round 1
        for (int i = 0; i < 16; i++) {
            uint32_t f = F(b, c, d);
            uint32_t temp = d; d = c; c = b;
            b = b + rotateLeft(a + f + x[i] + K[i], S[i]);
            a = temp;
        }
        // round 2
        for (int i = 0; i < 16; i++) {
            uint32_t f = G(b, c, d);
            uint32_t temp = d; d = c; c = b;
            b = b + rotateLeft(a + f + x[(5*i+1)%16] + K[16+i], S[16+i]);
            a = temp;
        }
        // round 3
        for (int i = 0; i < 16; i++) {
            uint32_t f = H(b, c, d);
            uint32_t temp = d; d = c; c = b;
            b = b + rotateLeft(a + f + x[(3*i+5)%16] + K[32+i], S[32+i]);
            a = temp;
        }
        // round 4
        for (int i = 0; i < 16; i++) {
            uint32_t f = I(b, c, d);
            uint32_t temp = d; d = c; c = b;
            b = b + rotateLeft(a + f + x[(7*i)%16] + K[48+i], S[48+i]);
            a = temp;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    }

    void update(const uint8_t* input, size_t length) {
        init();
        uint32_t index = count[0] / 8 % 64;
        if ((count[0] += (uint32_t)(length << 3)) < (uint32_t)(length << 3)) count[1]++;
        count[1] += (uint32_t)(length >> 29);
        uint32_t firstpart = 64 - index;
        size_t i = 0;
        if (length >= firstpart) {
            memcpy(&buffer[index], input, firstpart);
            transform(buffer);
            for (i = firstpart; i + 64 <= length; i += 64)
                transform(&input[i]);
            index = 0;
        }
        memcpy(&buffer[index], &input[i], length - i);
    }

    std::string finalize() {
        static uint8_t padding[64] = { 0x80 };
        uint8_t bits[8];
        for (int i = 0; i < 8; i++)
            bits[i] = (count[i/4] >> ((i%4)*8)) & 0xff;
        uint32_t index = count[0] / 8 % 64;
        uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
        update(padding, padLen);
        update(bits, 8);

        uint8_t digest[16];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                digest[i*4+j] = (state[i] >> (j*8)) & 0xff;

        char hex[33];
        for (int i = 0; i < 16; i++)
            snprintf(&hex[i*2], 3, "%02x", digest[i]);
        return std::string(hex, 32);
    }
};

const uint32_t MD5::S[64] = {
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5, 9,14,20,5, 9,14,20,5, 9,14,20,5, 9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
};

const uint32_t MD5::K[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};
