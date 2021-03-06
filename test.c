#include <stdio.h>
#include "config.h"
#include "cpu.h"
#include <memory.h>
#include "test.h"
#include "decode.h"

#ifdef RUN_TESTS
int last_exit_code = 0;
int bin_file_size = 0;
const int SUCCESS = 42;
const int FAIL = 0;
const int test_ram_size = 1 * 1024 * 1024;

void set_last_exit_code(int code) {
	last_exit_code = code;
}

void check_test_exit_code(State* state) {
	if (last_exit_code == FAIL) {
		printf("TEST FAILED! gp=0x%08x\n",state->x[3]);
		exit(1);
	}
}

void clear_state(State* state) {
	//clear the registers
	for (int i = 0; i < REGISTERS; i++)
		state->x[i] = 0;
	state->pc = 0;
	state->status = RUNNING;
	//clear the memory
	const int memory_size = 1024 * 1024;

	state->memory_map = phys_mem_map_init();
	cpu_register_ram(state->memory_map, 0, test_ram_size);

	set_last_exit_code(0);
}

void print_registers(State* state) {
	for (int i = 0; i < 32; i++) {
		printf("x%d %08X\t", i, state->x[i]);
		if ((i + 1) % 4 == 0)
			printf("\n");
	}
}

//void test_beq_1() {
//	int program[] = {
//		0x00000463, //beq x0,x0,0x4
//		0x00900093, //addi x1, x0, 0x9
//		0x00000013, //nop / addi x0,x0,0
//	};
//	State state;
//	clear_state(&state);
//	memcpy(state.memory, program, sizeof(program));
//	emulate_op(&state);
//	print_registers(&state);
//	emulate_op(&state);
//	if (state.x[1] != 0x0) {
//		printf("Assertion failed, x1 should be 0!");
//		exit(1);
//	}
//}
//
//void test_addi() {
//	int program[] = {
//		0x800000b7, //lui x1, 0x00080000
//		0xfff08093, //addi x1, x1, 0xffffffff
//		0x00108f13, //addi x30, x1, 0x001
//	};
//	State state;
//	clear_state(&state);
//	memcpy(state.memory, program, sizeof(program));
//	for (int i = 0; i < 3; i++)
//	{
//		emulate_op(&state);
//		print_registers(&state);
//	}
//	if (state.x[30] != 0x80000000)
//		printf("Assertion failed!");
//}

void assert_shamt(word instruction, int expected_shamt) {
	int actual_shamt = shamt(instruction);
	if (actual_shamt != expected_shamt) {
		printf("Unexpected shamt value: %d, expected %d", actual_shamt, expected_shamt);
		exit(1);
		return;
	}
}

//void test_shamt() {
//	word instr = 0x00009f13; //slli x30, x1, 0
//	assert_shamt(0x00009f13 /*slli x30, x1, 0*/, 0);
//	assert_shamt(0x00109f13 /*slli x30, x1, 1*/, 1);
//	assert_shamt(0x00709f13 /*slli x30, x1, 7*/, 7);
//	assert_shamt(0x00e09f13 /*slli x30, x1, 14*/, 14);
//	assert_shamt(0x01f09f13 /*slli x30, x1, 31*/, 31);
//}

void assert_b_imm(word instruction, int expected_imm) {
	int actual = get_b_imm(instruction);
	if (actual != expected_imm) {
		printf("Unexpected B-imm value: %d, expected %d", actual, expected_imm);
		exit(1);
		return;
	}
}

//void test_b_imm() {
//	assert_b_imm(0xfe000ee3 /* beq x0, x0, 0xfffffffe */, 0xfffffffc/*0xfffffffe * 2*/);
//	assert_b_imm(0x00108a63 /* beq x1, x1, 0x0000000a */, 0x0000000a * 2);
//	assert_b_imm(0x08108263 /* beq x1, x1, 0x00000042 */, 0x00000042 * 2);
//	assert_b_imm(0x16108663 /* beq x1, x1, 0x000000b6 */, 0x000000b6 * 2);
//	assert_b_imm(0x1a108e63 /* beq x1, x1, 0x000000de */, 0x000000de * 2);
//	assert_b_imm(0x1e108e63 /* beq x1, x1, 0x000000fe */, 0x000000fe * 2);
//	assert_b_imm(0x24108463 /* beq x1, x1, 0x00000124 */, 0x00000124 * 2);
//}
//
//void test_addi_2() {
//	int program[] = {
//		0x80000eb7,		//lui	    x29, 0xfff80000
//		0x00008093		//addi	x29,x29,0x0
//	};
//	State state;
//	clear_state(&state);
//	memcpy(state.memory, program, sizeof(program));
//	for (int i = 0; i < 2; i++)
//	{
//		emulate_op(&state);
//		print_registers(&state);
//	}
//	if (state.x[29] != 0x80000000)
//		printf("Assertion failed!");
//}
//
//void test_slli() {
//	int program[] = {
//		0x00033537,		//lui	    a0,0x33
//		0xbfb50513,		//addi	a0,a0,-1029
//		0x00e51513,		//slli	a0,a0,0xe
//		0xabe50513		//addi	a0,a0,-1346
//	};
//	State state;
//	clear_state(&state);
//	memcpy(state.memory, program, sizeof(program));
//	for (int i = 0; i < 4; i++)
//	{
//		emulate_op(&state);
//		print_registers(&state);
//	}
//	if (state.x[10] != 0xcafebabe)
//		printf("Assertion failed!");
//}

void test_bin(char* name) {
	printf("Starting test for: %s\n", name);
	byte* buffer = read_bin(name, &bin_file_size);
	if (buffer == NULL) {
		return;
	}

	State *state = mallocz(sizeof(State));
	clear_state(state);

	//load test binary
	memcpy(state->memory_map[0].phys_mem_range->phys_mem_ptr, buffer, bin_file_size);

	for (;;) {
		word* address = get_physical_address(state, state->pc);

		//word* address = state.memory + state.pc;
		printf("%08x:  %08x  ", state->pc, *address);
		emulate_op(state);
		//print_registers(&state);
		if (state->status == EXIT_TERMINATION) {
			check_test_exit_code(state);
			break;
		}
	}
	printf("Test %s passed\n", name);
}

void test_memory() {
	//TODO assume the compact configuration - .text at 0x0000, .data at 0x2000
	//TODO generalize test_simple_bin
}

void test_ecall_callback(State* state) {
	//tests use a7 = EXIT (93)
	if (state->x[SYSCALL_REG] == EXIT) {
		set_last_exit_code(state->x[SYSCALL_ARG0]);
		state->status = EXIT_TERMINATION;
	}
}

void run_tests() {
	//test_addi_2();
	//test_shamt();
	//test_b_imm();
	//test_beq_1();
	test_bin("test/simple.bin");
	test_bin("test/slli.bin");
	//integer register-register
	test_bin("test/add.bin");
	test_bin("test/slt.bin");
	test_bin("test/sltu.bin");
	test_bin("test/sub.bin");
	//register-immediate special
	test_bin("test/lui.bin");
	test_bin("test/auipc.bin");
	//register-register logical
	test_bin("test/and.bin");
	test_bin("test/or.bin");
	test_bin("test/xor.bin");
	//register-register shifts
	test_bin("test/sll.bin");
	test_bin("test/srl.bin");
	test_bin("test/sra.bin");
	//immediate shifts
	test_bin("test/slli.bin");
	test_bin("test/srli.bin");
	test_bin("test/srai.bin");

	//register-register immediate logical
	test_bin("test/andi.bin");
	test_bin("test/ori.bin");
	test_bin("test/xori.bin");

	//register-register immediates
	test_bin("test/addi.bin");
	test_bin("test/slti.bin");
	test_bin("test/sltiu.bin");

	//conditional branches
	test_bin("test/beq_bne_loop.bin");
	test_bin("test/beq.bin");
	test_bin("test/bltu.bin");
	test_bin("test/blt.bin");
	test_bin("test/bge.bin");
	test_bin("test/bgeu.bin");

	//load/save
	test_bin("test/lw_sw_offset.bin");
	test_bin("test/lh_sh.bin");
	test_bin("test/lb_sb.bin");
	test_bin("test/memory.bin");

	//jumps
	test_bin("test/jal_long.bin");
	test_bin("test/jal_simple.bin");
	test_bin("test/jalr.bin");

}

int run_test_suite() {
	set_ecall_callback(&test_ecall_callback);

	run_tests();

	printf("--------------------------\n");
	printf("ALL TESTS PASSED\n");
	printf("--------------------------\n");
}

int main() {
	run_test_suite();
}

#endif