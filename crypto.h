/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of simple-crypt
 * Created on: 4 июн. 2022 г.
 *
 * simple-crypt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * simple-crypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with simple-crypt. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CRYPTO_H_
#define CRYPTO_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

typedef uint64_t    key_hash_t;
constexpr size_t KEY_HASH_BITS      = sizeof(key_hash_t) * 8;

key_hash_t hash_key(const std::string & key)
{
    key_hash_t hash = 0xfc23ed30be4613ad;
    for (const char c: key)
    {
        hash ^= c * 0xcee3 + 0xaea7;
        hash  = (hash << 8) | (hash >> (KEY_HASH_BITS - 8));
    }
    return hash;
}

static const uint16_t PRIME_MUL_TAB[16] =
{
    0x80ab, 0x815f, 0x8d41, 0x9161,
    0x9463, 0x9b77, 0xabc1, 0xb567,
    0xc317, 0xd2a3, 0xd50b, 0xe095,
    0xecf5, 0xf67f, 0xfc37, 0xfff1
};

static const uint16_t PRIME_ADD_TAB[16] =
{
    0x80d7, 0x85db, 0x90b9, 0x9cbb,
    0xa0fd, 0xa60d, 0xb201, 0xb9f9,
    0xc23f, 0xc95f, 0xd50d, 0xd7bd,
    0xe2ff, 0xea6d, 0xf463, 0xfd2b
};

class Crypto
{
    private:
        typedef struct lgc_t
        {
            uint32_t    value;
            uint16_t    mul;
            uint16_t    add;
        } lgc_t;

        static constexpr size_t NUM_GENERATORS     = 8;

    private:
        lgc_t lgc[NUM_GENERATORS];
        uint32_t current;
        uint32_t counter;
        uint32_t period;

    public:
        explicit Crypto(key_hash_t key)
        {
            // Compute seed from the key
            key_hash_t seed = (key << (KEY_HASH_BITS >> 1)) | (key >> (KEY_HASH_BITS >> 1));
            seed = seed * 0x8119 + 0xd7fb;

            // Initialize LGC
            for (size_t i=0; i<NUM_GENERATORS; ++i)
            {
                lgc_t & lgc = this->lgc[i];
                lgc.mul     = PRIME_MUL_TAB[size_t((key >> (i*8)) & 0x0f)];
                lgc.add     = PRIME_ADD_TAB[size_t((key >> (i*8 + 4)) & 0x0f)];
                lgc.value   = uint32_t((seed >> (i * 8)) & 0xff);
            }

            // Initialize counter and period
            current         = 0;
            counter         = 0;
            period          = ((seed * 0xa187 + 0xfccd) & 0xfff) + 0x1001;
        }

        uint8_t gen()
        {
            // Select LGC and switch to the new LGC, make a jump over one LGC at specific period
            lgc_t & lgc = this->lgc[current++];
            if ((++counter) >= period)
            {
                ++current;
                counter = 0;
            }
            current %= NUM_GENERATORS;

            // Generate the value and update LGC
            lgc.value   = lgc.value * lgc.mul + lgc.add;
            return int8_t(lgc.value & 0xff);
        }
};

#endif /* CRYPTO_H_ */
