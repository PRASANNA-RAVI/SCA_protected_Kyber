## Side-Channel Attack Resistant Implementation of Kyber Key-Exchange Scheme

Kyber is a lattice-based Key-Exchange scheme which is currently a candidate in the NIST standardization process for Post-Quantum Cryptography. It heavily utilizes the Number Theoretic Transform (NTT) operation for polynomial multiplication. The NTT transform is very susceptible to side-channel attacks as shown by Pessl et al. in very recent works that have demonstrated single trace template style attacks over the Number Theoretic Transform. In this work, we attempt to protect the NTT operation by employing novel shuffling and masking countermeasures and implement it as part of the Kyber Key-Exchange scheme.

The countermeasures are incorporated over the first round implementation of Kyber
as present in the **pqm4** libary, which is an opensource benchmarking and testing framework for PQC schemes on the ARM Cortex-M4 microcontroller. More details about the **pqm4** library can be found in the link below:

https://github.com/mupq/pqm4/tree/Round1

This work mainly consists of the SCA protected implementation of the first round submission of the Kyber key exchange scheme. The implemented countermeasures can also be ported to the second round submission of Kyber and this work is under progress.

## SCA-protected NTT implementation

Our countermeasures employed over the NTT operation are mainly of two types. The first countermeasure involves randomizing the order of the butterfly operations within every stage of the NTT operation which we refer to as the Shuffled NTT. The second countermeasure involves performing multiplicative masking of the twiddle factors used in the NTT operation, which we refer to as the Randomized NTT. Masking the twiddle factors ensures that the intermediate values within the NTT operation are also masked, thus protecting the NTT from DPA style attacks. Moreover, we employ a cheap and low-entropy masking which involves multiplicative masking with powers of the twiddle factors whose products are already pre-computed. Thus, we save on the extra multiplication when computing the masked twiddle factor during every butterfly operation.

* The code for the SCA protected NTT and INTT are located in **crypto_kem/kyber768/ref/ntt.c** . Each of the NTT and INTT operations are protected over four levels (four variants).

- Level-0: Unprotected
- Level-1: Shuffled NTT
- Level-2: Randomized NTT
- Level-3: Shuffled-Randomized NTT

* The protection level of each NTT operation can be independently controlled from the **indcpa.c** file.
For example,

```c
int polyvec_invntt(&bp, 3); // Line 307 in indcpa.c
```
The inverse NTT structure over bp is implemented with level 3 protection level.

## Shuffling secret-sensitive operations in Kyber

We also implement the shuffling countermeasure for some sensitive operations in kyber. The sensitive operations
can be independently controllable to be either protected (1) or unprotected (0) in **indcpa.c**.
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

* **Compile command**: make

The binary to burn into the STM32 board will be available in the **bin** folder as **crypto_kem_kyber768_ref_test.bin**.

* **Command to burn the binary into the STM32Discovery board**:

openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg -c "program crypto_kem_kyber768_ref_test.bin 0x08000000 verify reset exit"

One can also use the openocd framework or also the ST-Link utility to download the binary onto the board.
