/*
 * Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ADS131E0X_H_
#define ADS131E0X_H_

/**
 * Minimum ADC rate in samples per second.
 */
#define ADS131E0X_RATE_MIN 1000

/**
 * ADS131E0x ADC command (extended to 32 bits for PRU use) definitions
 */
#define ADS131E0X_WAKEUP 0x02
#define ADS131E0X_STANDBY 0x04
#define ADS131E0X_RESET 0x06
#define ADS131E0X_START 0x08
#define ADS131E0X_STOP 0x0A
#define ADS131E0X_OFFSETCAL 0x1A
#define ADS131E0X_RDATAC 0x10
#define ADS131E0X_SDATAC 0x11
#define ADS131E0X_RDATA 0x12
#define ADS131E0X_RREG 0x20
#define ADS131E0X_WREG 0x40

/**
 * ADS131E0x ADC register definitions
 */
#define ADS131E0X_ID 0x00
#define ADS131E0X_CONFIG1 0x01
#define ADS131E0X_CONFIG2 0x02
#define ADS131E0X_CONFIG3 0x03
#define ADS131E0X_CH1SET 0x05
#define ADS131E0X_CH2SET 0x06
#define ADS131E0X_CH3SET 0x07
#define ADS131E0X_CH4SET 0x08
#define ADS131E0X_CH5SET 0x09
#define ADS131E0X_CH6SET 0x0A
#define ADS131E0X_CH7SET 0x0B
#define ADS131E0X_CH8SET 0x0C

/**
 * ADS131E0x ADC gain settings
 */
#define ADS131E0X_GAIN1 0x10
#define ADS131E0X_GAIN2 0x20
#define ADS131E0X_GAIN12 0x60

/**
 * ADS131E0x ADC sampling rates
 */
#define ADS131E0X_K1 0x06
#define ADS131E0X_K2 0x05
#define ADS131E0X_K4 0x04
#define ADS131E0X_K8 0x03
#define ADS131E0X_K16 0x02
#define ADS131E0X_K32 0x01
#define ADS131E0X_K64 0x00

/**
 * ADS131E0x precision defines ({@link ADS131E0X_PRECISION_HIGH} for low
 * sampling rates, {@link ADS131E0X_PRECISION_LOW} for high ones)
 */
#define ADS131E0X_PRECISION_HIGH 24
#define ADS131E0X_PRECISION_LOW 16

/**
 * ADS131E0x ADC configuration default value defines
 */
#define ADS131E0X_CONFIG1_DEFAULT 0x90
#define ADS131E0X_CONFIG2_DEFAULT 0xE0
#define ADS131E0X_CONFIG3_DEFAULT 0xE8

#endif /* ADS131E0X_H_ */
