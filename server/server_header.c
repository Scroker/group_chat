#include "server_header.h"
#define LOG_PATH "/tmp/Talk_Server_Log"

logger_t* main_logger;
logger_t*	client_logger;
int log_on = 0, debug_on = 0;

void 	server_exit ();

int 	server_routine(int argc, char const *argv[]) {
	int									server_desc , client_desc, client_addr_len, ret;
	char 								buffer[128];
	struct sockaddr_in	server_addr , client_addr;

	client_list = NULL;
	last_client = NULL;

	for (int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-l") == 0) {
			fprintf(stderr, "Log Enabled\n");
			log_on = 1;
		}
		if(strcmp(argv[i], "-ld") == 0) {
			fprintf(stderr, "Debug Log Enabled\n");
			log_on = 1;
			debug_on = 1;
		}
	}

	if (log_on) {
		strcpy(buffer, LOG_PATH);
		strcat(buffer, "/");
		strcat(buffer, "Server_log");
		strcat(buffer, " - ");
		strcat(buffer, get_time());
	}

	if (log_on) {
		if(mkdir(LOG_PATH, 0700) == -1 && errno != EEXIST) {
			if (debug_on) perror("mkdir");
			exit(EXIT_FAILURE);
		}
	}

	if (log_on) main_logger = new_log(buffer, O_WRONLY | O_CREAT | O_APPEND, 0666);

	if (atexit(server_exit) != 0) {
		if (log_on) write_log(main_logger, "atexit_faliure: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (signal(SIGINT, exit) == SIG_ERR) {
		if (log_on && debug_on) write_log(main_logger, "signal_faliure: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	ret = server_init(&server_desc, &server_addr);
	if (ret < 1) exit(EXIT_FAILURE);
	fprintf(stderr, "Server Started\n");
	//daemon(0,1);

 	// Ciclo sentinella
	client_addr_len = sizeof(client_addr);
	while(1) {
		client_desc = accept(server_desc, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len);
    if (client_desc < 0) {
			if (log_on && debug_on) write_log(main_logger, "main: error in accept: %s\n", strerror(errno));
			fprintf(stderr, "Impossibile connettersi al client\n");
			continue;
    }
		client_l thread_arg = malloc(sizeof(client_t));
		thread_arg->client_desc = client_desc;
		thread_arg->next = NULL;
		thread_arg->prev = NULL;
		sprintf(thread_arg->client_ip,
			"%d.%d.%d.%d",
			(int)(client_addr.sin_addr.s_addr&0xFF),
			(int)((client_addr.sin_addr.s_addr&0xFF00)>>8),
			(int)((client_addr.sin_addr.s_addr&0xFF0000)>>16),
			(int)((client_addr.sin_addr.s_addr&0xFF000000)>>24));
		pthread_t* init_client_thread = malloc(sizeof(pthread_t));
		ret = pthread_create(init_client_thread, NULL, client_routine,(void*) thread_arg);
		if (ret != 0) {
			if (log_on && debug_on) write_log(main_logger, "main: error in ptrhead_create: %s\n", strerror(ret));
			fprintf(stderr, "Impossibile connettersi al client\n");
			continue;
		}
		ret = pthread_detach(*init_client_thread);
		if (ret != 0) {
			if (log_on && debug_on) write_log(main_logger, "main: error in ptrhead_detach: %s\n", strerror(ret));
			fprintf(stderr, "Impossibile connettersi al client\n");
			continue;
		}
		memset(&client_addr, 0, sizeof(client_addr));
  }
  exit(EXIT_SUCCESS);
}

void*	client_routine(void *arg) {
	client_l 	client = (client_l) arg;

	int*			status = &client->client_status;
	int 			ret;
	int 			client_desc = client->client_desc;
	int   		bytes_read = 0;
	int   		query_ret;
	int  		 	query_recv;
	int*			client_id = &client->client_id;
	char*			client_name = client->client_name;
	char*			client_ip = client->client_ip;
	char*			client_port = client->client_port;
	char			query[5];
	char 			data[PACKET_LEN];
	char 			path[128];

  while(bytes_read < PACKET_LEN) {
    ret = recv(client_desc, data + bytes_read, 1, 0);
    if (ret == -1 && errno == EINTR) continue;
    if (ret == -1) {
			if (log_on && debug_on) write_log(main_logger, "recv_message: error in recv: %s\n", strerror(errno));
      pthread_exit(NULL);
    }
    if (ret == 0) {
			if (log_on && debug_on) write_log(main_logger, "recv_message: connection closed by client: %s\n", strerror(errno));
			pthread_exit(NULL);
    }
		bytes_read++;
  }

	memcpy (client_port	,data	 , 4);
	memcpy (client_name	,data+4, 12);

	if (log_on) {
		strcpy (path, LOG_PATH);
		strcat (path, "/");
		strcat (path, client_name);
		strcat (path, " - ");
		strcat (path, get_time());
		strcat (path, ".txt");
	}

	if (log_on) client_logger = new_log(path, O_WRONLY | O_CREAT | O_APPEND, 0666);
	*status = ONLINE;

	if (log_on) write_log(client_logger, "%s\nLog of client %s : %s\n", get_time(), client_ip, client_name);
	if (log_on) write_log(main_logger, "%s: New client connected %s : %s\n", get_time(), client_ip, client_name);
	add_cl(client);
	while (1) {
		query_recv = 0;
		memset(query, 0, QUERY_LEN);
	  while(1) {
	    query_ret = recv(client_desc, query + query_recv, 1, 0);
	    if (query_ret == -1 && errno == EINTR)
				continue;
	    if (query_ret == -1) {
				if (log_on && debug_on) write_log(client_logger, "recv_message: error in recv: %s\n", strerror(errno));
				pthread_exit(NULL);
	    }
	    if (query_ret == 0) {
				if (log_on) write_log(client_logger, "recv_message: connection closed by client");
				if (debug_on) perror("recv_message: connection closed by client");
	      pthread_exit(NULL);
	    }
	    query_recv++;
	    if (query[query_recv-1] == '\n' ||
					query[query_recv-1] ==  '\0' ||
					query_recv == QUERY_LEN)
				break;
	  }
	  query[query_recv-1] = '\0';
		if (strcmp(query, "QUIT\0") == 0) {
			remove_cl(*client_id);
			if (log_on) write_log(client_logger, "%s: Client disconnected\n", get_time());
			if (log_on) destroy_log(client_logger);
			if (log_on) write_log(main_logger, "%s: Client disconnected %s : %s\n", get_time(), client_ip, client_name);
			break;
		}
		if (strcmp(query, "STOF\0") == 0) {
			if (*status != OFFLINE) {
				*status = OFFLINE;
				if (log_on) write_log(client_logger, "%s: Change status Online => Offline\n", get_time());
				}
		}
		if (strcmp(query, "STON\0") == 0){
			if (*status != ONLINE) {
				*status = ONLINE;
				if (log_on) write_log(client_logger, "%s: Change status Offline => Online\n", get_time());
				}
		}
		if (strcmp(query, "LIST\0") == 0) {
			send_cl(client_desc);
			if (log_on) write_log(client_logger, "%s: Client asked the list\n", get_time());
			}
	}
	pthread_exit(NULL);
}

void 	server_exit () {
	if (log_on) write_log(main_logger, "%s: Server halted\n", get_time());
	if (debug_on && log_on) write_log(main_logger,"Last clients connected:\n");
	client_list_wait();
	while (client_list != NULL) {
		client_l aux = client_list;
		fprintf(stderr, "%s\n", aux->client_name);
		if (debug_on && log_on) write_log(main_logger, "%d %d %s %s\n", aux->client_id, aux->client_status, aux->client_ip, aux->client_name);
		client_list = client_list->next;
		free(aux);
	}
	client_list_post();
	if (debug_on && log_on) write_log(main_logger, "END\n");
	if (log_on) destroy_log(main_logger);
	sem_destroy(&client_list_semaphore);
	fprintf(stderr, "Server_Halted\n");
}