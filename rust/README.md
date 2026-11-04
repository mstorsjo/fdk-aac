# The Fraunhofer FDK AAC Codec Library in Rust Project

## Folder Structure

### `src`

This directory contains the Rust source files for the `aac` crate.

### `ffi`

This folder includes function wrappers that connect the C bindings to the Rust modules. The `src` folder contains the Rust wrappers, while the `include` folder contains the C interface that utilizes them. The main API header file to include is `aacdecoder_lib.h`.

### `framework`

This directory features a Rust example framewok, called (`aac_decoder`).

## Testing

### Unit Tests

You can run unit tests using the command: `cargo test`.

## Build

### Prerequisites

* Install Cargo:
  * `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`
* Use version 1.81.0:
  * `rustup install 1.81.0`
  * `rustup default 1.81.0`

### How to Build the Full Decoder in Rust

Build the package: `cargo build --package aac_framework --bin aac_decoder`

### How to Build the Documentation

* Generate documentation with: `cargo doc`

## Decode

* To run the decoder, use: `target/debug/aac_decoder`
