#include <string>
#include <iostream>
#include <expected>
#include <functional>

#include <logs.hpp>
#include <metrics.hpp>

#include <memory_resource>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

template <typename TopicType>
class Topic
{

public:
    Topic::Topic(std::pmr::synchronized_pool_resource *node_memory) : node_memory_(node_memory) {};

    std::expected<std::string, int> get_name() { return topic_name_ };

    std::expected<bool, int> publish(TopicType data)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            data_queue_.push(std::allocate_shared<TopicType>(node_memory_, data));
        }
        queue_cv_.notify_one();
    }

    std::expected<TopicType, int> readNonBlocking(int &value)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_, std::try_to_lock);
        if (lock.owns_lock() && !data_queue_.empty())
        {
            value = data_queue_.front();
            data_queue_.pop();
            return true;
        }
        return false;
    }

    void readBlocking()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]
                           { return !data_queue_.empty(); });

            int value = data_queue_.front();
            data_queue_.pop();
            // Process the value
        }
    }

    std::expected<bool, int> register_cb(std::function<void> user_cb);

private:
    std::expected<bool, int> call_cb();

    std::queue<std::shared_ptr<TopicType>> data_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    std::string topic_name_;

    std::pmr::synchronized_pool_resource *node_memory_;
};

class Node
{
public:
    Node() : node_memory_() {}

    // ... other methods ...

    void addData(int value)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            // Use the node_memory_ resource to allocate the int on the queue
            data_queue_.push(std::allocate_shared<int>(node_memory_, value));
        }
        queue_cv_.notify_one();
    }

    void readData()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]
                           { return !data_queue_.empty(); });

            // Get the shared_ptr from the queue
            auto value_ptr = data_queue_.front();
            data_queue_.pop();

            // Access the value through the shared_ptr
            int value = *value_ptr;

            // The shared_ptr will automatically deallocate the memory
            // when it goes out of scope, thanks to node_memory_

            // Process the value
        }
    }

private:
    std::pmr::synchronized_pool_resource node_memory_;
    std::queue<std::shared_ptr<int>> data_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

#include <tbb/parallel_pipeline.h>
#include <tbb/concurrent_queue.h>
#include <iostream>
#include <memory_resource>

// Define the size of the circular buffer
const size_t buffer_size = 1024;

// Structure to hold the data
struct Data
{
    int value;
};

// Function to generate data
void generate_data(tbb::concurrent_bounded_queue<Data *> &queue,
                   std::pmr::memory_resource *mr)
{
    for (int i = 0; i < 10000; ++i)
    {
        // Allocate data using the provided memory resource
        Data *data = new (mr->allocate(sizeof(Data))) Data{i};
        queue.push(data);
    }
}

// Function to process data
void process_data(Data *data)
{
    // Perform some operation on the data
    data->value *= 2;
}

// Function to consume data
void consume_data(Data *data, std::pmr::memory_resource *mr)
{
    std::cout << data->value << std::endl;
    // Deallocate data using the provided memory resource
    mr->deallocate(data, sizeof(Data));
}

int main()
{

    // Create a queue to hold pointers to the data in the circular buffer
    tbb::concurrent_bounded_queue<Data *> queue(buffer_size);

    // Create a pipeline with three stages
    tbb::parallel_pipeline(
        tbb::parallel_pipeline::default_num_tokens(),
        tbb::make_filter<void, Data *>(
            tbb::filter::serial_in_order,
            [&](tbb::flow_control &fc) -> Data *
            {
                if (queue.empty())
                {
                    fc.stop();
                    return nullptr;
                }
                Data *data;
                queue.pop(data);
                return data;
            }) &
            tbb::make_filter<Data *, Data *>(
                tbb::filter::parallel,
                [](Data *data) -> Data *
                {
                    process_data(data);
                    return data;
                }) &
            tbb::make_filter<Data *, void>(
                tbb::filter::serial_in_order,
                [&](Data *data)
                {
                    consume_data(data, &mr);
                    // Put the data back into the circular buffer
                    buffer[queue.size()] = data;
                    queue.push(data);
                }));

    // Generate data in a separate thread using the memory resource
    std::thread generator(generate_data, std::ref(queue), &mr);

    // Wait for the generator thread to finish
    generator.join();

    return 0;
}