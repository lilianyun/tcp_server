/**@Copyright All rights reserved.
 *@File session.cpp
 *@Auth lilianyun
 */

#include "session.h"

namespace ts {

//! thread handle
using THREAD_HANDLE = std::thread::native_handle_type;

//! Constructor
Session::Session(int fd, SESSION_HANDLE_FUN session_handle, SESSION_QUEUE *session_queue) {
    this->fd = fd;
	this->session_handle = session_handle;
	this->session_queue = session_queue;

	sem_init(&(this->sem), 0, 0);
    this->thread = std::thread(&Session::Thread, this);
	if (static_cast<THREAD_HANDLE>(0) != this->thread.native_handle()) {
		this->thread.detach();
	}
}

//! Destructor
Session::~Session() {
	destroy = true;
	sem_post(&sem);
}

//! Start thread to handle
bool Session::Handle(void) {
	destroy = false;
    return (0 == sem_post(&sem));
}

 //! Set parameters
void Session::SetParameters(int fd, SESSION_HANDLE_FUN session_handle, SESSION_QUEUE *session_queue) {
	this->fd = fd;
	this->session_handle = session_handle;
	this->session_queue = session_queue;
}

//! Session thread
void Session::Thread(void) {

	//! When destroy == true, exit thread.
	while (true) {

		//! Wait Semaphore
		int32_t retval = sem_wait(&sem);
		if (-1 == retval) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		else if ((0 == retval) && destroy) {
			break;
		}

		//! Handle
		session_handle(this->fd);

		//! Recovery session.
		session_queue->push(this);
	}
}

} //! namespace ts