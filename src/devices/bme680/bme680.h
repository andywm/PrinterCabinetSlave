
class WireBase;

namespace Local
{
    class BME680 final
    {
    public:
        static constexpr const unsigned i2cAddress = 0x76;

        void PollSensor( WireBase & wire );

    private:

    };
}
