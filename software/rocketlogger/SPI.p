/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point
 
#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back

// ADC commands
#define SDATAC 0x11000000
#define RREG   0x20000000


// commands
#define COMMAND_SIZE 24
#define COMMAND_WAIT 5	


// status
#define STATUS_SIZE 24
#define STATUS_MASK 0xF//0x1 // new: 0xF (for all 4 digital inputs)


// pru data (position in memory)
#define STATE_POS			0
#define PRECISION_POS		4
#define SAMPLE_SIZE_POS		8	// unused
#define BUFFER0_POS			12
#define BUFFER1_POS			16
#define BUFFER_SIZE_POS		20
#define SAMPLE_LIMIT_POS	24
#define NUMBER_COMMANDS_POS	28
#define MEM_COMMANDS_POS	32


// gpio registers
#define SCLK		r30.t0
#define MOSI		r30.t1
#define MISO1		r31.t2
#define MISO2		r31.t16
#define CS1			r30.t3
#define CS2			r30.t5
#define START_PIN	r30.t7
#define DR1			r31.t15
#define DR2			r31.t14 // unused


// value register assignement
#define ADC1_STATUS_REG r24
#define ADC2_STATUS_REG r25

#define V1_REG r13
#define V2_REG r14
#define V3_REG r18
#define V4_REG r19

#define I1H_REG r10
#define I1M_REG r11 // unused
#define I1L_REG r12

#define I2H_REG r15
#define I2M_REG r16 // unused
#define I2L_REG r17

// new channels
#define I1H_2_REG r20
#define I1L_2_REG r21
#define I2H_2_REG r22
#define I2L_2_REG r23


// other registers
#define STATUS			r0
#define BASE_ADDRESS	r1
#define CMD				r2
#define PRECISION		r3
#define LOOP_VAR		r4
#define BUFFER_NUMBER	r5
#define BUFFER_SIZE		r6
#define NUMBER_COMMANDS	r7 // could be reused?
#define MEM_POINTER		r8
#define NUMBER_SAMPLES	r9
#define COMMANDS_POS	r11 // is reused by I1M
// channel input regs	r10-r23
// status regs			r24-r25
#define MASK_NEG		r26
#define MASK_POS		r27
#define UNUSED			r28 // unused
#define WAIT_VAR		r29


// nop
#define NOP ADD r0, r0, 0
 
 
// --------------------------- Macros ------------------------------------------ //
 
 
// WAIT
 .macro wait
.mparam time

	mov WAIT_VAR, time
WAITLOOP:
	QBEQ ENDWAIT, WAIT_VAR, 0
	SUB WAIT_VAR, WAIT_VAR, 1
	QBA WAITLOOP
ENDWAIT:

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
	
	wait COMMAND_WAIT

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
	
	wait COMMAND_WAIT
	
.endm
 
 
// SEND COMMAND (send commands and write registers)
.macro  send_command       
.mparam cmd

	CLR CS1 // set the CS line low (active low)
	CLR CS2
	MOV LOOP_VAR, COMMAND_SIZE // #bits to write
	
CMD_BIT:					// loop
	SUB LOOP_VAR, LOOP_VAR, 1
	command_cycle cmd, r10	// perform command_cycle
	QBNE CMD_BIT, LOOP_VAR, 0
	
	wait 1000 // need to wait for ADC
	
	SET CS1 // pull the CS line high (end of sample)
	SET CS2
	
.endm


// RECEIVE WORD (receive a sample, this is a bit ugly, since it was optimized for time)
.macro receive_word
.mparam inp0, inp1, prec

	MOV LOOP_VAR, prec	// load precision
	SET SCLK		// set the clock high
RECEIVE_BIT:		// loop
	
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
	receive_word ADC1_STATUS_REG, ADC2_STATUS_REG, STATUS_SIZE 
	
	// read all channels
	receive_word I1H_REG,	I2H_REG,	PRECISION
	receive_word I1H_2_REG,	I2H_2_REG,	PRECISION
	receive_word I1M_REG,	I2M_REG,	PRECISION
	receive_word I1L_REG,	I2L_REG,	PRECISION
	receive_word I1L_2_REG,	I2L_2_REG,	PRECISION
	receive_word V1_REG,	V3_REG,		PRECISION
	receive_word V2_REG,	V4_REG,		PRECISION
	
	SET CS1 // pull the CS line high (end of sample)
	SET CS2

.endm


// SIGN EXTEND (24 to 32 bit)

.macro sign_extend
.mparam reg
	
	AND reg, reg, MASK_POS
	QBBC POS, reg.t23
	
	OR reg, reg, MASK_NEG
	
POS:

.endm

 
// --------------------------- Main Program ------------------------------------------ //
 
START:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr
 
	MOV BASE_ADDRESS, 0x0 // load the base address into r1
	
	// masks for sign extension
	MOV MASK_NEG, 0xFF
	LSL MASK_NEG, MASK_NEG, 24
	NOT MASK_POS, MASK_NEG
	
	SET CS1			// CS high
	SET CS2
	CLR MOSI		// MOSI low
	CLR START_PIN	// START low
	MOV r10, 0		// receive register (unused)
	
	// notify user-space program
	MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	
	
	// load number of commands to set
	LBBO NUMBER_COMMANDS, BASE_ADDRESS, NUMBER_COMMANDS_POS, 4
	
	// load command memory position
	MOV COMMANDS_POS, MEM_COMMANDS_POS
	
	
	
CONFIGURE:
	// check if all comands sent
	QBEQ LOAD, NUMBER_COMMANDS, 0
	SUB NUMBER_COMMANDS, NUMBER_COMMANDS, 1
	
	// send command
	LBBO CMD, BASE_ADDRESS, COMMANDS_POS, 4
	send_command CMD
	ADD COMMANDS_POS, COMMANDS_POS, 4 // increase command position
	
	wait 1000 // enough time for cs toggle and command to settle
	
	LSR r10, r10, 1
	
	QBA CONFIGURE

LOAD:
	
	// load configuration
	LBBO MEM_POINTER,		BASE_ADDRESS, BUFFER0_POS, 4		// buffer0 address
	LBBO NUMBER_SAMPLES,	BASE_ADDRESS, SAMPLE_LIMIT_POS, 4	// number of samples to get
	LBBO BUFFER_SIZE,		BASE_ADDRESS, BUFFER_SIZE_POS, 4	// buffer size
	LBBO PRECISION,			BASE_ADDRESS, PRECISION_POS, 4		// precision
	MOV BUFFER_NUMBER, 0										// buffer number
	
	// store buffer number to memory
	SBBO BUFFER_NUMBER, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	
	
SAMPLING:
	// set ADC start signal to one
	SET START_PIN

SAMPLINGLOOP:
	LBBO STATUS, BASE_ADDRESS, 0, 4 // load status

	// check if buffer full
	QBNE BUFFERNOTFULL, BUFFER_SIZE, 0
	
	// buffer full
	
	// check if still running
	QBBC FINISHED, STATUS.t0
	
	// check if in continuous mode
	QBBS NOTFINISHED, STATUS.t1
	
	// check if all samples done
	QBEQ FINISHED, NUMBER_SAMPLES, 0

NOTFINISHED:
	// load buffer address
	QBBC BUFFER1, BUFFER_NUMBER.t0
	LBBO MEM_POINTER, BASE_ADDRESS, BUFFER0_POS, 4  //  load buffer0 address
	QBA BUFFER0
BUFFER1:
	LBBO MEM_POINTER, BASE_ADDRESS, BUFFER1_POS, 4  //  load buffer1 address
BUFFER0:

	// increase buffer number
	ADD BUFFER_NUMBER, BUFFER_NUMBER, 1
	
	// store buffer number
	SBBO BUFFER_NUMBER, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	
	// buffer full (post event)
	MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0 // interrupt to userspace program
	LBBO BUFFER_SIZE, BASE_ADDRESS, BUFFER_SIZE_POS, 4 // reload buffer size
	
	// store buffer number
	
BUFFERNOTFULL:

	// update counters
	SUB BUFFER_SIZE, BUFFER_SIZE, 1
	QBEQ READ, NUMBER_SAMPLES, 0
	SUB NUMBER_SAMPLES, NUMBER_SAMPLES, 1
	
READ:
	// reset input register
	MOV r10, 0 // TODO: remove
	
	// wait for data ready
	WBC DR1
	
	// receive data
	receive_data
	
	// mask and store status (low range valid)
	AND ADC1_STATUS_REG, ADC1_STATUS_REG, STATUS_MASK
	AND ADC2_STATUS_REG, ADC2_STATUS_REG, STATUS_MASK
	LSL ADC2_STATUS_REG, ADC2_STATUS_REG, 8
	OR ADC1_STATUS_REG, ADC1_STATUS_REG, ADC2_STATUS_REG
	SBBO ADC1_STATUS_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	
	// sign extension and storing
	QBEQ HIGHPRECISION, PRECISION, 24
	
	
	// add current channels 
	ADD I1H_REG, I1H_REG, I1H_2_REG
	ADD I1L_REG, I1L_REG, I1L_2_REG
	ADD I2H_REG, I2H_REG, I2H_2_REG
	ADD I2L_REG, I2L_REG, I2L_2_REG
	
	
	// I1
	SBBO I1H_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	SBBO I1L_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	
	// V1,2
	SBBO V2_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	SBBO V1_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	
	// I2
	SBBO I2H_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	SBBO I2L_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	
	// V3,4
	SBBO V4_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	SBBO V3_REG, MEM_POINTER, 0, 2
	ADD MEM_POINTER, MEM_POINTER, 2
	
	QBA LOWPRECISION
	
HIGHPRECISION:

	// sign extension (24->32bit)
	sign_extend I1H_REG
	sign_extend I1H_2_REG
	sign_extend I1M_REG
	sign_extend I1L_REG
	sign_extend I1L_2_REG
	sign_extend V1_REG
	sign_extend V2_REG
	sign_extend I2H_REG
	sign_extend I2H_2_REG
	sign_extend I2M_REG
	sign_extend I2L_REG
	sign_extend I2L_2_REG
	sign_extend V3_REG
	sign_extend V4_REG
	
	
	// add current channels
	ADD I1H_REG, I1H_REG, I1H_2_REG
	ADD I1L_REG, I1L_REG, I1L_2_REG
	ADD I2H_REG, I2H_REG, I2H_2_REG
	ADD I2L_REG, I2L_REG, I2L_2_REG
	
	
	// I1
	SBBO I1H_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	SBBO I1L_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	
	// V1,2
	SBBO V2_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	SBBO V1_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	
	// I2
	SBBO I2H_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	SBBO I2L_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	
	// V3,4
	SBBO V4_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	SBBO V3_REG, MEM_POINTER, 0, 4
	ADD MEM_POINTER, MEM_POINTER, 4
	
LOWPRECISION:

	QBA SAMPLINGLOOP

FINISHED:

	
	// clear all ADC signals
	CLR START_PIN
	CLR CS1
	CLR CS2
	
	// set SDATAC command (RREG needed for SDATAC to work!)
	MOV CMD, SDATAC
	send_command CMD
	MOV CMD, RREG
	send_command CMD

	// interrupt to userspace program
	MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	
	// pause PRU
	HALT


