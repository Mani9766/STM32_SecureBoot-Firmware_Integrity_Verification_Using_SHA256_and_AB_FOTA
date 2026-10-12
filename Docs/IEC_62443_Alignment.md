# IEC 62443 Alignment Assessment
## STM32 Secure Boot: Firmware Integrity Verification Using SHA-256 and A/B FOTA

### 1. Purpose

This document assesses the alignment of the STM32F407 Secure Boot and Firmware Over-The-Air (FOTA) project with selected security concepts from the IEC 62443 series.

The assessment focuses on software integrity, secure firmware update handling, failure recovery, threat modeling, and verification evidence.

This is a project-level engineering assessment, not a formal conformity assessment, certification, or declaration of compliance with IEC 62443.

### 2. Applicable Standards

| Standard | Application to the project |
|---|---|
| IEC 62443-4-2:2019 | Technical security requirements for industrial automation and control system components. |
| IEC 62443-4-1:2018 | Secure product development lifecycle practices, including security requirements, design, verification, defect management, and maintenance. |


### 3. Technical Security Alignment

| Security area | Project implementation | Assessment | Gap or limitation |
|---|---|---|---|
| Software integrity verification | SHA-256 verification of firmware images against the expected digest stored in metadata | **Implemented and verified** through documented firmware integrity test cases and hash comparison evidence | SHA-256 verification alone does not establish firmware publisher authenticity. The expected digest and metadata must also be protected against unauthorized modification. |
| Firmware image validation | Image size, memory bounds, metadata fields and reset-handler validation | **Implemented and verified** through the documented validation test cases | The assessment covers the validation scenarios demonstrated by the available test evidence; it does not claim exhaustive coverage of every possible malformed image. |
| Secure update handling | Download/staging region, Slot A, Slot B and metadata-driven firmware update states | **Implemented and tested** through the documented update flow and associated test cases | Security against unauthorized firmware sources and modification of update data is not established by the update mechanism alone. |
| Recovery and rollback | Candidate image trial boot, A/B firmware cycling and recovery to a previously active image | **Implemented and verified** against the recovery and rollback scenarios covered by the executed test cases | The claim is limited to the recovery scenarios demonstrated by the test results; it does not imply recovery from every conceivable hardware or flash failure. |
| Integrity failure handling | Firmware integrity and validation failures prevent acceptance of an invalid candidate image | **Implemented and verified** through the documented negative and failure-path tests | Protection against all possible attack scenarios has not been established by the tested cases alone. |
| Firmware authenticity | Firmware integrity is checked using SHA-256; cryptographic signature verification and trusted signing-key validation are not implemented | **Not implemented in the current project scope** | A matching digest does not prove that the firmware originated from an authorized developer or source. Signed firmware and trusted-key verification would be a future enhancement. |
| Security event reporting | Bootloader diagnostic output and debugging evidence, including observed test results | **Implemented for development diagnostics and verification** | Persistent, protected security-event logging and tamper-resistant audit records are not established as implemented features. |
| Access control for firmware updates | Firmware update and flash operations are managed by the bootloader; a complete update-authorization mechanism has not been established | **Partially addressed; update authorization is outside the current implementation scope** | Additional controls may be required to prevent unauthorized update initiation, firmware replacement or flash modification. |

**Assessment note:** The statuses reflect the executed test cases and available verification evidence. Each "verified" claim should be traceable to its corresponding test results. This is a project-level assessment against selected IEC 62443 concepts, not a claim of full compliance or certification.

### 4. Development Lifecycle Alignment

| Development activity | Project evidence | Status |
|---|---|---|
| Security requirements and scope | Defined firmware integrity, update validation and recovery objectives | Documented |
| Threat modeling | Project threat analysis and identified risks | Completed for the documented scope |
| Secure design | Flash memory layout, A/B slots, metadata states and validation flow | Implemented and documented |
| Verification and validation | Unit tests, firmware hash checks, metadata checks and hardware debug evidence | Unit testing completed; remaining validation depends on the test plan |
| Defect management | Identified firmware integrity and validation issues with corrective actions | Track findings and closure evidence |
| Maintenance and future improvements | Identified gaps in authenticity, update authorization and security event handling | Planned |

### 5. Scope Limitations

The current implementation provides firmware integrity verification and update-state management. It does not provide a complete firmware authentication mechanism.

SHA-256 can detect a mismatch between an image and its expected digest, provided that the expected digest is trustworthy. If an attacker can modify both the firmware and its metadata, hashing alone does not establish authenticity.

The project does not claim an IEC 62443 Security Level, formal compliance, or certification. A complete assessment would require a defined product scope, applicable requirements, risk assessment, implementation review and sufficient verification evidence.

### 6. Planned Improvements

1. Complete and record A/B update, trial-boot, failure and rollback tests.
2. Verify behavior under interrupted updates and corrupted metadata.
3. Evaluate firmware signature verification using a trusted public key.
4. Assess protection against unauthorized update initiation and flash modification.
5. Define appropriate security event reporting and recovery behavior.
6. Maintain traceability between threats, security requirements, implementation, tests and defects.

### 7. Conclusion

The project demonstrates practical implementation of selected firmware integrity and recovery mechanisms relevant to industrial embedded security engineering. Its current scope supports a preliminary alignment assessment against selected IEC 62443 concepts; it does not establish full compliance with IEC 62443-4-1 or IEC 62443-4-2.
