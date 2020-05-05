// Copyright (c) 2015-2020 Clearmatics Technologies Ltd
//
// SPDX-License-Identifier: LGPL-3.0+

#ifndef __ZETH_FIELD_ELEMENT_UTILS_TCC__
#define __ZETH_FIELD_ELEMENT_UTILS_TCC__

#include "libzeth/core/field_element_utils.hpp"
#include "libzeth/core/utils.hpp"

#include <iomanip>

/// This file uses types and preprocessor variables defined in the `gmp.h`
/// header:
///  - `mp_size_t`
///  - `GMP_LIMB_BITS`
///  - `GMP_NAIL_BITS`

namespace libzeth
{

template<typename FieldT>
std::string bigint_to_hex(const libff::bigint<FieldT::num_limbs> &limbs)
{
    const unsigned bytes_per_limb = (GMP_LIMB_BITS + 8 - 1) / 8;

    uint8_t x[bytes_per_limb * FieldT::num_limbs];
    for (unsigned i = 0; i < FieldT::num_limbs; i++) {
        for (unsigned j = 0; j < bytes_per_limb; j++) {
            x[i * 8 + j] = uint8_t(
                uint64_t(limbs.data[(FieldT::num_limbs - 1) - i]) >>
                (GMP_LIMB_BITS - 8 * (j + 1)));
        }
    }

    std::stringstream ss;

    // Display every byte as 2 hexadecimal characters
    ss << std::setfill('0');
    for (unsigned i = 0; i < bytes_per_limb * FieldT::num_limbs; i++) {
        ss << std::hex << std::setw(2) << (int)x[i];
    }
    std::string str = ss.str();

    return str;
}

template<typename FieldT>
libff::bigint<FieldT::num_limbs> bigint_from_bytes(
    const uint8_t bytes[((GMP_LIMB_BITS + 8 - 1) / 8) * FieldT::num_limbs])
{
    const unsigned bytes_per_limb = (GMP_LIMB_BITS + 8 - 1) / 8;

    libff::bigint<FieldT::num_limbs> res;

    for (unsigned i = 0; i < FieldT::num_limbs; i++) {
        for (unsigned j = 0; j < bytes_per_limb; j++) {
            res.data[FieldT::num_limbs - i - 1] |=
                mp_limb_t(bytes[i * 8 + j]) << (GMP_LIMB_BITS - 8 * (j + 1));
        }
    }
    return res;
}

template<typename FieldT> FieldT field_element_from_hex(std::string field_str)
{
    // Remove prefix if any
    erase_substring(field_str, std::string("0x"));

    // 1 byte will be populated by 2 hexadecimal characters
    uint8_t val[field_str.size() / 2];

    char cstr[field_str.size() + 1];
    strcpy(cstr, field_str.c_str());

    int res = hex_to_bytes(cstr, val);
    if (res == 0) {
        throw std::invalid_argument("Invalid hexadecimal string");
    }

    libff::bigint<FieldT::num_limbs> el = bigint_from_bytes<FieldT>(val);
    return FieldT(el);
}

template<typename FieldT> std::string field_element_to_hex(FieldT field_el)
{
    return bigint_to_hex(field_el.as_bigint());
}

} // namespace libzeth

#endif // __ZETH_FIELD_ELEMENT_UTILS_TCC__
