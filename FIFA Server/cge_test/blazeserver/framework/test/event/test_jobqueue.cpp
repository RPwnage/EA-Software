/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>

#include "framework/blaze.h"
#include "eathread/eathread_thread.h"
#include "framework/system/job.h"
#include "framework/system/jobqueue.h"
#include "framework/test/event/mockjob.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

class TestJobQueue;

struct runnerContext {
    TestJobQueue* testInstance;
    bool producerOn;
    Blaze::JobQueue jobQueue;
}; // runnerContext


/*** Helper Functions ****************************************************************************/

// ** Note: there should only be 1 producer thread because the mutex for when the producer is off
// is
intptr_t runProducerThread(void* pContext)
{
    const int max_jobs = 20;
    runnerContext* context = static_cast<runnerContext*>(pContext);

    std::ostringstream oss;
    char threadId[100];
    sprintf(threadId, "%p", (void*)EA::Thread::GetThreadId());

    // Turn on status.
    context->producerOn = true;

    oss << "--> producer (" << threadId << ") running. Building " << max_jobs << " jobs..." << std::endl;
    std::cout << oss.str();
    oss.str("");

    // Start pushing things into the queue.
    for (int i = 0; i < max_jobs; i++)
    {
        oss << "Job " << i;
        Job* job = new MockJob(oss.str());
        context->jobQueue.push(job);
        oss.str("");

        oss << ">producer (" << EA::Thread::GetThreadId() << ") pushing \"Job " << i << "\"." << std::endl;
        std::cout << oss.str();
        oss.str("");
    } // for

    // Quit and turn off status.
    context->producerOn = false;

    oss << "--> producer (" << EA::Thread::GetThreadId() << ") quitting..." << std::endl;
    std::cout << oss.str();
    oss.str("");

    return 0;
}

intptr_t runConsumerThread(void* pContext)
{
    bool bRunning = true;
    std::ostringstream oss;
    runnerContext* context = static_cast<runnerContext*>(pContext);

    oss << "--> consumer (" << EA::Thread::GetThreadId() << ") running... " << std::endl;
    std::cout << oss.str();
    oss.str("");

    while (bRunning)
    {
        oss << "consumer(" << EA::Thread::GetThreadId() << ") working..." << std::endl;
        std::cout << oss.str();
        oss.str("");

        // Get the next job.
        Job* job = context->jobQueue.pop(-1);

        // if Job is not blank, output.
        if (job != nullptr)
        {
            MockJob* mock = (MockJob*) job;
            oss << "<consumer(" << EA::Thread::GetThreadId() << ") consuming: " << mock->toString() << std::endl;
            std::cout << oss.str();
            oss.str("");
            delete mock;
        }
        else
        {
            oss << "~consumer(" << EA::Thread::GetThreadId() << ") found nothing. "; 
            oss << "keep running? " << context->producerOn << std::endl;
            std::cout << oss.str();
            oss.str("");

            // If job blank and producer is still working, keep running.
            bRunning = context->producerOn;
            //
        } // if

    } // while.

    // Deleting stuff (if required?)

    oss << "--> consumer (" << EA::Thread::GetThreadId() << ") quitting... " << std::endl;
    std::cout << oss.str();
    oss.str("");

    return 0;
}



/*** Test Classes ********************************************************************************/
class TestJobQueue
{

// TODO: Build a real UNIT test w/ ASSERTS.

public:
    bool norun(bool test)
    {
        std::cout << "NO RUN NO RUN NO RUNNN!!!!!" << std::endl;
        return false;
    }
    /// testSingleThreadQueue
    /// Creates a queue and then pushes and pops from it single threadedly.
    void testSingleThreadQueue()
    {
        std::cout << std::endl << "------------------------------" << std::endl;
        std::cout << "Testing JobQueue" << std::endl << std::endl;
        std::cout << "------------------------------" << std::endl;

        std::cout << "Creating 5 jobs..." << std::endl;

        Blaze::JobQueue que;
        std::cout << "empty? " << que.empty() << std::endl;

        for (int i = 0; i < 5; i++)
        {
            std::ostringstream oss;
            oss << "Job number: " << i;
            Blaze::Job* mock = new Blaze::MockJob(oss.str());
            mock = que.push(mock);

            std::cout << "Pushed in job: " << ((Blaze::MockJob*) mock)->toString();
            std::cout << " new size: " << que.size();
            std::cout << " empty: " << que.empty() << std::endl;
        }

        std::cout << std::endl << "Finish pushing... now to pop." << std::endl << std::endl;

        size_t size = que.size();
        for (size_t i = 0; i < size; i++)
        {
            Blaze::Job* job = que.pop(-1);

            std::cout << "Popped job: " << ((Blaze::MockJob*) job)->toString();
            std::cout << " new size: " << que.size();
            std::cout << " empty: " << que.empty() << std::endl;
        }

        std::cout << std::endl << "------------------------------" << std::endl << std::endl;
    } // testSingleThreadQueue

    void testMultiThreadQueue()
    {
        std::cout << std::endl << "------------------------------" << std::endl;
        std::cout << "testMultiThreadQueue (start)" << std::endl;
        std::cout << "------------------------------" << std::endl << std::endl;

        std::ostringstream oss;

        // Test parameters
        const int max_consumers = 10;
        EA::Thread::Thread consumers[max_consumers];

        runnerContext* context = new runnerContext();
        context->testInstance = this;
        context->producerOn = true;

        // start producer.
        EA::Thread::Thread producerThread;
        producerThread.SetName("producer_thread");
        producerThread.Begin(runProducerThread, context);

        // start consumers.
        for (int i = 0; i < max_consumers; i++)
        {
            oss << "consumer_thread_" << i;
            consumers[i].SetName(oss.str().c_str());
            consumers[i].Begin(runConsumerThread, context);
            oss.str("");
        }

        // Wait for the producer thread to quit.
        producerThread.WaitForEnd();

        // Wait for each of the consumers to end.
        for (int i = 0; i < max_consumers; i++)
        {
            consumers[i].WaitForEnd();
        } // for

        // Delete anything required.
        delete context;

        std::cout << std::endl << "------------------------------" << std::endl;
        std::cout << "testMultiThreadQueue (end)" << std::endl;
        std::cout << "------------------------------" << std::endl << std::endl;

    } // testMultiThreadQueue

private:
    bool mProducerRunning;

};




// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_JOBQUEUE\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestJobQueue testJobQueue;
    testJobQueue.testSingleThreadQueue();
    testJobQueue.testMultiThreadQueue();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

