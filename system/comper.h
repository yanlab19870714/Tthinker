#ifndef COMPER_H_
#define COMPER_H_

#include "global.h"
#include <deque>
#include <queue>
#include "ioser.h"

using namespace std;
using namespace std::chrono;

template <class TaskT, class DataT>
class Comper
{
public:
    typedef Comper<TaskT, DataT> ComperT;
    // Used in worker.h
    typedef TaskT TaskType;
    // Used in worker.h
    typedef DataT DataType;
    typedef typename TaskT::ContextType ContextT;
    typedef stack<DataT *> DataStack;
    int thread_id;
    FILE *gfpout;

    int big_tasks_count = 0;

    typedef std::chrono::_V2::steady_clock::time_point timepoint;
    // Comper's start time
    timepoint start_time;
    // To save latest elapsed time in seconds, in order to find exact duration for each method.
    float latest_elapsed_time = 0;

    thread main_thread;

    DataStack &data_stack = *(DataStack *)global_data_stack;
    typedef deque<TaskT *> TaskQ;
    TaskQ &Qreg = *(TaskQ *)global_Qreg;
    TaskQ &Qbig = *(TaskQ *)global_Qbig;

    Comper()
    {
    }

    virtual ~Comper()
    {
        fclose(gfpout);
        main_thread.join();
    }

    // UDF1
    virtual bool task_spawn(DataT &data) = 0;

    // UDF2
    virtual void compute(ContextT &context) = 0;
    // UDF2 wrapper
    void compute(TaskT *task)
    {
        compute(task->context);
    }

    // UDF3
    virtual bool is_bigTask(TaskT *task)
    {
        return false;
    }

    void start(int thread_id)
    {
        gfpout = fopen(("output_" + to_string(thread_id)).c_str(), "wt");
        this->thread_id = thread_id;

        start_time = steady_clock::now();

        main_thread = thread(&ComperT::run, this);
    }

    string fname;

    long long bigFileSeqNo = 1;
    void set_bigTask_fname()
    {
        fname = "";
        fname += TASK_DISK_BUFFER_DIR + "/" + to_string(thread_id) + "_" + to_string(bigFileSeqNo) + "_bt";
        bigFileSeqNo++;
    }

    long long regFileSeqNo = 1;
    void set_regTask_fname()
    {
        fname = "";
        fname += TASK_DISK_BUFFER_DIR + "/" + to_string(thread_id) + "_" + to_string(regFileSeqNo) + "_rt";
        regFileSeqNo++;
    }

    bool refill_Qbig()
    {
        string file;
        bool succ = global_Lbig.dequeue(file);
        if (!succ)
            // "global_Lbig" is empty
            return false; 
        else
        {
            global_Lbig_num--;
            ofbinstream in(file.c_str());
            while (!in.eof())
            {
                TaskT *task;
                in >> task;
                add_task(task);
            }
            in.close();

            if (remove(file.c_str()) != 0)
            {
                log("Error removing file: " + file);
                log("Error printed by perror");
            }
            return true;
        }
    }

    bool refill_Qreg()
    {
        string file;
        bool succ = global_Lreg.dequeue(file);
        // 1- Lreg is not empty, refill from disk.
        if (succ)
        {
            log("Comper:refill from Lreg !!");
            global_Lreg_num--;
            ofbinstream in(file.c_str());
            while (!in.eof())
            {
                TaskT *task;
                in >> task;
                add_task(task);
            }
            in.close();
            if (remove(file.c_str()) != 0)
            {
                log("Error removing file: " + file);
                log("Error printed by perror");
            }
            return true;
        }
        // 2- Check data_stack to refill
        else
        {
            data_stack_mtx.lock();
            if (!data_stack.empty())
            {

                int temp_vector_size = min(MINI_BATCH_NUM, data_stack.size());

                vector<DataT *> temp_vector;
                for (int i = 0; i < temp_vector_size; i++)
                {
                    temp_vector.push_back(data_stack.top());
                    data_stack.pop();
                }
                data_stack_mtx.unlock();

                int i = 0;
                for (; i < temp_vector_size; i++)
                {
                    // Spawn tasks from data_array
                    // If is_bigtask is true, means big task was spawned, so stop spwaning.
                    bool is_bigtask = task_spawn(*(temp_vector[i]));

                    if (is_bigtask)
                    {
                        i++;
                        break;
                    }
                }

                // If spawning breaks, because a big task was found, then return other tasks to data_array stack.
                if (i < temp_vector_size)
                {
                    data_stack_mtx.lock();
                    for (; i < temp_vector_size; i++)
                    {
                        data_stack.push(temp_vector[i]);
                    }
                    log("data_stack size after: " + to_string(data_stack.size()));
                    data_stack_mtx.unlock();
                }
                return true;
            }
            data_stack_mtx.unlock();
            // Nothing to refill.
            return false;
        }
    }

    void spill_Qbig()
    {
        int i = 0;
        // Fetch tasks into local queue first, to avoid locking while spilling to disk.
        queue<TaskT *> collector;
        while (i < BT_TASKS_PER_FILE && !Qbig.empty())
        {
            // Get a task from the tail
            TaskT *t = Qbig.back();
            Qbig.pop_back();
            collector.push(t);
            i++;
        }
        Qbig_mtx.unlock();

        if (!collector.empty())
        {
            set_bigTask_fname();
            ifbinstream bigTask_out(fname.c_str());

            while (!collector.empty())
            {

                TaskT *t = collector.front();
                collector.pop();
                // Stream to file
                bigTask_out << t;
                // Release from memory
                delete t;
            }

            bigTask_out.close();
            global_Lbig.enqueue(fname);
            global_Lbig_num++;
        }
    }

    void spill_Qreg()
    {
        int i = 0;
        queue<TaskT *> collector;
        while (i < RT_TASKS_PER_FILE && !Qreg.empty())
        {
            TaskT *t = Qreg.back();
            Qreg.pop_back();
            collector.push(t);
            i++;
        }
        Qreg_mtx.unlock();

        if (!collector.empty())
        {
            set_regTask_fname();
            ifbinstream regTask_out(fname.c_str());

            while (!collector.empty())
            {
                TaskT *t = collector.front();
                collector.pop();
                // Stream to file
                regTask_out << t;
                // Release from memory
                delete t;
            }
            regTask_out.close();
            global_Lreg.enqueue(fname);
            global_Lreg_num++;
        }
    }

    bool add_task(TaskT *task)
    {
        if (is_bigTask(task))
        {
            add_bigTask(task);
            return true;
        }

        add_regTask(task);
        return false;
    }

    void add_bigTask(TaskT *task)
    {
        Qbig_mtx.lock();
        // Check if spill is needed.
        while (Qbig.size() >= Qbig_capacity)
        {
            spill_Qbig();
            Qbig_mtx.lock();
        }
        Qbig.push_back(task);
        Qbig_mtx.unlock();
    }

    void add_regTask(TaskT *task)
    {
        Qreg_mtx.lock();
        while (Qreg.size() >= Qreg_capacity)
        {
            spill_Qreg();
            Qreg_mtx.lock();
        }
        Qreg.push_back(task);
        Qreg_mtx.unlock();
    }

    bool get_and_process_tasks()
    {
        // 1- Check Qbig first
        // - if Qbig's size is less than Qbig_capacity, refill using Lbig
        // - if Qbig is not empty, pop a task and compute.
        // - if Qbig is empty, check Qreg
        // 2- Check Qreg
        // - if Qreg's size is less than Qreg_capacity, refill using Lreg
        // - if Qreg is not empty, pop a task
        // - if Qreg is empty, check data_array
        TaskT *task = NULL;

        if (Qbig_mtx.try_lock())
        {
            if (Qbig.size() < BT_THRESHOLD_FOR_REFILL)
            {
                Qbig_mtx.unlock();
                refill_Qbig();
            }
            else
                Qbig_mtx.unlock();

            if (Qbig_mtx.try_lock())
            {
                if (!Qbig.empty())
                {
                    task = Qbig.front();
                    Qbig.pop_front();

                    Qbig_mtx.unlock();
                    compute(task);

                    delete task;
                    return true;
                }
                else
                    Qbig_mtx.unlock();
            }
        }

        // Means Qbig is empty
        if (task == NULL) 
        {
            Qreg_mtx.lock();
            // Refill Qreg using Lreg.
            // If Lreg is empty check data_array.
            bool refilled = false;
            if (Qreg.size() < RT_THRESHOLD_FOR_REFILL)
            {
                Qreg_mtx.unlock();
                refilled = refill_Qreg();
            }
            else
                Qreg_mtx.unlock();

            queue<TaskT *> collector;
            size_t tasks_per_fetch = tasks_per_fetch_g; // _g: global variable
            Qreg_mtx.lock();
            while (!Qreg.empty() && tasks_per_fetch > 0)
            {
                TaskT *task = Qreg.front();
                Qreg.pop_front();
                collector.push(task);
                tasks_per_fetch--;
            }
            Qreg_mtx.unlock();

            if (collector.empty() && !refilled)
            {
                log("collector is empty...");
                return false;
            }

            // Process tasks in "collector"
            while (!collector.empty())
            {
                TaskT *task = collector.front();
                collector.pop();
                compute(task);
                delete task;
            }

            return true;
        }
    }

    void run()
    {
        while (global_end_label == false) // Otherwise, thread terminates
        {
            // Process task or batch of tasks
            bool task_found = get_and_process_tasks();
            // Means that the Queues are empty
            if (!task_found)
            {
                unique_lock<mutex> lck(mtx_go);
                ready_go = false;
                global_num_idle++;
                while (!ready_go)
                {
                    log("wait!!!");
                    cv_go.wait(lck);
                }
                global_num_idle--;
            }
        }
    }
};

#endif