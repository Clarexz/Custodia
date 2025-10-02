#pragma once

#include <cstdint>
#include <vector>
#include <utility>

namespace interop {
namespace meshtastic {

// Protobuf wire types
enum class WireType : uint8_t {
    VARINT = 0,
    BIT64 = 1,
    LENGTH_DELIMITED = 2,
    // START_GROUP = 3, // deprecated
    // END_GROUP = 4,   // deprecated
    BIT32 = 5,
};

// Minimal cursor for decoding
struct Cursor {
    const uint8_t* p;
    const uint8_t* end;
    Cursor(const uint8_t* b, const uint8_t* e) : p(b), end(e) {}
    bool eof() const { return p >= end; }
    size_t remaining() const { return static_cast<size_t>(end - p); }
};

// Encoding primitives
void encodeKey(uint32_t fieldNumber, WireType wt, std::vector<uint8_t>& out);
void encodeVarint(uint64_t v, std::vector<uint8_t>& out);
void encodeVarintField(uint32_t fieldNumber, uint64_t v, std::vector<uint8_t>& out);
void encodeFixed32Field(uint32_t fieldNumber, uint32_t v, std::vector<uint8_t>& out);
void encodeBytesField(uint32_t fieldNumber, const std::vector<uint8_t>& bytes, std::vector<uint8_t>& out);
void encodeBytesField(uint32_t fieldNumber, const uint8_t* data, size_t len, std::vector<uint8_t>& out);

// Decoding primitives
bool readVarint(Cursor& c, uint64_t& out);
bool readKey(Cursor& c, uint32_t& fieldNumber, WireType& wt);
bool readBytes(Cursor& c, std::vector<uint8_t>& out);
bool readFixed32(Cursor& c, uint32_t& out);
bool skipField(Cursor& c, WireType wt);

// Minimal subset of meshtastic.Data we care about
struct DataMsg {
    // Portnum is an enum in proto, here we store as uint32
    uint32_t portnum = 0;
    std::vector<uint8_t> payload; // bytes

    // Optional routing-related fields (fixed32)
    bool has_dest = false;
    uint32_t dest = 0;
    bool has_source = false;
    uint32_t source = 0;

    // Optional request/reply ids
    bool has_request_id = false;
    uint32_t request_id = 0;
    bool has_reply_id = false;
    uint32_t reply_id = 0;
};

void encodeData(const DataMsg& d, std::vector<uint8_t>& out);
bool decodeData(const uint8_t* buf, size_t len, DataMsg& out);

// Minimal subset of meshtastic.MeshPacket for decoded payloads
struct MeshPacketMsg {
    bool has_from = false;
    uint32_t from = 0; // fixed32 field 1
    bool has_to = false;
    uint32_t to = 0;   // fixed32 field 2

    bool has_channel = false;
    uint32_t channel = 0; // uint32 field 3

    // oneof payload_variant: either decoded (Data) or encrypted (bytes)
    bool has_decoded = false;
    DataMsg decoded;

    bool has_encrypted = false;
    std::vector<uint8_t> encrypted;

    bool has_id = false;
    uint32_t id = 0; // fixed32 field 6

    bool has_hop_limit = false;
    uint32_t hop_limit = 0; // uint32 field 9

    bool has_want_ack = false;
    bool want_ack = false; // bool field 10
};

void encodeMeshPacket(const MeshPacketMsg& m, std::vector<uint8_t>& out);
bool decodeMeshPacket(const uint8_t* buf, size_t len, MeshPacketMsg& out);

} // namespace meshtastic
} // namespace interop

