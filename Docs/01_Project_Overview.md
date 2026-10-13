# Secure Boot Project – Project Overview

## 1. Introduction

This project focuses on designing and implementing a Secure Boot and dual-image firmware update architecture for an STM32F407-based embedded system.

The implementation focuses on firmware integrity verification using SHA-256, application image validation, controlled application handover, and candidate/active firmware management.

The project demonstrates firmware integrity and recovery mechanisms. Digital-signature-based firmware authenticity verification is outside the current implementation scope.

---

## 2. Problem Statement

In a conventional boot flow, the microcontroller may directly execute application firmware after reset.

If the firmware is corrupted or modified, the system may execute unintended code.

A Secure Bootloader introduces a verification step before application execution to ensure that the firmware image satisfies the defined validation and integrity requirements.

For firmware update scenarios, the system also needs to evaluate a new candidate image without unnecessarily overwriting the currently active firmware.

---

## 3. Project Objective

The project objectives are to:

* Start bootloader execution after MCU reset.
* Identify and validate firmware using its metadata.
* Validate firmware image size, memory boundaries, and reset-handler address.
* Verify firmware integrity using SHA-256.
* Manage firmware staging, active and inactive slots, and metadata-driven update states.
* Validate staged firmware and independently verify the programmed destination image.
* Reject invalid candidate firmware before trial boot.
* Manage candidate trial boot and explicit application confirmation.
* Roll back to the previously confirmed firmware when a candidate fails to confirm, according to the implemented recovery policy.
* Enter the configured BootSafe/fail-safe behavior when the selected active image fails integrity verification.
* Demonstrate firmware update handling and recovery using the STM32F407 hardware platform.

---

## 4. High-Level Architecture

The system consists of three logical areas:

### Bootloader

The Bootloader executes first after MCU reset.

Responsibilities include:

* Boot decision
* Firmware metadata handling
* Application image boundary validation
* Firmware version validation
* SHA-256 integrity verification
* Candidate image validation
* Active-image fallback
* Failure handling
* Application handover

### Candidate Firmware

The candidate firmware represents a new image being evaluated by the bootloader.

The incoming image is first stored in the download/staging region. After validating the staged image metadata, image size, version policy, and SHA-256 digest, the bootloader programs the inactive firmware slot.

The bootloader then independently calculates the SHA-256 digest of the programmed destination image and compares it with the expected digest. If validation succeeds, the candidate is marked `BOOT_PENDING` and selected for trial boot.

The candidate application must explicitly confirm successful startup. If the candidate fails to confirm and the MCU resets, the bootloader detects the unconfirmed state and returns to the previously confirmed firmware according to the implemented recovery policy.

If candidate validation fails before activation, the invalid candidate is rejected and is not selected for trial boot.
