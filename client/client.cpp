#include "ipc_val_client.h"
#include "../ipc_com/types.h"
#include <thread>
#include <chrono>
#include <string>

void communicate(int Case1) {
	switch (Case1) {
	case 1:
		set_tcp("127.0.0.1", 31921);
		break;
	case 2:
		set_udp("127.0.0.1", 31921);
		break;
	case 3: {
		printf("give the pid and the data_addr!\n");
		int pid;
		kk::u64 addr;
		char addressStr[128];
		scanf("%d%s", &pid, addressStr);
		addr = strtoull(addressStr, NULL, 16);
		set_pid(pid, addr);
		break;
	}
	case 4: {
		int key;
		printf("give the key!\n");
		scanf("%d", &key);
		set_shm(key);
		break;
	}
	case 5: {
		char namepipe[128];
		printf("give the pipe's name!\n");
		scanf("%s", namepipe);
		set_npp(namepipe);
		break;
	}
	default:
		break;
	}

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		printf("please select a function!\n\t1. read all vals 2. read val 3. print val 4. print list 5. set val 6. exit \n");
		int Case2;
		scanf("%d", &Case2);
		switch (Case2) {
		case 1:
			if (Case1 == 1) read_all4tcp();
			if (Case1 == 2) read_all4udp();
			if (Case1 == 3) read_all4pid();
			if (Case1 == 4) read_all4shm();
			if (Case1 == 5) read_all4npp();
			break;
		case 2: {
			printf("give the val's name and the size!\n");
			char buf[128];
			int size;
			scanf("%s%d", buf, &size);
			if (Case1 == 1) read_val4tcp(buf, size);
			if (Case1 == 2) read_val4udp(buf, size);
			if (Case1 == 3) read_val4pid(buf, size);
			if (Case1 == 4) read_val4shm(buf, size);
			if (Case1 == 5) read_val4npp(buf, size);
			break;
		}
		case 3: {
			printf("give the val's name, the size!\n");
			char buf[128];
			int size;
			scanf("%s%d", buf, &size);
			printf_val_client(buf, size);
			break;
		}
		case 4:
			printf_vals_client();
			break;
		case 5: {
			printf("give the val's name, the size and the val!\n");
			char buf[128];
			int size;
			kk::u64 v;
			scanf("%s%d%lld", buf, &size, &v);
			if (Case1 == 1) set_val4tcp(buf, size, v);
			if (Case1 == 2) set_val4udp(buf, size, v);
			if (Case1 == 3) set_val4pid(buf, size, v);
			if (Case1 == 4) set_val4shm(buf, size, v);
			if (Case1 == 5) set_val4npp(buf, size, v);
			break;
		}
		case 6:
			if (Case1 == 1) close_tcp();
			if (Case1 == 2) close_udp();
			if (Case1 == 3) close_pid();
			if (Case1 == 4) close_shm();
			if (Case1 == 5) close_npp();
			break;
		default:
			break;
		}
	}
}

int main() {
	int Case1;
	printf("please select a communication method!\n\t1. tcp 2. udp 3. pid 4. shm 5. npp\n");
	scanf("%d", &Case1);

	communicate(Case1);

	return 0;
}
