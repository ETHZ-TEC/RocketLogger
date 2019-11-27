/**
 * Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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

.origin 0                        // start of program in PRU memory
.entrypoint MAIN                 // program entry point

// PRU control register defines for cycle counter
#define PRU0_CTRL_BASE      0x00022000
#define PRU_CTRL_OFFSET     0x00
#define PRU_CYCLE_OFFSET    0x0C
#define PRU_CYCLE_RESET     0x0000

// PRU control register LSB defines for counter ON/OFF control
#define PRU_CRTL_CTR_ON     0x0B
#define PRU_CRTL_CTR_OFF    0x03

#define PRU0_R31_VEC_VALID  32   // allows notification of program completion
#define PRU_EVTOUT_0        3    // the event number that is sent back

// ADC commands
#define ADC_SDATAC 0x11000000
#define ADC_RREG   0x20000000

// commands
#define ADC_COMMAND_SIZE         24
#define ADC_COMMAND_WAIT_CYCLES  5  


// ADC status size and mask
#define ADC_STATUS_SIZE  24
#define ADC_STATUS_MASK  0xF//0x1 // new: 0xF (for all 4 digital inputs)

// total channel count
#define CHANNEL_COUNT             9

// pru data layout (position in memory)
#define STATE_OFFSET              0x00
#define BUFFER0_OFFSET            0x04
#define BUFFER1_OFFSET            0x08
#define BUFFER_LENGTH_OFFSET      0x0C
#define SAMPLE_LIMIT_OFFSET       0x10
#define ADC_PRECISION_OFFSET      0x14
#define ADC_COMMAND_COUNT_OFFSET  0x18
#define ADC_COMMAND_BASE_OFFSET   0x1C

// buffer data layout
#define BUFFER_INDEX_SIZE   4
#define BUFFER_DIGITAL_SIZE 4
#define BUFFER_DATA_SIZE    4

#define BUFFER_BLOCK_SIZE   (BUFFER_DIGITAL_SIZE + \
                             CHANNEL_COUNT * BUFFER_DATA_SIZE)

// gpio register bits
#define SCLK        r30.t0
#define MOSI        r30.t1
#define MISO1       r31.t2
#define MISO2       r31.t16
#define CS1         r30.t3
#define CS2         r30.t5
#define START_PIN   r30.t7
#define DR1         r31.t15
#define DR2         r31.t14 // unused
#define LED_ERROR   r30.t14 // user space controlled, PRU mapped for analysis
#define LED_STATUS  r30.t15 // user space controlled


// value register assignment
#define ADC1_STATUS_REG r10

// analog channel registers
#define V1_REG  r11
#define V2_REG  r12
#define V3_REG  r13
#define V4_REG  r14
#define I2L_REG r15
#define I1L_REG r16
#define I1H_REG r17
#define I2H_REG r18

// PRU cycle counter register
#define CNT_REG r19

// unused medium current channels
#define I1M_REG r20 // unused
#define I2M_REG r21 // unused

// over sampling and second digital channels for aggregation
#define ADC2_STATUS_REG r22
#define I1H_2_REG r23
#define I1L_2_REG r24
#define I2H_2_REG r25
#define I2L_2_REG r26

// other registers
#define STATE               r0
#define BASE_ADDRESS        r1
#define ADC_CMD             r2
#define PRECISION           r3
#define LOOP_VAR            r4
#define BUFFER_INDEX        r5
#define BUFFER_SIZE         r6
#define ADC_COMMAND_COUNT   r7 // could be reused?
#define MEM_POINTER         r8
#define SAMPLES_COUNT       r9
#define ADC_COMMAND_OFFSET  r11 // is reused by V1
// channel input regs          r11-r25
// status regs                 r10,r26
#define CTRL_ADDRESS        r27
#define WAIT_VAR            r28
#define TMP                 r29


// nop
#define NOP ADD r0, r0, 0
 
 
// --------------------------- Macros ------------------------------------------ //
 
 
// WAIT
.macro  wait
.mparam time

    mov WAIT_VAR, time
WAITLOOP:
    QBEQ ENDWAIT, WAIT_VAR, 0
    SUB WAIT_VAR, WAIT_VAR, 1
    QBA WAITLOOP
ENDWAIT:

.endm


// START CYCLE COUNTER (overwrite LSB of control register), 5 cycles start delay
.macro start_counter
    LDI     TMP, PRU_CRTL_CTR_ON
    SBBO    &TMP, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
.endm


// STOP CYCLE COUNTER (overwrite LSB of control register), 5 cycles stop delay
.macro stop_counter
    LDI     TMP, PRU_CRTL_CTR_OFF
    SBBO    &TMP, CTRL_ADDRESS, PRU_CTRL_OFFSET, 1
.endm

// TIMESTAMP AND RESTART (store cycle count to register and restart counter)
.macro timestamp_restart
.mparam reg
    stop_counter
    LBBO    &reg, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4 // read counter value
    LDI     TMP, PRU_CYCLE_RESET                    // load counter reset value
    SBBO    &TMP, CTRL_ADDRESS, PRU_CYCLE_OFFSET, 4 // reset counter
    start_counter
.endm


// COMMAND CYCLE (SPI cycle when sending commands)
.macro  command_cycle   
.mparam oup, inp


// clock high phase
    SET SCLK // set the clock high

    QBBC OUPLOW, oup.t31 // write data
    SET MOSI
    QBA OUPHIGH
OUPLOW:
    CLR MOSI
    NOP // to have fixed timing
OUPHIGH:
    LSL oup, oup, 1 // shift output register

    wait ADC_COMMAND_WAIT_CYCLES

// clock low phase
    CLR SCLK // set the clock low

    QBBC INPLOW, MISO1 // get data
    OR inp, inp, 0x1 // TODO: remove
    QBA INPHIGH
INPLOW:
    NOP
    NOP
INPHIGH:
    LSL inp, inp, 1 //shift input register

    wait ADC_COMMAND_WAIT_CYCLES

.endm
 
 
// SEND COMMAND (send commands and write registers)
.macro  send_command   
.mparam cmd

    CLR CS1 // set the CS line low (active low)
    CLR CS2
    MOV LOOP_VAR, ADC_COMMAND_SIZE // #bits to write

CMD_BIT:                    // loop
    SUB LOOP_VAR, LOOP_VAR, 1
    command_cycle cmd, r10  // perform command_cycle
    QBNE CMD_BIT, LOOP_VAR, 0

    wait 1000 // need to wait for ADC

    SET CS1 // pull the CS line high (end of sample)
    SET CS2

.endm


// RECEIVE WORD (receive a sample, this is a bit ugly, since it was optimized for time)
.macro receive_word
.mparam inp0, inp1, prec

    MOV LOOP_VAR, prec  // load precision
    SET SCLK            // set the clock high
RECEIVE_BIT:            // loop

    // clock high phase
    SUB LOOP_VAR, LOOP_VAR, 1
    LSL inp0, inp0, 1
    LSL inp1, inp1, 1

    NOP

    // clock low phase
    CLR SCLK // set the clock low
    NOP

    // get data1
    QBBS INP0HIGH, MISO1
    NOP
    QBA INP0LOW

INP0HIGH:
    OR inp0, inp0, 0x1

INP0LOW:

    // get data2
    QBBS INP1HIGH, MISO2 
    NOP
    QBA INP1LOW

INP1HIGH:
    OR inp1, inp1, 0x1

INP1LOW:
    SET SCLK // set the clock high
    QBNE RECEIVE_BIT, LOOP_VAR, 0

.endm


// RECEIVE DATA (receive all samples)
.macro receive_data

    CLR CS1 // set the CS line low (active low)
    CLR CS2

    wait 1

    // read status word
    receive_word ADC1_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_SIZE 

    // read all channels
    receive_word I1H_REG,   I2H_REG,    PRECISION
    receive_word I1H_2_REG, I2H_2_REG,  PRECISION
    receive_word I1M_REG,   I2M_REG,    PRECISION
    receive_word I1L_REG,   I2L_REG,    PRECISION
    receive_word I1L_2_REG, I2L_2_REG,  PRECISION
    receive_word V1_REG,    V3_REG,     PRECISION
    receive_word V2_REG,    V4_REG,     PRECISION

    SET CS1 // pull the CS line high (end of sample)
    SET CS2

.endm


// SIGN EXTEND (24 to 32 bit)
.macro sign_extend_24
.mparam reg

    MOV reg.b3, 0x00
    QBBC POS, reg.t23

    MOV reg.b3, 0xFF

POS:

.endm

 
// --------------------------- Main Program ------------------------------------------ //
 
MAIN:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
    LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
    CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
    SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr
 
    MOV BASE_ADDRESS, 0x0    // load the base address into r1
    MOV CTRL_ADDRESS, PRU0_CTRL_BASE    // load the CTRL base address to register

    CLR LED_ERROR   // reset error indicator
    SET LED_STATUS  // indicate sampling active

    SET CS1         // CS high
    SET CS2
    CLR MOSI        // MOSI low
    CLR START_PIN   // START low
    MOV r10, 0      // receive register (unused)

    // notify user-space program
    MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0


    // load number of commands to set
    LBBO ADC_COMMAND_COUNT, BASE_ADDRESS, ADC_COMMAND_COUNT_OFFSET, 4

    // load command memory position
    MOV ADC_COMMAND_OFFSET, ADC_COMMAND_BASE_OFFSET

CONFIGURE:
    // check if all commands sent
    QBEQ LOAD, ADC_COMMAND_COUNT, 0
    SUB ADC_COMMAND_COUNT, ADC_COMMAND_COUNT, 1

    // send command
    LBBO ADC_CMD, BASE_ADDRESS, ADC_COMMAND_OFFSET, 4
    send_command ADC_CMD
    ADD ADC_COMMAND_OFFSET, ADC_COMMAND_OFFSET, 4 // increase command position

    wait 1000 // enough time for cs toggle and command to settle

    LSR r10, r10, 1

    QBA CONFIGURE

LOAD:
    // load configuration
    LBBO MEM_POINTER,       BASE_ADDRESS,   BUFFER0_OFFSET,         4   // buffer0 address
    LBBO SAMPLES_COUNT,     BASE_ADDRESS,   SAMPLE_LIMIT_OFFSET,    4   // number of samples to get
    LBBO BUFFER_SIZE,       BASE_ADDRESS,   BUFFER_LENGTH_OFFSET,   4   // buffer size
    LBBO PRECISION,         BASE_ADDRESS,   ADC_PRECISION_OFFSET,   4   // precision

RUN:
    // initialize buffer index to zero
    MOV BUFFER_INDEX,      0
    
    // set ADC start signal to one to start ADC conversion
    SET START_PIN

    // jump right into into sampling loop
    QBA START

SAMPLINGLOOP:
    // if buffer not full, continue reading next sample
    QBNE NEXTSAMPLE, BUFFER_SIZE, 0

    // on full buffer:
    // post interrupt to user space program
    MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0

    // load state
    LBBO STATE, BASE_ADDRESS, STATE_OFFSET, 4

    // if not enabled anymore, stop sampling
    QBBC STOP, STATE.t0

    // if in continuous mode, continue sampling
    QBBS NEXTBUFFER, STATE.t1

    // if finite number of samples acquired, stop sampling
    QBEQ STOP, SAMPLES_COUNT, 0

NEXTBUFFER:
    // increment buffer index
    ADD BUFFER_INDEX, BUFFER_INDEX, 1

START:
    // reload buffer size
    LBBO BUFFER_SIZE, BASE_ADDRESS, BUFFER_LENGTH_OFFSET, 4 // reload buffer size

    // select data buffer
    QBBC BUFFER0, BUFFER_INDEX.t0
    
    // point to buffer1 for odd indexes
    LBBO MEM_POINTER, BASE_ADDRESS, BUFFER1_OFFSET, 4
    QBA BUFFER1
BUFFER0:
    // point to buffer0 for even indexes
    LBBO MEM_POINTER, BASE_ADDRESS, BUFFER0_OFFSET, 4
BUFFER1:

    // store buffer index
    SBBO BUFFER_INDEX, MEM_POINTER, 0, 4
    ADD MEM_POINTER, MEM_POINTER, BUFFER_INDEX_SIZE


NEXTSAMPLE:
    // update buffer and total counters
    SUB BUFFER_SIZE, BUFFER_SIZE, 1

    // if continuous mode or filling last buffer, skip sample counting
    QBEQ READ, SAMPLES_COUNT, 0
    SUB SAMPLES_COUNT, SAMPLES_COUNT, 1

READ:
    // reset input register
    MOV r10, 0 // TODO: remove

//    SET LED_ERROR // use erorr LED for busy wait analysis, signaling start

    // wait for data ready
    WBC DR1

    // immediately timestamp data
    timestamp_restart CNT_REG

//    CLR LED_ERROR // use erorr LED for busy wait analysis, signaling end

    // receive data
    receive_data

    // mask and store status (low range valid)
    AND ADC1_STATUS_REG, ADC1_STATUS_REG, ADC_STATUS_MASK
    AND ADC2_STATUS_REG, ADC2_STATUS_REG, ADC_STATUS_MASK
    LSL ADC2_STATUS_REG, ADC2_STATUS_REG, 8
    OR ADC1_STATUS_REG, ADC1_STATUS_REG, ADC2_STATUS_REG

    // extend 16 bit data to 24 bit full range
    QBEQ HIGHPRECISION, PRECISION, 24

    LSL I1H_REG, I1H_REG, 8
    LSL I1H_2_REG, I1H_2_REG, 8
    LSL I1M_REG, I1M_REG, 8
    LSL I1L_REG, I1L_REG, 8
    LSL I1L_2_REG, I1L_2_REG, 8
    LSL V1_REG, V1_REG, 8
    LSL V2_REG, V2_REG, 8
    LSL I2H_REG, I2H_REG, 8
    LSL I2H_2_REG, I2H_2_REG, 8
    LSL I2M_REG, I2M_REG, 8
    LSL I2L_REG, I2L_REG, 8
    LSL I2L_2_REG, I2L_2_REG, 8
    LSL V3_REG, V3_REG, 8
    LSL V4_REG, V4_REG, 8

HIGHPRECISION:

    // sign extension (24->32bit)
    sign_extend_24 I1H_REG
    sign_extend_24 I1H_2_REG
    sign_extend_24 I1M_REG
    sign_extend_24 I1L_REG
    sign_extend_24 I1L_2_REG
    sign_extend_24 V1_REG
    sign_extend_24 V2_REG
    sign_extend_24 I2H_REG
    sign_extend_24 I2H_2_REG
    sign_extend_24 I2M_REG
    sign_extend_24 I2L_REG
    sign_extend_24 I2L_2_REG
    sign_extend_24 V3_REG
    sign_extend_24 V4_REG

DATAPROCESSING:
    // store the data in shared user space memory

    // add current channels
    ADD I1H_REG, I1H_REG, I1H_2_REG
    ADD I1L_REG, I1L_REG, I1L_2_REG
    ADD I2H_REG, I2H_REG, I2H_2_REG
    ADD I2L_REG, I2L_REG, I2L_2_REG


    // single block transfer of all channels
    SBBO ADC1_STATUS_REG, MEM_POINTER, 0, BUFFER_BLOCK_SIZE
    ADD MEM_POINTER, MEM_POINTER, BUFFER_BLOCK_SIZE

    QBA SAMPLINGLOOP

STOP:

    // clear all ADC signals
    CLR START_PIN
    CLR CS1
    CLR CS2

    // set SDATAC command (RREG needed for SDATAC to work!)
    MOV ADC_CMD, ADC_SDATAC
    send_command ADC_CMD
    MOV ADC_CMD, ADC_RREG
    send_command ADC_CMD

    // post interrupt to user space program
    MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0

    // indicate sampling inactive
    CLR LED_STATUS

    // pause PRU
    HALT
