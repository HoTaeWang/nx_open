// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "transaction_transport_header.h"

#include <nx/fusion/model_functions.h>

namespace ec2
{
    static QAtomicInt qn_transportHeader_sequence(1);

    void QnTransactionTransportHeader::fillSequence(
        const nx::Uuid& moduleId,
        const nx::Uuid& runningInstanceGUID)
    {
        if (sequence == 0)
        {
            sequence = qn_transportHeader_sequence.fetchAndAddAcquire(1);
            sender = moduleId;
            senderRuntimeID = runningInstanceGUID;
        }
    }

    QString toString(const QnTransactionTransportHeader& header)
    {
        return NX_FMT("ttSeq=%1 sender=%2:%3", header.sequence, header.sender, header.senderRuntimeID);
    }

    bool QnTransactionTransportHeader::isNull() const
    {
        return senderRuntimeID.isNull();
    }

QN_FUSION_ADAPT_STRUCT_FUNCTIONS(QnTransactionTransportHeader,
    (ubjson)(json), QnTransactionTransportHeader_Fields, (optional, false))
}
