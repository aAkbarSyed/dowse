#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <b64/cencode.h>
#include "dowse.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#include <hiredis/hiredis.h>

redisContext *log_redis  = NULL;//connect_redis("127.0.0.1", 6379, 0);
int logredis_retry_to_connect=1;

base64_encodestate b64_state;
redisReply *minimal_cmd_redis(redisContext *redis, const char *format, ...) ;
void _minimal_err(char *msg,int sizeof_msg,const char *fmt, ...);
redisContext *connect_redis(char *host, int port, int db);
redisContext *minimal_connect_redis(char *host, int port, int db,int minimal_log) ;

void toredis(char *pfx, char *msg) {
  return ; /* WIP */
  
    if ((!log_redis) && (logredis_retry_to_connect)){
        if (1) { /* TODO sostituire con getenv() */

            log_redis = minimal_connect_redis(REDIS_HOST, REDIS_PORT, db_dynamic,1); /* we call in minimal_log */

            if (!log_redis) {
                char msg[256];

                _minimal_err(msg,sizeof(msg),"Redis server is not running");

                return;
            }
        } else {
            return ;
        }
    }


    if (log_redis) {
        redisReply *reply;

        reply=redisCommand(log_redis, "PUBLISH log-channel (%d):%s:%ld:%s", getpid(),pfx,time(NULL), msg);

        if (reply && reply->len) {

            fprintf(stderr,"%s %d redis_reply %s\n",
                    __FILE__,__LINE__,reply->str);
        }
        if (reply) {

            freeReplyObject(reply);
        }

        char command[256];
        char b64_encoded[512];

        base64_init_encodestate(&b64_state);


        int rv=base64_encode_block(msg, strlen(msg), b64_encoded, &b64_state);


        int rv2=base64_encode_blockend(b64_encoded+rv,&b64_state);


        b64_encoded[rv+rv2-1]=0;


        sprintf(command,"PUBLISH log-queue %d:%s:%ld:%s", getpid(),(pfx),time(NULL), b64_encoded);
        /* Using the plain msg variable the values are splitted by the blank character */
        //        sprintf(command,"LPUSH log-queue %s:%s", pfx,msg);

        /**/
//        reply=minimal_cmd_redis(log_redis,"%s",command);
        reply=minimal_cmd_redis(log_redis,command,NULL);

        if (reply && reply->len) {
            fprintf(stderr,"%s %d redis_reply %s\n",
                    __FILE__, __LINE__,reply->str);
        }
        if (reply) {
            freeReplyObject(reply);
        } else {
	  if (log_redis) {
	    redisFree(log_redis);
	  }
	  log_redis = NULL;
	}

    }
}

void func(const char *fmt, ...) {
#if (DEBUG==1)
	va_list args;

	char msg[256];
	size_t len;

	va_start(args, fmt);

	vsnprintf(msg, sizeof(msg), fmt, args);
	len = strlen(msg);
	write(2, ANSI_COLOR_BLUE " [D] " ANSI_COLOR_RESET, 5+5+4);
	write(2, msg, len);
	write(2, "\n", 1);
	fsync(2);

	va_end(args);

	// toredis("DEBUG", msg);
#endif
	return;
}

void _minimal_err(char *msg,int sizeof_msg,const char *fmt, ...) {
    size_t len;

    va_list args;
    va_start(args, fmt);

    vsnprintf(msg, sizeof_msg, fmt, args);
     len = strlen(msg);
    write(2, ANSI_COLOR_RED " [!] " ANSI_COLOR_RESET, 5+5+4);
    write(2, msg, len);
    write(2, "\n", 1);
    fsync(2);

    va_end(args);

}


void err(const char *fmt, ...) {
    va_list args;
	char msg[256];
	va_start(args, fmt);
	_minimal_err(msg,sizeof(msg),fmt,args);
	va_end(args);
	// toredis("ERROR", msg);

}


void notice(const char *fmt, ...) {
	va_list args;

	char msg[256];
	size_t len;

	va_start(args, fmt);

	vsnprintf(msg, sizeof(msg), fmt, args);
	len = strlen(msg);
	write(2, ANSI_COLOR_GREEN " (*) " ANSI_COLOR_RESET, 5+5+4);
	write(2, msg, len);
	write(2, "\n", 1);
	fsync(2);

	va_end(args);

	// toredis("NOTICE", msg);
}


void act(const char *fmt, ...) {
	va_list args;

	char msg[256];
	size_t len;

	va_start(args, fmt);

	vsnprintf(msg, sizeof(msg), fmt, args);
	len = strlen(msg);
	write(2, "  .  ", 5);
	write(2, msg, len);
	write(2, "\n", 1);
	fsync(2);

	va_end(args);

	// toredis("ACT", msg);
}



void warn(const char *fmt, ...) {
	va_list args;

	char msg[256];
	size_t len;

	va_start(args, fmt);

	vsnprintf(msg, sizeof(msg), fmt, args);
	len = strlen(msg);
	write(2, ANSI_COLOR_YELLOW " (*) " ANSI_COLOR_RESET, 5+5+4);
	write(2, msg, len);
	write(2, "\n", 1);
	fsync(2);

	va_end(args);

	// toredis("WARN", msg);
}
