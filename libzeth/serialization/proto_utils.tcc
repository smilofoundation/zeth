// Copyright (c) 2015-2020 Clearmatics Technologies Ltd
//
// SPDX-License-Identifier: LGPL-3.0+

#ifndef __ZETH_SERIALIZATION_PROTO_UTILS_TCC__
#define __ZETH_SERIALIZATION_PROTO_UTILS_TCC__

#include "libzeth/serialization/proto_utils.hpp"

#include <cassert>

namespace libzeth
{

template<typename ppT>
zeth_proto::HexPointBaseGroup1Affine point_g1_affine_to_proto(
    const libff::G1<ppT> &point)
{
    assert(!point.is_zero());
    libff::G1<ppT> aff = point;
    aff.to_affine_coordinates();

    zeth_proto::HexPointBaseGroup1Affine res;
    res.set_x_coord(field_element_to_json(aff.X));
    res.set_y_coord(field_element_to_json(aff.Y));
    return res;
}

template<typename ppT>
libff::G1<ppT> point_g1_affine_from_proto(
    const zeth_proto::HexPointBaseGroup1Affine &point)
{
    using Fq = libff::Fq<ppT>;
    Fq x_coordinate = field_element_from_json<Fq>(point.x_coord());
    Fq y_coordinate = field_element_from_json<Fq>(point.y_coord());
    return libff::G1<ppT>(x_coordinate, y_coordinate, Fq::one());
}

template<typename ppT>
zeth_proto::HexPointBaseGroup2Affine point_g2_affine_to_proto(
    const libff::G2<ppT> &point)
{
    assert(!point.is_zero());
    libff::G2<ppT> aff = point;
    aff.to_affine_coordinates();

    zeth_proto::HexPointBaseGroup2Affine res;
    res.set_x_coord(field_element_to_json(aff.X));
    res.set_y_coord(field_element_to_json(aff.Y));
    return res;
}

template<typename ppT>
libff::G2<ppT> point_g2_affine_from_proto(
    const zeth_proto::HexPointBaseGroup2Affine &point)
{
    using TwistField = typename libff::G2<ppT>::twist_field;
    const TwistField X = field_element_from_json<TwistField>(point.x_coord());
    const TwistField Y = field_element_from_json<TwistField>(point.y_coord());
    return libff::G2<ppT>(X, Y, TwistField::one());
}

template<typename FieldT, size_t TreeDepth>
joinsplit_input<FieldT, TreeDepth> joinsplit_input_from_proto(
    const zeth_proto::JoinsplitInput &input)
{
    if (TreeDepth != input.merkle_path_size()) {
        throw std::invalid_argument("Invalid merkle path length");
    }

    std::vector<FieldT> input_merkle_path;
    for (size_t i = 0; i < TreeDepth; i++) {
        FieldT mk_node =
            base_field_element_from_hex<FieldT>(input.merkle_path(i));
        input_merkle_path.push_back(mk_node);
    }

    return joinsplit_input<FieldT, TreeDepth>(
        std::move(input_merkle_path),
        bits_addr<TreeDepth>::from_size_t(input.address()),
        zeth_note_from_proto(input.note()),
        bits256::from_hex(input.spending_ask()),
        bits256::from_hex(input.nullifier()));
}

template<typename ppT>
std::string primary_inputs_to_string(
    const std::vector<libff::Fr<ppT>> &public_inputs)
{
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < public_inputs.size(); ++i) {
        ss << "\"0x"
           << libzeth::bigint_to_hex<libff::Fr<ppT>>(
                  public_inputs[i].as_bigint())
           << "\"";
        if (i < public_inputs.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    std::string inputs_json_str = ss.str();

    return inputs_json_str;
}

template<typename ppT>
std::vector<libff::Fr<ppT>> primary_inputs_from_string(
    const std::string &input_str)
{
    std::vector<libff::Fr<ppT>> res;
    size_t next_hex_pos = input_str.find("0x");
    while (next_hex_pos != std::string::npos) {
        // TODO: avoid the string copy here
        const size_t end_hex = input_str.find("\"", next_hex_pos);
        const std::string next_hex =
            input_str.substr(next_hex_pos, end_hex - next_hex_pos);
        res.push_back(base_field_element_from_hex<libff::Fr<ppT>>(next_hex));
        next_hex_pos = input_str.find("0x", end_hex);
    }
    return res;
}

template<typename ppT>
std::string accumulation_vector_to_string(
    const libsnark::accumulation_vector<libff::G1<ppT>> &acc_vector)
{
    std::stringstream ss;
    unsigned vect_length = acc_vector.rest.indices.size() + 1;
    ss << "[" << point_affine_to_json(acc_vector.first);
    for (size_t i = 0; i < vect_length - 1; ++i) {
        ss << ", " << point_affine_to_json(acc_vector.rest.values[i]);
    }
    ss << "]";
    std::string vect_json_str = ss.str();

    return vect_json_str;
}

template<typename ppT>
libsnark::accumulation_vector<libff::G1<ppT>> accumulation_vector_from_string(
    const std::string &acc_vector_str)
{
    static const char prefix[] = "[\"";
    static const char suffix[] = "\"]";

    if (acc_vector_str.length() < (sizeof(prefix) + sizeof(suffix))) {
        throw std::invalid_argument("invalid accumulation vector string");
    }

    size_t start_idx = acc_vector_str.find(prefix);
    if (start_idx == std::string::npos) {
        throw std::invalid_argument("invalid accumulation vector string");
    }

    // TODO: Remove the temporary string.

    // Allocate once and reuse.
    std::string element_str;

    // Extract first element
    size_t end_idx = acc_vector_str.find(suffix, start_idx);
    if (end_idx == std::string::npos) {
        throw std::invalid_argument("invalid accumulation vector string");
    }

    // Extract the string '["....", "...."]'
    //                     ^             ^
    //                start_idx       end_idx

    element_str = acc_vector_str.substr(start_idx, end_idx + 2 - start_idx);
    libff::G1<ppT> front = point_affine_from_json<libff::G1<ppT>>(element_str);
    start_idx = acc_vector_str.find(prefix, end_idx);

    // Extract remaining elements
    std::vector<libff::G1<ppT>> rest;
    do {
        end_idx = acc_vector_str.find(suffix, start_idx);
        if (end_idx == std::string::npos) {
            throw std::invalid_argument("invalid accumulation vector string");
        }

        element_str = acc_vector_str.substr(start_idx, end_idx + 2 - start_idx);
        rest.push_back(point_affine_from_json<libff::G1<ppT>>(element_str));
        start_idx = acc_vector_str.find(prefix, end_idx);
    } while (start_idx != std::string::npos);

    return libsnark::accumulation_vector<libff::G1<ppT>>(
        std::move(front), std::move(rest));
}

} // namespace libzeth

#endif // __ZETH_SERIALIZATION_PROTO_UTILS_TCC__