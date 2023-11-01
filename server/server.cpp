#include "ipc_val_server.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <Windows.h>
#include <string>
#include "../ipc_com/types.h"

void communicate(int Case1) {
	switch (Case1) {
	case 1: {
		create4local();
		set_local();
	}
		  break;
	case 2: {
		int port = 31921;
		create4tcp(port);
	}
		  break;
	case 3: {
		int port = 31921;
		create4udp(port);
	}
		  break;
	case 4: {
		create4pid();
	}
		  break;
	case 5: {
		int key;
		printf("give the key!\n");
		scanf("%d", &key);
		create4shm(key);
		std::string s = "client.exe 5 ";
		s += std::to_string(key);
	}
		  break;
	case 6: {
		char namepipe[128];
		printf("give the pipe's name!\n");
		scanf("%s", namepipe);
		create4npp(namepipe);
	}
		  break;
	}

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (Case1 == 1) { printf("please select a function!\n\t1. add val 2. del val 3. print val in server 4. print list in server 5. read all vals 6. read val 7. print val in client 8. print list in client 9. set val 10. exit \n"); }
		else { printf("please select a function!\n\t1. add val 2. del val 3. print val 4. print list 10. exit \n"); }
		int Case2;
		scanf("%d", &Case2);
		// local use 1~10,others use 1~4,10
		switch (Case2) {
		case 1: {
			printf("give the val's name, the size, the value!\n");
			char buf[128]; int size; int x;
			scanf("%s%d%d", buf, &size, &x);
			if (Case1 == 1) add_val4local(buf, size, type_of(x), &x);
			if (Case1 == 2) add_val4tcp(buf, size, type_of(x), &x);
			if (Case1 == 3) add_val4udp(buf, size, type_of(x), &x);
			if (Case1 == 4) add_val4pid(buf, size, type_of(x), &x);
			if (Case1 == 5) add_val4shm(buf, size, type_of(x), &x);
			if (Case1 == 6) add_val4npp(buf, size, type_of(x), &x);
		}
			  break;
		case 2: {
			printf("give the val's name, the size!\n");
			char buf[128]; int size;
			scanf("%s%d", buf, &size);
			del_val4local(buf, size);
			if (Case1 == 1) del_val4local(buf, size);
			if (Case1 == 2) del_val4tcp(buf, size);
			if (Case1 == 3) del_val4udp(buf, size);
			if (Case1 == 4) del_val4pid(buf, size);
			if (Case1 == 5) del_val4shm(buf, size);
			if (Case1 == 6) del_val4npp(buf, size);
		}
			  break;
		case 3: {
			printf("give the val's name, the size!\n");
			char buf[128]; int size;
			scanf("%s%d", buf, &size);
			if (Case1 == 1) printf_val4local_server(buf, size);
			if (Case1 == 2) printf_val4tcp_server(buf, size);
			if (Case1 == 3) printf_val4udp_server(buf, size);
			if (Case1 == 4) printf_val4pid_server(buf, size);
			if (Case1 == 5) printf_val4shm_server(buf, size);
			if (Case1 == 6) printf_val4npp_server(buf, size);
		}
			  break;
		case 4: {
			if (Case1 == 1) printf_vals4local_server();
			if (Case1 == 2) printf_vals4tcp_server();
			if (Case1 == 3) printf_vals4udp_server();
			if (Case1 == 4) printf_vals4pid_server();
			if (Case1 == 5) printf_vals4shm_server();
			if (Case1 == 6) printf_vals4npp_server();
		}
			  break;
		case 5: {
			read_all4local();
		}
			  break;
		case 6: {
			printf("give the val's name and the size!\n");
			char buf[128]; int size;
			scanf("%s%d", buf, &size);
			read_val4local(buf, size);
		}
			  break;
		case 7: {
			printf("give the val's name, the size!\n");
			char buf[128]; int size;
			scanf("%s%d", buf, &size);
			printf_val_client(buf, size);
		}
			  break;
		case 8: {
			printf_vals_client();
		}
			  break;
		case 9: {
			printf("give the val's name, the size and the val!\n");
			char buf[128]; int size; kk::u64 v;
			scanf("%s%d%lld", buf, &size, &v);
			set_val4local(buf, size, v);
		}
			  break;
		case 10: {
			if (Case1 == 1) { close_local(); del4local(); }
			if (Case1 == 2) del4tcp();
			if (Case1 == 3) del4udp();
			if (Case1 == 4) del4pid();
			if (Case1 == 5) del4shm();
			if (Case1 == 6) del4npp();
			return;
		}
			   break;
		default:
			break;
		}


	}
}

int main() {
	int Case1;
	printf("please select a communication method!\n\t1. local 2. tcp 3. udp 4. pid 5. shm 6. npp\n");
	scanf("%d", &Case1);

	communicate(Case1);
	return 0;
}