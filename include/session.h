/**@Copyright All rights reserved.
 *@File session.h
 *@Auth lilianyun
 */

#include <semaphore.h>
#include <atomic>
#include <thread>
#include <functional>

#include "no_copy.h"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_hash_map.h"

namespace ts {

class Session;

//! Session queue
using SESSION_QUEUE = tbb::concurrent_queue<ts::Session *>;

//! Session Handle
using SESSION_HANDLE_FUN = std::function<void (int)>;

//! TCP Session
class Session : utils::NoCopy {
public:
    //! Constructor
    Session(int fd, SESSION_HANDLE_FUN session_handle, SESSION_QUEUE *session_queue);

    //! Destructor
    ~Session();

public:
    //! Open session
    bool Handle(void);

    //! Set parameters
    void SetParameters(int fd, SESSION_HANDLE_FUN session_handle, SESSION_QUEUE *session_queue);

private:
    int fd = -1;

    //! Session Handle
    SESSION_HANDLE_FUN session_handle;
    
    //! Session queue
    SESSION_QUEUE *session_queue = nullptr;

    //! Semaphore
	sem_t sem;

    //! The thread handle
	std::thread thread;

    //! Session thread
    void Thread(void);

    //! Destroy flag
    bool destroy = false;
};

} //! namespace ts