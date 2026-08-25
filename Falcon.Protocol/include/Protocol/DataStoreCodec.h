#pragma once

#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"
#include "protocol/types/DataStoreTypes.h"

class DataStoreCodec {
public:
    static void writeScalar(BinaryStream &stream, const DataStoreScalar &value);

    static DataStoreScalar readScalar(ReadOnlyBinaryStream &stream);

    static void writeChangeValue(BinaryStream &stream, const DataStoreChangeValue &value);

    static DataStoreChangeValue readChangeValue(ReadOnlyBinaryStream &stream);

    static void writeAction(BinaryStream &stream, const DataStoreAction &action, bool withPathUpdateCount);

    static DataStoreAction readAction(ReadOnlyBinaryStream &stream, bool withPathUpdateCount);
};
