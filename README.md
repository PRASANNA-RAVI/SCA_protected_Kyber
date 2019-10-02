## SCA protected Kyber

The implementation of the SCA resistant Kyber is present in the crypto_kem folder. This can be viewed as a fork of the **pqm4** library, which is the opensource benchmarking and testing framework for PQC schemes on the ARM Cortex-M4 microcontroller.

This is the SCA protected implementation of the first round submission of the Kyber key exchange scheme.
The implemented countermeasures can also be ported to the second round submission of Kyber and this work is under progress.

## SCA-protected NTT implementation

* The code for the SCA protected NTT and INTT are located in ntt.c. Each of the NTT and INTT
operations are protected over four levels (four variants).

- Level-0 : Unproteted
- Level-1: Shuffled NTT
- Level-2: Randomized NTT
- Level-3: Shuffled-Randomized NTT

* The protection level of each NTT operation can be independently controlled from the indcpa.c file.
For example,

```c
int polyvec_invntt(&bp, 3); // Line 307 in indcpa.c
```
The inverse NTT structure over bp is implemented with level 3 protection level.

## Shuffling secret-sensitive operations in Kyber

We also implement the shuffling countermeasure for some sensitive operations in kyber. The sensitive operations
can be independently controllable to be either protected (1) or unprotected (0) in indcpa.c.
For example,

```c
polyvec_add(&bp, &bp, &ep, 1); // Line 308, Protected with shuffling countermeasure
polyvec_add(&bp, &bp, &ep, 0); // Unprotected
```

## Hardware Setup and Installation:

The instructions to setup the STM32Discvovery board to run the **pqm4** library is available in the link below:

https://github.com/mupq/pqm4/tree/Round1

One can also use the ST-Link utility to burn the binary onto the STM32 discovery board. The hardware setup of the STM32Discovery board

## How to run the code:

* Compile command:

make

The binary to burn into the STM32 board will be available in the bin folder as crypto_kem_kyber768_ref_test.bin.

* Command to burn the binary into the STM32Discovery board:

openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg -c "program crypto_kem_kyber768_ref_test.bin 0x08000000 verify reset exit"

One can also use the openocd framework or also the ST-Link utility to download the binary onto the board.
