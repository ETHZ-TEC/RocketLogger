; Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
; * Redistributions of source code must retain the above copyright notice, this
;   list of conditions and the following disclaimer.
;
; * Redistributions in binary form must reproduce the above copyright notice,
;   this list of conditions and the following disclaimer in the documentation
;   and/or other materials provided with the distribution.
;
; * Neither the name of the copyright holder nor the names of its
;   contributors may be used to endorse or promote products derived from
;   this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.


; PRU data RAM address definitions
PRU0_DATA_RAM_BASE          .set    0x00000000

; PRU control register definitions
PRU0_CTRL_BASE              .set    0x00022000
PRU_CTRL_BASE               .set    PRU0_CTRL_BASE
PRU_CTRL_OFFSET             .set    0x00
PRU_CYCLE_OFFSET            .set    0x0C
PRU_CYCLE_RESET             .set    0x0000

; PRU control register LS byte constants for cycle counter control
PRU_CRTL_CTR_ON             .set    0x0B
PRU_CRTL_CTR_OFF            .set    0x03

; PRU interrupt event register definitions
PRU_R31_VEC_VALID           .set    0x20    ; valid interrupt bit flag
PRU_EVTOUT_0                .set    0x03    ; user space interrupt event number

; SPI delay cycle definitions
SPI_SELECT_GUARD_CYCLES     .set    4   ; min. 20ns after CS before SCLK rise
SPI_DESELECT_GUARD_CYCLES   .set    400 ; min. 4 t_CLK (2us) before CS rise
SPI_DECODE_GUARD_CYCLES     .set    320 ; min. 4 t_CLK (2us) burst period
SPI_IDLE_GUARD_CYCLES       .set    200 ; min. 2 t_CLK (1us) CS high

; ADC command and status definitions
ADC_COMMAND_SDATAC          .set    0x11000000  ; stop ADC data acquisition
ADC_COMMAND_RREG            .set    0x20000000  ; read ADC register
ADC_COMMAND_SIZE            .set    24
ADC_STATUS_SIZE             .set    24
ADC_STATUS_MASK             .set    0x0F ; for all 4 digital inputs
ADC_WRITE_GUARD_CYCLES      .set    4000


; pru data layout (position in memory)
STATE_OFFSET                .set    0x00
BUFFER0_OFFSET              .set    0x04
BUFFER1_OFFSET              .set    0x08
BUFFER_LENGTH_OFFSET        .set    0x0C
SAMPLE_LIMIT_OFFSET         .set    0x10
ADC_PRECISION_OFFSET        .set    0x14
ADC_COMMAND_COUNT_OFFSET    .set    0x18
ADC_COMMAND_BASE_OFFSET     .set    0x1C

; buffer data layout
; total channel count
BUFFER_CHANNEL_COUNT        .set    10
BUFFER_INDEX_SIZE           .set    4
BUFFER_DATA_SIZE            .set    4
BUFFER_BLOCK_SIZE           .set    (BUFFER_CHANNEL_COUNT * BUFFER_DATA_SIZE)

; Status GPIO in/out register bit definitions
STATUS_OUT_REG              .set    r30
LED_ERROR_BIT               .set    14  ; user space, cleared on start
LED_STATUS_BIT              .set    15  ; user space, busy wait indicator

; ADC GPIO in/out register and bit definitions
ADC_OUT_REG                 .set    r30
SCLK_BIT                    .set    0
MOSI_BIT                    .set    1
CS1_BIT                     .set    3
CS2_BIT                     .set    5
START_BIT                   .set    7
ADC_IN_REG                  .set    r31
MISO1_BIT                   .set    2
MISO2_BIT                   .set    16
DR1_BIT                     .set    15
DR2_BIT                     .set    14

; PRU register definitions: channel data from ADC to transfer to DDR
DI_REG                      .set    r0
V1_REG                      .set    r1
V2_REG                      .set    r2
V3_REG                      .set    r3
V4_REG                      .set    r4
I1L_REG                     .set    r5
I1H_REG                     .set    r6
I2L_REG                     .set    r7
I2H_REG                     .set    r8
DT_REG                      .set    r9  ; PRU cycle counter

; PRU register definitions: double sampled channel data from ADC
I1L_2_REG                   .set    r10
I1H_2_REG                   .set    r11
I2L_2_REG                   .set    r12
I2H_2_REG                   .set    r13

; PRU register definitions: unused medium current channel data from ADC
I1M_REG                     .set    r14 ; unused ADC data
I2M_REG                     .set    r15 ; unused ADC data

; PRU register definitions: temporary registers (highest registers)
TMP_REG                     .set    r16

; PRU register definitions: temporary macro register (to be optimized???)
MTMP_REG                    .set    r17

; PRU register definitions: data memory addresses
RAM_ADDRESS                 .set    r29
CTRL_ADDRESS                .set    r28

; PRU register definitions: PRU local sampling state
BUFFER_SIZE                 .set    r27
BUFFER_INDEX                .set    r26
SAMPLES_COUNT               .set    r25
MEM_POINTER                 .set    r24

; PRU register definitions: PRU user space configuration
PRECISION                   .set    r23

; PRU register definitions: ADC status and configuration register aliases
ADC_COMMAND_COUNT_REG       .set    V1_REG  ; reuse data register for ADC setup
ADC_COMMAND_OFFSET_REG      .set    V2_REG  ; reuse data register for ADC setup
ADC1_STATUS_REG             .set    DI_REG
ADC2_STATUS_REG             .set    TMP_REG

; registers r18-r22 unused


; --------------------------------- Macros --------------------------------- ;

; WAIT CYCLES (wait for defined number of cycles, min. 3 cycles)
wait_cycles .macro cycles
    LDI     MTMP_REG, cycles
    SUB     MTMP_REG, MTMP_REG, 3
    LOOP    ENDLOOP?, MTMP_REG
    NOP
ENDLOOP?:
    .endm


; START CYCLE COUNTER (overwrite LSB of control register), 5 cycles start delay
start_counter .macro
    LDI     MTMP_REG, PRU_CRTL_CTR_ON
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
    .endm


; STOP CYCLE COUNTER (overwrite LSB of control register), 5 cycles stop delay
stop_counter .macro
    LDI     MTMP_REG, PRU_CRTL_CTR_OFF
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
    .endm


; TIMESTAMP AND RESTART (store cycle count to register and restart counter)
timestamp_restart .macro reg
    stop_counter
    LBBO    &reg, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4      ; read counter value
    LDI     MTMP_REG, PRU_CYCLE_RESET                    ; load reset value
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4 ; reset counter
    start_counter
    .endm


; SPI WRITE DATA (write SPI data block of `size` bits, optimized for timing)
spi_write_data .macro data_reg, size
    ; setup SPI bit write loop
    LOOP    END_SPI_BIT_WRITE?, size

    ; clock high phase (5 cycles total, offset by 1 cycle for QBBC)

    ; set SPI output data (3 cycles)
    QBBC    OUTLOW?, data_reg, 31
    SET     ADC_OUT_REG, ADC_OUT_REG, SCLK_BIT
    SET     ADC_OUT_REG, ADC_OUT_REG, MOSI_BIT
    JMP     OUTHIGH?
OUTLOW?:
    SET     ADC_OUT_REG, ADC_OUT_REG, SCLK_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, MOSI_BIT   
OUTHIGH?:
    ; timed delay (2 cycles)
    NOP
    NOP

    ; clock low phase (5 cycles total)
    CLR     ADC_OUT_REG, ADC_OUT_REG, SCLK_BIT

    ; shift output data register (1 cycle)
    LSL     data_reg, data_reg, 1
    ; timed delay (2 cycles)
    NOP
    NOP
    ; QBBC (1 cycle) at begining of next loop iteration

END_SPI_BIT_WRITE?:
    .endm


; SPI READ DATA (read SPI data block of `size` bits, optimized for timing)
spi_read_data .macro data0_reg, data1_reg, size
    ; dummy read of MISO2 in first iteration
    ZERO    &MTMP_REG, 4

    ; setup SPI bit read loop
    LOOP    END_SPI_BIT_READ?, size

    ; clock high phase (5 cycles total)
    SET     ADC_OUT_REG, ADC_OUT_REG, SCLK_BIT

    ; decode SPI2 input bit from previous loop iteration (2 cycles)
    QBBS    IN1HIGH?, MTMP_REG, MISO2_BIT
    JMP     IN1LOW?
IN1HIGH?:
    OR      data1_reg, data1_reg, 0x01
IN1LOW?:
    ; shift input data register for current loop (2 cycles)
    LSL     data0_reg, data0_reg, 1
    LSL     data1_reg, data1_reg, 1

    ; clock low phase (5 cycles total)
    CLR     ADC_OUT_REG, ADC_OUT_REG, SCLK_BIT

    ; timed delay: before input bit read; due to input delay? (1 cycle)
    NOP

    ; read input bits (1 cycle)
    AND     MTMP_REG, ADC_IN_REG, ADC_IN_REG

    ; decode and store SPI1 input bit (2 cycles)
    QBBS    IN0HIGH?, MTMP_REG, MISO1_BIT
    JMP     IN0LOW?
IN0HIGH?:
    OR      data0_reg, data0_reg, 0x01
IN0LOW?:

END_SPI_BIT_READ?:
    ; decode final SPI2 input bit from the last loop iteration (2 cycles)
    QBBS    IN2HIGH?, MTMP_REG, MISO2_BIT
    JMP     IN2LOW?
IN2HIGH?:
    OR      data1_reg, data1_reg, 0x01
IN2LOW?:
    .endm


; ADC SEND COMMAND (send command of 24 bits to the ADC)
adc_send_command .macro command_reg
    ; start ADC command frame with (negative) chip select
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    wait_cycles SPI_SELECT_GUARD_CYCLES

    ; write ADC command to SPI (byte 1, MSB)
    spi_write_data command_reg, 8
    wait_cycles SPI_DECODE_GUARD_CYCLES

    ; write ADC command to SPI (byte 2)
    spi_write_data command_reg, 8
    wait_cycles SPI_DECODE_GUARD_CYCLES

    ; write ADC command to SPI (byte 3, LSB)
    spi_write_data command_reg, 8

    ; end ADC command frame with (positive) chip de-select
    wait_cycles SPI_DESELECT_GUARD_CYCLES
    SET     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    SET     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    .endm


; ADC READ (read full ADC sample data to pre-assigned registers)
adc_read .macro
    ; start ADC data transfer frame with (negative) chip select
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    wait_cycles SPI_SELECT_GUARD_CYCLES

    ; read status data
    spi_read_data ADC1_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_SIZE 

    ; read all channel data (HW channel order: IHA, IHB, IM, ILA, ILB, VB, VA)
    spi_read_data I1H_REG,   I2H_REG,    PRECISION
    spi_read_data I1H_2_REG, I2H_2_REG,  PRECISION
    spi_read_data I1M_REG,   I2M_REG,    PRECISION
    spi_read_data I1L_REG,   I2L_REG,    PRECISION
    spi_read_data I1L_2_REG, I2L_2_REG,  PRECISION
    spi_read_data V2_REG,    V4_REG,     PRECISION
    spi_read_data V1_REG,    V3_REG,     PRECISION

    ; end ADC data transfer frame with (positive) chip de-select
    ; wait_cycles SPI_DESELECT_GUARD_CYCLES ; skip guard for data burst read
    SET     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    SET     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    .endm


; SIGN EXTEND 24 BIT (24 to 32 bit)
sign_extend_24 .macro reg
    LDI     reg.b3, 0x00
    QBBC    POS?, reg, 23
    LDI     reg.b3, 0xFF
POS?:
    .endm

 
; ------------------------------ Main Program ------------------------------ ;
    .text
    .retain ".text"

    .global MAIN
MAIN:
    ; Enable the OCP master port -- allows transfer of data to Linux userspace
    LBCO    &TMP_REG, C4, 4, 4  ; load SYSCFG reg (use c4 const addr)
    CLR     TMP_REG, TMP_REG, 4 ; clear bit 4 of SYSCFG (STANDBY_INIT)
    SBCO    &TMP_REG, C4, 4, 4  ; store back modified SYSCFG value
 
    LDI32   RAM_ADDRESS, PRU0_DATA_RAM_BASE ; load the RAM base address
    LDI32   CTRL_ADDRESS, PRU0_CTRL_BASE    ; load the CTRL base addr  ess

    CLR     STATUS_OUT_REG, STATUS_OUT_REG, LED_ERROR_BIT   ; reset error indicator

    SET     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT    ; CS high
    SET     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, MOSI_BIT   ; MOSI low
    CLR     ADC_OUT_REG, ADC_OUT_REG, START_BIT  ; START low

    ; notify user-space program
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

INIT:
    ; get ADC initialization command list address offset
    LDI     ADC_COMMAND_OFFSET_REG, ADC_COMMAND_BASE_OFFSET

    ; get ADC initialization command list count
    LBBO    &ADC_COMMAND_COUNT_REG, RAM_ADDRESS, ADC_COMMAND_COUNT_OFFSET, 4

ADC_SETUP:
    ; load and send ADC command
    LBBO    &TMP_REG, RAM_ADDRESS, ADC_COMMAND_OFFSET_REG, 4
    adc_send_command TMP_REG

    ; min delay for chip deselect
    wait_cycles SPI_IDLE_GUARD_CYCLES

    ; increment ADC command list address offset
    ADD     ADC_COMMAND_OFFSET_REG, ADC_COMMAND_OFFSET_REG, 4

    ; decrement command counter and check for init done
    SUB     ADC_COMMAND_COUNT_REG, ADC_COMMAND_COUNT_REG, 1
    QBLT    ADC_SETUP, ADC_COMMAND_COUNT_REG, 0

END_ADC_SETUP:

    ; load configuration and initialize state with user space settings
    LBBO    &MEM_POINTER,    RAM_ADDRESS,    BUFFER0_OFFSET,         4
    LBBO    &SAMPLES_COUNT,  RAM_ADDRESS,    SAMPLE_LIMIT_OFFSET,    4
    LBBO    &BUFFER_SIZE,    RAM_ADDRESS,    BUFFER_LENGTH_OFFSET,   4
    LBBO    &PRECISION,      RAM_ADDRESS,    ADC_PRECISION_OFFSET,   4

    ; initialize buffer index to zero
    ZERO    &BUFFER_INDEX,  4
    
    ; set ADC start signal to one to start ADC conversion
    SET     ADC_OUT_REG, ADC_OUT_REG, START_BIT

    ; jump right into into sampling loop
    JMP     START

SAMPLINGLOOP:
    ; if buffer not full, continue reading next sample
    QBLT    NEXTSAMPLE, BUFFER_SIZE, 0

    ; on full buffer:
    ; signal interrupt to user space program
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

    ; load and check PRU state set by user space
    LBBO    &TMP_REG, RAM_ADDRESS, STATE_OFFSET, 4
    QBBC    STOP, TMP_REG, 0       ; if disabled, stop sampling
    QBBS    NEXTBUFFER, TMP_REG, 1 ; if continuous mode, continue sampling

    ; if finite number of samples acquired, stop sampling
    QBGE    STOP, SAMPLES_COUNT, 0

NEXTBUFFER:
    ; increment buffer index
    ADD     BUFFER_INDEX, BUFFER_INDEX, 1

START:
    ; reload buffer size
    LBBO    &BUFFER_SIZE, RAM_ADDRESS, BUFFER_LENGTH_OFFSET, 4 ; reload buffer size

    ; select data buffer
    QBBC    BUFFER0, BUFFER_INDEX, 0
    
    ; point to buffer1 for odd indexes
    LBBO    &MEM_POINTER, RAM_ADDRESS, BUFFER1_OFFSET, 4
    JMP     BUFFER1
BUFFER0:
    ; point to buffer0 for even indexes
    LBBO    &MEM_POINTER, RAM_ADDRESS, BUFFER0_OFFSET, 4
BUFFER1:

    ; store buffer index
    SBBO    &BUFFER_INDEX, MEM_POINTER, 0, 4
    ADD     MEM_POINTER, MEM_POINTER, BUFFER_INDEX_SIZE


NEXTSAMPLE:
    ; update buffer and total counters
    SUB     BUFFER_SIZE, BUFFER_SIZE, 1

    ; if continuous mode or filling last buffer, skip sample counting
    QBEQ    READ, SAMPLES_COUNT, 0
    SUB     SAMPLES_COUNT, SAMPLES_COUNT, 1

READ:
    ; signal start wait, using status LED as busy wait indicator
    SET     STATUS_OUT_REG, STATUS_OUT_REG, LED_STATUS_BIT

    ; wait for data ready
    WBC     ADC_IN_REG, DR1_BIT

    ; immediately timestamp data
    timestamp_restart DT_REG

    ; signal end wait, using status LED as busy wait indicator
    CLR     STATUS_OUT_REG, STATUS_OUT_REG, LED_STATUS_BIT

    ; read data from ADC
    adc_read

    ; mask digital inputs and store in digital data register
    AND     ADC1_STATUS_REG, ADC1_STATUS_REG, ADC_STATUS_MASK
    AND     ADC2_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_MASK
    LSL     ADC2_STATUS_REG, ADC2_STATUS_REG, 8
    OR      DI_REG, ADC1_STATUS_REG, ADC2_STATUS_REG

    ; extend 16 bit data to 24 bit in case of lower precision data
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
    ; sign extension (24->32bit)
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
    ; combine parallel sampled current channels
    ADD     I1H_REG, I1H_REG, I1H_2_REG
    ADD     I1L_REG, I1L_REG, I1L_2_REG
    ADD     I2H_REG, I2H_REG, I2H_2_REG
    ADD     I2L_REG, I2L_REG, I2L_2_REG

    ; single block transfer to DDR of all data channels
    SBBO    &DI_REG, MEM_POINTER, 0, BUFFER_BLOCK_SIZE
    ADD     MEM_POINTER, MEM_POINTER, BUFFER_BLOCK_SIZE

    JMP     SAMPLINGLOOP

STOP:
    ; clear all ADC signals
    CLR     ADC_OUT_REG, ADC_OUT_REG, START_BIT

    ; set SDATAC command (RREG needed for SDATAC to work!)
    LDI32   TMP_REG, ADC_COMMAND_SDATAC
    adc_send_command TMP_REG
    LDI32   TMP_REG, ADC_COMMAND_RREG
    adc_send_command TMP_REG

    ; signal interrupt to user space program
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

    ; clear status LED
    CLR     STATUS_OUT_REG, STATUS_OUT_REG, LED_STATUS_BIT

    ; halt the PRU
    HALT
