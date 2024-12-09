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

#include <topic.hpp>

template <typename NodeType>
concept Node = requires(NodeType node) {
    { node.run() } -> std::same_as<void>;
    { node.get_name() } -> std::same_as<std::expected<std::string, int>>;
};

template <typename NodeType>
class NodeWrapper
{
public:
    NodeWrapper(std::string node_name, int node_memory_qty, std::atomic<bool> stay_alive) : node_memory_(node_memory_qty), // Initialize the memory resource with a size
                                                                                            node_name_(std::pmr::string{node_mame, &node_memory_}),
                                                                                            stay_alive_(stay_alive)
    {
        
    }

    // Use CRTP to access derived class methods, we don't need dynamic VTBL based polymorphism
    std::expected<std::string, int> get_name() { return static_cast<NodeType &>(*this).get_name(); }
    void run() { return static_cast<NodeType &>(*this).run(); }

    std::expected<bool, int> add_topic(std::string topic_name)
    {
        topics_[topic_name](Topic{topic_name, &node_memory_});
    }

private:
    std::string node_mame;
    std::atomic<bool> stay_alive_;

    cenhive::utils::Logs log_manager;
    cenhive::utils::Metrics metric_manager;

    std::function<void> user_cb_;
    std::pmr::synchronized_pool_resource node_memory_;

    std::pmr::unordered_map<std::string, Topic> topics_{&node_memory_};
};



