/**
 * Simple Zeromq subscriber server application.
 * See also: <http://zguide.zeromq.org/page:all#Getting-the-Message-Out>
 */
#include <zmq.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>

#define DATA_BLOCK_SIZE 1000
#define ZMQ_DATA_SOCKET "tcp://127.0.0.1:5555"

int64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((int64_t)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

int main(void) {
    void *context = zmq_ctx_new();
    void *publisher = zmq_socket(context, ZMQ_PUB);
    int rc = zmq_bind(publisher, ZMQ_DATA_SOCKET);
    assert(rc == 0);

    printf("zeromq publish server started (DATA_BLOCK_SIZE=%lu)\n", DATA_BLOCK_SIZE);
    float data[DATA_BLOCK_SIZE];
    int64_t time_ms;
    while (1) {
        for (int i = 0; i < DATA_BLOCK_SIZE; i++) {
            data[i] = (float)rand() / (float)rand();
        }
        time_ms = get_time_ms();
        zmq_send(publisher, &time_ms, sizeof(time_ms), ZMQ_SNDMORE);
        zmq_send(publisher, &data, sizeof(data), 0);
        printf("pub: %lli, [ %f, %f, %f, ... ]\n", time_ms, data[0], data[1], data[2]);

        // wait and increment
        sleep(1);
    }

    zmq_close(publisher);
    zmq_ctx_destroy(context);

    return 0;
}
