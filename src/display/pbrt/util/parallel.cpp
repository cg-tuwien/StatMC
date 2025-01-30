// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

/*
    This file contains modifications to the original pbrt source code for the
    paper "A Statistical Approach to Monte Carlo Denoising"
    (https://www.cg.tuwien.ac.at/StatMC).
    
    Copyright © 2024-2025 Hiroyuki Sakai for the modifications.
    Original copyright and license (refer to the top of the file) remain
    unaffected.
 */

#include <pbrt/util/parallel.h>

#include <pbrt/util/check.h>
#include <pbrt/util/print.h>
#ifdef PBRT_BUILD_GPU_RENDERER
#include <pbrt/gpu/util.h>
#endif  // PBRT_BUILD_GPU_RENDERER

#include <algorithm>
#include <iterator>
#include <list>
#include <thread>
#include <vector>

namespace pbrtv4 {

std::string AtomicFloat::ToString() const {
    return StringPrintf("%f", float(*this));
}

std::string AtomicDouble::ToString() const {
    return StringPrintf("%f", double(*this));
}

// Barrier Method Definitions
bool Barrier::Block() {
    std::unique_lock<std::mutex> lock(mutex);

    --numToBlock;
    PBRT_V4_CHECK_GE(numToBlock, 0);

    if (numToBlock > 0) {
        cv.wait(lock, [this]() { return numToBlock == 0; });
    } else
        cv.notify_all();

    return --numToExit == 0;
}

ThreadPool *ParallelJob::threadPool;

// ThreadPool Method Definitions
ThreadPool::ThreadPool(int nThreads) {
    for (int i = 0; i < nThreads - 1; ++i)
        threads.push_back(std::thread(&ThreadPool::Worker, this));
}

void ThreadPool::Worker() {
    LOG_VERBOSE("Started execution in worker thread");

#ifdef PBRT_BUILD_GPU_RENDERER
    GPUThreadInit();
#endif  // PBRT_BUILD_GPU_RENDERER

    std::unique_lock<std::mutex> lock(mutex);
    while (!shutdownThreads)
        WorkOrWait(&lock, false);

    LOG_VERBOSE("Exiting worker thread");
}

std::unique_lock<std::mutex> ThreadPool::AddToJobList(ParallelJob *job) {
    std::unique_lock<std::mutex> lock(mutex);
    // Add _job_ to head of _jobList_
    if (jobList)
        jobList->prev = job;
    job->next = jobList;
    jobList = job;

    jobListCondition.notify_all();
    return lock;
}

void ThreadPool::WorkOrWait(std::unique_lock<std::mutex> *lock, bool isEnqueuingThread) {
    PBRT_V4_DCHECK(lock->owns_lock());
    // Return if this is a worker thread and the thread pool is disabled
    if (!isEnqueuingThread && disabled) {
        jobListCondition.wait(*lock);
        return;
    }

    ParallelJob *job = jobList;
    while (job && !job->HaveWork())
        job = job->next;
    if (job) {
        // Execute work for _job_
        job->activeWorkers++;
        job->RunStep(lock);
        // Handle post-job-execution details
        PBRT_V4_DCHECK(!lock->owns_lock());
        lock->lock();
        job->activeWorkers--;
        if (job->Finished())
            jobListCondition.notify_all();

    } else
        // Wait for new work to arrive or the job to finish
        jobListCondition.wait(*lock);
}

void ThreadPool::RemoveFromJobList(ParallelJob *job) {
    PBRT_V4_DCHECK(!job->removed);

    if (job->prev)
        job->prev->next = job->next;
    else {
        PBRT_V4_DCHECK(jobList == job);
        jobList = job->next;
    }
    if (job->next)
        job->next->prev = job->prev;

    job->removed = true;
}

bool ThreadPool::WorkOrReturn() {
    std::unique_lock<std::mutex> lock(mutex);

    ParallelJob *job = jobList;
    while (job && !job->HaveWork())
        job = job->next;
    if (!job)
        return false;

    // Execute work for _job_
    job->activeWorkers++;
    job->RunStep(&lock);
    PBRT_V4_DCHECK(!lock.owns_lock());
    lock.lock();
    job->activeWorkers--;
    if (job->Finished())
        jobListCondition.notify_all();

    return true;
}

void ThreadPool::ForEachThread(std::function<void(void)> func) {
    Barrier *barrier = new Barrier(threads.size() + 1);

    ParallelFor(0, threads.size() + 1, [barrier, &func](int64_t) {
        func();
        if (barrier->Block())
            delete barrier;
    });
}

void ThreadPool::Disable() {
    PBRT_V4_CHECK(!disabled);
    disabled = true;
    PBRT_V4_CHECK(jobList == nullptr);  // Nothing should be running when Disable() is called.
}

void ThreadPool::Reenable() {
    PBRT_V4_CHECK(disabled);
    disabled = false;
}

ThreadPool::~ThreadPool() {
    if (threads.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(mutex);
        shutdownThreads = true;
        jobListCondition.notify_all();
    }

    for (std::thread &thread : threads)
        thread.join();
}

std::string ThreadPool::ToString() const {
    std::string s = StringPrintf("[ ThreadPool threads.size(): %d shutdownThreads: %s ",
                                 threads.size(), shutdownThreads);
    if (mutex.try_lock()) {
        s += "jobList: [ ";
        ParallelJob *job = jobList;
        while (job) {
            s += job->ToString() + " ";
            job = job->next;
        }
        s += "] ";
        mutex.unlock();
    } else
        s += "(job list mutex locked) ";
    return s + "]";
}

bool DoParallelWork() {
    PBRT_V4_CHECK(ParallelJob::threadPool);
    // lock should be held when this is called...
    return ParallelJob::threadPool->WorkOrReturn();
}

// ParallelForLoop1D Definition
class ParallelForLoop1D : public ParallelJob {
  public:
    // ParallelForLoop1D Public Methods
    ParallelForLoop1D(int64_t startIndex, int64_t endIndex, int chunkSize,
                      std::function<void(int64_t, int64_t)> func)
        : func(std::move(func)),
          nextIndex(startIndex),
          endIndex(endIndex),
          chunkSize(chunkSize) {}

    bool HaveWork() const { return nextIndex < endIndex; }

    void RunStep(std::unique_lock<std::mutex> *lock);

    std::string ToString() const {
        return StringPrintf("[ ParallelForLoop1D nextIndex: %d endIndex: %d "
                            "chunkSize: %d ]",
                            nextIndex, endIndex, chunkSize);
    }

  private:
    // ParallelForLoop1D Private Members
    std::function<void(int64_t, int64_t)> func;
    int64_t nextIndex, endIndex;
    int chunkSize;
};

class ParallelForLoop2D : public ParallelJob {
  public:
    ParallelForLoop2D(const Bounds2i &extent, int chunkSize,
                      std::function<void(Bounds2i)> func)
        : func(std::move(func)),
          extent(extent),
          nextStart(extent.pMin),
          chunkSize(chunkSize) {}

    bool HaveWork() const { return nextStart.y < extent.pMax.y; }
    void RunStep(std::unique_lock<std::mutex> *lock);

    std::string ToString() const {
        return StringPrintf("[ ParallelForLoop2D extent: %s nextStart: %s "
                            "chunkSize: %d ]",
                            extent, nextStart, chunkSize);
    }

  private:
    std::function<void(Bounds2i)> func;
    const Bounds2i extent;
    Point2i nextStart;
    int chunkSize;
};

// ParallelForLoop1D Method Definitions
void ParallelForLoop1D::RunStep(std::unique_lock<std::mutex> *lock) {
    // Determine the range of loop iterations to run in this step
    int64_t indexStart = nextIndex;
    int64_t indexEnd = std::min(indexStart + chunkSize, endIndex);
    nextIndex = indexEnd;

    // Remove job from list if all work has been started
    if (!HaveWork())
        threadPool->RemoveFromJobList(this);

    // Release lock and execute loop iterations in _[indexStart, indexEnd)_
    lock->unlock();
    func(indexStart, indexEnd);
}

void ParallelForLoop2D::RunStep(std::unique_lock<std::mutex> *lock) {
    // Compute extent for this step
    Point2i end = nextStart + Vector2i(chunkSize, chunkSize);
    Bounds2i b = Intersect(Bounds2i(nextStart, end), extent);
    PBRT_V4_CHECK(!b.IsEmpty());

    // Advance to be ready for the next extent.
    nextStart.x += chunkSize;
    if (nextStart.x >= extent.pMax.x) {
        nextStart.x = extent.pMin.x;
        nextStart.y += chunkSize;
    }

    if (!HaveWork())
        threadPool->RemoveFromJobList(this);

    lock->unlock();

    // Run the loop iteration
    func(b);
}

// Parallel Function Definitions
void ParallelFor(int64_t start, int64_t end, std::function<void(int64_t, int64_t)> func) {
    PBRT_V4_CHECK(ParallelJob::threadPool);
    if (start == end)
        return;
    // Compute chunk size for parallel loop
    int64_t chunkSize = std::max<int64_t>(1, (end - start) / (8 * RunningThreads()));

    // Create and enqueue _ParallelForLoop1D_ for this loop
    ParallelForLoop1D loop(start, end, chunkSize, std::move(func));
    std::unique_lock<std::mutex> lock = ParallelJob::threadPool->AddToJobList(&loop);

    // Help out with parallel loop iterations in the current thread
    while (!loop.Finished())
        ParallelJob::threadPool->WorkOrWait(&lock, true);
}

void ParallelFor2D(const Bounds2i &extent, std::function<void(Bounds2i)> func) {
    PBRT_V4_CHECK(ParallelJob::threadPool);

    if (extent.IsEmpty())
        return;
    if (extent.Area() == 1) {
        func(extent);
        return;
    }

    // Want at least 8 tiles per thread, subject to not too big and not too
    // small.
    // TODO: should we do non-square?
    int tileSize = Clamp(int(std::sqrt(extent.Diagonal().x * extent.Diagonal().y /
                                       (8 * RunningThreads()))),
                         1, 32);

    ParallelForLoop2D loop(extent, tileSize, std::move(func));
    std::unique_lock<std::mutex> lock = ParallelJob::threadPool->AddToJobList(&loop);

    // Help out with parallel loop iterations in the current thread
    while (!loop.Finished())
        ParallelJob::threadPool->WorkOrWait(&lock, true);
}

///////////////////////////////////////////////////////////////////////////

int AvailableCores() {
    return std::max<int>(1, std::thread::hardware_concurrency());
}

int RunningThreads() {
    return ParallelJob::threadPool ? (1 + ParallelJob::threadPool->size()) : 1;
}

void ParallelInit(int nThreads) {
    PBRT_V4_CHECK(!ParallelJob::threadPool);
    if (nThreads <= 0)
        nThreads = AvailableCores();
    ParallelJob::threadPool = new ThreadPool(nThreads);
}

void ParallelCleanup() {
    delete ParallelJob::threadPool;
    ParallelJob::threadPool = nullptr;
}

void ForEachThread(std::function<void(void)> func) {
    if (ParallelJob::threadPool)
        ParallelJob::threadPool->ForEachThread(std::move(func));
}

void DisableThreadPool() {
    PBRT_V4_CHECK(ParallelJob::threadPool);
    ParallelJob::threadPool->Disable();
}

void ReenableThreadPool() {
    PBRT_V4_CHECK(ParallelJob::threadPool);
    ParallelJob::threadPool->Reenable();
}

}  // namespace pbrtv4
