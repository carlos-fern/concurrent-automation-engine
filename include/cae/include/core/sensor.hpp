#include <string>
#include <iostream>
#include <expected>
#include <functional>

enum class SENSOR_STATE : uint8_t
{
  BOOTING = 1,
  CONNECTING,
  ON,
  OFF,
  RESETTING,
  IDLE,
};

struct
{
  std::string name;
  std::string frame;
} SensorInfo;

template <typename T>
concept Sensor = requires(T sensor) {
  { sensor.get_name() } -> std::same_as<std::expected<std::string, int>>;
  { sensor.get_type() } -> std::same_as<std::expected<int, int>>;
  { sensor.register_cb(std::function<void>()) } -> std::same_as<bool>;
  { sensor.control(SENSOR_STATE{}) } -> std::same_as<std::expected<bool, int>>;
  { sensor.configure(int{}) };
  { sensor.connect() };
};

template <typename SensorType, typename DataType>
  requires Sensor<SensorType>
class SensorWrapper
{
public:
  SensorWrapper::SensorWrapper(std::pmr::memory_resource *node_memory) : sensor_data_(node_memory)
  {
    configure_sensor(3);
    connect();
  };

  // Use CRTP to access derived class methods
  std::expected<std::string, int> get_name() { return static_cast<SensorType &>(*this).get_name(); }
  bool publish() { return static_cast<SensorType &>(*this).publish(); }
  std::expected<int, int> get_type() { return static_cast<SensorType &>(*this).get_type(); }
  bool register_cb(std::function<void> user_cb) { return static_cast<SensorType &>(*this).register_cb(user_cb); }
  std::expected<bool, int> control(SENSOR_STATE desired_state) { return static_cast<SensorType &>(*this).control(desired_state); }

private:
  std::expected<std::nullptr, int> configure_sensor(int value) { static_cast<SensorType &>(*this).configure(value); }
  void connect() { static_cast<SensorType &>(*this).connect(); }

  std::pmr::vector<DataType> sensor_data_;
};