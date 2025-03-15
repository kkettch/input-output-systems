typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

#include "kernel.h"

extern char __bss[], __bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

long getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}

void print_str(const char *s) {
    while (*s) {
        putchar(*s++);
    }
}

long get_sbi_spec_version() {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 0x10);
    return ret.value;
}

void print_sbi_version(uint32_t version) {
    putchar('S'); putchar('B'); putchar('I'); putchar(' ');
    putchar('v');
    putchar('0' + ((version >> 24) & 0xF)); 
    putchar('.');
    putchar('0' + ((version >> 0) & 0xF));  
    putchar('\n');
}

long get_number_of_counters(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 0x504D55); 
    return ret.value; 
}

void print_number_of_counters(long counters) {
    if (counters == 0) {
        putchar('0');
    } else {
        char buffer[10];  
        int i = 0;

        while (counters > 0) {
            buffer[i++] = '0' + (counters % 10);  
            counters /= 10; 
        }

        for (int j = i - 1; j >= 0; j--) {
            putchar(buffer[j]);
        }
    }
    putchar('\n');
}

long get_details_of_a_counter(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 1, 0x504D55); 
    return ret.value; 
}

struct counter_info {
    long csr;    
    long width;   
    long type;   
};

void print_counter_details(long counter_id) {
    long num_counters = get_number_of_counters(); 
    
    if (counter_id < 0 || counter_id >= num_counters) {
        print_str("Error: Invalid counter index.\n");
        return;
    }

    struct sbiret ret = sbi_call(counter_id, 0, 0, 0, 0, 0, 1, 0x504D55); 
    
    if (ret.error != 0) {
        print_str("Error: Unable to retrieve counter details.\n");
        return;
    }

    struct counter_info info;
    info.csr = ret.value & 0xFFF;  
    info.width = (ret.value >> 12) & 0xF;  
    info.type = (ret.value >> (32 - 1)) & 0x1;  

    // Печатаем детали счетчика
    print_str("Counter Information:\n");

    print_str("CSR: ");
    print_number_of_counters(info.csr);

    print_str("Width: ");
    print_number_of_counters(info.width + 1); 

    print_str("Type: ");
    if (info.type == 0) {
        print_str("Hardware\n");
    } else {
        print_str("Firmware\n");
    }
}
long get_counter_id_from_input() {
    char input[10];  
    int i = 0;
    int ch;

    print_str("Enter counter number: ");

    while (1) {
        ch = getchar();
        if (ch == '\n' || ch == '\r') {  
            break;
        }
        if (ch >= '0' && ch <= '9' && i < sizeof(input) - 1) {
            input[i++] = ch;  
        } else {
            continue;
        }
    }

    input[i] = '\0';  

    long counter_id = 0;
    for (int j = 0; j < i; j++) {
        counter_id = counter_id * 10 + (input[j] - '0');
    }

    return counter_id;
}

void system_shutdown(void) {
    sbi_call(0, 0, 0, 0, 0, 0, 0, 0x08);
}

void kernel_main(void) {

    print_str("OpenSBI list of commands: \n");
    print_str("1. Get SBI Specification Version: \n");
    print_str("2.  Get number of counters: \n");
    print_str("3.  Get details of a counter: \n");
    print_str("4.  System Shutdown: \n");

    while (1) {

        int ch = getchar();
        if (ch != -1) {
            putchar((char)ch);
            putchar('\n');
            switch (ch) {
                case '1': // Get SBI Specification Version
                {
                    long version = get_sbi_spec_version();
                    print_sbi_version(version);
                    break;
                }
                case '2': // Get number of counters
                {
                    long counters = get_number_of_counters();
                    print_number_of_counters(counters);
                    break;
                }
                case '3': // Get details of a counter (должно быть возможно задавать номер счетчика)
                {
                    long counter_id = get_counter_id_from_input();
                    print_counter_details(counter_id);  
                    break;
                }
                case '4': // System Shutdown
                { 
                    system_shutdown();
                    break;
                }
                default: 
                    print_str("\nInvalid input\n");
                    break;
            }
        }
    }

    get_sbi_spec_version();

    for (;;) {
        __asm__ __volatile__("wfi");
    }
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // Устанавливаем указатель стека
        "j kernel_main\n"       // Переходим к функции main ядра
        :
        : [stack_top] "r" (__stack_top) // Передаём верхний адрес стека в виде %[stack_top]
    );
}