// Copyright (c) 2026
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-model-translator.h"

#include <cctype>

namespace ns3
{

NrKpmTranslationResult
NrModelTranslator::TranslateKpm(const NrReportingSnapshot& snapshot, NrMappingPolicy policy)
{
    NrKpmTranslationResult result;
    Ptr<OCuUpContainerValues> cuUp;

    if (snapshot.hasNativePdcp)
    {
        cuUp = Create<OCuUpContainerValues>();
        cuUp->m_plmId = snapshot.plmnId;
        cuUp->m_pDCPBytesDL = snapshot.nativePdcpBytesDl;
        cuUp->m_pDCPBytesUL = snapshot.nativePdcpBytesUl;
        result.mappings.push_back({"PDCPBytesDL", "nativePdcpBytesDl", NrMeasurementLayer::Pdcp,
                                   "bytes", snapshot.reportingInterval, false});
        result.mappings.push_back({"PDCPBytesUL", "nativePdcpBytesUl", NrMeasurementLayer::Pdcp,
                                   "bytes", snapshot.reportingInterval, false});
    }
    else if (policy == NrMappingPolicy::LegacyCompatibility)
    {
        cuUp = Create<OCuUpContainerValues>();
        cuUp->m_plmId = snapshot.plmnId;
        if (snapshot.byteCounterLayer == NrMeasurementLayer::Rlc)
        {
            cuUp->m_pDCPBytesDL = snapshot.hasDlTxBytes ? snapshot.dlTxBytes : 0;
            cuUp->m_pDCPBytesUL = snapshot.hasUlTxBytes ? snapshot.ulTxBytes : 0;
            result.diagnostics.push_back({NrTranslationDiagnostic::Severity::Warning,
                                          "LEGACY_LAYER_MAPPING",
                                          "RLC byte counters were placed in PDCP-labelled fields",
                                          snapshot.byteCounterSource});
            result.mappings.push_back({"PDCPBytesDL", snapshot.byteCounterSource,
                                       NrMeasurementLayer::Rlc, "bytes",
                                       snapshot.reportingInterval, true});
            result.mappings.push_back({"PDCPBytesUL", snapshot.byteCounterSource,
                                       NrMeasurementLayer::Rlc, "bytes",
                                       snapshot.reportingInterval, true});
        }
        else
        {
            result.diagnostics.push_back({NrTranslationDiagnostic::Severity::Warning,
                                          "LEGACY_MISSING_COUNTER",
                                          "Legacy CU-UP fields are retained with zero values because no source counter is available",
                                          snapshot.byteCounterSource});
        }
    }
    else
    {
        const auto severity = policy == NrMappingPolicy::ExplicitRejection
                                  ? NrTranslationDiagnostic::Severity::Rejected
                                  : NrTranslationDiagnostic::Severity::Warning;
        result.diagnostics.push_back({severity, "UNMAPPABLE_PDCP_COUNTER",
                                      "No native PDCP byte counter is available",
                                      snapshot.byteCounterSource});
    }

    result.values.m_pmContainerValues = cuUp;
    if (policy == NrMappingPolicy::MetadataPreserving &&
        snapshot.byteCounterLayer == NrMeasurementLayer::Rlc)
    {
        auto items = Create<MeasurementItemList>();
        if (snapshot.hasDlTxBytes)
        {
            items->AddItem("RLC.DL.TxBytes", static_cast<double>(snapshot.dlTxBytes));
        }
        if (snapshot.hasUlTxBytes)
        {
            items->AddItem("RLC.UL.TxBytes", static_cast<double>(snapshot.ulTxBytes));
        }
        result.values.m_cellMeasurementItems = items;
        result.diagnostics.push_back({NrTranslationDiagnostic::Severity::Info,
                                      "METADATA_LAYER_MAPPING",
                                      "RLC counters are exposed as explicitly named cell measurements",
                                      snapshot.byteCounterSource});
    }
    return result;
}

NrCccTranslationResult
NrModelTranslator::TranslateCcc(const CccControlMessage& message, size_t expectedPorts)
{
    NrCccTranslationResult result;
    const auto cells = message.GetCellsControlled();
    for (size_t cell = 0; cell < cells.size(); ++cell)
    {
        for (const auto& structure : cells[cell].configuration_structures)
        {
            if (structure.ran_configuration_structure_name != "O-NESPolicy")
            {
                continue;
            }
            if (!structure.new_values_of_attributes.contains("antennaMask"))
            {
                result.diagnostics.push_back({NrTranslationDiagnostic::Severity::Info,
                                              "UNSUPPORTED_CCC_ATTRIBUTE",
                                              "O-NESPolicy has no antennaMask attribute",
                                              "antennaMask"});
                continue;
            }
            const auto& value = structure.new_values_of_attributes.at("antennaMask");
            if (!value.is_string() || value.get<std::string>().empty())
            {
                result.diagnostics.push_back({NrTranslationDiagnostic::Severity::Rejected,
                                              "INVALID_ANTENNA_MASK",
                                              "antennaMask must be a non-empty string",
                                              "antennaMask"});
                continue;
            }
            std::vector<double> ports;
            for (const char character : value.get<std::string>())
            {
                if (character == '0' || character == '1')
                {
                    ports.push_back(character == '1' ? 1.0 : 0.0);
                }
                else if (!std::isspace(static_cast<unsigned char>(character)))
                {
                    ports.clear();
                    break;
                }
            }
            if (ports.empty() || (expectedPorts != 0 && ports.size() != expectedPorts))
            {
                result.diagnostics.push_back({NrTranslationDiagnostic::Severity::Rejected,
                                              "INVALID_ANTENNA_MASK",
                                              "antennaMask contains invalid characters or dimensions",
                                              "antennaMask"});
                continue;
            }
            result.command = NrPortPowerCommand{ports, cells[cell].nr_cell_identity};
        }
    }
    return result;
}

} // namespace ns3