// Copyright (c) 2026
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef NR_MODEL_TRANSLATOR_H
#define NR_MODEL_TRANSLATOR_H

#include "ccc-message.h"
#include "kpm-indication.h"

#include "ns3/nr-export.h"
#include "ns3/nstime.h"

#include <optional>
#include <string>
#include <vector>

namespace ns3
{

enum class NrMappingPolicy
{
    ExplicitRejection,
    MetadataPreserving,
    LegacyCompatibility
};

enum class NrMeasurementLayer
{
    Unknown,
    Rlc,
    Pdcp,
    Phy
};

struct NrMeasurementMetadata
{
    std::string targetName;
    std::string sourceName;
    NrMeasurementLayer sourceLayer{NrMeasurementLayer::Unknown};
    std::string unit;
    Time reportingInterval{Seconds(0)};
    bool derived{false};
};

struct NrTranslationDiagnostic
{
    enum class Severity
    {
        Info,
        Warning,
        Rejected
    };

    Severity severity{Severity::Info};
    std::string code;
    std::string message;
    std::string sourceName;
};

struct NrReportingSnapshot
{
    uint16_t cellId{0};
    std::string plmnId{"00101"};
    Time reportingInterval{Seconds(0)};
    uint64_t dlTxBytes{0};
    uint64_t dlRxBytes{0};
    uint64_t ulTxBytes{0};
    uint64_t ulRxBytes{0};
    bool hasDlTxBytes{false};
    bool hasDlRxBytes{false};
    bool hasUlTxBytes{false};
    bool hasUlRxBytes{false};
    NrMeasurementLayer byteCounterLayer{NrMeasurementLayer::Unknown};
    std::string byteCounterSource;
    long nativePdcpBytesDl{0};
    long nativePdcpBytesUl{0};
    bool hasNativePdcp{false};
};

struct NrPortPowerCommand
{
    std::vector<double> portPower;
    std::string cellId;
};

struct NrKpmTranslationResult
{
    KpmIndicationMessage::KpmIndicationMessageValues values;
    std::vector<NrMeasurementMetadata> mappings;
    std::vector<NrTranslationDiagnostic> diagnostics;
};

struct NrCccTranslationResult
{
    std::optional<NrPortPowerCommand> command;
    std::vector<NrTranslationDiagnostic> diagnostics;
};

class NR_EXPORT NrModelTranslator
{
  public:
    static NrKpmTranslationResult TranslateKpm(const NrReportingSnapshot& snapshot,
                                               NrMappingPolicy policy);
    static NrCccTranslationResult TranslateCcc(const CccControlMessage& message,
                                                size_t expectedPorts = 0);
};

} // namespace ns3

#endif /* NR_MODEL_TRANSLATOR_H */