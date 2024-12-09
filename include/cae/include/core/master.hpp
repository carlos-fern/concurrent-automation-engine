
#include <iostream>
#include <string>
#include <expected>
#include <vector>
#include <thread>
#include <stop_token> // for jthread
#include <atomic>
#include <node.hpp>
#include <topic.hpp>

class Master
{

public:
    Master::Master()
    {

        nodes_.reserve(num_nodes);

        // Create and start the nodes
        for (int i = 0; i < num_nodes; ++i)
        {
            nodes.emplace_back([i](std::stop_token st)
                               { 
          NodeWrapper node(i); 
          node.run(st); });
        }
    };

    ~Master::Master(){

        live_ = 0};

private:
    void addTopic(std::unique_ptr<Topic> topic)
    {
        topics_[topic->get_name()] = std::move(topic);
    };

    std::vector<std::jthread> nodes_;
    std::atomic<bool> live_;
    std::vector<std::atomic<bool>> thread_keep_alives_;

    std::pmr::synchronized_pool_resource master_memory_;

    std::pmr::unordered_map<std::string, std::unique_ptr<Topic>> topics_{&master_memory_};
};