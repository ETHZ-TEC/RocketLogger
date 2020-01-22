/**
 * Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

.origin 0           // start of program in PRU instruction memory
.entrypoint MAIN    // program entry point

// PRU data RAM address definitions
#define PRU0_DATA_RAM_BASE          0x00000000

// PRU control register definitions
#define PRU0_CTRL_BASE              0x00022000
#define PRU_CTRL_BASE               PRU0_CTRL_BASE
#define PRU_CTRL_OFFSET             0x00
#define PRU_CYCLE_OFFSET            0x0C
#define PRU_CYCLE_RESET             0x0000

// PRU control register LS byte constants for cycle counter control
#define PRU_CRTL_CTR_ON             0x0B
#define PRU_CRTL_CTR_OFF            0x03

// PRU interrupt event register definitions
#define PRU_R31_VEC_VALID           0x20    // valid interrupt bit flag
#define PRU_EVTOUT_0                0x03    // user space interrupt event number

// SPI delay cycle definitions
#define SPI_SELECT_GUARD_CYCLES     4   // min. 20ns after CS before SCLK rise
#define SPI_DESELECT_GUARD_CYCLES   400 // min. 4 t_CLK (2us) before CS rise
#define SPI_DECODE_GUARD_CYCLES     320 // min. 4 t_CLK (2us) burst period
#define SPI_IDLE_GUARD_CYCLES       200 // min. 2 t_CLK (1us) CS high

// ADC command and status definitions
#define ADC_COMMAND_SDATAC          0x11000000  // stop ADC data acquisition
#define ADC_COMMAND_RREG            0x20000000  // read ADC register
#define ADC_COMMAND_SIZE            24
#define ADC_STATUS_SIZE             24
#define ADC_STATUS_MASK             0x0F // for all 4 digital inputs
#define ADC_WRITE_GUARD_CYCLES      4000


// pru data layout (position in memory)
#define STATE_OFFSET                0x00
#define BUFFER0_OFFSET              0x04
#define BUFFER1_OFFSET              0x08
#define BUFFER_LENGTH_OFFSET        0x0C
#define SAMPLE_LIMIT_OFFSET         0x10
#define ADC_PRECISION_OFFSET        0x14
#define ADC_COMMAND_COUNT_OFFSET    0x18
#define ADC_COMMAND_BASE_OFFSET     0x1C

// buffer data layout
// total channel count
#define BUFFER_CHANNEL_COUNT        10
#define BUFFER_INDEX_SIZE           4
#define BUFFER_DATA_SIZE            4
#define BUFFER_BLOCK_SIZE           (BUFFER_CHANNEL_COUNT * BUFFER_DATA_SIZE)

// GPIO in/out register bit definitions
#define SCLK                        r30.t0
#define MOSI                        r30.t1
#define MISO1                       r31.t2
#define MISO2                       r31.t16
#define CS1                         r30.t3
#define CS2                         r30.t5
#define START_PIN                   r30.t7
#define DR1                         r31.t15
#define DR2                         r31.t14 // unused
#define LED_ERROR                   r30.t14 // user space, cleared on start
#define LED_STATUS                  r30.t15 // user space, busy wait indicator

// SPI GPIO in/out register and bit definitions
#define SCLK_REG                    r30
#define SCLK_BIT                    0
#define MOSI_REG                    r30
#define MOSI_BIT                    1
#define CS_REG                      r30
#define CS1_BIT                     3
#define CS2_BIT                     5
#define MISO_REG                    r31
#define MISO1_BIT                   2
#define MISO2_BIT                   16
#define DR_REG                      r31
#define DR1_BIT                     15
#define DR2_BIT                     14

// PRU register definitions: channel data from ADC to transfer to DDR
#define DI_REG                      r0
#define V1_REG                      r1
#define V2_REG                      r2
#define V3_REG                      r3
#define V4_REG                      r4
#define I1L_REG                     r5
#define I1H_REG                     r6
#define I2L_REG                     r7
#define I2H_REG                     r8
#define DT_REG                      r9  // PRU cycle counter

// PRU register definitions: double sampled channel data from ADC
#define I1L_2_REG                   r10
#define I1H_2_REG                   r11
#define I2L_2_REG                   r12
#define I2H_2_REG                   r13

// PRU register definitions: unused medium current channel data from ADC
#define I1M_REG                     r14 // unused ADC data
#define I2M_REG                     r15 // unused ADC data

// PRU register definitions: temporary registers (highest registers)
#define TMP_REG                     r16

// PRU register definitions: temporary macro register (to be optimized???)
#define MTMP_REG                    r17

// PRU register definitions: data memory addresses
#define RAM_ADDRESS                 r29
#define CTRL_ADDRESS                r28

// PRU register definitions: PRU local sampling state
#define BUFFER_SIZE                 r27
#define BUFFER_INDEX                r26
#define SAMPLES_COUNT               r25
#define MEM_POINTER                 r24

// PRU register definitions: PRU user space configuration
#define PRECISION                   r23

// PRU register definitions: ADC status and configuration register aliases
#define ADC_COMMAND_COUNT_REG       V1_REG  // reuse data register for ADC setup
#define ADC_COMMAND_OFFSET_REG      V2_REG  // reuse data register for ADC setup
#define ADC1_STATUS_REG             DI_REG
#define ADC2_STATUS_REG             TMP_REG

// registers r18-r22 unused

// NOP pseudo instruction definition
#define NOP ADD MTMP_REG, MTMP_REG, 0
 
 
// --------------------------------- Macros --------------------------------- //

// WAIT CYCLES (wait for defined number of cycles, min. 3 cycles)
.macro  wait_cycles
.mparam cycles
    LDI     MTMP_REG, cycles
    SUB     MTMP_REG, MTMP_REG, 3
    LOOP    ENDLOOP, MTMP_REG
    NOP
ENDLOOP:
.endm


// START CYCLE COUNTER (overwrite LSB of control register), 5 cycles start delay
.macro start_counter
    LDI     MTMP_REG, PRU_CRTL_CTR_ON
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
.endm


// STOP CYCLE COUNTER (overwrite LSB of control register), 5 cycles stop delay
.macro stop_counter
    LDI     MTMP_REG, PRU_CRTL_CTR_OFF
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
.endm


// TIMESTAMP AND RESTART (store cycle count to register and restart counter)
.macro timestamp_restart
.mparam reg
    stop_counter
    LBBO    &reg, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4      // read counter value
    LDI     MTMP_REG, PRU_CYCLE_RESET                    // load reset value
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4 // reset counter
    start_counter
.endm


// SPI WRITE DATA (write SPI data block of `size` bits, optimized for timing)
.macro  spi_write_data
.mparam data_reg, size
    // setup SPI bit write loop
    LOOP    END_SPI_BIT_WRITE, size

    // clock high phase (5 cycles total, offset by 1 cycle for QBBC)

    // set SPI output data (3 cycles)
    QBBC    OUTLOW, data_reg, 31
    SET     SCLK
    SET     MOSI
    QBA     OUTHIGH
OUTLOW:
    SET     SCLK
    CLR     MOSI
    NOP
OUTHIGH:
    // timed delay (2 cycles)
    NOP
    NOP

    // clock low phase (5 cycles total)
    CLR     SCLK

    // shift output data register (1 cycle)
    LSL     data_reg, data_reg, 1
    // timed delay (2 cycles)
    NOP
    NOP
    // QBBC (1 cycle) at begining of next loop iteration

END_SPI_BIT_WRITE:
.endm


// SPI READ DATA (read SPI data block of `size` bits, optimized for timing)
.macro spi_read_data
.mparam data0_reg, data1_reg, size
    // dummy read of MISO2 in first iteration
    ZERO    &MTMP_REG, 4

    // setup SPI bit read loop
    LOOP    END_SPI_BIT_READ, size

    // clock high phase (5 cycles total)
    SET     SCLK

    // decode SPI2 input bit from previous loop iteration (2 cycles)
    QBBS    IN1HIGH, MTMP_REG, MISO2_BIT
    QBA     IN1LOW
IN1HIGH:
    OR      data1_reg, data1_reg, 0x01
IN1LOW:
    // shift input data register for current loop (2 cycles)
    LSL     data0_reg, data0_reg, 1
    LSL     data1_reg, data1_reg, 1

    // clock low phase (5 cycles total)
    CLR     SCLK

    // timed delay: before input bit read; due to input delay? (1 cycle)
    NOP

    // read input bits (1 cycle)
    MOV     MTMP_REG, MISO_REG

    // decode and store SPI1 input bit (2 cycles)
    QBBS    IN0HIGH, MTMP_REG, MISO1_BIT
    QBA     IN0LOW
IN0HIGH:
    OR      data0_reg, data0_reg, 0x01
IN0LOW:

END_SPI_BIT_READ:
    // decode final SPI2 input bit from the last loop iteration (2 cycles)
    QBBS    IN2HIGH, MTMP_REG, MISO2_BIT
    QBA     IN2LOW
IN2HIGH:
    OR      data1_reg, data1_reg, 0x01
IN2LOW:
.endm


// ADC SEND COMMAND (send command of 24 bits to the ADC)
.macro adc_send_command
.mparam command_reg
    // start ADC command frame with (negative) chip select
    CLR     CS1
    CLR     CS2
    wait_cycles SPI_SELECT_GUARD_CYCLES

    // write ADC command to SPI (byte 1, MSB)
    spi_write_data command_reg, 8
    wait_cycles SPI_DECODE_GUARD_CYCLES

    // write ADC command to SPI (byte 2)
    spi_write_data command_reg, 8
    wait_cycles SPI_DECODE_GUARD_CYCLES

    // write ADC command to SPI (byte 3, LSB)
    spi_write_data command_reg, 8

    // end ADC command frame with (positive) chip de-select
    wait_cycles SPI_DESELECT_GUARD_CYCLES
    SET     CS1
    SET     CS2
.endm


// ADC READ (read full ADC sample data to pre-assigned registers)
.macro adc_read
    // start ADC data transfer frame with (negative) chip select
    CLR     CS1
    CLR     CS2
    wait_cycles SPI_SELECT_GUARD_CYCLES

    // read status data
    spi_read_data ADC1_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_SIZE 

    // read all channel data (HW channel order: IHA, IHB, IM, ILA, ILB, VB, VA)
    spi_read_data I1H_REG,   I2H_REG,    PRECISION
    spi_read_data I1H_2_REG, I2H_2_REG,  PRECISION
    spi_read_data I1M_REG,   I2M_REG,    PRECISION
    spi_read_data I1L_REG,   I2L_REG,    PRECISION
    spi_read_data I1L_2_REG, I2L_2_REG,  PRECISION
    spi_read_data V2_REG,    V4_REG,     PRECISION
    spi_read_data V1_REG,    V3_REG,     PRECISION

    // end ADC data transfer frame with (positive) chip de-select
    // wait_cycles SPI_DESELECT_GUARD_CYCLES // skip guard for data burst read
    SET     CS1
    SET     CS2
.endm


// SIGN EXTEND 24 BIT (24 to 32 bit)
.macro sign_extend_24
.mparam reg
    MOV     reg.b3, 0x00
    QBBC    POS, reg, 23
    MOV     reg.b3, 0xFF
POS:
.endm

 
// ------------------------------ Main Program ------------------------------ //
MAIN:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
    LBCO    TMP_REG, C4, 4, 4       // load SYSCFG reg (use c4 const addr)
    CLR     TMP_REG, TMP_REG, 4     // clear bit 4 of SYSCFG (STANDBY_INIT)
    SBCO    TMP_REG, C4, 4, 4       // store back modified SYSCFG value
 
    MOV     RAM_ADDRESS, PRU0_DATA_RAM_BASE    // load the RAM base address
    MOV     CTRL_ADDRESS, PRU0_CTRL_BASE        // load the CTRL base addr  ess

    CLR     LED_ERROR   // reset error indicator

    SET     CS1         // CS high
    SET     CS2
    CLR     MOSI        // MOSI low
    CLR     START_PIN   // START low

    // notify user-space program
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

INIT:
    // get ADC initialization command list address offset
    LDI     ADC_COMMAND_OFFSET_REG, ADC_COMMAND_BASE_OFFSET

    // get ADC initialization command list count
    LBBO    ADC_COMMAND_COUNT_REG, RAM_ADDRESS, ADC_COMMAND_COUNT_OFFSET, 4

ADC_SETUP:
    // load and send ADC command
    LBBO    TMP_REG, RAM_ADDRESS, ADC_COMMAND_OFFSET_REG, 4
    adc_send_command TMP_REG

    // min delay for chip deselect
    wait_cycles SPI_IDLE_GUARD_CYCLES

    // increment ADC command list address offset
    ADD     ADC_COMMAND_OFFSET_REG, ADC_COMMAND_OFFSET_REG, 4

    // decrement command counter and check for init done
    SUB     ADC_COMMAND_COUNT_REG, ADC_COMMAND_COUNT_REG, 1
    QBLT    ADC_SETUP, ADC_COMMAND_COUNT_REG, 0

END_ADC_SETUP:

    // load configuration and initialize state with user space settings
    LBBO    MEM_POINTER,    RAM_ADDRESS,    BUFFER0_OFFSET,         4
    LBBO    SAMPLES_COUNT,  RAM_ADDRESS,    SAMPLE_LIMIT_OFFSET,    4
    LBBO    BUFFER_SIZE,    RAM_ADDRESS,    BUFFER_LENGTH_OFFSET,   4
    LBBO    PRECISION,      RAM_ADDRESS,    ADC_PRECISION_OFFSET,   4

    // initialize buffer index to zero
    ZERO    &BUFFER_INDEX,  4
    
    // set ADC start signal to one to start ADC conversion
    SET     START_PIN

    // jump right into into sampling loop
    QBA     START

SAMPLINGLOOP:
    // if buffer not full, continue reading next sample
    QBLT    NEXTSAMPLE, BUFFER_SIZE, 0

    // on full buffer:
    // signal interrupt to user space program
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

    // load and check PRU state set by user space
    LBBO    TMP_REG, RAM_ADDRESS, STATE_OFFSET, 4
    QBBC    STOP, TMP_REG, 0       // if disabled, stop sampling
    QBBS    NEXTBUFFER, TMP_REG, 1 // if continuous mode, continue sampling

    // if finite number of samples acquired, stop sampling
    QBGE    STOP, SAMPLES_COUNT, 0

NEXTBUFFER:
    // increment buffer index
    ADD     BUFFER_INDEX, BUFFER_INDEX, 1

START:
    // reload buffer size
    LBBO    BUFFER_SIZE, RAM_ADDRESS, BUFFER_LENGTH_OFFSET, 4 // reload buffer size

    // select data buffer
    QBBC    BUFFER0, BUFFER_INDEX, 0
    
    // point to buffer1 for odd indexes
    LBBO    MEM_POINTER, RAM_ADDRESS, BUFFER1_OFFSET, 4
    QBA     BUFFER1
BUFFER0:
    // point to buffer0 for even indexes
    LBBO    MEM_POINTER, RAM_ADDRESS, BUFFER0_OFFSET, 4
BUFFER1:

    // store buffer index
    SBBO    BUFFER_INDEX, MEM_POINTER, 0, 4
    ADD     MEM_POINTER, MEM_POINTER, BUFFER_INDEX_SIZE


NEXTSAMPLE:
    // update buffer and total counters
    SUB     BUFFER_SIZE, BUFFER_SIZE, 1

    // if continuous mode or filling last buffer, skip sample counting
    QBEQ    READ, SAMPLES_COUNT, 0
    SUB     SAMPLES_COUNT, SAMPLES_COUNT, 1

READ:
    // signal start wait, using status LED as busy wait indicator
    SET     LED_STATUS

    // wait for data ready
    WBC     DR1

    // immediately timestamp data
    timestamp_restart DT_REG

    // signal end wait, using status LED as busy wait indicator
    CLR     LED_STATUS

    // read data from ADC
    adc_read

    // mask digital inputs and store in digital data register
    AND     ADC1_STATUS_REG, ADC1_STATUS_REG, ADC_STATUS_MASK
    AND     ADC2_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_MASK
    LSL     ADC2_STATUS_REG, ADC2_STATUS_REG, 8
    OR      DI_REG, ADC1_STATUS_REG, ADC2_STATUS_REG

    // extend 16 bit data to 24 bit in case of lower precision data
    QBEQ    HIGHPRECISION, PRECISION, 24

    LSL     V1_REG, V1_REG, 8
    LSL     V2_REG, V2_REG, 8
    LSL     V3_REG, V3_REG, 8
    LSL     V4_REG, V4_REG, 8
    LSL     I1H_REG, I1H_REG, 8
    LSL     I1H_2_REG, I1H_2_REG, 8
    LSL     I1M_REG, I1M_REG, 8
    LSL     I1L_REG, I1L_REG, 8
    LSL     I1L_2_REG, I1L_2_REG, 8
    LSL     I2H_REG, I2H_REG, 8
    LSL     I2H_2_REG, I2H_2_REG, 8
    LSL     I2M_REG, I2M_REG, 8
    LSL     I2L_REG, I2L_REG, 8
    LSL     I2L_2_REG, I2L_2_REG, 8

HIGHPRECISION:
    // sign extension (24->32bit)
    sign_extend_24 V1_REG
    sign_extend_24 V2_REG
    sign_extend_24 V3_REG
    sign_extend_24 V4_REG
    sign_extend_24 I1H_REG
    sign_extend_24 I1H_2_REG
    sign_extend_24 I1M_REG
    sign_extend_24 I1L_REG
    sign_extend_24 I1L_2_REG
    sign_extend_24 I2H_REG
    sign_extend_24 I2H_2_REG
    sign_extend_24 I2M_REG
    sign_extend_24 I2L_REG
    sign_extend_24 I2L_2_REG

DATAPROCESSING:
    // combine parallel sampled current channels
    ADD     I1H_REG, I1H_REG, I1H_2_REG
    ADD     I1L_REG, I1L_REG, I1L_2_REG
    ADD     I2H_REG, I2H_REG, I2H_2_REG
    ADD     I2L_REG, I2L_REG, I2L_2_REG

    // single block transfer to DDR of all data channels
    SBBO    DI_REG, MEM_POINTER, 0, BUFFER_BLOCK_SIZE
    ADD     MEM_POINTER, MEM_POINTER, BUFFER_BLOCK_SIZE

    QBA     SAMPLINGLOOP

STOP:
    // clear all ADC signals
    CLR     START_PIN

    // set SDATAC command (RREG needed for SDATAC to work!)
    MOV     TMP_REG, ADC_COMMAND_SDATAC
    adc_send_command TMP_REG
    MOV     TMP_REG, ADC_COMMAND_RREG
    adc_send_command TMP_REG

    // signal interrupt to user space program
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

    // clear status LED
    CLR     LED_STATUS

    // halt the PRU
    HALT
