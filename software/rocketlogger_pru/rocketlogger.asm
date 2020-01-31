; Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

; -------------------------- CONSTANT DEFINITIONS --------------------------- ;

; ------------------------- PRU Memory Definitions -------------------------- ;
; PRU control and data RAM address definitions
PRU0_DATA_RAM_BASE          .set    0x00000000
PRU0_CTRL_BASE              .set    0x00022000

; PRU control register definitions
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

; PRU constants table alias definitions
C_PRU_INTC                  .set    C0      ; PRU-ICSS INTC (local) 0x0002_0000
C_PRU_ECAP                  .set    C3      ; PRU-ICSS eCAP (local) 0x0003_0000
C_PRU_CFG                   .set    C4      ; PRU-ICSS CFG (local) 0x0002_6000
C_EHRPWM0                   .set    C18     ; eHRPWM0/eCAP0/eQEP0 0x4830_0000
C_EHRPWM1                   .set    C19     ; eHRPWM1/eCAP1/eQEP1 0x4830_2000
C_EHRPWM2                   .set    C20     ; eHRPWM1/eCAP1/eQEP1 0x4830_4000

; PRU data layout (address offets in PRU local memory)
STATE_OFFSET                .set    0x00
SAMPLE_RATE_OFFSET          .set    0x04
SAMPLE_LIMIT_OFFSET         .set    0x08
BUFFER_LENGTH_OFFSET        .set    0x0C
BUFFER0_ADDR_OFFSET         .set    0x10
BUFFER1_ADDR_OFFSET         .set    0x14

; shared DDR buffer data layout
BUFFER_CHANNEL_COUNT        .set    10
BUFFER_INDEX_SIZE           .set    4
BUFFER_DATA_SIZE            .set    4
BUFFER_BLOCK_SIZE           .set    (BUFFER_CHANNEL_COUNT * BUFFER_DATA_SIZE)


; -------------------------- PRU Register Mappings -------------------------- ;
; Status GPIO in/out register bit definitions
STATUS_OUT_REG              .set    r30
LED_ERROR_BIT               .set    14      ; user space, cleared on start
LED_STATUS_BIT              .set    15      ; user space, busy wait indicator

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
DT_REG                      .set    r9      ; PRU cycle counter

; PRU register definitions: double sampled channel data from ADC
I1L_2_REG                   .set    r10
I1H_2_REG                   .set    r11
I2L_2_REG                   .set    r12
I2H_2_REG                   .set    r13

; PRU register definitions: unused medium current channel data from ADC
I1M_REG                     .set    r14     ; unused ADC data
I2M_REG                     .set    r15     ; unused ADC data

; PRU register definitions: temporary registers
TMP_REG                     .set    r16

; PRU register definitions: temporary macro register
MTMP_REG                    .set    r17

; PRU register definitions: data memory addresses
RAM_ADDRESS                 .set    r29
CTRL_ADDRESS                .set    r28

; PRU register definitions: PRU local sampling state
BUFFER_SIZE                 .set    r27
BUFFER_INDEX                .set    r26
SAMPLES_COUNT               .set    r25
MEM_POINTER                 .set    r24
PRECISION                   .set    r23

; PRU register definitions: PRU user space configuration
SAMPLE_RATE                 .set    r22

; registers r18-r21 unused

; PRU register definitions: ADC status and configuration register aliases
ADC1_STATUS_REG             .set    DI_REG
ADC2_STATUS_REG             .set    TMP_REG


; ---------------------- CLOCK MANAGEMENT Definitions ----------------------- ;
; clock managment register offsets
CM_PER_BASE                 .set    0x44E00000
CM_PER_EPWMSS1_OFFSET       .set    0xCC
CM_PER_EPWMSS0_OFFSET       .set    0xD4
CM_PER_EPWMSS2_OFFSET       .set    0xD8

; clock managements register values (selection)
CM_MODULEMODE_DISABLE       .set    0x00
CM_MODULEMODE_ENABLE        .set    0x02
CM_IDLESTATUS_FUNC          .set    0x000000
CM_IDLESTATUS_TRANS         .set    0x010000
CM_IDLESTATUS_IDLE          .set    0x020000
CM_IDLESTATUS_DISABLE       .set    0x030000


; --------------------------- CONTROL Definitions --------------------------- ;
; control register offsets and PWMSS values (selection)
CONTROL_PWMSS_CTRL          .set    0x44E10664
PWMSS_CTRL_PWMSS_RESET      .set    0x00
PWMSS_CTRL_PWMSS0_TBCLKEN   .set    0x01
PWMSS_CTRL_PWMSS1_TBCLKEN   .set    0x02
PWMSS_CTRL_PWMSS2_TBCLKEN   .set    0x04


; ----------------------------- PWM Definitions ----------------------------- ;
; ePWM module and configuration register offsets
EPWM0_BASE                  .set    0x48300200  ; ePWM0 module base address
EPWM1_BASE                  .set    0x48302200  ; ePWM1 module base address
EPWM2_BASE                  .set    0x48304200  ; ePWM2 module base address
EPWM_TBCTL_OFFSET           .set    0x00    ; Time-Base Control Register
EPWM_TBSTS_OFFSET           .set    0x02    ; Time-Base Status Register
EPWM_TBPHS_OFFSET           .set    0x06    ; Time-Base Phase Register
EPWM_TBCNT_OFFSET           .set    0x08    ; Time-Base Counter Register
EPWM_TBPRD_OFFSET           .set    0x0A    ; Time-Base Period Register
EPWM_CMPCTL_OFFSET          .set    0x0E    ; Counter-Compare Control Register
EPWM_CMPA_OFFSET            .set    0x12    ; Counter-Compare A Register
EPWM_CMPB_OFFSET            .set    0x14    ; Counter-Compare B Register
EPWM_AQCTLA_OFFSET          .set    0x16    ; Action-Qualifier Control Register for Output A (EPWMxA)
EPWM_AQCTLB_OFFSET          .set    0x18    ; Action-Qualifier Control Register for Output B (EPWMxB)

; ePWM configuration register values (selection)
TBCTL_FREERUN               .set    0xC000  ; TBCTL default value after reset
TBCTL_COUNT_UP              .set    0x0000  ; Up counting mode
TBCTL_COUNT_UP_DOWN         .set    0x0002  ; Up-down counting mode
TBCTL_PRDLD                 .set    0x0008  ; Period register double buffer disable
TBCTL_CLKDIV_1              .set    0x0000  ; Use counter clock prescaler of 1
TBCTL_CLKDIV_2              .set    0x0400  ; Use counter clock prescaler of 2

AQ_ZROCLR                   .set    0x0001  ; Action qualifier: clear on zero
AQ_ZROSET                   .set    0x0002  ; Action qualifier: set on zero
AQ_PRDCLR                   .set    0x0004  ; Action qualifier: clear on period
AQ_PRDSET                   .set    0x0008  ; Action qualifier: clear set on period
AQ_A_INCCLR                 .set    0x0010  ; Action qualifier: clear on compare A when incrementing
AQ_A_INCSET                 .set    0x0020  ; Action qualifier: set on compare A when incrementing
AQ_A_DECCLR                 .set    0x0040  ; Action qualifier: clear on compare A when decrementing
AQ_A_DECSET                 .set    0x0080  ; Action qualifier: set on compare A when decrementing
AQ_B_INCCLR                 .set    0x0100  ; Action qualifier: clear on compare B when incrementing
AQ_B_INCSET                 .set    0x0200  ; Action qualifier: set on compare B when incrementing
AQ_B_DECCLR                 .set    0x0400  ; Action qualifier: clear on compare B when decrementing
AQ_B_DECSET                 .set    0x0800  ; Action qualifier: set on compare B when decrementing

; ePWM configuration values for ADC clock and range reset pulses
PWM_ADC_CLOCK_PERIOD        .set    49              ; ADC master clock period (in units of 10 ns)
PWM_RANGE_RESET_PERIOD_BASE .set    (50000 + 5000)  ; Range latch base period, 0.5 kHz + 10% margin (in units of 10 ns)
PWM_RANGE_RESET_PULSE_WIDTH .set    50              ; Range latch reset pulse width (in units of 10 ns)


; ----------------------------- SPI Definitions ----------------------------- ;
; SPI delay cycle definitions
SPI_SELECT_GUARD_CYCLES     .set    4       ; min. 20ns after CS before SCLK rise
SPI_DESELECT_GUARD_CYCLES   .set    400     ; min. 4 t_CLK (2us) before CS rise
SPI_DECODE_GUARD_CYCLES     .set    320     ; min. 4 t_CLK (2us) total burst period
SPI_IDLE_GUARD_CYCLES       .set    200     ; min. 2 t_CLK (1us) CS high


; ----------------------------- ADC Definitions ----------------------------- ;
; ADC command and delay cycle definitions
ADC_COMMAND_RESET           .set    0x06    ; reset configuration
ADC_COMMAND_RDATAC          .set    0x10    ; enable continuous data mode
ADC_COMMAND_SDATAC          .set    0x11    ; disable continuous data mode
ADC_COMMAND_RREG            .set    0x20    ; read register data
ADC_COMMAND_WREG            .set    0x40    ; write register data
ADC_RESET_CYCLES            .set    1800    ; min. 18 t_CLK (9us) to perform reset
ADC_SDATAC_CYCLES           .set    400     ; min. 4 t_CLK (2us) to process

; ADC register addresses and value definitions
ADC_REGISTER_ID             .set    0x00
ADC_REGISTER_CONFIG1        .set    0x01
ADC_REGISTER_CONFIG2        .set    0x02
ADC_REGISTER_CONFIG3        .set    0x03
ADC_REGISTER_CH1SET         .set    0x05
ADC_REGISTER_CH2SET         .set    0x06
ADC_REGISTER_CH3SET         .set    0x07
ADC_REGISTER_CH4SET         .set    0x08
ADC_REGISTER_CH5SET         .set    0x09
ADC_REGISTER_CH6SET         .set    0x0A
ADC_REGISTER_CH7SET         .set    0x0B
ADC_REGISTER_CH8SET         .set    0x0C

; ADC register value definitions and constants
ADC_CONFIG1_VALUE           .set    0x90    ; no daisy-chain, clock out, data rate omitted
ADC_CONFIG2_VALUE           .set    0xE0    ; reset value of test configuration
ADC_CONFIG3_VALUE           .set    0xE8    ; 4V reference, internal opamp ref
ADC_CONFIG1_DR_BASE         .set    0x06    ; base value for DR calcualtion
ADC_CHANNEL_GAIN1           .set    0x10
ADC_CHANNEL_GAIN2           .set    0x20
ADC_PRECISION_LOW_BITS      .set    16      ; data bits for >= 32 kSPS sample rate
ADC_PRECISION_HIGH_BITS     .set    24      ; data bits for < 32 kSPS sample rate
ADC_STATUS_BITS             .set    24
ADC_STATUS_GPIO_MASK        .set    0x0F    ; for all 4 digital inputs
; --------------------------------------------------------------------------- ;


; ---------------------------- MACRO DEFINITIONS ---------------------------- ;

; ------------------------------- Time Macros ------------------------------- ;
; WAIT CYCLES
; (wait for defined number of `cycles`, min. 3 cycles)
wait_cycles .macro cycles
    LDI     MTMP_REG.w1, cycles
    SUB     MTMP_REG.w1, MTMP_REG.w1, 3
    LOOP    ENDLOOP?, MTMP_REG.w1
    NOP
ENDLOOP?:
    .endm


; START CYCLE COUNTER
; (overwrite LSB of control register, 5 cycles start delay)
start_counter .macro
    LDI     MTMP_REG, PRU_CRTL_CTR_ON
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
    .endm


; STOP CYCLE COUNTER
; (overwrite LSB of control register, 5 cycles stop delay)
stop_counter .macro
    LDI     MTMP_REG, PRU_CRTL_CTR_OFF
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
    .endm


; TIMESTAMP AND RESTART
; (store cycle count to register and restart counter)
timestamp_restart .macro reg
    stop_counter
    LBBO    &reg, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4      ; read counter value
    LDI     MTMP_REG, PRU_CYCLE_RESET                    ; load reset value
    SBBO    &MTMP_REG, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4 ; reset counter
    start_counter
    .endm


; ------------------------------- SPI Macros -------------------------------- ;
; SPI RESET
; (reset SPI output signal to idle levels)
spi_reset .macro
    SET     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT   ; both CS high
    SET     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, MOSI_BIT  ; MOSI low
    CLR     ADC_OUT_REG, ADC_OUT_REG, SCLK_BIT  ; SCLK low
    .endm


; SPI WRITE BYTE
; (write SPI data block of 8 bits, optimized for timing)
spi_write_byte .macro data_reg
    ; setup SPI bit write loop
    LOOP    END_SPI_BIT_WRITE?, 8

    ; clock high phase (5 cycles total, offset by 1 cycle for QBBC)

    ; set SPI output data (3 cycles)
    QBBC    OUTLOW?, data_reg, 7
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
    ; MOSI idle low (optional, helps debugging)
    CLR     ADC_OUT_REG, ADC_OUT_REG, MOSI_BIT   
    .endm


; SPI READ DATA
; (read SPI data block of `size` bits, optimized for timing, no CS handling)
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


; ------------------------------- ADC Macros -------------------------------- ;
; ADC SEND COMMAND
; (send non-data ADC command defined in `command` constant)
adc_send_command .macro command

    ; start ADC command frame with (negative) chip select
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    wait_cycles SPI_SELECT_GUARD_CYCLES

    ; load and send ADC command byte to SPI
    LDI     MTMP_REG.b0, command
    spi_write_byte MTMP_REG.b0

    ; end ADC command frame with (positive) chip de-select
    wait_cycles SPI_DESELECT_GUARD_CYCLES
    SET     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    SET     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    .endm


; ADC WRITE DATA REGISTER
; (write register at address `register` with 8 bit data in register `data_reg`)
adc_write_register .macro register, data_reg
    ; load command bytes to temporary register
    LDI     TMP_REG.b0, ADC_COMMAND_WREG
    LDI     TMP_REG.b1, register
    OR      TMP_REG.b0, TMP_REG.b0, TMP_REG.b1
    ZERO    &TMP_REG.b1, 1
    AND     TMP_REG.b2, data_reg, data_reg

    ; start ADC command frame with (negative) chip select
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    wait_cycles SPI_SELECT_GUARD_CYCLES

    ; write ADC command to SPI (byte 1)
    spi_write_byte TMP_REG.b0
    wait_cycles SPI_DECODE_GUARD_CYCLES

    ; write ADC command to SPI (byte 2)
    spi_write_byte TMP_REG.b1
    wait_cycles SPI_DECODE_GUARD_CYCLES

    ; write ADC command to SPI (byte 3)
    spi_write_byte TMP_REG.b2

    ; end ADC command frame with (positive) chip de-select
    wait_cycles SPI_DESELECT_GUARD_CYCLES
    SET     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    SET     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    .endm


; ADC INITIALIZATION
; (initialize ADC for sampling at a rate of `sample_rate_reg` ksps)
adc_init .macro sample_rate_reg
    ; reset ADC START signal
    CLR     ADC_OUT_REG, ADC_OUT_REG, START_BIT

    ; initialize SPI interface
    spi_reset
    wait_cycles SPI_IDLE_GUARD_CYCLES

    ; reset ADC configuration
    adc_send_command ADC_COMMAND_RESET
    wait_cycles ADC_RESET_CYCLES

    ; disable continuous data output mode
    adc_send_command ADC_COMMAND_SDATAC
    wait_cycles ADC_SDATAC_CYCLES

    ; set ADC precision configuration
    LDI     PRECISION, ADC_PRECISION_HIGH_BITS
    QBGT    ADC_INIT_REG?, sample_rate_reg, 32
    LDI     PRECISION, ADC_PRECISION_LOW_BITS
ADC_INIT_REG?:

    ; calculate sample rate configuration, i.e. 0x06 - log2(sample_rate_reg)
    LDI     MTMP_REG.b1, ADC_CONFIG1_DR_BASE
    LMBD    MTMP_REG.b0, sample_rate_reg, 1     ; = floor(log2(sample_rate_reg))
    SUB     MTMP_REG.b1, MTMP_REG.b1, MTMP_REG.b0

    ; set register CONFIG1
    LDI     MTMP_REG.b0, ADC_CONFIG1_VALUE
    OR      MTMP_REG.b0, MTMP_REG.b0, MTMP_REG.b1       ; data rate
    adc_write_register ADC_REGISTER_CONFIG1, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES

    ; set register CONFIG2
    LDI     MTMP_REG.b0, ADC_CONFIG2_VALUE
    adc_write_register ADC_REGISTER_CONFIG2, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES

    ; set register CONFIG3
    LDI     MTMP_REG.b0, ADC_CONFIG3_VALUE
    adc_write_register ADC_REGISTER_CONFIG3, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES

    ; set channel configurations: gain 2 for high currents, gain 1 for others
    ; HW channel order: IHA, IHB, IM, ILA, ILB, VB, VA, CH8 unused
    LDI     MTMP_REG.b0, ADC_CHANNEL_GAIN2
    adc_write_register ADC_REGISTER_CH1SET, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES
    adc_write_register ADC_REGISTER_CH2SET, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES

    LDI     MTMP_REG.b0, ADC_CHANNEL_GAIN1
    adc_write_register ADC_REGISTER_CH3SET, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES
    adc_write_register ADC_REGISTER_CH4SET, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES
    adc_write_register ADC_REGISTER_CH5SET, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES
    adc_write_register ADC_REGISTER_CH6SET, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES
    adc_write_register ADC_REGISTER_CH7SET, MTMP_REG.b0
    wait_cycles SPI_IDLE_GUARD_CYCLES

    ; enable continuous data output mode again
    adc_send_command ADC_COMMAND_RDATAC
    .endm


; ADC START SAMPLING
; (set ADC start signal to start sampling)
adc_start .macro
    SET     ADC_OUT_REG, ADC_OUT_REG, START_BIT
    .endm


; ADC STOP SAMPLING
; (clear ADC start signal to stop sampling)
adc_stop .macro
    CLR     ADC_OUT_REG, ADC_OUT_REG, START_BIT
    .endm


; ADC READ CONTINUOUS
; (read ADC data frame to pre-assigned registers)
adc_read_continuous .macro
    ; start ADC data transfer frame with (negative) chip select
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS1_BIT
    CLR     ADC_OUT_REG, ADC_OUT_REG, CS2_BIT
    wait_cycles SPI_SELECT_GUARD_CYCLES

    ; read status data
    spi_read_data ADC1_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_BITS 

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


; ------------------------------- PWM Macros -------------------------------- ;
; PWM MODULE INITIALIZATION
; (setup/enable PWM clock sources and signals)
pwm_init .macro
; DO NOTHING HERE => HANDLED BY SYSFS INTERFACE
;    ; load CM_PER base address
;    LDI32   TMP_REG, CM_PER_BASE
;
;    ; enable EPWMSS0 and EPWMSS1 interface clocks
;    LDI32   MTMP_REG, CM_IDLESTATUS_FUNC | CM_MODULEMODE_ENABLE
;    SBBO    &MTMP_REG, TMP_REG, CM_PER_EPWMSS0_OFFSET, 4
;    SBBO    &MTMP_REG, TMP_REG, CM_PER_EPWMSS1_OFFSET, 4
;
;    ; enable EPWMSS0 and EPWMSS1 counter clocks (handled via sysfs driver)
;    ;LDI32   TMP_REG, CONTROL_PWMSS_CTRL
;    ;LDI     MTMP_REG, PWMSS_CTRL_PWMSS1_TBCLKEN | PWMSS_CTRL_PWMSS0_TBCLKEN
;    ;SBBO    &MTMP_REG, TMP_REG, 0, 2

    .endm


; PWM MODULE DE-INITIALIZATION
; (reset/disable PWM clock sources and signals)
pwm_deinit .macro
; DO NOTHING HERE => HANDLED BY SYSFS INTERFACE
;    ; load CM_PER base address
;    LDI32   TMP_REG, CM_PER_BASE
;
;    ; disable EPWMSS0 and EPWMSS1 interface clocks
;    LDI32   MTMP_REG, CM_IDLESTATUS_DISABLE | CM_MODULEMODE_DISABLE
;    SBBO    &MTMP_REG, TMP_REG, CM_PER_EPWMSS0_OFFSET, 4
;    SBBO    &MTMP_REG, TMP_REG, CM_PER_EPWMSS1_OFFSET, 4
;
;    ; disable EPWMSS0 and EPWMSS1 counter clocks (handled via sysfs driver)
;    LDI32   TMP_REG, CONTROL_PWMSS_CTRL
;    LDI     MTMP_REG, PWMSS_CTRL_PWMSS_RESET
;    SBBO    &MTMP_REG, TMP_REG, 0, 2

    .endm


; PWM MODULE STOP
; (stop PWM counter and outputs)
pwm_stop .macro
    ; initialize value register to zero
    ZERO    &MTMP_REG, 4

    ; load EPWM0 base address from constants memory
    LDI32   TMP_REG, EPWM0_BASE

    ; reset EPWM0 config and counters to initial zero state
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBCTL_OFFSET, 2
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBPRD_OFFSET, 2
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBCNT_OFFSET, 2

    ; load EPWM1 base address from constants memory
    LDI32   TMP_REG, EPWM1_BASE

    ; reset EPWM1 config and counters to initial zero state
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBCTL_OFFSET, 2
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBPRD_OFFSET, 2
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBCNT_OFFSET, 2

    .endm


; SETUP PWM FOR ADC CLOCK GENERATION
; (configure PWM for ADC clock generation)
pwm_setup_adc_clock .macro
    ; load EPWM0 base address from constants memory
    LDI32   TMP_REG, EPWM0_BASE

    ; set period and compare register values (interval = TBPRD + 1)
    LDI     MTMP_REG, (PWM_ADC_CLOCK_PERIOD - 1)
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBPRD_OFFSET, 2
    LSR     MTMP_REG, MTMP_REG, 1
    SBBO    &MTMP_REG, TMP_REG, EPWM_CMPA_OFFSET, 2

    ; set action qualifiers for both channels
    LDI     MTMP_REG, AQ_A_INCSET | AQ_ZROCLR
    SBBO    &MTMP_REG, TMP_REG, EPWM_AQCTLA_OFFSET, 2

    ; set clock prescaler 1 and up counting mode, no period double buffering
    LDI     MTMP_REG, TBCTL_FREERUN | TBCTL_CLKDIV_1 | TBCTL_PRDLD | TBCTL_COUNT_UP
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBCTL_OFFSET, 2

    .endm


; SETUP PWM FOR RANGE VALID RESET SIGNALS
; (configure PWM for range reset signals for a sample rate of `sample_rate_reg`)
pwm_setup_range_valid_reset .macro sample_rate_reg

    ; calculate period from samle rate
    LDI32   MTMP_REG, (PWM_RANGE_RESET_PERIOD_BASE)
    LMBD    TMP_REG, sample_rate_reg, 1   ; get exponent of 2 of the sample rate
    LSR     MTMP_REG, MTMP_REG, TMP_REG

    ; load EPWM1 base address from constants memory
    LDI32   TMP_REG, EPWM1_BASE

    ; store calculated period value
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBPRD_OFFSET, 2

    ; set compare register values (first value depends on period)
    SUB     MTMP_REG, MTMP_REG, (PWM_RANGE_RESET_PULSE_WIDTH / 2)
    SBBO    &MTMP_REG, TMP_REG, EPWM_CMPA_OFFSET, 2
    LDI     MTMP_REG, (PWM_RANGE_RESET_PULSE_WIDTH / 2)
    SBBO    &MTMP_REG, TMP_REG, EPWM_CMPB_OFFSET, 2

    ; set action qualifiers for both channels
    LDI     MTMP_REG, AQ_A_INCSET | AQ_A_DECCLR
    SBBO    &MTMP_REG, TMP_REG, EPWM_AQCTLA_OFFSET, 2
    LDI     MTMP_REG, AQ_B_INCCLR | AQ_B_DECSET
    SBBO    &MTMP_REG, TMP_REG, EPWM_AQCTLB_OFFSET, 2

    ; configure PWM operational mode
    LDI     MTMP_REG, TBCTL_FREERUN | TBCTL_CLKDIV_2 | TBCTL_PRDLD | TBCTL_COUNT_UP_DOWN
    SBBO    &MTMP_REG, TMP_REG, EPWM_TBCTL_OFFSET, 2

    .endm


; ------------------------------- Misc Macros ------------------------------- ;
; SIGN EXTEND 24 BIT
; (sign extend 24 bit data to 32 bit)
sign_extend_24 .macro reg
    LDI     reg.b3, 0x00
    QBBC    POS?, reg, 23
    LDI     reg.b3, 0xFF
POS?:
    .endm
; --------------------------------------------------------------------------- ;


; ------------------------------ MAIN PROGRAM ------------------------------- ;
    .text
    .retain ".text"

    .global MAIN
MAIN:
    ; enable the OCP master port -- allows transfer of data to Linux userspace
    LBCO    &TMP_REG, C_PRU_CFG, 4, 4   ; load SYSCFG reg (use c4 const addr)
    CLR     TMP_REG, TMP_REG, 4         ; clear bit 4 of SYSCFG (STANDBY_INIT)
    SBCO    &TMP_REG, C_PRU_CFG, 4, 4   ; store back modified SYSCFG value
 
    LDI32   CTRL_ADDRESS, PRU0_CTRL_BASE    ; load the CTRL base addr  ess
    LDI32   RAM_ADDRESS, PRU0_DATA_RAM_BASE ; load the RAM base address

    ; load configuration and initialize state with user space settings
    LBBO    &SAMPLE_RATE,   RAM_ADDRESS,    SAMPLE_RATE_OFFSET,     4
    LBBO    &BUFFER_SIZE,   RAM_ADDRESS,    BUFFER_LENGTH_OFFSET,   4
    LBBO    &SAMPLES_COUNT, RAM_ADDRESS,    SAMPLE_LIMIT_OFFSET,    4
    LBBO    &MEM_POINTER,   RAM_ADDRESS,    BUFFER0_ADDR_OFFSET,    4

INIT:
    ; initialize GPIO states
    CLR     STATUS_OUT_REG, STATUS_OUT_REG, LED_ERROR_BIT   ; reset error LED

    ; initialize PWM modules and enable ADC clock signal
    pwm_init
    pwm_setup_adc_clock

    ; initialize the ADC
    adc_init SAMPLE_RATE

    ; configure PWM for range reset signals
    pwm_setup_range_valid_reset SAMPLE_RATE

INIT_COMPLETE:
    ; initialize buffer index to zero
    ZERO    &BUFFER_INDEX,  4
    
    ; start ADC conversion
    adc_start

    ; notify user-space program of measurement start
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

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
    LBBO    &MEM_POINTER, RAM_ADDRESS, BUFFER1_ADDR_OFFSET, 4
    JMP     BUFFER1
BUFFER0:
    ; point to buffer0 for even indexes
    LBBO    &MEM_POINTER, RAM_ADDRESS, BUFFER0_ADDR_OFFSET, 4
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
    adc_read_continuous

    ; mask digital inputs and store in digital data register
    AND     ADC1_STATUS_REG, ADC1_STATUS_REG, ADC_STATUS_GPIO_MASK
    AND     ADC2_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_GPIO_MASK
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
    ; stop ADC sampling
    adc_stop

    ; signal interrupt to user space program
    LDI     r31.b0, PRU_R31_VEC_VALID | PRU_EVTOUT_0

    ; stop and deinitialize PWM modules
    pwm_stop
    pwm_deinit

    ; clear status LED
    CLR     STATUS_OUT_REG, STATUS_OUT_REG, LED_STATUS_BIT

    ; halt the PRU
    HALT
; --------------------------------------------------------------------------- ;
