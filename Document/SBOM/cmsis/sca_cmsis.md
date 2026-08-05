# CMSIS Third-Party Component Description (for SCA / SBOM)

This document records the SBOM metadata supported by the files in `Library/CMSIS/Include`.

## 1) Component Identity

- Component name: `CMSIS-Core`
- Component type: `library`
- Supplier: `ARM Limited`
- Component version: `3.1.0`
- Bundled DSP header version: `1.1.0`
- License: `ARM CMSIS License` (no SPDX identifier asserted)
- Evidence path: `Library/CMSIS/Include`

The component contains 11 headers:

- CMSIS-DSP: `arm_common_tables.h`, `arm_math.h`
- CMSIS Cortex-M/SecurCore: `core_cm0.h`, `core_cm0plus.h`, `core_cm3.h`, `core_cm4.h`, `core_cm4_simd.h`, `core_cmFunc.h`, `core_cmInstr.h`, `core_sc000.h`, `core_sc300.h`

## 2) Version Evidence

- `core_cm0.h`, `core_cm0plus.h`, `core_cm3.h`, `core_cm4.h`, `core_sc000.h`, and `core_sc300.h` declare CMSIS version main `0x03` and sub `0x01`; their file headers identify the release as `V3.01` dated 13 March 2012.
- `arm_math.h` identifies the CMSIS DSP Library release as `1.1.0`, dated 15 February 2012.
- `arm_common_tables.h` identifies its own file revision as `1.0.2`; it is treated as part of the bundled DSP headers, not as a separate SBOM component.

The normalized component version is therefore `3.1.0`. This is a file-derived CMSIS-Core version, not a claim that the repository contains a complete CMSIS package release.

## 3) License Evidence and Handling

The included headers carry ARM copyright and redistribution notices. The notices permit distribution for use with development tools supporting ARM Cortex-M processor-based microcontrollers and disclaim warranties. They do not identify Apache-2.0 or another SPDX license.

For CycloneDX, record the non-SPDX license name:

```json
"licenses": [{ "license": { "name": "ARM CMSIS License" } }]
```

Preserve all upstream copyright, permission, and warranty notices.

## 4) CycloneDX Mapping

- `type`: `library`
- `name`: `CMSIS-Core`
- `version`: `3.1.0`
- `scope`: `required`
- `author`: `ARM Limited`
- `purl`: `pkg:generic/cmsis-core@3.1.0`
- `bom-ref`: `pkg:generic/cmsis-core@3.1.0?source=vendored&path=Library%2FCMSIS%2FInclude`
- `src_path`: `Library/CMSIS/Include`
- `integration`: `vendored_source`
- `cmsis_core_version`: `3.1.0`
- `cmsis_dsp_header_version`: `1.1.0`

## 5) Scope Notes

- Only files present in `Library/CMSIS/Include` are asserted by this record.
- This record does not claim the presence of CMSIS Driver, RTOS, RTOS2, DAP, SVD, Pack, libraries, documentation, or a complete upstream CMSIS package.
- If finer-grained tracking is required, model the two DSP headers as a separate CMSIS-DSP 1.1.0 component.
