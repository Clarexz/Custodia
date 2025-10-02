#include "proto_min.h"

namespace interop {
namespace meshtastic {

// ----- Encoding primitives -----

void encodeVarint(uint64_t v, std::vector<uint8_t>& out)
{
    while (v >= 0x80) {
        out.push_back(static_cast<uint8_t>(v) | 0x80);
        v >>= 7;
    }
    out.push_back(static_cast<uint8_t>(v));
}

void encodeKey(uint32_t fieldNumber, WireType wt, std::vector<uint8_t>& out)
{
    uint32_t key = (fieldNumber << 3) | static_cast<uint32_t>(wt);
    encodeVarint(key, out);
}

void encodeVarintField(uint32_t fieldNumber, uint64_t v, std::vector<uint8_t>& out)
{
    encodeKey(fieldNumber, WireType::VARINT, out);
    encodeVarint(v, out);
}

void encodeFixed32Field(uint32_t fieldNumber, uint32_t v, std::vector<uint8_t>& out)
{
    encodeKey(fieldNumber, WireType::BIT32, out);
    // little endian 32-bit
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

void encodeBytesField(uint32_t fieldNumber, const std::vector<uint8_t>& bytes, std::vector<uint8_t>& out)
{
    encodeBytesField(fieldNumber, bytes.data(), bytes.size(), out);
}

void encodeBytesField(uint32_t fieldNumber, const uint8_t* data, size_t len, std::vector<uint8_t>& out)
{
    encodeKey(fieldNumber, WireType::LENGTH_DELIMITED, out);
    encodeVarint(len, out);
    out.insert(out.end(), data, data + len);
}

// ----- Decoding primitives -----

bool readVarint(Cursor& c, uint64_t& out)
{
    out = 0;
    int shift = 0;
    while (!c.eof()) {
        uint8_t b = *c.p++;
        out |= static_cast<uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0)
            return true;
        shift += 7;
        if (shift > 63)
            return false; // overflow
    }
    return false; // truncated
}

bool readKey(Cursor& c, uint32_t& fieldNumber, WireType& wt)
{
    uint64_t key = 0;
    if (!readVarint(c, key))
        return false;
    fieldNumber = static_cast<uint32_t>(key >> 3);
    wt = static_cast<WireType>(key & 0x07);
    return true;
}

bool readBytes(Cursor& c, std::vector<uint8_t>& out)
{
    uint64_t len = 0;
    if (!readVarint(c, len))
        return false;
    if (len > c.remaining())
        return false;
    out.assign(c.p, c.p + static_cast<size_t>(len));
    c.p += static_cast<size_t>(len);
    return true;
}

bool readFixed32(Cursor& c, uint32_t& out)
{
    if (c.remaining() < 4)
        return false;
    uint32_t v = 0;
    v |= static_cast<uint32_t>(c.p[0]);
    v |= static_cast<uint32_t>(c.p[1]) << 8;
    v |= static_cast<uint32_t>(c.p[2]) << 16;
    v |= static_cast<uint32_t>(c.p[3]) << 24;
    c.p += 4;
    out = v;
    return true;
}

bool skipField(Cursor& c, WireType wt)
{
    switch (wt) {
    case WireType::VARINT: {
        uint64_t tmp;
        return readVarint(c, tmp);
    }
    case WireType::BIT64: {
        if (c.remaining() < 8)
            return false;
        c.p += 8;
        return true;
    }
    case WireType::LENGTH_DELIMITED: {
        uint64_t len = 0;
        if (!readVarint(c, len))
            return false;
        if (len > c.remaining())
            return false;
        c.p += static_cast<size_t>(len);
        return true;
    }
    case WireType::BIT32: {
        if (c.remaining() < 4)
            return false;
        c.p += 4;
        return true;
    }
    default:
        return false;
    }
}

// ----- Data -----

void encodeData(const DataMsg& d, std::vector<uint8_t>& out)
{
    // field 1: PortNum (varint)
    encodeVarintField(1, d.portnum, out);
    // field 2: payload (bytes)
    if (!d.payload.empty())
        encodeBytesField(2, d.payload, out);

    if (d.has_dest)
        encodeFixed32Field(4, d.dest, out);
    if (d.has_source)
        encodeFixed32Field(5, d.source, out);
    if (d.has_request_id)
        encodeFixed32Field(6, d.request_id, out);
    if (d.has_reply_id)
        encodeFixed32Field(7, d.reply_id, out);
}

bool decodeData(const uint8_t* buf, size_t len, DataMsg& out)
{
    Cursor c(buf, buf + len);
    while (!c.eof()) {
        uint32_t fn = 0;
        WireType wt = WireType::VARINT;
        if (!readKey(c, fn, wt))
            return false;
        switch (fn) {
        case 1: { // portnum varint
            uint64_t v = 0;
            if (!readVarint(c, v))
                return false;
            out.portnum = static_cast<uint32_t>(v);
            break;
        }
        case 2: { // payload bytes
            if (!readBytes(c, out.payload))
                return false;
            break;
        }
        case 4: { // dest fixed32
            uint32_t v = 0;
            if (!readFixed32(c, v))
                return false;
            out.has_dest = true;
            out.dest = v;
            break;
        }
        case 5: { // source fixed32
            uint32_t v = 0;
            if (!readFixed32(c, v))
                return false;
            out.has_source = true;
            out.source = v;
            break;
        }
        case 6: { // request_id fixed32
            uint32_t v = 0;
            if (!readFixed32(c, v))
                return false;
            out.has_request_id = true;
            out.request_id = v;
            break;
        }
        case 7: { // reply_id fixed32
            uint32_t v = 0;
            if (!readFixed32(c, v))
                return false;
            out.has_reply_id = true;
            out.reply_id = v;
            break;
        }
        default:
            if (!skipField(c, wt))
                return false;
            break;
        }
    }
    return true;
}

// ----- MeshPacket (subset) -----

void encodeMeshPacket(const MeshPacketMsg& m, std::vector<uint8_t>& out)
{
    if (m.has_from)
        encodeFixed32Field(1, m.from, out);
    if (m.has_to)
        encodeFixed32Field(2, m.to, out);
    if (m.has_channel)
        encodeVarintField(3, m.channel, out);

    if (m.has_decoded) {
        // field 4: decoded (submessage Data)
        std::vector<uint8_t> dataBuf;
        encodeData(m.decoded, dataBuf);
        encodeBytesField(4, dataBuf, out);
    }
    if (m.has_encrypted) {
        // field 5: encrypted (bytes)
        encodeBytesField(5, m.encrypted, out);
    }

    if (m.has_id)
        encodeFixed32Field(6, m.id, out);
    if (m.has_hop_limit)
        encodeVarintField(9, m.hop_limit, out);
    if (m.has_want_ack)
        encodeVarintField(10, m.want_ack ? 1 : 0, out);
}

bool decodeMeshPacket(const uint8_t* buf, size_t len, MeshPacketMsg& out)
{
    Cursor c(buf, buf + len);
    while (!c.eof()) {
        uint32_t fn = 0;
        WireType wt = WireType::VARINT;
        if (!readKey(c, fn, wt))
            return false;
        switch (fn) {
        case 1: { // from fixed32
            uint32_t v = 0;
            if (!readFixed32(c, v))
                return false;
            out.has_from = true;
            out.from = v;
            break;
        }
        case 2: { // to fixed32
            uint32_t v = 0;
            if (!readFixed32(c, v))
                return false;
            out.has_to = true;
            out.to = v;
            break;
        }
        case 3: { // channel varint
            uint64_t v = 0;
            if (!readVarint(c, v))
                return false;
            out.has_channel = true;
            out.channel = static_cast<uint32_t>(v);
            break;
        }
        case 4: { // decoded submessage
            std::vector<uint8_t> bytes;
            if (!readBytes(c, bytes))
                return false;
            DataMsg d;
            if (!decodeData(bytes.data(), bytes.size(), d))
                return false;
            out.decoded = d;
            out.has_decoded = true;
            break;
        }
        case 5: { // encrypted bytes
            if (!readBytes(c, out.encrypted))
                return false;
            out.has_encrypted = true;
            break;
        }
        case 6: { // id fixed32
            uint32_t v = 0;
            if (!readFixed32(c, v))
                return false;
            out.has_id = true;
            out.id = v;
            break;
        }
        case 9: { // hop_limit varint
            uint64_t v = 0;
            if (!readVarint(c, v))
                return false;
            out.has_hop_limit = true;
            out.hop_limit = static_cast<uint32_t>(v);
            break;
        }
        case 10: { // want_ack bool(varint)
            uint64_t v = 0;
            if (!readVarint(c, v))
                return false;
            out.has_want_ack = true;
            out.want_ack = (v != 0);
            break;
        }
        default:
            if (!skipField(c, wt))
                return false;
            break;
        }
    }
    return true;
}

} // namespace meshtastic
} // namespace interop

