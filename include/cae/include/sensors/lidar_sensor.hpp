#include <core/sensor.hpp>


class LidarSensor : public SensorWrapper<LidarSensor>
{
public:
    std::expected<SensorInfo, int> get_name() { return static_cast<S &>(*this).info(); }
};