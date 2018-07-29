/**@Copyright All rights reserved.
 *@File core.cpp
 *@Auth lilianyun
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <memory>

#include "session.h"
#include "mystring.h"

#define TCP_SERVER_PORT   8080

//! Hash compare
struct StringHashCompare {

    //! equals() method tests for key equalit
	bool equal(const utils::MyString &x, const utils::MyString &y) const { 
		return (x == y); 
	}

	//! hash() gets hash value.
	unsigned long hash(const utils::MyString &key) const  {
		unsigned long seed = 131, hash = 0; const char *p = key.c_str();
		while ('\0' != *p) { 
			hash = ((hash * seed) + (*p++)); 
		}
		return (hash & 0x7FFFFFFF);
	}
};

//! CONN_TABLE type based on concurrent_hash_map
using KV_TABLE = tbb::concurrent_hash_map<utils::MyString, utils::MyString, StringHashCompare>;

//! Safe close
#define SAFE_CLOSE(fd) do { if(-1 != fd) { close(fd); (fd)=-1; } } while(0)

//! Free session queue
static ts::SESSION_QUEUE g_session_queue;

//! Buffer size
#define  BUFF_SIZE    1024

//! Key value table
static KV_TABLE g_kvtable;

//! Send n bytes
static int sendn(int fd, const char *buf, size_t n)
{
    int left = n, written = 0;
 	const char *ptr = buf;

	while (0 < left) {
		if ((written = send(fd, ptr, left, MSG_NOSIGNAL)) == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) {
                written = 0;
            } else {
				return -1;
			}
		}
		left -= written;
		ptr   += written;
	}
	return static_cast<int>(n);
}

//! Set key value
static void SetKeyValue(const utils::MyString &key, const utils::MyString &value) {
    g_kvtable.insert(std::make_pair(key, value));
}

//! Set key value
static void GetKeyValue(const utils::MyString &key, utils::MyString &value) {
    KV_TABLE::const_accessor result;
	if (g_kvtable.find(result, key)) {
        value = result->second;
	}	
}

//! Split string
static void SplitString(char *in, const char *delim, int parts, char **token) {

    if (nullptr == in) {
        return;
    }

    int i = 0;
    char *p = in;
    char *saveptr = nullptr;
    for (i = 0; i < parts; i++, p = NULL) {
        token[i] = strtok_r(p, delim, &saveptr);
        if (nullptr == token[i]) {
            break;
        }
    }
}

//! Data handle
static bool DataHandle(char *buf, utils::MyString &cmd, utils::MyString &key, utils::MyString &value) {

    //! 0:GET, 1:KEY, 2: VALUE
    char *token[3] = { nullptr };

    //! Get 3 parts
    SplitString(buf, " \t", 3, token);

    //! Check
    if ((nullptr == token[0]) || (nullptr == token[1])) {
        return false;
    }

    //! PUT/SET
    if ((0 == strcmp(token[0], "PUT")) && (nullptr != token[2])) {
        cmd = utils::MyString("PUT");
        key = utils::MyString(token[1]);
        value = utils::MyString(token[2]);
    } else if (0 == strcmp(token[0], "GET")) {
        cmd = utils::MyString("GET");
        key = utils::MyString(token[1]);
    } else {
        return false;
    }
    return true;
}
//! Handle
static void SessionHandle(int fd)
{
    if (0 > fd) {
        return;
    }

    int retval = -1;
    fd_set rset;
    char buf[BUFF_SIZE + 1] = {'\0'};
    struct timeval timeout = {120, 0};

    utils::MyString cmd;
    utils::MyString key;
    utils::MyString value;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    if (select(fd + 1, &rset, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(fd, &rset)) {

            //! Recieve data.
            memset(buf, 0, sizeof(buf));
            int len = recv(fd, buf, BUFF_SIZE, 0);

            std::cout << "Receive: " << buf << std::endl;

            //! Data handle
            if (DataHandle(buf, cmd, key, value)) {
                if (cmd == "PUT") {
                    SetKeyValue(key, value);
                } else {
                    GetKeyValue(key, value);
                    retval = sendn(fd, value.data(), value.length());
                    if (retval == value.length()) {
                        std::cout << "Send success." << std::endl;
                    }
                }
                std::cout << "CMD: " << cmd << ", Key: " << key << ", Value: " << value << std::endl;
            }
        }
    }

    struct linger tcp_linger;
    tcp_linger.l_onoff = 1;
    tcp_linger.l_linger = 2;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&tcp_linger, sizeof(struct linger));
    SAFE_CLOSE(fd);
}

int main(int argc, char const *argv[]) {

    if (argc > 2) {
        std::cout << "Invalid.\n";
        return -1;
    }

    short port = TCP_SERVER_PORT;
    if (2 == argc) {
        port = std::stoi(std::string(argv[1]), nullptr, 10);
    }

    std::cout << "TCP Server Port is " << port << std::endl;

    //! Client fd
    int client_fd = -1;

    //! Open a stream socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > fd) {
        return -1;
    } 

    //! Reuse
    int addreuse = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&addreuse, sizeof(int))) {
        SAFE_CLOSE(fd);
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family            = AF_INET;
    server_addr.sin_port              = htons(port);
    server_addr.sin_addr.s_addr       = htonl(INADDR_ANY);

    //! bind port
    if (bind(fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) != 0) {
        SAFE_CLOSE(fd);
        return -1;
    }

    //! Listen
    listen(fd, 64);

    while (true) {

        //! Accept a client
        if ((client_fd = accept(fd, NULL, NULL)) < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        ts::Session *sptr = nullptr;

        //! Get a new seesion
        if (!g_session_queue.empty()) {
            if (g_session_queue.try_pop(sptr)) {
                sptr->SetParameters(client_fd, std::bind(&SessionHandle, std::placeholders::_1), &g_session_queue);
            }
        } else {
            sptr = new(std::nothrow) ts::Session(client_fd, std::bind(&SessionHandle, std::placeholders::_1), &g_session_queue);
        }

        //! Session Handle
        if (nullptr != sptr) {
            sptr->Handle();
        }
    }

    return 0;
}
