/******************************************************************************/
/*!
 * @file   bf_job_api.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *    API for a multithreading job system.
 * 
 *    References:
 *      [https://blog.molecular-matters.com/2015/08/24/job-system-2-0-lock-free-work-stealing-part-1-basics/]
 *      [https://manu343726.github.io/2017-03-13-lock-free-job-stealing-task-system-with-modern-c/]
 *      [https://github.com/cdwfs/cds_job/blob/master/cds_job.h]
 *      [https://github.com/cyshi/logbook/blob/master/src/common/work_stealing_queue.h]
 *
 * @version 0.0.1
 * @date    2020-09-03
 *
 * @copyright Copyright (c) 2020 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef BF_JOB_API_HPP
#define BF_JOB_API_HPP

#include "bf_job_config.hpp" /* size_t, Config Constants */

#include <cstdint> /* uint16_t      */
#include <new>     /* placement new */
#include <utility> /* forward       */

namespace bf
{
  namespace job
  {
    // Fwd Declarations

    struct Task;

    enum class QueueType : std::uint16_t;

    // Private

    namespace detail
    {
      void      checkTaskDataSize(std::size_t data_size) noexcept;
      QueueType taskQType(const Task* task) noexcept;
    }  // namespace detail

    // Type Aliases

    using WorkerID = std::uint16_t;    //!< The id type of each workter thread.
    using TaskFn   = void (*)(Task*);  //!< The signature of the type of function for a single Task.

    // Enums

    /*!
     * @brief 
     *   The priority that you want a task to run at.
     */
    enum class QueueType : std::uint16_t
    {
      MAIN       = 0,  //!< Use this value when you need a certain task to be run specifically by the main thread.
      HIGH       = 1,  //!< Normally you will want tasks to go into this queue.
      NORMAL     = 2,  //!< Slightly lower priority than 'QueueType::HIGH'.
      BACKGROUND = 3,  //!< Lowest priority, good for asset loading.
    };

    // Struct Defintions

    /*!
     * @brief 
     *   The runtime configuration for the Job System.
     */
    struct JobSystemCreateOptions
    {
      std::size_t num_threads;  //!< Use 0 to indicate using the number of cores available on the system.
    };

    // Main System API
    //
    // API functions can only be called by the thread that called 'bf::job::initialize' or from a within a Task function.
    //

    /*!
     * @brief
     *   Sets up the Job system and creates all the worker threads.
     *   The thread that calls 'bf::job::initialize' is considered (and should be) the main thread.
     * 
     * @param params
     *   The customization parameters to initialize the system with.
     * 
     * @return true
     *   The system was able to successfully be initialized.
     * 
     * @return false
     *   The system was NOT able to successfully be initialized.
     */
    bool initialize(const JobSystemCreateOptions& params = {0u}) noexcept;

    /*!
     * @brief 
     *   Returns the number of workers created by the system.
     *   This function can be called by any thread concurrently.
     * 
     * @return std::size_t
     *   The number of workers created by the system.
     */
    std::size_t numWorkers() noexcept;

    /*!
     * @brief
     *   Makes some system calls to grab the number threads / processors on the device.
     *   This function can be called by any thread concurrently.
     * 
     * @return std::size_t
     *   The number threads / processors on the computer.
     */
    std::size_t numSystemThreads() noexcept;

    /*!
     * @brief 
     *   An implementation defined name for the CPU architecture of the device.
     *   This function can be called by any thread concurrently.
     * 
     * @return const char*
     *   Nul terminated name for the CPU architecture of the device.
     */
    const char* processorArchitectureName() noexcept;

    /*!
     * @brief 
     *   The current id of the current thread.
     *   This function can be called by any thread concurrently.
     * 
     * @return WorkerID
     *   The current id of the current thread.
     */
    WorkerID currentWorker() noexcept;

    /*!
     * @brief 
     *   This should be called as frequently as you want the 
     *   main thread's queue to be flushed.
     * 
     *   This function may only be called by the main thread.
     */
    void tick();

    /*!
     * @brief 
     *   This will deallocate any memory used by the system
     *   and shutdown any threads created by 'bf::job::initialize'.
     * 
     *   This function may only be called by the main thread.
     */
    void shutdown() noexcept;

    // Task API

    /*!
     * @brief 
     *   Pair of a pointer and the size of the buffer you can write to.
     *   Essentially a buffer for userdata, maybe large enough to store content inline.
     * 
     *   If you store non trivial data remember to manually call it's destructor at the bottom of the task function.
     * 
     *   If you call 'taskEmplaceData' or 'taskSetData' and need to update the data once more be sure to
     *   free the previous contents correctly if the data stored in the buffer is not trivial.
     */
    struct TaskData
    {
      void*       ptr;   //!< The start of the buffer you may write to.
      std::size_t size;  //!< The size of the buffer.
    };

    /*!
     * @brief 
     *   Creates a new Task that should be later submitted by calling 'taskSubmit'.
     *   
     * @param function 
     *   The function you want run by the scheduler.
     * 
     * @param parent
     *   An optional parent Task used in conjuction with 'waitOnTask' to force dependencies.
     * 
     * @return Task* 
     *   The newly created task.
     */
    Task* taskMake(TaskFn function, Task* parent = nullptr) noexcept;

    /*!
     * @brief
     *   Returns you the userdata buffer you way write to get data into your TaskFn.
     * 
     * @param task
     *   The task whose userdata you want to grab.
     * 
     * @return TaskData
     *   The userdata buffer you may read and write.
     */
    TaskData taskGetData(Task* task) noexcept;

    /*!
     * @brief
     *   A 'continuation' is a task that will be added to a queue after the 'self'
     *   Task has finished running.
     * 
     *   Continuations will be added to the same queue as the queue from the task that submits it.
     * 
     * @param self
     *   The task to add the 'continuation' to.
     * 
     * @param continuation
     *   The Task to run after 'self' has finished.
     */
    void taskAddContinuation(Task* self, const Task* continuation) noexcept;

    /*!
     * @brief
     *   Submits the task to the specified queue.
     * 
     *   The Task is not required to have been created on the same thread that submits.
     *   You may now wait on this task using 'waitOnTask'.
     * 
     * @param self
     *   The task to sumbit.
     * 
     * @param queue
     *   The queue you want the task to run on.
     */
    void taskSubmit(Task* self, QueueType queue = QueueType::HIGH) noexcept;

    /*!
     * @brief
     *   Grabs the userdata pointer as the T you specified.
     *   No safety is guranteed, this is just a dumb cast.
     * 
     * @tparam T
     *   The type you want to receive the userdata buffer as.
     * 
     * @param task
     *   The task whose data you are retreiving.
     * 
     * @return T&
     *   The userdata buffer casted as a T.
     */
    template<typename T>
    T& taskDataAs(Task* task) noexcept;

    /*!
     * @brief
     *   Calls the constructor of T on the userdata buffer.
     * 
     * @tparam T
     *   The type of T you want constructed inplace into the userdata buffer.
     * 
     * @tparam Args
     *   The Argument types passed into the T constructor.
     * 
     * @param task
     *   The task whose userdata buffer is affected.
     * 
     * @param args
     *   The arguments passed into the constructor of the userdata buffer casted as a T.
     */
    template<typename T, typename... Args>
    void taskEmplaceData(Task* task, Args&&... args);

    /*!
     * @brief
     *   Copies 'data' into the userdata buffer by calling the T copy constructor.
     * 
     * @tparam T
     *   The data type that will be emplaced into the userdata buffer.
     * 
     * @param task
     *   The task whose userdata buffer is affected.
     * 
     * @param data
     *   The data copied into the userdata buffer.
     */
    template<typename T>
    void taskSetData(Task* task, const T& data);

    /*!
     * @brief
     *    Creates a new task by using the userdata buffer to store the closure.
     *      
     *    When a task is created the `function` is copied into the userdata buffer
     *    so the first sizeof(Closure) bytes are storing the callable.
     * 
     *    If you want to store more userdata either be very careful to not 
     *    overwrite this function object or just store all needed data in
     *    the function object itself (the latter is much nicer to do and safer).
     * 
     * @tparam Closure
     *   The type of the callable.
     * 
     * @param function
     *   The non pointer callable you want to store.
     * 
     * @param parent
     *   An optional parent Task used in conjuction with 'waitOnTask' to force dependencies.
     * 
     * @return Task*
     *   The newly created task.
     */
    template<typename Closure>
    Task* taskMake(Closure&& function, Task* parent = nullptr);

    /*!
     * @brief
     *   Waits until the specifed `task` is done executing.
     *   This function will block but do work while being blocked so there is no wasted time.
     * 
     *   You may only call this function with a task created on the current 'Worker'.
     * 
     *   It is a logic bug to call this function on a task that has not been taskSubmit'd.
     * 
     * @param task
     *   The task to wait to finish executing.
     */
    void waitOnTask(const Task* task) noexcept;

    // Template Function Implementation

    template<typename T>
    T& taskDataAs(Task* task) noexcept
    {
      detail::checkTaskDataSize(sizeof(T));

      return *static_cast<T*>(taskGetData(task).ptr);
    }

    template<typename T, typename... Args>
    void taskEmplaceData(Task* task, Args&&... args)
    {
      detail::checkTaskDataSize(sizeof(T));

      new (taskGetData(task).ptr) T(std::forward<Args>(args)...);
    }

    template<typename T>
    void taskSetData(Task* task, const T& data)
    {
      taskEmplaceData<T>(task, data);
    }

    template<typename Closure>
    Task* taskMake(Closure&& function, Task* parent)
    {
      Task* const task = taskMake(+[](Task* task) {
        Closure& function = taskDataAs<Closure>(task);
        function(task);
        function.~Closure();
      }, parent);

      taskEmplaceData<Closure>(task, std::forward<Closure>(function));

      return task;
    }
  }  // namespace job
}  // namespace bf

#endif /* BF_JOB_API_HPP */
